// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#include "dbushelper.h"

#include <QDBusReply>
#include <QDBusMetaType>
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

    QDBusReply<bool> reply = m_interface->call(QStringLiteral("isauthorized"));
    if (reply.isValid()) {
        return reply.value();
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
        return reply.toBool();
    }

    return false;
}

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
