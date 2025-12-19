# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2024 cpupower-gui contributors

cmake_minimum_required(VERSION 3.16)

project(cpupower-gui-helper VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Export compile commands for LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core DBus)

# Helper executable
add_executable(cpupower-gui-helper
    src/main.cpp
    src/helperservice.cpp
    src/helperservice.h
)

target_link_libraries(cpupower-gui-helper PRIVATE
    Qt6::Core
    Qt6::DBus
)

# Installation paths
include(GNUInstallDirs)

set(HELPER_INSTALL_DIR "${CMAKE_INSTALL_LIBEXECDIR}" CACHE PATH "Helper installation directory")
set(DBUS_SYSTEM_SERVICES_DIR "${CMAKE_INSTALL_DATADIR}/dbus-1/system-services" CACHE PATH "D-Bus system services directory")
set(DBUS_SYSTEM_CONF_DIR "${CMAKE_INSTALL_DATADIR}/dbus-1/system.d" CACHE PATH "D-Bus system configuration directory")
set(POLKIT_ACTIONS_DIR "${CMAKE_INSTALL_DATADIR}/polkit-1/actions" CACHE PATH "PolicyKit actions directory")
set(SYSTEMD_SYSTEM_UNIT_DIR "${CMAKE_INSTALL_PREFIX}/lib/systemd/system" CACHE PATH "Systemd system unit directory")

# Configure files with install paths
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/data/io.github.cpupower_gui.qt.helper.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/io.github.cpupower_gui.qt.helper.service
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/data/cpupower-gui-helper.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/cpupower-gui-helper.service
    @ONLY
)

# Install helper binary
install(TARGETS cpupower-gui-helper
    RUNTIME DESTINATION ${HELPER_INSTALL_DIR}
)

# Install D-Bus service file
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/io.github.cpupower_gui.qt.helper.service
    DESTINATION ${DBUS_SYSTEM_SERVICES_DIR}
)

# Install D-Bus configuration
install(FILES data/io.github.cpupower_gui.qt.helper.conf
    DESTINATION ${DBUS_SYSTEM_CONF_DIR}
)

# Install PolicyKit policy
install(FILES data/io.github.cpupower_gui.qt.policy
    DESTINATION ${POLKIT_ACTIONS_DIR}
)

# Install systemd service (optional)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cpupower-gui-helper.service
    DESTINATION ${SYSTEMD_SYSTEM_UNIT_DIR}
    OPTIONAL
)

message(STATUS "Helper will be installed to: ${CMAKE_INSTALL_PREFIX}/${HELPER_INSTALL_DIR}")
message(STATUS "D-Bus services dir: ${DBUS_SYSTEM_SERVICES_DIR}")
message(STATUS "D-Bus config dir: ${DBUS_SYSTEM_CONF_DIR}")
message(STATUS "PolicyKit actions dir: ${POLKIT_ACTIONS_DIR}")
