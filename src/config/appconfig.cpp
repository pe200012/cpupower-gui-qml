// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#include "appconfig.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>

AppConfig::AppConfig(QObject *parent)
    : QObject(parent)
{
    reload();
}

QString AppConfig::defaultProfile() const
{
    return m_defaultProfile;
}

void AppConfig::setDefaultProfile(const QString &profile)
{
    if (m_defaultProfile == profile) {
        return;
    }
    m_defaultProfile = profile;
    emit defaultProfileChanged();
    emit configChanged();
}

bool AppConfig::minimizeToTray() const
{
    return m_minimizeToTray;
}

void AppConfig::setMinimizeToTray(bool minimize)
{
    if (m_minimizeToTray == minimize) {
        return;
    }
    m_minimizeToTray = minimize;
    emit minimizeToTrayChanged();
    emit configChanged();
}

bool AppConfig::startMinimized() const
{
    return m_startMinimized;
}

void AppConfig::setStartMinimized(bool minimized)
{
    if (m_startMinimized == minimized) {
        return;
    }
    m_startMinimized = minimized;
    emit startMinimizedChanged();
    emit configChanged();
}

bool AppConfig::allCpusDefault() const
{
    return m_allCpusDefault;
}

void AppConfig::setAllCpusDefault(bool allCpus)
{
    if (m_allCpusDefault == allCpus) {
        return;
    }
    m_allCpusDefault = allCpus;
    emit allCpusDefaultChanged();
    emit configChanged();
}

bool AppConfig::tickMarksEnabled() const
{
    return m_tickMarksEnabled;
}

void AppConfig::setTickMarksEnabled(bool enabled)
{
    if (m_tickMarksEnabled == enabled) {
        return;
    }
    m_tickMarksEnabled = enabled;
    emit tickMarksEnabledChanged();
    emit configChanged();
}

bool AppConfig::frequencyTicksNumeric() const
{
    return m_frequencyTicksNumeric;
}

void AppConfig::setFrequencyTicksNumeric(bool numeric)
{
    if (m_frequencyTicksNumeric == numeric) {
        return;
    }
    m_frequencyTicksNumeric = numeric;
    emit frequencyTicksNumericChanged();
    emit configChanged();
}

bool AppConfig::energyPrefPerCpu() const
{
    return m_energyPrefPerCpu;
}

void AppConfig::setEnergyPrefPerCpu(bool perCpu)
{
    if (m_energyPrefPerCpu == perCpu) {
        return;
    }
    m_energyPrefPerCpu = perCpu;
    emit energyPrefPerCpuChanged();
    emit configChanged();
}

void AppConfig::save()
{
    const QString userDir = userConfigDir();
    QDir dir(userDir);
    if (!dir.exists()) {
        dir.mkpath(userDir);
    }

    const QString configFile = userDir + QStringLiteral("/00-cpg.conf");
    QSettings settings(configFile, QSettings::IniFormat);

    settings.beginGroup(QStringLiteral("Profile"));
    settings.setValue(QStringLiteral("profile"), m_defaultProfile);
    settings.endGroup();

    settings.beginGroup(QStringLiteral("GUI"));
    settings.setValue(QStringLiteral("minimize_to_tray"), m_minimizeToTray);
    settings.setValue(QStringLiteral("start_minimized"), m_startMinimized);
    settings.setValue(QStringLiteral("all_cpus_default"), m_allCpusDefault);
    settings.setValue(QStringLiteral("tick_marks_enabled"), m_tickMarksEnabled);
    settings.setValue(QStringLiteral("frequency_ticks_numeric"), m_frequencyTicksNumeric);
    settings.setValue(QStringLiteral("energy_pref_per_cpu"), m_energyPrefPerCpu);
    settings.endGroup();

    settings.sync();
}

void AppConfig::reload()
{
    // Reset to defaults
    m_defaultProfile = QStringLiteral("Balanced");
    m_minimizeToTray = false;
    m_startMinimized = false;
    m_allCpusDefault = false;
    m_tickMarksEnabled = true;
    m_frequencyTicksNumeric = false;
    m_energyPrefPerCpu = false;

    loadSystemConfig();
    loadUserConfig();

    emit defaultProfileChanged();
    emit minimizeToTrayChanged();
    emit startMinimizedChanged();
    emit allCpusDefaultChanged();
    emit tickMarksEnabledChanged();
    emit frequencyTicksNumericChanged();
    emit energyPrefPerCpuChanged();
    emit configChanged();
}

QString AppConfig::systemConfigPath()
{
    return QStringLiteral("/etc/cpupower_gui.conf");
}

QString AppConfig::systemConfigDir()
{
    return QStringLiteral("/etc/cpupower_gui.d");
}

QString AppConfig::userConfigDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) 
           + QStringLiteral("/cpupower_gui");
}

void AppConfig::loadSystemConfig()
{
    // Load main system config
    const QString sysConf = systemConfigPath();
    if (QFile::exists(sysConf)) {
        QSettings settings(sysConf, QSettings::IniFormat);

        settings.beginGroup(QStringLiteral("Profile"));
        m_defaultProfile = settings.value(QStringLiteral("profile"), m_defaultProfile).toString();
        settings.endGroup();

        settings.beginGroup(QStringLiteral("GUI"));
        m_minimizeToTray = settings.value(QStringLiteral("minimize_to_tray"), m_minimizeToTray).toBool();
        m_startMinimized = settings.value(QStringLiteral("start_minimized"), m_startMinimized).toBool();
        m_allCpusDefault = settings.value(QStringLiteral("all_cpus_default"), m_allCpusDefault).toBool();
        m_tickMarksEnabled = settings.value(QStringLiteral("tick_marks_enabled"), m_tickMarksEnabled).toBool();
        m_frequencyTicksNumeric = settings.value(QStringLiteral("frequency_ticks_numeric"), m_frequencyTicksNumeric).toBool();
        m_energyPrefPerCpu = settings.value(QStringLiteral("energy_pref_per_cpu"), m_energyPrefPerCpu).toBool();
        settings.endGroup();
    }

    // Load drop-in configs from /etc/cpupower_gui.d/*.conf
    const QString sysDir = systemConfigDir();
    QDir dir(sysDir);
    if (dir.exists()) {
        const QStringList confFiles = dir.entryList({QStringLiteral("*.conf")}, QDir::Files, QDir::Name);
        for (const QString &file : confFiles) {
            QSettings settings(dir.absoluteFilePath(file), QSettings::IniFormat);

            settings.beginGroup(QStringLiteral("Profile"));
            if (settings.contains(QStringLiteral("profile"))) {
                m_defaultProfile = settings.value(QStringLiteral("profile")).toString();
            }
            settings.endGroup();

            settings.beginGroup(QStringLiteral("GUI"));
            if (settings.contains(QStringLiteral("minimize_to_tray"))) {
                m_minimizeToTray = settings.value(QStringLiteral("minimize_to_tray")).toBool();
            }
            if (settings.contains(QStringLiteral("start_minimized"))) {
                m_startMinimized = settings.value(QStringLiteral("start_minimized")).toBool();
            }
            if (settings.contains(QStringLiteral("all_cpus_default"))) {
                m_allCpusDefault = settings.value(QStringLiteral("all_cpus_default")).toBool();
            }
            if (settings.contains(QStringLiteral("tick_marks_enabled"))) {
                m_tickMarksEnabled = settings.value(QStringLiteral("tick_marks_enabled")).toBool();
            }
            if (settings.contains(QStringLiteral("frequency_ticks_numeric"))) {
                m_frequencyTicksNumeric = settings.value(QStringLiteral("frequency_ticks_numeric")).toBool();
            }
            if (settings.contains(QStringLiteral("energy_pref_per_cpu"))) {
                m_energyPrefPerCpu = settings.value(QStringLiteral("energy_pref_per_cpu")).toBool();
            }
            settings.endGroup();
        }
    }
}

void AppConfig::loadUserConfig()
{
    const QString userDir = userConfigDir();
    QDir dir(userDir);
    if (!dir.exists()) {
        return;
    }

    // Load user configs from ~/.config/cpupower_gui/*.conf
    const QStringList confFiles = dir.entryList({QStringLiteral("*.conf")}, QDir::Files, QDir::Name);
    for (const QString &file : confFiles) {
        QSettings settings(dir.absoluteFilePath(file), QSettings::IniFormat);

        settings.beginGroup(QStringLiteral("Profile"));
        if (settings.contains(QStringLiteral("profile"))) {
            m_defaultProfile = settings.value(QStringLiteral("profile")).toString();
        }
        settings.endGroup();

        settings.beginGroup(QStringLiteral("GUI"));
        if (settings.contains(QStringLiteral("minimize_to_tray"))) {
            m_minimizeToTray = settings.value(QStringLiteral("minimize_to_tray")).toBool();
        }
        if (settings.contains(QStringLiteral("start_minimized"))) {
            m_startMinimized = settings.value(QStringLiteral("start_minimized")).toBool();
        }
        if (settings.contains(QStringLiteral("all_cpus_default"))) {
            m_allCpusDefault = settings.value(QStringLiteral("all_cpus_default")).toBool();
        }
        if (settings.contains(QStringLiteral("tick_marks_enabled"))) {
            m_tickMarksEnabled = settings.value(QStringLiteral("tick_marks_enabled")).toBool();
        }
        if (settings.contains(QStringLiteral("frequency_ticks_numeric"))) {
            m_frequencyTicksNumeric = settings.value(QStringLiteral("frequency_ticks_numeric")).toBool();
        }
        if (settings.contains(QStringLiteral("energy_pref_per_cpu"))) {
            m_energyPrefPerCpu = settings.value(QStringLiteral("energy_pref_per_cpu")).toBool();
        }
        settings.endGroup();
    }
}
