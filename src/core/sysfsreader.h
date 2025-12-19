// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#ifndef SYSFSREADER_H
#define SYSFSREADER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QPair>
#include <QList>

/**
 * @brief Direct sysfs reader for CPU information
 * 
 * This class reads CPU information directly from /sys/devices/system/cpu/
 * for non-privileged operations (reading current frequencies, governors, etc.)
 */
class SysfsReader : public QObject
{
    Q_OBJECT

public:
    explicit SysfsReader(QObject *parent = nullptr);
    ~SysfsReader() override = default;

    // Frequency info (in kHz)
    Q_INVOKABLE int currentFreq(int cpu) const;
    Q_INVOKABLE QPair<int, int> freqLimits(int cpu) const;      // Hardware min/max
    Q_INVOKABLE QPair<int, int> scalingFreqs(int cpu) const;    // Current scaling min/max
    Q_INVOKABLE QList<int> availableFrequencies(int cpu) const;
    
    // Convenience methods for individual frequency values (in kHz)
    Q_INVOKABLE qint64 minFrequency(int cpu) const;              // Current scaling min
    Q_INVOKABLE qint64 maxFrequency(int cpu) const;              // Current scaling max
    Q_INVOKABLE qint64 minFrequencyHardware(int cpu) const;      // Hardware min
    Q_INVOKABLE qint64 maxFrequencyHardware(int cpu) const;      // Hardware max

    // Governor/energy
    Q_INVOKABLE QString currentGovernor(int cpu) const;
    Q_INVOKABLE QStringList availableGovernors(int cpu) const;
    Q_INVOKABLE QStringList availableEnergyPrefs(int cpu) const;
    Q_INVOKABLE QString currentEnergyPref(int cpu) const;
    Q_INVOKABLE bool isEnergyPrefAvailable(int cpu) const;

    // Online state
    Q_INVOKABLE bool isOnline(int cpu) const;
    Q_INVOKABLE QList<int> onlineCpus() const;
    Q_INVOKABLE QList<int> presentCpus() const;
    Q_INVOKABLE QList<int> availableCpus() const;

private:
    QString readFile(const QString &path) const;
    QStringList parseList(const QString &content) const;
    QList<int> parseCpuList(const QString &content) const;

    QString cpuPath(int cpu) const;

    static constexpr const char *SYS_CPU_PATH = "/sys/devices/system/cpu";
    static constexpr const char *CPUFREQ_PATH = "cpufreq";
    static constexpr const char *SCALING_CUR_FREQ = "scaling_cur_freq";
    static constexpr const char *SCALING_MIN_FREQ = "scaling_min_freq";
    static constexpr const char *SCALING_MAX_FREQ = "scaling_max_freq";
    static constexpr const char *CPUINFO_MIN_FREQ = "cpuinfo_min_freq";
    static constexpr const char *CPUINFO_MAX_FREQ = "cpuinfo_max_freq";
    static constexpr const char *SCALING_AVAILABLE_FREQ = "scaling_available_frequencies";
    static constexpr const char *SCALING_GOVERNOR = "scaling_governor";
    static constexpr const char *SCALING_AVAILABLE_GOV = "scaling_available_governors";
    static constexpr const char *ENERGY_PERF_AVAIL = "energy_performance_available_preferences";
    static constexpr const char *ENERGY_PERF_PREF = "energy_performance_preference";
    static constexpr const char *ONLINE_FILE = "online";
    static constexpr const char *PRESENT_FILE = "present";
};

#endif // SYSFSREADER_H
