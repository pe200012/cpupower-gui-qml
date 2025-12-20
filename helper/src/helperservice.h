// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#ifndef HELPERSERVICE_H
#define HELPERSERVICE_H

#include <QObject>
#include <QDBusContext>
#include <QDBusConnection>
#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QTimer>

/**
 * @brief D-Bus helper service for privileged CPU operations
 * 
 * This service runs as root and provides methods to modify CPU settings
 * that require elevated privileges. It uses PolicyKit for authorization.
 * 
 * The service will automatically exit after being idle for a configurable
 * timeout (default 60 seconds) to conserve resources when not in use.
 */
class HelperService : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.cpupower_gui.qt.helper")

public:
    explicit HelperService(QObject *parent = nullptr);
    ~HelperService() override = default;

    bool registerService();
    
    // Set idle timeout in seconds (0 = disabled)
    void setIdleTimeout(int seconds);

public Q_SLOTS:
    // Authorization
    int isauthorized();

    // CPU queries (read-only, no auth needed)
    QList<int> get_cpus_available();
    QList<int> get_cpus_online();
    QList<int> get_cpus_offline();
    QList<int> get_cpus_present();
    
    QStringList get_cpu_governors(int cpu);
    QStringList get_cpu_energy_preferences(int cpu);
    QString get_cpu_governor(int cpu);
    QString get_cpu_energy_preference(int cpu);
    
    QList<int> get_cpu_frequencies(int cpu);  // Returns [min, max]
    QList<int> get_cpu_limits(int cpu);       // Returns [hw_min, hw_max]
    
    int cpu_allowed_offline(int cpu);

    // CPU mutations (require auth)
    int update_cpu_settings(int cpu, int freq_min, int freq_max);
    int update_cpu_governor(int cpu, const QString &governor);
    int update_cpu_energy_prefs(int cpu, const QString &pref);
    int set_cpu_online(int cpu);
    int set_cpu_offline(int cpu);

    // Service control
    Q_NOREPLY void quit();

private Q_SLOTS:
    void onIdleTimeout();

private:
    void resetIdleTimer();
    
    bool isAuthorized(const QString &actionId = QStringLiteral("io.github.cpupower_gui.qt.apply_runtime"));
    bool checkPolkitAuthorization(const QString &sender, const QString &actionId);
    
    // Sysfs operations
    QString readSysfsFile(const QString &path) const;
    bool writeSysfsFile(const QString &path, const QString &value);
    
    QList<int> parseCpuList(const QString &content) const;
    QStringList parseList(const QString &content) const;
    
    bool isOnline(int cpu) const;
    bool isPresent(int cpu) const;
    
    QString cpuPath(int cpu) const;
    QString cpufreqPath(int cpu) const;

    // Cache authorized senders
    QMap<QString, bool> m_authorizedSenders;
    
    // Idle timeout
    QTimer m_idleTimer;
    int m_idleTimeoutSecs = 60;  // Default 60 seconds

    static constexpr const char *SYS_CPU_PATH = "/sys/devices/system/cpu";
    static constexpr const char *CPUFREQ_DIR = "cpufreq";
    static constexpr const char *SCALING_MIN_FREQ = "scaling_min_freq";
    static constexpr const char *SCALING_MAX_FREQ = "scaling_max_freq";
    static constexpr const char *CPUINFO_MIN_FREQ = "cpuinfo_min_freq";
    static constexpr const char *CPUINFO_MAX_FREQ = "cpuinfo_max_freq";
    static constexpr const char *SCALING_GOVERNOR = "scaling_governor";
    static constexpr const char *SCALING_AVAILABLE_GOV = "scaling_available_governors";
    static constexpr const char *ENERGY_PERF_AVAIL = "energy_performance_available_preferences";
    static constexpr const char *ENERGY_PERF_PREF = "energy_performance_preference";
    static constexpr const char *ONLINE_FILE = "online";
    static constexpr const char *PRESENT_FILE = "present";
};

#endif // HELPERSERVICE_H
