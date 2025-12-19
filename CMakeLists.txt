# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2024 cpupower-gui contributors

cmake_minimum_required(VERSION 3.16)

project(cpupower-gui-qml VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Export compile commands for LSP (clangd)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Force Qt6 - needed when both Qt5 and Qt6 are installed
set(QT_DEFAULT_MAJOR_VERSION 6)
set(QT_VERSION_MAJOR 6)

# Find required packages
find_package(ECM 5.68 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMSetupVersion)
include(ECMQtDeclareLoggingCategory)
include(ECMPoQmTools)

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX CPUPOWER_GUI
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

# Set QT_QML_GENERATE_QMLLS_INI for better QML tooling
set(QT_QML_GENERATE_QMLLS_INI ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Quick
    QuickControls2
    DBus
    Widgets
    Qml
)

find_package(KF6 REQUIRED COMPONENTS
    Kirigami
    I18n
    CoreAddons
    Config
    StatusNotifierItem
)

# Application ID
set(APPLICATION_ID "io.github.cpupower_gui.qt")
add_definitions(-DAPPLICATION_ID="${APPLICATION_ID}")

# Source files
set(CORE_SOURCES
    src/core/dbushelper.cpp
    src/core/dbushelper.h
    src/core/sysfsreader.cpp
    src/core/sysfsreader.h
    src/core/cpusettings.cpp
    src/core/cpusettings.h
)

set(MODEL_SOURCES
    src/models/cpulistmodel.cpp
    src/models/cpulistmodel.h
    src/models/profilemodel.cpp
    src/models/profilemodel.h
    src/models/governormodel.cpp
    src/models/governormodel.h
    src/models/energyprefmodel.cpp
    src/models/energyprefmodel.h
)

set(CONFIG_SOURCES
    src/config/appconfig.cpp
    src/config/appconfig.h
    src/config/profilemanager.cpp
    src/config/profilemanager.h
)

set(TRAY_SOURCES
    src/tray/trayicon.cpp
    src/tray/trayicon.h
)

set(SOURCES
    src/main.cpp
    src/application.cpp
    src/application.h
    ${CORE_SOURCES}
    ${MODEL_SOURCES}
    ${CONFIG_SOURCES}
    ${TRAY_SOURCES}
)

# Create executable
qt_add_executable(cpupower-gui-qml ${SOURCES})

# Add QML module - all files in flat structure for proper module resolution
qt_add_qml_module(cpupower-gui-qml
    URI io.github.cpupower_gui.qt
    VERSION 1.0
    NO_RESOURCE_TARGET_PATH
    QML_FILES
        qml/Main.qml
        qml/pages/SettingsPage.qml
        qml/pages/ProfilesPage.qml
        qml/pages/PreferencesPage.qml
        qml/components/CpuTable.qml
        qml/components/FrequencySlider.qml
        qml/components/CpuSelector.qml
        qml/components/ProfileSelector.qml
)

target_link_libraries(cpupower-gui-qml PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::DBus
    Qt6::Widgets
    Qt6::Qml
    KF6::Kirigami
    KF6::I18n
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::StatusNotifierItem
)

target_include_directories(cpupower-gui-qml PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Install
install(TARGETS cpupower-gui-qml DESTINATION ${KDE_INSTALL_BINDIR})
install(FILES io.github.cpupower_gui.qt.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES io.github.cpupower_gui.qt.svg DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps)

# i18n
ki18n_install(po)
