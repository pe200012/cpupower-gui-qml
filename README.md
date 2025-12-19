# cpupower-gui (Qt/Kirigami)

A graphical utility for managing CPU frequency scaling and governor settings on Linux systems. This is a Qt6/Kirigami port of the original [cpupower-gui](https://github.com/vagnum08/cpupower-gui) project, rewritten in C++ with a modern QML interface.

The application provides an accessible way to adjust CPU performance parameters that would otherwise require command-line tools like `cpupower`. It reads frequency limits and available governors from the kernel's sysfs interface and applies changes through a privileged D-Bus helper service.

## Features

- Adjust minimum and maximum CPU frequency limits per core or across all cores simultaneously
- Select CPU governors (performance, powersave, schedutil, etc.)
- Configure Intel P-state energy performance preferences when supported
- Save and restore settings through named profiles
- System tray integration for quick access and background operation
- Real-time frequency monitoring

## Dependencies

### Build dependencies

- CMake 3.16 or later
- Qt 6.5 or later (Core, Quick, QuickControls2, DBus, Widgets, Qml)
- KDE Frameworks 6 (Kirigami, KI18n, KCoreAddons, KConfig, KStatusNotifierItem)
- Extra CMake Modules (ECM)

### Runtime dependencies

- Qt 6.5 or later
- KDE Frameworks 6
- PolicyKit (polkit) for privilege escalation

## Building

The project consists of two components: the main GUI application and a privileged helper service.

### Main application

```sh
mkdir build && cd build
cmake ..
make -j$(nproc)
```

The resulting binary will be at `build/bin/cpupower-gui-qml`.

### Helper service

The helper runs as root and handles operations that require elevated privileges. Without it, the application can display CPU information but cannot modify settings.

```sh
mkdir helper/build && cd helper/build
cmake ..
make -j$(nproc)
```

### Installation

For the main application:

```sh
cd build
sudo make install
```

For the helper service:

```sh
cd helper/build
sudo make install
```

This installs the helper binary to `/usr/libexec/`, the D-Bus service files to the appropriate system directories, and the PolicyKit policy that controls authentication.

## Gentoo

An ebuild is available for Gentoo users. Copy `cpupower-gui-qml-9999.ebuild` to your local overlay under `sys-power/cpupower-gui-qml/` and run:

```sh
emerge sys-power/cpupower-gui-qml
```

The `systemd` USE flag controls whether the systemd service unit is installed.

## How it works

The application reads CPU information directly from `/sys/devices/system/cpu/` to display current frequencies, available governors, and frequency limits. When the user requests a change, the GUI communicates with the helper service over D-Bus.

The helper service (`cpupower-gui-helper`) runs with root privileges and performs the actual writes to sysfs. PolicyKit handles authentication, prompting users for credentials when needed. Members of the `wheel` group can authenticate with their own password rather than the root password.

D-Bus activation starts the helper on demand when the GUI needs it, so there is no need to run a persistent daemon unless you prefer that approach.

## Configuration

Profiles are stored in `~/.config/cpupower-gui-qml/` as INI files. Each profile records frequency limits, governor selection, and energy preferences for each CPU core.

Application settings (window geometry, default profile, tray behavior) are stored through KConfig in the standard KDE configuration location.

## Notes on CPU frequency drivers

The available options depend on which scaling driver your kernel uses:

- **intel_pstate**: Provides `powersave` and `performance` governors along with energy performance preferences. Frequency limits work differently than with acpi-cpufreq.
- **amd-pstate**: Similar to intel_pstate, available on newer AMD processors.
- **acpi-cpufreq**: The traditional driver offering multiple governors including ondemand, conservative, and userspace.

The application detects the active driver and adapts the interface accordingly. Energy performance preferences only appear when the hardware and driver support them.

## License

This project is licensed under the GNU General Public License v3.0 or later. See the COPYING file for the full license text.

The original cpupower-gui project by vagnum08 is also licensed under GPL-3.0-or-later.
