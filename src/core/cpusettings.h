// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#ifndef CPUSETTINGS_H
#define CPUSETTINGS_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QPair>

class DbusHelper;
class SysfsReader;

/**
 * @brief Per-CPU settings state management
 * 
 * This class tracks the current and pending settings for a single CPU,
 * providing change detection and the ability to apply or reset changes.
 */
class CpuSettings : public QObject
{
    Q_OBJECT

    // CPU index (constant)
    Q_PROPERTY(int cpu READ cpu CONSTANT)

    // Frequency properties (in MHz for QML convenience)
    Q_PROPERTY(double freqMin READ freqMin WRITE setFreqMin NOTIFY freqMinChanged)
    Q_PROPERTY(double freqMax READ freqMax WRITE setFreqMax NOTIFY freqMaxChanged)
    Q_PROPERTY(double freqMinHw READ freqMinHw CONSTANT)
    Q_PROPERTY(double freqMaxHw READ freqMaxHw CONSTANT)
    Q_PROPERTY(double currentFreq READ currentFreq NOTIFY currentFreqChanged)

    // Governor properties
    Q_PROPERTY(QString governor READ governor WRITE setGovernor NOTIFY governorChanged)
    Q_PROPERTY(QStringList governors READ governors CONSTANT)
    Q_PROPERTY(int governorIndex READ governorIndex WRITE setGovernorIndex NOTIFY governorChanged)

    // Energy preference properties
    Q_PROPERTY(QString energyPref READ energyPref WRITE setEnergyPref NOTIFY energyPrefChanged)
    Q_PROPERTY(QStringList energyPrefs READ energyPrefs CONSTANT)
    Q_PROPERTY(int energyPrefIndex READ energyPrefIndex WRITE setEnergyPrefIndex NOTIFY energyPrefChanged)
    Q_PROPERTY(bool energyPrefAvailable READ isEnergyPrefAvailable CONSTANT)

    // Online state
    Q_PROPERTY(bool online READ online WRITE setOnline NOTIFY onlineChanged)
    Q_PROPERTY(bool canGoOffline READ canGoOffline CONSTANT)

    // Change tracking
    Q_PROPERTY(bool changed READ isChanged NOTIFY changedStateChanged)
    Q_PROPERTY(bool freqChanged READ isFreqChanged NOTIFY changedStateChanged)
    Q_PROPERTY(bool governorChanged READ isGovernorChanged NOTIFY changedStateChanged)
    Q_PROPERTY(bool energyPrefChanged READ isEnergyPrefChanged NOTIFY changedStateChanged)
    Q_PROPERTY(bool onlineChanged READ isOnlineChanged NOTIFY changedStateChanged)

public:
    explicit CpuSettings(int cpu, DbusHelper *dbus, SysfsReader *sysfs, QObject *parent = nullptr);
    ~CpuSettings() override = default;

    // CPU index
    int cpu() const { return m_cpu; }

    // Frequency (MHz)
    double freqMin() const;
    void setFreqMin(double freq);
    double freqMax() const;
    void setFreqMax(double freq);
    double freqMinHw() const;
    double freqMaxHw() const;
    double currentFreq() const;

    // Frequency (kHz for D-Bus calls)
    int freqMinKHz() const { return m_newFreqMin; }
    int freqMaxKHz() const { return m_newFreqMax; }

    // Governor
    QString governor() const;
    void setGovernor(const QString &gov);
    QStringList governors() const { return m_governors; }
    int governorIndex() const;
    void setGovernorIndex(int index);

    // Energy preference
    QString energyPref() const;
    void setEnergyPref(const QString &pref);
    QStringList energyPrefs() const { return m_energyPrefs; }
    int energyPrefIndex() const;
    void setEnergyPrefIndex(int index);
    bool isEnergyPrefAvailable() const { return m_energyPrefAvailable; }

    // Online state
    bool online() const { return m_newOnline; }
    void setOnline(bool on);
    bool canGoOffline() const { return m_canGoOffline; }

    // Change tracking
    bool isChanged() const;
    bool isFreqChanged() const;
    bool isGovernorChanged() const;
    bool isEnergyPrefChanged() const;
    bool isOnlineChanged() const;

    // Actions
    Q_INVOKABLE void resetToSystem();
    Q_INVOKABLE void updateFromSystem();
    Q_INVOKABLE int applyChanges();

    // Available frequency steps for slider marks
    Q_INVOKABLE QList<int> frequencySteps() const { return m_freqSteps; }

signals:
    void freqMinChanged();
    void freqMaxChanged();
    void currentFreqChanged();
    void governorChanged();
    void energyPrefChanged();
    void onlineChanged();
    void changedStateChanged();

private:
    void loadFromSystem();
    void emitChangedSignals();

    int m_cpu;
    DbusHelper *m_dbus;
    SysfsReader *m_sysfs;

    // Hardware limits (constant after init)
    int m_freqMinHw = 0;
    int m_freqMaxHw = 0;
    QStringList m_governors;
    QStringList m_energyPrefs;
    QList<int> m_freqSteps;
    bool m_energyPrefAvailable = false;
    bool m_canGoOffline = false;

    // Original system values
    int m_origFreqMin = 0;
    int m_origFreqMax = 0;
    QString m_origGovernor;
    QString m_origEnergyPref;
    bool m_origOnline = true;

    // New (pending) values
    int m_newFreqMin = 0;
    int m_newFreqMax = 0;
    QString m_newGovernor;
    QString m_newEnergyPref;
    bool m_newOnline = true;
};

#endif // CPUSETTINGS_H
