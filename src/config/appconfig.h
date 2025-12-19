// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>

/**
 * @brief Application configuration class
 * 
 * Handles reading/writing configuration from:
 * - /etc/cpupower_gui.conf
 * - /etc/cpupower_gui.d/*.conf
 * - ~/.config/cpupower_gui/*.conf
 */
class AppConfig : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString defaultProfile READ defaultProfile WRITE setDefaultProfile NOTIFY defaultProfileChanged)
    Q_PROPERTY(bool minimizeToTray READ minimizeToTray WRITE setMinimizeToTray NOTIFY minimizeToTrayChanged)
    Q_PROPERTY(bool startMinimized READ startMinimized WRITE setStartMinimized NOTIFY startMinimizedChanged)
    Q_PROPERTY(bool allCpusDefault READ allCpusDefault WRITE setAllCpusDefault NOTIFY allCpusDefaultChanged)
    Q_PROPERTY(bool tickMarksEnabled READ tickMarksEnabled WRITE setTickMarksEnabled NOTIFY tickMarksEnabledChanged)
    Q_PROPERTY(bool frequencyTicksNumeric READ frequencyTicksNumeric WRITE setFrequencyTicksNumeric NOTIFY frequencyTicksNumericChanged)
    Q_PROPERTY(bool energyPrefPerCpu READ energyPrefPerCpu WRITE setEnergyPrefPerCpu NOTIFY energyPrefPerCpuChanged)

public:
    explicit AppConfig(QObject *parent = nullptr);
    ~AppConfig() override = default;

    // Profile settings
    QString defaultProfile() const;
    void setDefaultProfile(const QString &profile);

    // GUI settings
    bool minimizeToTray() const;
    void setMinimizeToTray(bool minimize);

    bool startMinimized() const;
    void setStartMinimized(bool minimized);

    bool allCpusDefault() const;
    void setAllCpusDefault(bool allCpus);

    bool tickMarksEnabled() const;
    void setTickMarksEnabled(bool enabled);

    bool frequencyTicksNumeric() const;
    void setFrequencyTicksNumeric(bool numeric);

    bool energyPrefPerCpu() const;
    void setEnergyPrefPerCpu(bool perCpu);

    // Persistence
    Q_INVOKABLE void save();
    Q_INVOKABLE void reload();

    // Paths
    static QString systemConfigPath();
    static QString systemConfigDir();
    static QString userConfigDir();

signals:
    void defaultProfileChanged();
    void minimizeToTrayChanged();
    void startMinimizedChanged();
    void allCpusDefaultChanged();
    void tickMarksEnabledChanged();
    void frequencyTicksNumericChanged();
    void energyPrefPerCpuChanged();
    void configChanged();

private:
    void loadSystemConfig();
    void loadUserConfig();

    QString m_defaultProfile{QStringLiteral("Balanced")};
    bool m_minimizeToTray{false};
    bool m_startMinimized{false};
    bool m_allCpusDefault{false};
    bool m_tickMarksEnabled{true};
    bool m_frequencyTicksNumeric{false};
    bool m_energyPrefPerCpu{false};
};

#endif // APPCONFIG_H
