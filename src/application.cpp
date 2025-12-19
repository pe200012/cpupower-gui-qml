// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#include "application.h"

#include "core/sysfsreader.h"
#include "core/dbushelper.h"
#include "core/cpusettings.h"
#include "config/appconfig.h"
#include "config/profilemanager.h"
#include "models/cpulistmodel.h"
#include "models/profilemodel.h"
#include "models/governormodel.h"
#include "models/energyprefmodel.h"
#include "tray/trayicon.h"

#include <QQmlContext>
#include <QTimer>
#include <QQuickWindow>
#include <QDebug>

Application::Application(QObject *parent)
    : QObject(parent)
{
    initializeBackend();
}

Application::~Application() = default;

void Application::initializeBackend()
{
    // Create core objects
    m_sysfsReader = std::make_unique<SysfsReader>(this);
    m_dbusHelper = std::make_unique<DbusHelper>(this);
    m_config = std::make_unique<AppConfig>(this);
    m_profileManager = std::make_unique<ProfileManager>(m_sysfsReader.get(), this);

    // Create models
    m_cpuModel = std::make_unique<CpuListModel>(m_dbusHelper.get(), m_sysfsReader.get(), this);
    m_profileModel = std::make_unique<ProfileModel>(m_profileManager.get(), this);
    m_governorModel = std::make_unique<GovernorModel>(this);
    m_energyPrefModel = std::make_unique<EnergyPrefModel>(this);

    // Create tray icon
    m_trayIcon = std::make_unique<TrayIcon>(this);

    // Connect D-Bus signals
    connect(m_dbusHelper.get(), &DbusHelper::helperReady, this, &Application::onDbusHelperReady);
    connect(m_dbusHelper.get(), &DbusHelper::errorOccurred, this, &Application::onDbusError);
    connect(m_dbusHelper.get(), &DbusHelper::batchCompleted, this, &Application::onBatchCompleted);

    // Initialize models for first CPU
    if (!m_sysfsReader->availableCpus().isEmpty()) {
        m_currentCpu = m_sysfsReader->availableCpus().first();
        updateGovernorModel();
        updateEnergyPrefModel();
    }

    setStatusMessage(tr("Ready"));

    // Start frequency monitoring timer
    m_freqMonitorTimer = new QTimer(this);
    connect(m_freqMonitorTimer, &QTimer::timeout, this, [this]() {
        m_cpuModel->updateCurrentFrequencies();
    });
    m_freqMonitorTimer->start(FREQ_MONITOR_INTERVAL_MS);
}

void Application::setupQmlEngine(QQmlApplicationEngine *engine)
{
    m_engine = engine;
    QQmlContext *context = engine->rootContext();

    // Expose application object
    context->setContextProperty(QStringLiteral("app"), this);

    // Expose models
    context->setContextProperty(QStringLiteral("cpuModel"), m_cpuModel.get());
    context->setContextProperty(QStringLiteral("profileModel"), m_profileModel.get());
    context->setContextProperty(QStringLiteral("governorModel"), m_governorModel.get());
    context->setContextProperty(QStringLiteral("energyPrefModel"), m_energyPrefModel.get());

    // Expose managers
    context->setContextProperty(QStringLiteral("appConfig"), m_config.get());
    context->setContextProperty(QStringLiteral("profileManager"), m_profileManager.get());

    // Connect tray icon signals
    connect(m_trayIcon.get(), &TrayIcon::activateRequested, this, &Application::showMainWindow);
    connect(m_trayIcon.get(), &TrayIcon::profileSelected, this, &Application::applyProfile);
    connect(m_trayIcon.get(), &TrayIcon::quitRequested, qApp, &QCoreApplication::quit);

    // Set profile manager for tray
    m_trayIcon->setProfileManager(m_profileManager.get());
}

void Application::showMainWindow()
{
    if (!m_engine) {
        return;
    }

    const QList<QObject *> rootObjects = m_engine->rootObjects();
    for (QObject *obj : rootObjects) {
        QQuickWindow *window = qobject_cast<QQuickWindow *>(obj);
        if (window) {
            window->show();
            window->raise();
            window->requestActivate();
            return;
        }
    }
}

void Application::setCurrentCpu(int cpu)
{
    if (m_currentCpu == cpu) {
        return;
    }

    m_currentCpu = cpu;
    m_allCpusSelected = false;

    updateGovernorModel();
    updateEnergyPrefModel();

    emit currentCpuChanged();
    emit allCpusSelectedChanged();
    emit currentCpuStateChanged();
}

void Application::setAllCpusSelected(bool all)
{
    if (m_allCpusSelected == all) {
        return;
    }

    m_allCpusSelected = all;
    emit allCpusSelectedChanged();
}

qint64 Application::currentMinFreq() const
{
    return m_sysfsReader->minFrequency(m_currentCpu);
}

qint64 Application::currentMaxFreq() const
{
    return m_sysfsReader->maxFrequency(m_currentCpu);
}

qint64 Application::hardwareMinFreq() const
{
    return m_sysfsReader->minFrequencyHardware(m_currentCpu);
}

qint64 Application::hardwareMaxFreq() const
{
    return m_sysfsReader->maxFrequencyHardware(m_currentCpu);
}

QString Application::currentGovernor() const
{
    return m_sysfsReader->currentGovernor(m_currentCpu);
}

QString Application::currentEnergyPref() const
{
    return m_sysfsReader->currentEnergyPref(m_currentCpu);
}

bool Application::energyPrefAvailable() const
{
    return m_sysfsReader->isEnergyPrefAvailable(m_currentCpu);
}

bool Application::cpuOnline() const
{
    return m_sysfsReader->isOnline(m_currentCpu);
}

void Application::setMinFrequency(qint64 freqKhz)
{
    m_pendingMinFreq = freqKhz;
    m_hasPendingMinFreq = true;
    setUnsavedChanges(true);
}

void Application::setMaxFrequency(qint64 freqKhz)
{
    m_pendingMaxFreq = freqKhz;
    m_hasPendingMaxFreq = true;
    setUnsavedChanges(true);
}

void Application::setGovernor(const QString &governor)
{
    m_pendingGovernor = governor;
    m_hasPendingGovernor = true;
    setUnsavedChanges(true);
}

void Application::setEnergyPref(const QString &pref)
{
    m_pendingEnergyPref = pref;
    m_hasPendingEnergyPref = true;
    setUnsavedChanges(true);
}

void Application::setCpuOnline(bool online)
{
    m_pendingOnline = online;
    m_hasPendingOnline = true;
    setUnsavedChanges(true);
}

void Application::applyChanges()
{
    if (!m_hasUnsavedChanges) {
        setStatusMessage(tr("No changes to apply"));
        return;
    }

    // Check if D-Bus helper is available
    if (!m_dbusHelper->isConnected()) {
        setStatusMessage(tr("D-Bus helper not connected - cannot apply changes"));
        emit applyFailed(tr("D-Bus helper not available"));
        return;
    }

    // Check if an operation is already in progress
    if (m_dbusHelper->isOperationInProgress()) {
        setStatusMessage(tr("Operation already in progress"));
        return;
    }

    setStatusMessage(tr("Applying changes..."));

    QList<int> cpusToApply;
    if (m_allCpusSelected) {
        cpusToApply = m_sysfsReader->availableCpus();
    } else {
        cpusToApply.append(m_currentCpu);
    }

    // Begin batch mode - queue all operations
    m_dbusHelper->beginBatch();

    for (int cpu : cpusToApply) {
        // Apply frequency settings (min and max together)
        if (m_hasPendingMinFreq || m_hasPendingMaxFreq) {
            qint64 fmin = m_hasPendingMinFreq ? m_pendingMinFreq : m_sysfsReader->minFrequency(cpu);
            qint64 fmax = m_hasPendingMaxFreq ? m_pendingMaxFreq : m_sysfsReader->maxFrequency(cpu);

            m_dbusHelper->updateCpuSettingsAsync(cpu, static_cast<int>(fmin), static_cast<int>(fmax));
        }

        // Apply governor
        if (m_hasPendingGovernor && !m_pendingGovernor.isEmpty()) {
            m_dbusHelper->updateCpuGovernorAsync(cpu, m_pendingGovernor);
        }

        // Apply energy preference
        if (m_hasPendingEnergyPref && !m_pendingEnergyPref.isEmpty()) {
            if (m_sysfsReader->isEnergyPrefAvailable(cpu)) {
                m_dbusHelper->updateCpuEnergyPrefsAsync(cpu, m_pendingEnergyPref);
            }
        }

        // Apply online/offline state (CPU 0 cannot be offlined)
        if (m_hasPendingOnline && cpu != 0) {
            if (m_pendingOnline) {
                m_dbusHelper->setCpuOnlineAsync(cpu);
            } else {
                m_dbusHelper->setCpuOfflineAsync(cpu);
            }
        }
    }

    // Clear pending changes - results will be handled in onBatchCompleted
    clearPendingChanges();
    setUnsavedChanges(false);

    // End batch and start processing - completion will trigger onBatchCompleted
    m_dbusHelper->endBatch();
}

void Application::clearPendingChanges()
{
    m_pendingMinFreq = 0;
    m_pendingMaxFreq = 0;
    m_pendingGovernor.clear();
    m_pendingEnergyPref.clear();
    m_pendingOnline = true;

    m_hasPendingMinFreq = false;
    m_hasPendingMaxFreq = false;
    m_hasPendingGovernor = false;
    m_hasPendingEnergyPref = false;
    m_hasPendingOnline = false;
}

void Application::onBatchCompleted(bool allSucceeded, const QStringList &errors)
{
    // Refresh CPU info to show current state
    refreshCpuInfo();

    if (allSucceeded) {
        setStatusMessage(tr("Changes applied successfully"));
        emit applySuccess();
    } else {
        setStatusMessage(tr("Some changes failed to apply"));
        emit applyFailed(errors.join(QStringLiteral("; ")));
    }
}

void Application::resetChanges()
{
    clearPendingChanges();
    setUnsavedChanges(false);
    emit currentCpuStateChanged();
    setStatusMessage(tr("Changes discarded"));
}

void Application::applyProfile(const QString &profileName)
{
    const Profile *profile = m_profileManager->profile(profileName);
    if (!profile) {
        emit errorOccurred(tr("Profile not found: %1").arg(profileName));
        return;
    }

    // Check if D-Bus helper is available
    if (!m_dbusHelper->isConnected()) {
        setStatusMessage(tr("D-Bus helper not connected - cannot apply profile"));
        emit applyFailed(tr("D-Bus helper not available"));
        return;
    }

    // Check if an operation is already in progress
    if (m_dbusHelper->isOperationInProgress()) {
        setStatusMessage(tr("Operation already in progress"));
        return;
    }

    setStatusMessage(tr("Applying profile: %1").arg(profileName));

    // Begin batch mode - queue all operations
    m_dbusHelper->beginBatch();

    // Apply settings for each CPU in the profile
    for (auto it = profile->settings.constBegin(); it != profile->settings.constEnd(); ++it) {
        int cpu = it.key();
        const CpuProfileEntry &entry = it.value();

        // Check if this CPU exists
        if (!m_sysfsReader->availableCpus().contains(cpu)) {
            qWarning() << "Profile references non-existent CPU" << cpu;
            continue;
        }

        // Apply online/offline state first (CPU 0 cannot be offlined)
        if (cpu != 0) {
            if (entry.online) {
                m_dbusHelper->setCpuOnlineAsync(cpu);
            } else {
                m_dbusHelper->setCpuOfflineAsync(cpu);
                // If we're setting CPU offline, skip other settings for this CPU
                continue;
            }
        }

        // Apply frequency settings
        if (entry.freqMin > 0 && entry.freqMax > 0) {
            m_dbusHelper->updateCpuSettingsAsync(cpu, static_cast<int>(entry.freqMin), static_cast<int>(entry.freqMax));
        }

        // Apply governor
        if (!entry.governor.isEmpty()) {
            m_dbusHelper->updateCpuGovernorAsync(cpu, entry.governor);
        }

        // Apply energy preference
        if (!entry.energyPref.isEmpty() && m_sysfsReader->isEnergyPrefAvailable(cpu)) {
            m_dbusHelper->updateCpuEnergyPrefsAsync(cpu, entry.energyPref);
        }
    }

    // End batch and start processing - completion will trigger onBatchCompleted
    m_dbusHelper->endBatch();
}

void Application::refreshCpuInfo()
{
    m_cpuModel->refresh();
    updateGovernorModel();
    updateEnergyPrefModel();
    emit currentCpuStateChanged();
    setStatusMessage(tr("CPU info refreshed"));
}

void Application::onDbusHelperReady(bool ready)
{
    if (ready) {
        setStatusMessage(tr("D-Bus helper connected"));
    } else {
        setStatusMessage(tr("D-Bus helper not available - running in read-only mode"));
    }
}

void Application::onDbusError(const QString &error)
{
    setStatusMessage(tr("D-Bus error: %1").arg(error));
    emit errorOccurred(error);
}

void Application::updateGovernorModel()
{
    const QStringList governors = m_sysfsReader->availableGovernors(m_currentCpu);
    m_governorModel->setGovernors(governors);
}

void Application::updateEnergyPrefModel()
{
    if (m_sysfsReader->isEnergyPrefAvailable(m_currentCpu)) {
        const QStringList prefs = m_sysfsReader->availableEnergyPrefs(m_currentCpu);
        m_energyPrefModel->setPreferences(prefs);
    } else {
        m_energyPrefModel->setPreferences({});
    }
}

void Application::setStatusMessage(const QString &msg)
{
    if (m_statusMessage == msg) {
        return;
    }
    m_statusMessage = msg;
    emit statusMessageChanged();
}

void Application::setUnsavedChanges(bool changed)
{
    if (m_hasUnsavedChanges == changed) {
        return;
    }
    m_hasUnsavedChanges = changed;
    emit unsavedChangesChanged();
}
