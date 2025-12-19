// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#include "cpulistmodel.h"
#include "core/cpusettings.h"
#include "core/dbushelper.h"
#include "core/sysfsreader.h"

CpuListModel::CpuListModel(DbusHelper *dbus, SysfsReader *sysfs, QObject *parent)
    : QAbstractListModel(parent)
    , m_dbus(dbus)
    , m_sysfs(sysfs)
{
    loadCpus();
}

CpuListModel::~CpuListModel()
{
    qDeleteAll(m_cpuSettings);
}

void CpuListModel::loadCpus()
{
    beginResetModel();

    qDeleteAll(m_cpuSettings);
    m_cpuSettings.clear();

    const QList<int> cpus = m_sysfs->availableCpus();
    for (int cpu : cpus) {
        auto *settings = new CpuSettings(cpu, m_dbus, m_sysfs, this);
        connectCpuSignals(settings);
        m_cpuSettings.append(settings);
    }

    endResetModel();
    Q_EMIT countChanged();

    if (!m_cpuSettings.isEmpty() && m_currentIndex >= m_cpuSettings.size()) {
        setCurrentIndex(0);
    }
}

void CpuListModel::connectCpuSignals(CpuSettings *cpu)
{
    connect(cpu, &CpuSettings::freqMinChanged, this, &CpuListModel::onCpuSettingsChanged);
    connect(cpu, &CpuSettings::freqMaxChanged, this, &CpuListModel::onCpuSettingsChanged);
    connect(cpu, &CpuSettings::governorChanged, this, &CpuListModel::onCpuSettingsChanged);
    connect(cpu, &CpuSettings::onlineChanged, this, &CpuListModel::onCpuSettingsChanged);
    connect(cpu, &CpuSettings::changedStateChanged, this, &CpuListModel::hasChangesChanged);
}

void CpuListModel::onCpuSettingsChanged()
{
    auto *cpu = qobject_cast<CpuSettings*>(sender());
    if (!cpu) return;

    int row = m_cpuSettings.indexOf(cpu);
    if (row >= 0) {
        QModelIndex idx = index(row);
        Q_EMIT dataChanged(idx, idx);
    }
}

int CpuListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_cpuSettings.count();
}

QVariant CpuListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_cpuSettings.count()) {
        return QVariant();
    }

    CpuSettings *cpu = m_cpuSettings.at(index.row());

    switch (role) {
    case CpuNumberRole:
        return cpu->cpu();
    case OnlineRole:
        return cpu->online();
    case FreqMinRole:
        return cpu->freqMin();
    case FreqMaxRole:
        return cpu->freqMax();
    case GovernorRole:
        return cpu->governor();
    case CurrentFreqRole:
        return cpu->currentFreq();
    case ChangedRole:
        return cpu->isChanged();
    case SettingsRole:
        return QVariant::fromValue(cpu);
    default:
        return QVariant();
    }
}

bool CpuListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_cpuSettings.count()) {
        return false;
    }

    CpuSettings *cpu = m_cpuSettings.at(index.row());

    switch (role) {
    case OnlineRole:
        cpu->setOnline(value.toBool());
        return true;
    case FreqMinRole:
        cpu->setFreqMin(value.toDouble());
        return true;
    case FreqMaxRole:
        cpu->setFreqMax(value.toDouble());
        return true;
    default:
        return false;
    }
}

Qt::ItemFlags CpuListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QHash<int, QByteArray> CpuListModel::roleNames() const
{
    return {
        {CpuNumberRole, "cpuNumber"},
        {OnlineRole, "online"},
        {FreqMinRole, "freqMin"},
        {FreqMaxRole, "freqMax"},
        {GovernorRole, "governor"},
        {CurrentFreqRole, "currentFreq"},
        {ChangedRole, "changed"},
        {SettingsRole, "settings"}
    };
}

void CpuListModel::setCurrentIndex(int index)
{
    if (index >= 0 && index < m_cpuSettings.count() && index != m_currentIndex) {
        m_currentIndex = index;
        Q_EMIT currentIndexChanged();
        Q_EMIT currentCpuChanged();
    }
}

CpuSettings* CpuListModel::currentCpu() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_cpuSettings.count()) {
        return m_cpuSettings.at(m_currentIndex);
    }
    return nullptr;
}

void CpuListModel::setApplyToAll(bool apply)
{
    if (m_applyToAll != apply) {
        m_applyToAll = apply;
        Q_EMIT applyToAllChanged();
    }
}

bool CpuListModel::hasChanges() const
{
    for (const auto *cpu : m_cpuSettings) {
        if (cpu->isChanged()) {
            return true;
        }
    }
    return false;
}

CpuSettings* CpuListModel::cpuAt(int index) const
{
    if (index >= 0 && index < m_cpuSettings.count()) {
        return m_cpuSettings.at(index);
    }
    return nullptr;
}

void CpuListModel::refresh()
{
    if (m_currentIndex >= 0 && m_currentIndex < m_cpuSettings.count()) {
        m_cpuSettings.at(m_currentIndex)->updateFromSystem();
    }
}

void CpuListModel::refreshAll()
{
    for (auto *cpu : m_cpuSettings) {
        cpu->updateFromSystem();
    }
}

void CpuListModel::resetAll()
{
    for (auto *cpu : m_cpuSettings) {
        cpu->resetToSystem();
    }
}

int CpuListModel::applyAll()
{
    int result = 0;

    for (auto *cpu : m_cpuSettings) {
        if (cpu->isChanged()) {
            int ret = cpu->applyChanges();
            if (ret != 0) {
                result = ret;
                // Continue trying to apply other CPUs
            }
        }
    }

    return result;
}

void CpuListModel::updateCurrentFrequencies()
{
    // Update current frequency for display (called by timer)
    for (int i = 0; i < m_cpuSettings.count(); ++i) {
        QModelIndex idx = index(i);
        Q_EMIT dataChanged(idx, idx, {CurrentFreqRole});
    }
}

void CpuListModel::copyCurrentToAll()
{
    CpuSettings *current = currentCpu();
    if (!current) return;

    for (auto *cpu : m_cpuSettings) {
        if (cpu != current) {
            cpu->setFreqMin(current->freqMin());
            cpu->setFreqMax(current->freqMax());
            cpu->setGovernor(current->governor());
            cpu->setOnline(current->online());
            if (current->isEnergyPrefAvailable() && cpu->isEnergyPrefAvailable()) {
                cpu->setEnergyPref(current->energyPref());
            }
        }
    }
}
