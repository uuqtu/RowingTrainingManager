{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  name = "rowing-manager-dev";

  nativeBuildInputs = with pkgs; [
    cmake
    ninja
    pkg-config
    qt6.wrapQtAppsHook
  ];

  buildInputs = with pkgs; [
    qt6.qtbase
    qt6.qtwayland
    sqlite
    libusb1          # ESC/POS receipt printer support via USB
  ];

  # Qt6 requires these env vars to find plugins at runtime
  shellHook = ''
    export QT_QPA_PLATFORM_PLUGIN_PATH="${pkgs.qt6.qtbase}/lib/qt-6/plugins/platforms"
    export QT_PLUGIN_PATH="${pkgs.qt6.qtbase}/lib/qt-6/plugins"

    echo ""
    echo "╔══════════════════════════════════════════╗"
    echo "║       Rowing Manager — Dev Shell         ║"
    echo "╠══════════════════════════════════════════╣"
    echo "║  Build:                                  ║"
    echo "║    mkdir -p build && cd build            ║"
    echo "║    cmake .. -G Ninja                     ║"
    echo "║    ninja                                 ║"
    echo "║                                          ║"
    echo "║  Run:                                    ║"
    echo "║    ./build/RowingManager                 ║"
    echo "║                                          ║"
    echo "║  Printer (Linux):                        ║"
    echo "║    Add udev rule or run as root for USB  ║"
    echo "╚══════════════════════════════════════════╝"
    echo ""
  '';
}
