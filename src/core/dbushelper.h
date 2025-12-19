// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#ifndef DBUSHELPER_H
#define DBUSHELPER_H

#include <QObject>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QList>
#include <QString>
#include <QStringList>
#include <QQueue>
#include <functional>

/**
 * @brief D-Bus helper class for communicating with cpupower-gui-helper service
 * 
 * This class wraps all D-Bus communication with the io.github.cpupower_gui.qt.helper
 * service which runs as root and performs privileged operations on CPU settings.
 * 
 * Mutation operations are asynchronous to avoid blocking the UI during PolicyKit
 * authentication prompts.
 */
class DbusHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool authorized READ isAuthorized NOTIFY authorizedChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(bool operationInProgress READ isOperationInProgress NOTIFY operationInProgressChanged)

public:
    explicit DbusHelper(QObject *parent = nullptr);
    ~DbusHelper() override;

    // Connection status
    bool isConnected() const;
    bool isAuthorized();
    bool isOperationInProgress() const { return m_operationInProgress; }

    // CPU queries (synchronous - no auth needed)
    Q_INVOKABLE QList<int> cpusAvailable();
    Q_INVOKABLE QList<int> cpusOnline();
    Q_INVOKABLE QList<int> cpusOffline();
    Q_INVOKABLE QList<int> cpusPresent();
    Q_INVOKABLE QStringList getCpuGovernors(int cpu);
    Q_INVOKABLE bool cpuAllowedOffline(int cpu);

    // CPU mutations (asynchronous - may trigger PolicyKit auth)
    // These queue operations and execute them sequentially
    Q_INVOKABLE void updateCpuSettingsAsync(int cpu, int fmin, int fmax);
    Q_INVOKABLE void updateCpuGovernorAsync(int cpu, const QString &governor);
    Q_INVOKABLE void updateCpuEnergyPrefsAsync(int cpu, const QString &pref);
    Q_INVOKABLE void setCpuOnlineAsync(int cpu);
    Q_INVOKABLE void setCpuOfflineAsync(int cpu);

    // Batch operations - queue multiple and signal when all complete
    void beginBatch();
    void endBatch();  // Will emit batchCompleted when all queued operations finish

    // Synchronous versions (for internal use, may block)
    int updateCpuSettings(int cpu, int fmin, int fmax);
    int updateCpuGovernor(int cpu, const QString &governor);
    int updateCpuEnergyPrefs(int cpu, const QString &pref);
    int setCpuOnline(int cpu);
    int setCpuOffline(int cpu);

signals:
    void authorizedChanged();
    void connectedChanged();
    void operationInProgressChanged();
    void operationFailed(const QString &error);
    void operationSucceeded();
    void batchCompleted(bool allSucceeded, const QStringList &errors);
    void helperReady(bool ready);
    void errorOccurred(const QString &error);

private slots:
    void onAsyncCallFinished(QDBusPendingCallWatcher *watcher);

private:
    struct QueuedOperation {
        QString method;
        QVariantList args;
        QString description;
    };

    void connectToService();
    QVariant callMethod(const QString &method, const QVariantList &args = {});
    void queueOperation(const QString &method, const QVariantList &args, const QString &description);
    void processNextOperation();
    void setOperationInProgress(bool inProgress);

    QDBusInterface *m_interface = nullptr;
    bool m_connected = false;
    bool m_operationInProgress = false;
    bool m_batchMode = false;
    QQueue<QueuedOperation> m_operationQueue;
    QStringList m_batchErrors;
    bool m_batchHadErrors = false;

    static constexpr const char *SERVICE_NAME = "io.github.cpupower_gui.qt.helper";
    static constexpr const char *OBJECT_PATH = "/io/github/cpupower_gui/qt/helper";
    static constexpr const char *INTERFACE_NAME = "io.github.cpupower_gui.qt.helper";
};

#endif // DBUSHELPER_H
