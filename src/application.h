// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>
#include <QQmlApplicationEngine>
#include <QTimer>
#include <memory>

// Include full headers for types exposed via Q_PROPERTY
#include "core/sysfsreader.h"
#include "core/dbushelper.h"
#include "config/appconfig.h"
#include "config/profilemanager.h"
#include "models/cpulistmodel.h"
#include "models/profilemodel.h"
#include "models/governormodel.h"
#include "models/energyprefmodel.h"

class TrayIcon;

/**
 * @brief Main application controller
 * 
 * Owns all backend objects and exposes them to QML.
 * Coordinates between the UI and the system.
 */
class Application : public QObject
{
    Q_OBJECT

    // Expose models to QML
    Q_PROPERTY(CpuListModel* cpuModel READ cpuModel CONSTANT)
    Q_PROPERTY(ProfileModel* profileModel READ profileModel CONSTANT)
    Q_PROPERTY(GovernorModel* governorModel READ governorModel CONSTANT)
    Q_PROPERTY(EnergyPrefModel* energyPrefModel READ energyPrefModel CONSTANT)

    // Expose managers
    Q_PROPERTY(AppConfig* config READ config CONSTANT)
    Q_PROPERTY(ProfileManager* profileManager READ profileManager CONSTANT)
    Q_PROPERTY(DbusHelper* dbusHelper READ dbusHelper CONSTANT)
    Q_PROPERTY(SysfsReader* sysfsReader READ sysfsReader CONSTANT)

    // Current CPU selection
    Q_PROPERTY(int currentCpu READ currentCpu WRITE setCurrentCpu NOTIFY currentCpuChanged)
    Q_PROPERTY(bool allCpusSelected READ allCpusSelected WRITE setAllCpusSelected NOTIFY allCpusSelectedChanged)

    // Current CPU state (for selected CPU)
    Q_PROPERTY(qint64 currentMinFreq READ currentMinFreq NOTIFY currentCpuStateChanged)
    Q_PROPERTY(qint64 currentMaxFreq READ currentMaxFreq NOTIFY currentCpuStateChanged)
    Q_PROPERTY(qint64 hardwareMinFreq READ hardwareMinFreq NOTIFY currentCpuStateChanged)
    Q_PROPERTY(qint64 hardwareMaxFreq READ hardwareMaxFreq NOTIFY currentCpuStateChanged)
    Q_PROPERTY(QString currentGovernor READ currentGovernor NOTIFY currentCpuStateChanged)
    Q_PROPERTY(QString currentEnergyPref READ currentEnergyPref NOTIFY currentCpuStateChanged)
    Q_PROPERTY(bool energyPrefAvailable READ energyPrefAvailable NOTIFY currentCpuStateChanged)
    Q_PROPERTY(bool cpuOnline READ cpuOnline NOTIFY currentCpuStateChanged)

    // Status
    Q_PROPERTY(bool hasUnsavedChanges READ hasUnsavedChanges NOTIFY unsavedChangesChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit Application(QObject *parent = nullptr);
    ~Application() override;

    // Initialize the QML engine
    void setupQmlEngine(QQmlApplicationEngine *engine);

    // Show main window (for tray icon activation)
    Q_INVOKABLE void showMainWindow();

    // Model accessors
    CpuListModel *cpuModel() const { return m_cpuModel.get(); }
    ProfileModel *profileModel() const { return m_profileModel.get(); }
    GovernorModel *governorModel() const { return m_governorModel.get(); }
    EnergyPrefModel *energyPrefModel() const { return m_energyPrefModel.get(); }

    // Manager accessors
    AppConfig *config() const { return m_config.get(); }
    ProfileManager *profileManager() const { return m_profileManager.get(); }
    DbusHelper *dbusHelper() const { return m_dbusHelper.get(); }
    SysfsReader *sysfsReader() const { return m_sysfsReader.get(); }

    // CPU selection
    int currentCpu() const { return m_currentCpu; }
    void setCurrentCpu(int cpu);
    bool allCpusSelected() const { return m_allCpusSelected; }
    void setAllCpusSelected(bool all);

    // Current CPU state
    qint64 currentMinFreq() const;
    qint64 currentMaxFreq() const;
    qint64 hardwareMinFreq() const;
    qint64 hardwareMaxFreq() const;
    QString currentGovernor() const;
    QString currentEnergyPref() const;
    bool energyPrefAvailable() const;
    bool cpuOnline() const;

    // Status
    bool hasUnsavedChanges() const { return m_hasUnsavedChanges; }
    QString statusMessage() const { return m_statusMessage; }

    // Actions (invokable from QML)
    Q_INVOKABLE void setMinFrequency(qint64 freqKhz);
    Q_INVOKABLE void setMaxFrequency(qint64 freqKhz);
    Q_INVOKABLE void setGovernor(const QString &governor);
    Q_INVOKABLE void setEnergyPref(const QString &pref);
    Q_INVOKABLE void setCpuOnline(bool online);

    Q_INVOKABLE void applyChanges();
    Q_INVOKABLE void resetChanges();
    Q_INVOKABLE void applyProfile(const QString &profileName);
    Q_INVOKABLE void refreshCpuInfo();

signals:
    void currentCpuChanged();
    void allCpusSelectedChanged();
    void currentCpuStateChanged();
    void unsavedChangesChanged();
    void statusMessageChanged();
    void errorOccurred(const QString &message);
    void applySuccess();
    void applyFailed(const QString &reason);

private slots:
    void onDbusHelperReady(bool ready);
    void onDbusError(const QString &error);

private:
    void initializeBackend();
    void updateGovernorModel();
    void updateEnergyPrefModel();
    void setStatusMessage(const QString &msg);
    void setUnsavedChanges(bool changed);

    // Backend objects
    std::unique_ptr<SysfsReader> m_sysfsReader;
    std::unique_ptr<DbusHelper> m_dbusHelper;
    std::unique_ptr<AppConfig> m_config;
    std::unique_ptr<ProfileManager> m_profileManager;

    // Models
    std::unique_ptr<CpuListModel> m_cpuModel;
    std::unique_ptr<ProfileModel> m_profileModel;
    std::unique_ptr<GovernorModel> m_governorModel;
    std::unique_ptr<EnergyPrefModel> m_energyPrefModel;

    // Tray
    std::unique_ptr<TrayIcon> m_trayIcon;

    // State
    int m_currentCpu{0};
    bool m_allCpusSelected{false};
    bool m_hasUnsavedChanges{false};
    QString m_statusMessage;

    // Pending changes (to be applied on applyChanges())
    qint64 m_pendingMinFreq{0};
    qint64 m_pendingMaxFreq{0};
    QString m_pendingGovernor;
    QString m_pendingEnergyPref;
    bool m_pendingOnline{true};

    // Flags indicating which changes are pending
    bool m_hasPendingMinFreq{false};
    bool m_hasPendingMaxFreq{false};
    bool m_hasPendingGovernor{false};
    bool m_hasPendingEnergyPref{false};
    bool m_hasPendingOnline{false};

    // Helper methods
    void clearPendingChanges();
    bool applyChangesToCpu(int cpu);

    // Frequency monitoring timer
    QTimer *m_freqMonitorTimer{nullptr};
    static constexpr int FREQ_MONITOR_INTERVAL_MS = 500;

    // QML engine reference for window management
    QQmlApplicationEngine *m_engine{nullptr};
};

#endif // APPLICATION_H
