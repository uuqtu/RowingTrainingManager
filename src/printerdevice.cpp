#include "printerdevice.h"
#include <QDateTime>
#include <QDebug>
#include <cstring>

PrinterDevice::PrinterDevice()
{
    int rc = libusb_init(&m_ctx);
    if (rc < 0) {
        m_statusMessage = QString("libusb_init failed: %1")
                          .arg(libusb_strerror(static_cast<libusb_error>(rc)));
        m_ctx = nullptr;
    }
#ifndef NDEBUG
    if (m_ctx)
        libusb_set_option(m_ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);
#endif
}

PrinterDevice::~PrinterDevice()
{
    disconnect();
    if (m_ctx) { libusb_exit(m_ctx); m_ctx = nullptr; }
}

bool PrinterDevice::findAndConnect()
{
    if (!m_ctx) { m_statusMessage = "libusb context not initialised"; return false; }
    disconnect();

    for (int i = 0; KNOWN_PRINTERS[i].name != nullptr; ++i) {
        const auto& ep = KNOWN_PRINTERS[i];
        libusb_device_handle* h =
            libusb_open_device_with_vid_pid(m_ctx, ep.idVendor, ep.idProduct);
        if (h) {
            libusb_device* dev = libusb_get_device(h);
            libusb_close(h);
            if (openDevice(dev)) {
                m_statusMessage = QString("Connected: %1 (VID=%2 PID=%3)")
                    .arg(ep.name)
                    .arg(ep.idVendor,  4, 16, QChar('0'))
                    .arg(ep.idProduct, 4, 16, QChar('0'));
                return true;
            }
        }
    }

    libusb_device** devs = nullptr;
    ssize_t cnt = libusb_get_device_list(m_ctx, &devs);
    if (cnt < 0) {
        m_statusMessage = QString("libusb_get_device_list: %1")
                          .arg(libusb_strerror(static_cast<libusb_error>(cnt)));
        return false;
    }

    auto isKnownVendor = [](uint16_t vid) -> const char* {
        for (int i = 0; KNOWN_PRINTERS[i].name != nullptr; ++i)
            if (KNOWN_PRINTERS[i].idVendor == vid) return KNOWN_PRINTERS[i].name;
        return nullptr;
    };

    bool found = false;
    for (ssize_t i = 0; i < cnt && !found; ++i) {
        libusb_device_descriptor desc{};
        if (libusb_get_device_descriptor(devs[i], &desc) != 0) continue;
        const char* vendorName = isKnownVendor(desc.idVendor);
        if (!vendorName) continue;

        bool isPrinterClass = (desc.bDeviceClass == 0x07 || desc.bDeviceClass == 0x00);
        if (!isPrinterClass) {
            libusb_config_descriptor* cfg = nullptr;
            if (libusb_get_active_config_descriptor(devs[i], &cfg) == 0) {
                for (uint8_t c = 0; c < cfg->bNumInterfaces && !isPrinterClass; ++c)
                    for (int a = 0; a < cfg->interface[c].num_altsetting; ++a) {
                        uint8_t cls = cfg->interface[c].altsetting[a].bInterfaceClass;
                        if (cls == 0x07 || cls == 0xFF || cls == 0x00) { isPrinterClass = true; break; }
                    }
                libusb_free_config_descriptor(cfg);
            }
        }
        if (!isPrinterClass) continue;

        if (openDevice(devs[i])) {
            m_statusMessage = QString("%1 (VID=%2 PID=%3)")
                .arg(vendorName)
                .arg(desc.idVendor,  4, 16, QChar('0'))
                .arg(desc.idProduct, 4, 16, QChar('0'));
            found = true;
        }
    }
    libusb_free_device_list(devs, 1);

    if (!found)
        m_statusMessage =
            "No printer found.\n"
            "Check: printer connected & powered on.\n"
            "Windows: install WinUSB driver via Zadig.";
    return found;
}

bool PrinterDevice::connectByVidPid(uint16_t idVendor, uint16_t idProduct)
{
    if (!m_ctx) { m_statusMessage = "libusb context not initialised"; return false; }
    disconnect();
    libusb_device** devs = nullptr;
    ssize_t cnt = libusb_get_device_list(m_ctx, &devs);
    if (cnt < 0) { m_statusMessage = libusb_strerror(static_cast<libusb_error>(cnt)); return false; }
    bool found = false;
    for (ssize_t i = 0; i < cnt && !found; ++i) {
        libusb_device_descriptor desc{};
        if (libusb_get_device_descriptor(devs[i], &desc) != 0) continue;
        if (desc.idVendor == idVendor && desc.idProduct == idProduct)
            found = openDevice(devs[i]);
    }
    libusb_free_device_list(devs, 1);
    if (!found)
        m_statusMessage = QString("Device %1:%2 not found or access denied")
            .arg(idVendor, 4, 16, QChar('0')).arg(idProduct, 4, 16, QChar('0'));
    return found;
}

void PrinterDevice::disconnect()
{
    if (!m_handle) return;
    if (m_interface >= 0) libusb_release_interface(m_handle, m_interface);
#ifndef Q_OS_WIN
    if (m_kernelDetached && m_interface >= 0) {
        libusb_attach_kernel_driver(m_handle, m_interface);
        m_kernelDetached = false;
    }
#endif
    m_interface = -1;
    libusb_close(m_handle);
    m_handle    = nullptr;
    m_connected = false;
    m_endpointOut = 0;
}

bool PrinterDevice::openDevice(libusb_device* dev)
{
    int rc = libusb_open(dev, &m_handle);
    if (rc != 0) {
        m_statusMessage = QString("libusb_open: %1")
                          .arg(libusb_strerror(static_cast<libusb_error>(rc)));
        m_handle = nullptr;
        return false;
    }
    uint8_t bus  = libusb_get_bus_number(dev);
    uint8_t port = libusb_get_port_number(dev);
    m_deviceDesc = QString("USB bus%1 port%2").arg(bus).arg(port);
    if (!claimPrinterInterface()) { libusb_close(m_handle); m_handle = nullptr; return false; }
    if (!escposInit()) { disconnect(); return false; }
    m_connected = true;
    return true;
}

bool PrinterDevice::claimPrinterInterface()
{
    libusb_device* dev = libusb_get_device(m_handle);
    libusb_config_descriptor* cfg = nullptr;
    int rc = libusb_get_active_config_descriptor(dev, &cfg);
    if (rc != 0) {
        m_statusMessage = QString("get_active_config_descriptor: %1")
                          .arg(libusb_strerror(static_cast<libusb_error>(rc)));
        return false;
    }
    bool claimed = false;
    for (uint8_t i = 0; i < cfg->bNumInterfaces && !claimed; ++i) {
        for (int a = 0; a < cfg->interface[i].num_altsetting && !claimed; ++a) {
            const libusb_interface_descriptor& iface = cfg->interface[i].altsetting[a];
            if (iface.bInterfaceClass != 0x07 &&
                iface.bInterfaceClass != 0xFF &&
                iface.bInterfaceClass != 0x00) continue;
            if (!findEndpoints(cfg)) continue;
            int ifaceNum = static_cast<int>(iface.bInterfaceNumber);
#ifndef Q_OS_WIN
            if (libusb_kernel_driver_active(m_handle, ifaceNum) == 1) {
                rc = libusb_detach_kernel_driver(m_handle, ifaceNum);
                if (rc != 0) {
                    m_statusMessage = QString("Could not detach kernel driver: %1")
                                      .arg(libusb_strerror(static_cast<libusb_error>(rc)));
                    continue;
                }
                m_kernelDetached = true;
            }
#endif
            rc = libusb_claim_interface(m_handle, ifaceNum);
            if (rc != 0) {
                m_statusMessage = QString("libusb_claim_interface %1: %2")
                    .arg(ifaceNum).arg(libusb_strerror(static_cast<libusb_error>(rc)));
#ifndef Q_OS_WIN
                if (m_kernelDetached) { libusb_attach_kernel_driver(m_handle, ifaceNum); m_kernelDetached = false; }
#endif
                continue;
            }
            m_interface = ifaceNum;
            claimed = true;
        }
    }
    libusb_free_config_descriptor(cfg);
    if (!claimed && m_statusMessage.isEmpty())
        m_statusMessage = "No suitable printer interface found";
    return claimed;
}

bool PrinterDevice::findEndpoints(const libusb_config_descriptor* cfg)
{
    for (uint8_t i = 0; i < cfg->bNumInterfaces; ++i)
        for (int a = 0; a < cfg->interface[i].num_altsetting; ++a) {
            const libusb_interface_descriptor& iface = cfg->interface[i].altsetting[a];
            for (uint8_t e = 0; e < iface.bNumEndpoints; ++e) {
                const libusb_endpoint_descriptor& ep = iface.endpoint[e];
                bool isOut  = (ep.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT;
                bool isBulk = (ep.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK;
                if (isOut && isBulk) { m_endpointOut = ep.bEndpointAddress; return true; }
            }
        }
    return false;
}

bool PrinterDevice::bulkWrite(const uint8_t* data, int length)
{
    if (!m_handle || m_endpointOut == 0) {
        m_statusMessage = "No valid USB handle / endpoint";
        m_connected = false;
        return false;
    }
    int offset = 0;
    while (offset < length) {
        int transferred = 0;
        int rc = libusb_bulk_transfer(m_handle, m_endpointOut,
                                       const_cast<uint8_t*>(data + offset),
                                       length - offset, &transferred, USB_TIMEOUT_MS);
        if (rc != 0 && rc != LIBUSB_ERROR_TIMEOUT) {
            m_statusMessage = QString("bulk_transfer: %1")
                              .arg(libusb_strerror(static_cast<libusb_error>(rc)));
            m_connected = false;
            return false;
        }
        if (transferred == 0) { m_statusMessage = "bulk_transfer: zero bytes (timeout?)"; return false; }
        offset += transferred;
    }
    return true;
}

bool PrinterDevice::escposInit()
{
    const uint8_t cmd[] = { static_cast<uint8_t>(ESC), '@' };
    return bulkWrite(cmd, sizeof(cmd));
}

bool PrinterDevice::escposCut()
{
    const uint8_t cmd[] = { static_cast<uint8_t>(GS), 'V', 0 };
    return bulkWrite(cmd, sizeof(cmd));
}

bool PrinterDevice::printText(const QString& text)
{
    if (!m_connected) { m_statusMessage = "Printer not connected"; return false; }
    if (!escposInit()) return false;

    QString sanitized = text;
    sanitized.replace("\xc3\xa4", "ae"); // ä
    sanitized.replace("\xc3\xb6", "oe"); // ö
    sanitized.replace("\xc3\xbc", "ue"); // ü
    sanitized.replace("\xc3\x9f", "ss"); // ß
    sanitized.replace("\xc3\x84", "Ae"); // Ä
    sanitized.replace("\xc3\x96", "Oe"); // Ö
    sanitized.replace("\xc3\x9c", "Ue"); // Ü
    sanitized.replace("ä", "ae");
    sanitized.replace("ö", "oe");
    sanitized.replace("ü", "ue");
    sanitized.replace("ß", "ss");
    sanitized.replace("Ä", "Ae");
    sanitized.replace("Ö", "Oe");
    sanitized.replace("Ü", "Ue");

    QByteArray bytes = sanitized.toLatin1();
    // Ensure trailing newlines for paper feed
    if (!bytes.endsWith('\n')) bytes.append('\n');
    for (int i = 0; i < 4; ++i) bytes.append('\n');

    if (!bulkWrite(reinterpret_cast<const uint8_t*>(bytes.constData()), bytes.length()))
        return false;

    // Cut
    return escposCut();
}
