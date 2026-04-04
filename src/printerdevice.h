#pragma once
#include <QString>
#include <cstdint>
#include <libusb-1.0/libusb.h>

struct KnownPrinter {
    uint16_t idVendor;
    uint16_t idProduct;
    const char* name;
};

// Common Epson receipt printer VID/PIDs
static const KnownPrinter KNOWN_PRINTERS[] = {
    { 0x04B8, 0x0202, "Epson TM-T88II"   },
    { 0x04B8, 0x0203, "Epson TM-T88III"  },
    { 0x04B8, 0x0E03, "Epson TM-T20"     },
    { 0x04B8, 0x0E15, "Epson TM-T20II"   },
    { 0x04B8, 0x0E27, "Epson TM-T20III"  },
    { 0x04B8, 0x0E28, "Epson TM-T88VI"   },
    { 0x04B8, 0x0E2B, "Epson TM-T88VII"  },
    { 0x04B8, 0x0E15, "Epson TM-T20"     },
    { 0x04B8, 0x0E15, "Epson TM-T82"     },
    { 0x04B8, 0x0E66, "Epson TM-T82III"  },
    { 0,      0,      nullptr             }  // sentinel
};

class PrinterDevice {
public:
    PrinterDevice();
    ~PrinterDevice();

    // Try to auto-detect and connect a printer
    bool findAndConnect();
    // Connect by explicit VID:PID
    bool connectByVidPid(uint16_t idVendor, uint16_t idProduct);
    void disconnect();

    bool isConnected() const { return m_connected; }
    QString statusMessage() const { return m_statusMessage; }
    QString deviceDesc()    const { return m_deviceDesc; }

    // Print plain text (sanitises umlauts, adds paper feed)
    bool printText(const QString& text);

private:
    static constexpr int USB_TIMEOUT_MS = 3000;
    static constexpr uint8_t ESC = 0x1B;
    static constexpr uint8_t GS  = 0x1D;
    static constexpr uint8_t LF  = 0x0A;

    bool openDevice(libusb_device* dev);
    bool claimPrinterInterface();
    bool findEndpoints(const libusb_config_descriptor* cfg);
    bool bulkWrite(const uint8_t* data, int length);
    bool escposInit();
    bool escposCut();

    libusb_context*      m_ctx           = nullptr;
    libusb_device_handle* m_handle       = nullptr;
    uint8_t              m_endpointOut   = 0;
    int                  m_interface     = -1;
    bool                 m_kernelDetached = false;
    bool                 m_connected     = false;
    QString              m_statusMessage;
    QString              m_deviceDesc;
};
