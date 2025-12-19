// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#include "cpusettings.h"
#include "dbushelper.h"
#include "sysfsreader.h"

CpuSettings::CpuSettings(int cpu, DbusHelper *dbus, SysfsReader *sysfs, QObject *parent)
    : QObject(parent)
    , m_cpu(cpu)
    , m_dbus(dbus)
    , m_sysfs(sysfs)
{
    loadFromSystem();
}

void CpuSettings::loadFromSystem()
{
    // Hardware limits (constant)
    auto limits = m_sysfs->freqLimits(m_cpu);
    m_freqMinHw = limits.first;
    m_freqMaxHw = limits.second;

    // Available governors and energy prefs
    m_governors = m_sysfs->availableGovernors(m_cpu);
    m_energyPrefs = m_sysfs->availableEnergyPrefs(m_cpu);
    m_energyPrefAvailable = m_sysfs->isEnergyPrefAvailable(m_cpu);
    m_freqSteps = m_sysfs->availableFrequencies(m_cpu);
    m_canGoOffline = m_dbus->cpuAllowedOffline(m_cpu);

    // Current values from system
    updateFromSystem();
}

void CpuSettings::updateFromSystem()
{
    auto scalingFreqs = m_sysfs->scalingFreqs(m_cpu);
    m_origFreqMin = scalingFreqs.first;
    m_origFreqMax = scalingFreqs.second;
    m_origGovernor = m_sysfs->currentGovernor(m_cpu);
    m_origEnergyPref = m_sysfs->currentEnergyPref(m_cpu);
    m_origOnline = m_sysfs->isOnline(m_cpu);

    // Also update new values
    m_newFreqMin = m_origFreqMin;
    m_newFreqMax = m_origFreqMax;
    m_newGovernor = m_origGovernor;
    m_newEnergyPref = m_origEnergyPref;
    m_newOnline = m_origOnline;

    // Update available governors (may change when CPU goes online/offline)
    m_governors = m_sysfs->availableGovernors(m_cpu);

    emitChangedSignals();
}

void CpuSettings::resetToSystem()
{
    m_newFreqMin = m_origFreqMin;
    m_newFreqMax = m_origFreqMax;
    m_newGovernor = m_origGovernor;
    m_newEnergyPref = m_origEnergyPref;
    m_newOnline = m_origOnline;

    emitChangedSignals();
}

void CpuSettings::emitChangedSignals()
{
    Q_EMIT freqMinChanged();
    Q_EMIT freqMaxChanged();
    Q_EMIT currentFreqChanged();
    Q_EMIT governorChanged();
    Q_EMIT energyPrefChanged();
    Q_EMIT onlineChanged();
    Q_EMIT changedStateChanged();
}

// Frequency accessors (MHz)
double CpuSettings::freqMin() const
{
    return m_newFreqMin / 1000.0;
}

void CpuSettings::setFreqMin(double freq)
{
    int freqKHz = static_cast<int>(freq * 1000);
    if (m_newFreqMin != freqKHz) {
        m_newFreqMin = freqKHz;
        Q_EMIT freqMinChanged();
        Q_EMIT changedStateChanged();
    }
}

double CpuSettings::freqMax() const
{
    return m_newFreqMax / 1000.0;
}

void CpuSettings::setFreqMax(double freq)
{
    int freqKHz = static_cast<int>(freq * 1000);
    if (m_newFreqMax != freqKHz) {
        m_newFreqMax = freqKHz;
        Q_EMIT freqMaxChanged();
        Q_EMIT changedStateChanged();
    }
}

double CpuSettings::freqMinHw() const
{
    return m_freqMinHw / 1000.0;
}

double CpuSettings::freqMaxHw() const
{
    return m_freqMaxHw / 1000.0;
}

double CpuSettings::currentFreq() const
{
    return m_sysfs->currentFreq(m_cpu) / 1000.0;
}

// Governor accessors
QString CpuSettings::governor() const
{
    return m_newGovernor;
}

void CpuSettings::setGovernor(const QString &gov)
{
    if (m_newGovernor != gov && m_governors.contains(gov)) {
        m_newGovernor = gov;
        Q_EMIT governorChanged();
        Q_EMIT changedStateChanged();
    }
}

int CpuSettings::governorIndex() const
{
    return m_governors.indexOf(m_newGovernor);
}

void CpuSettings::setGovernorIndex(int index)
{
    if (index >= 0 && index < m_governors.size()) {
        setGovernor(m_governors.at(index));
    }
}

// Energy preference accessors
QString CpuSettings::energyPref() const
{
    return m_newEnergyPref;
}

void CpuSettings::setEnergyPref(const QString &pref)
{
    if (m_newEnergyPref != pref && m_energyPrefs.contains(pref)) {
        m_newEnergyPref = pref;
        Q_EMIT energyPrefChanged();
        Q_EMIT changedStateChanged();
    }
}

int CpuSettings::energyPrefIndex() const
{
    return m_energyPrefs.indexOf(m_newEnergyPref);
}

void CpuSettings::setEnergyPrefIndex(int index)
{
    if (index >= 0 && index < m_energyPrefs.size()) {
        setEnergyPref(m_energyPrefs.at(index));
    }
}

// Online state
void CpuSettings::setOnline(bool on)
{
    if (m_newOnline != on) {
        m_newOnline = on;
        Q_EMIT onlineChanged();
        Q_EMIT changedStateChanged();
    }
}

// Change tracking
bool CpuSettings::isChanged() const
{
    return isFreqChanged() || isGovernorChanged() || isEnergyPrefChanged() || isOnlineChanged();
}

bool CpuSettings::isFreqChanged() const
{
    return m_newFreqMin != m_origFreqMin || m_newFreqMax != m_origFreqMax;
}

bool CpuSettings::isGovernorChanged() const
{
    return m_newGovernor != m_origGovernor;
}

bool CpuSettings::isEnergyPrefChanged() const
{
    return m_newEnergyPref != m_origEnergyPref;
}

bool CpuSettings::isOnlineChanged() const
{
    return m_newOnline != m_origOnline;
}

int CpuSettings::applyChanges()
{
    int ret = 0;

    // Handle online/offline first
    if (isOnlineChanged()) {
        if (m_newOnline) {
            ret = m_dbus->setCpuOnline(m_cpu);
        } else if (m_canGoOffline) {
            ret = m_dbus->setCpuOffline(m_cpu);
        }

        if (ret != 0) {
            return ret;
        }
    }

    // Only apply other settings if CPU is online
    if (m_newOnline) {
        if (isFreqChanged()) {
            ret = m_dbus->updateCpuSettings(m_cpu, m_newFreqMin, m_newFreqMax);
            if (ret != 0) {
                return -13; // Setting frequencies failed
            }
        }

        if (isGovernorChanged()) {
            ret = m_dbus->updateCpuGovernor(m_cpu, m_newGovernor);
            if (ret != 0) {
                return -11; // Setting governor failed
            }
        }

        if (isEnergyPrefChanged() && m_energyPrefAvailable) {
            ret = m_dbus->updateCpuEnergyPrefs(m_cpu, m_newEnergyPref);
            if (ret != 0) {
                return -12; // Setting energy preferences failed
            }
        }
    }

    // Refresh from system
    updateFromSystem();

    return 0;
}
