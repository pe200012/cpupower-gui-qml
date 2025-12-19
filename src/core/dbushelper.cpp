// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#include "dbushelper.h"

#include <QDBusReply>
#include <QDBusMetaType>
#include <QDBusPendingReply>
#include <QDebug>

DbusHelper::DbusHelper(QObject *parent)
    : QObject(parent)
{
    connectToService();
}

DbusHelper::~DbusHelper()
{
    delete m_interface;
}

void DbusHelper::connectToService()
{
    m_interface = new QDBusInterface(
        SERVICE_NAME,
        OBJECT_PATH,
        INTERFACE_NAME,
        QDBusConnection::systemBus(),
        this
    );

    m_connected = m_interface->isValid();
    if (!m_connected) {
        qWarning() << "Failed to connect to D-Bus service:" << SERVICE_NAME;
        qWarning() << "Error:" << m_interface->lastError().message();
        Q_EMIT errorOccurred(tr("D-Bus helper not available - running in read-only mode"));
    }
    Q_EMIT connectedChanged();
    Q_EMIT helperReady(m_connected);
}

bool DbusHelper::isConnected() const
{
    return m_connected;
}

bool DbusHelper::isAuthorized()
{
    if (!m_connected) {
        return false;
    }

    QDBusReply<int> reply = m_interface->call(QStringLiteral("isauthorized"));
    if (reply.isValid()) {
        return reply.value() != 0;
    }

    qWarning() << "isauthorized call failed:" << reply.error().message();
    return false;
}

QVariant DbusHelper::callMethod(const QString &method, const QVariantList &args)
{
    if (!m_connected) {
        Q_EMIT operationFailed(tr("Not connected to D-Bus service"));
        return QVariant();
    }

    QDBusMessage msg = m_interface->callWithArgumentList(QDBus::Block, method, args);

    if (msg.type() == QDBusMessage::ErrorMessage) {
        QString error = msg.errorMessage();
        qWarning() << "D-Bus call failed:" << method << "-" << error;
        Q_EMIT operationFailed(error);
        return QVariant();
    }

    if (msg.arguments().isEmpty()) {
        return QVariant();
    }

    return msg.arguments().first();
}

void DbusHelper::setOperationInProgress(bool inProgress)
{
    if (m_operationInProgress != inProgress) {
        m_operationInProgress = inProgress;
        Q_EMIT operationInProgressChanged();
    }
}

void DbusHelper::queueOperation(const QString &method, const QVariantList &args, const QString &description)
{
    m_operationQueue.enqueue({method, args, description});
    
    // If not currently processing and not in batch mode, start processing
    if (!m_operationInProgress && !m_batchMode) {
        processNextOperation();
    }
}

void DbusHelper::processNextOperation()
{
    if (m_operationQueue.isEmpty()) {
        setOperationInProgress(false);
        
        // If we were in batch mode, emit completion signal
        if (m_batchMode) {
            m_batchMode = false;
            Q_EMIT batchCompleted(!m_batchHadErrors, m_batchErrors);
            m_batchErrors.clear();
            m_batchHadErrors = false;
        }
        return;
    }

    setOperationInProgress(true);
    
    QueuedOperation op = m_operationQueue.dequeue();
    
    if (!m_connected) {
        QString error = tr("Not connected to D-Bus service");
        qWarning() << "Cannot execute" << op.description << ":" << error;
        m_batchHadErrors = true;
        m_batchErrors.append(op.description + QStringLiteral(": ") + error);
        Q_EMIT operationFailed(error);
        
        // Continue with next operation
        processNextOperation();
        return;
    }

    qDebug() << "Executing async D-Bus call:" << op.method << "(" << op.description << ")";

    QDBusMessage msg = QDBusMessage::createMethodCall(
        SERVICE_NAME,
        OBJECT_PATH,
        INTERFACE_NAME,
        op.method
    );
    msg.setArguments(op.args);

    QDBusPendingCall pendingCall = QDBusConnection::systemBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall, this);
    
    // Store the description in the watcher for error reporting
    watcher->setProperty("operationDescription", op.description);
    
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &DbusHelper::onAsyncCallFinished);
}

void DbusHelper::onAsyncCallFinished(QDBusPendingCallWatcher *watcher)
{
    QString description = watcher->property("operationDescription").toString();
    QDBusPendingReply<int> reply = *watcher;
    
    if (reply.isError()) {
        QString error = reply.error().message();
        qWarning() << "Async D-Bus call failed:" << description << "-" << error;
        m_batchHadErrors = true;
        m_batchErrors.append(description + QStringLiteral(": ") + error);
        Q_EMIT operationFailed(error);
    } else {
        int result = reply.value();
        if (result == 0) {
            qDebug() << "Async D-Bus call succeeded:" << description;
            Q_EMIT operationSucceeded();
        } else {
            QString error = tr("Operation failed with code %1").arg(result);
            qWarning() << "Async D-Bus call returned error:" << description << "-" << error;
            m_batchHadErrors = true;
            m_batchErrors.append(description + QStringLiteral(": ") + error);
            Q_EMIT operationFailed(error);
        }
    }
    
    watcher->deleteLater();
    
    // Process next operation in queue
    processNextOperation();
}

void DbusHelper::beginBatch()
{
    m_batchMode = true;
    m_batchErrors.clear();
    m_batchHadErrors = false;
}

void DbusHelper::endBatch()
{
    // If nothing was queued, emit completion immediately
    if (m_operationQueue.isEmpty() && !m_operationInProgress) {
        m_batchMode = false;
        Q_EMIT batchCompleted(true, QStringList());
        return;
    }
    
    // Otherwise, start processing the queue
    if (!m_operationInProgress) {
        processNextOperation();
    }
    // When queue empties, processNextOperation will emit batchCompleted
}

QList<int> DbusHelper::cpusAvailable()
{
    QList<int> result;
    QVariant reply = callMethod(QStringLiteral("get_cpus_available"));

    if (reply.isValid()) {
        const QVariantList list = reply.toList();
        for (const QVariant &v : list) {
            result.append(v.toInt());
        }
    }

    return result;
}

QList<int> DbusHelper::cpusOnline()
{
    QList<int> result;
    QVariant reply = callMethod(QStringLiteral("get_cpus_online"));

    if (reply.isValid()) {
        const QVariantList list = reply.toList();
        for (const QVariant &v : list) {
            result.append(v.toInt());
        }
    }

    return result;
}

QList<int> DbusHelper::cpusOffline()
{
    QList<int> result;
    QVariant reply = callMethod(QStringLiteral("get_cpus_offline"));

    if (reply.isValid()) {
        const QVariantList list = reply.toList();
        for (const QVariant &v : list) {
            result.append(v.toInt());
        }
    }

    return result;
}

QList<int> DbusHelper::cpusPresent()
{
    QList<int> result;
    QVariant reply = callMethod(QStringLiteral("get_cpus_present"));

    if (reply.isValid()) {
        const QVariantList list = reply.toList();
        for (const QVariant &v : list) {
            result.append(v.toInt());
        }
    }

    return result;
}

QStringList DbusHelper::getCpuGovernors(int cpu)
{
    QStringList result;
    QVariant reply = callMethod(QStringLiteral("get_cpu_governors"), {cpu});

    if (reply.isValid()) {
        result = reply.toStringList();
    }

    return result;
}

bool DbusHelper::cpuAllowedOffline(int cpu)
{
    QVariant reply = callMethod(QStringLiteral("cpu_allowed_offline"), {cpu});

    if (reply.isValid()) {
        return reply.toInt() != 0;
    }

    return false;
}

// Synchronous versions
int DbusHelper::updateCpuSettings(int cpu, int fmin, int fmax)
{
    QVariant reply = callMethod(QStringLiteral("update_cpu_settings"), {cpu, fmin, fmax});

    if (reply.isValid()) {
        return reply.toInt();
    }

    return -1;
}

int DbusHelper::updateCpuGovernor(int cpu, const QString &governor)
{
    QVariant reply = callMethod(QStringLiteral("update_cpu_governor"), {cpu, governor});

    if (reply.isValid()) {
        return reply.toInt();
    }

    return -1;
}

int DbusHelper::updateCpuEnergyPrefs(int cpu, const QString &pref)
{
    QVariant reply = callMethod(QStringLiteral("update_cpu_energy_prefs"), {cpu, pref});

    if (reply.isValid()) {
        return reply.toInt();
    }

    return -1;
}

int DbusHelper::setCpuOnline(int cpu)
{
    QVariant reply = callMethod(QStringLiteral("set_cpu_online"), {cpu});

    if (reply.isValid()) {
        return reply.toInt();
    }

    return -1;
}

int DbusHelper::setCpuOffline(int cpu)
{
    QVariant reply = callMethod(QStringLiteral("set_cpu_offline"), {cpu});

    if (reply.isValid()) {
        return reply.toInt();
    }

    return -1;
}

// Asynchronous versions - now queue operations
void DbusHelper::updateCpuSettingsAsync(int cpu, int fmin, int fmax)
{
    queueOperation(QStringLiteral("update_cpu_settings"), 
                   {cpu, fmin, fmax},
                   tr("Set CPU %1 frequency %2-%3 kHz").arg(cpu).arg(fmin).arg(fmax));
}

void DbusHelper::updateCpuGovernorAsync(int cpu, const QString &governor)
{
    queueOperation(QStringLiteral("update_cpu_governor"), 
                   {cpu, governor},
                   tr("Set CPU %1 governor to %2").arg(cpu).arg(governor));
}

void DbusHelper::updateCpuEnergyPrefsAsync(int cpu, const QString &pref)
{
    queueOperation(QStringLiteral("update_cpu_energy_prefs"), 
                   {cpu, pref},
                   tr("Set CPU %1 energy preference to %2").arg(cpu).arg(pref));
}

void DbusHelper::setCpuOnlineAsync(int cpu)
{
    queueOperation(QStringLiteral("set_cpu_online"), 
                   {cpu},
                   tr("Set CPU %1 online").arg(cpu));
}

void DbusHelper::setCpuOfflineAsync(int cpu)
{
    queueOperation(QStringLiteral("set_cpu_offline"), 
                   {cpu},
                   tr("Set CPU %1 offline").arg(cpu));
}
