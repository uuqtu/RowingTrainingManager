cmake_minimum_required(VERSION 3.16)
project(RowingManager VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/boat.cpp
    src/rower.cpp
    src/assignment.cpp
    src/databasemanager.cpp
    src/boattablemodel.cpp
    src/rowertablemodel.cpp
    src/assignmentdialog.cpp
    src/assignmentgenerator.cpp
    src/printerdevice.cpp
    src/assignmentviewdialog.cpp
)

set(HEADERS
    src/mainwindow.h
    src/boat.h
    src/rower.h
    src/assignment.h
    src/databasemanager.h
    src/boattablemodel.h
    src/rowertablemodel.h
    src/assignmentdialog.h
    src/assignmentgenerator.h
    src/printerdevice.h
    src/assignmentviewdialog.h
)

add_executable(RowingManager ${SOURCES} ${HEADERS})

target_link_libraries(RowingManager PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    ${LIBUSB_LIBRARIES}
)
target_include_directories(RowingManager PRIVATE ${LIBUSB_INCLUDE_DIRS})
