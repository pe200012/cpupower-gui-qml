// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#ifndef CPULISTMODEL_H
#define CPULISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QPointer>

class CpuSettings;
class DbusHelper;
class SysfsReader;

/**
 * @brief List model for CPUs
 * 
 * Provides a QAbstractListModel interface for the list of available CPUs,
 * exposing CpuSettings objects for each CPU.
 */
class CpuListModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(CpuSettings* currentCpu READ currentCpu NOTIFY currentCpuChanged)
    Q_PROPERTY(bool applyToAll READ applyToAll WRITE setApplyToAll NOTIFY applyToAllChanged)
    Q_PROPERTY(bool hasChanges READ hasChanges NOTIFY hasChangesChanged)

public:
    enum Roles {
        CpuNumberRole = Qt::UserRole + 1,
        OnlineRole,
        FreqMinRole,
        FreqMaxRole,
        GovernorRole,
        CurrentFreqRole,
        ChangedRole,
        SettingsRole  // Returns CpuSettings* for direct access
    };

    explicit CpuListModel(DbusHelper *dbus, SysfsReader *sysfs, QObject *parent = nullptr);
    ~CpuListModel() override;

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Properties
    int count() const { return m_cpuSettings.count(); }
    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);
    CpuSettings* currentCpu() const;
    bool applyToAll() const { return m_applyToAll; }
    void setApplyToAll(bool apply);
    bool hasChanges() const;

    // Actions
    Q_INVOKABLE CpuSettings* cpuAt(int index) const;
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void refreshAll();
    Q_INVOKABLE void resetAll();
    Q_INVOKABLE int applyAll();
    Q_INVOKABLE void updateCurrentFrequencies();

    // Copy settings from current CPU to all others
    Q_INVOKABLE void copyCurrentToAll();

signals:
    void countChanged();
    void currentIndexChanged();
    void currentCpuChanged();
    void applyToAllChanged();
    void hasChangesChanged();
    void errorOccurred(const QString &error);

private slots:
    void onCpuSettingsChanged();

private:
    void loadCpus();
    void connectCpuSignals(CpuSettings *cpu);

    DbusHelper *m_dbus;
    SysfsReader *m_sysfs;
    QList<CpuSettings*> m_cpuSettings;
    int m_currentIndex = 0;
    bool m_applyToAll = false;
};

#endif // CPULISTMODEL_H
