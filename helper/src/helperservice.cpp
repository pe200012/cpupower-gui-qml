// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#include "helperservice.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QRegularExpression>

HelperService::HelperService(QObject *parent)
    : QObject(parent)
{
}

bool HelperService::registerService()
{
    QDBusConnection systemBus = QDBusConnection::systemBus();
    
    if (!systemBus.isConnected()) {
        qCritical() << "Cannot connect to system D-Bus";
        return false;
    }
    
    if (!systemBus.registerService(QStringLiteral("io.github.cpupower_gui.qt.helper"))) {
        qCritical() << "Cannot register D-Bus service:" << systemBus.lastError().message();
        return false;
    }
    
    if (!systemBus.registerObject(QStringLiteral("/io/github/cpupower_gui/qt/helper"), 
                                   this,
                                   QDBusConnection::ExportAllSlots)) {
        qCritical() << "Cannot register D-Bus object:" << systemBus.lastError().message();
        return false;
    }
    
    qInfo() << "D-Bus helper service registered successfully";
    return true;
}

// ============================================================================
// Authorization
// ============================================================================

int HelperService::isauthorized()
{
    return isAuthorized() ? 1 : 0;
}

bool HelperService::isAuthorized(const QString &actionId)
{
    if (!calledFromDBus()) {
        return true; // Local calls are always authorized
    }
    
    QString sender = message().service();
    return checkPolkitAuthorization(sender, actionId);
}

bool HelperService::checkPolkitAuthorization(const QString &sender, const QString &actionId)
{
    // Check cache first
    QString cacheKey = sender + actionId;
    if (m_authorizedSenders.contains(cacheKey)) {
        return m_authorizedSenders[cacheKey];
    }
    
    // Query PolicyKit
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.PolicyKit1"),
        QStringLiteral("/org/freedesktop/PolicyKit1/Authority"),
        QStringLiteral("org.freedesktop.PolicyKit1.Authority"),
        QStringLiteral("CheckAuthorization")
    );
    
    // Subject: system-bus-name
    QVariantMap subjectDetails;
    subjectDetails[QStringLiteral("name")] = sender;
    
    QVariantList subject;
    subject << QStringLiteral("system-bus-name") << subjectDetails;
    
    // Details (empty)
    QVariantMap details;
    
    // Flags: AllowUserInteraction = 1
    quint32 flags = 1;
    
    // Cancellation ID (empty)
    QString cancellationId;
    
    msg << QVariant::fromValue(QDBusArgument() << QStringLiteral("system-bus-name") << subjectDetails);
    
    // Build the subject tuple manually
    QDBusConnection systemBus = QDBusConnection::systemBus();
    
    QDBusMessage polkitMsg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.PolicyKit1"),
        QStringLiteral("/org/freedesktop/PolicyKit1/Authority"),
        QStringLiteral("org.freedesktop.PolicyKit1.Authority"),
        QStringLiteral("CheckAuthorization")
    );
    
    // Create subject as (sa{sv})
    QDBusArgument subjectArg;
    subjectArg.beginStructure();
    subjectArg << QStringLiteral("system-bus-name");
    subjectArg.beginMap(QMetaType::QString, qMetaTypeId<QDBusVariant>());
    subjectArg.beginMapEntry();
    subjectArg << QStringLiteral("name") << QDBusVariant(sender);
    subjectArg.endMapEntry();
    subjectArg.endMap();
    subjectArg.endStructure();
    
    polkitMsg << QVariant::fromValue(subjectArg);  // subject
    polkitMsg << actionId;                          // action_id
    polkitMsg << details;                           // details
    polkitMsg << flags;                             // flags
    polkitMsg << cancellationId;                    // cancellation_id
    
    QDBusMessage reply = systemBus.call(polkitMsg);
    
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "PolicyKit authorization failed:" << reply.errorMessage();
        return false;
    }
    
    // Parse result: (bba{ss})
    if (reply.arguments().isEmpty()) {
        return false;
    }
    
    QDBusArgument resultArg = reply.arguments().at(0).value<QDBusArgument>();
    bool isAuthorized = false;
    bool isChallenge = false;
    QVariantMap resultDetails;
    
    resultArg.beginStructure();
    resultArg >> isAuthorized >> isChallenge >> resultDetails;
    resultArg.endStructure();
    
    // Cache result
    m_authorizedSenders[cacheKey] = isAuthorized;
    
    return isAuthorized;
}

// ============================================================================
// CPU Queries (read-only)
// ============================================================================

QList<int> HelperService::get_cpus_available()
{
    // Available CPUs = Present CPUs that can be brought online
    return get_cpus_present();
}

QList<int> HelperService::get_cpus_online()
{
    QString content = readSysfsFile(QStringLiteral("%1/online").arg(SYS_CPU_PATH));
    return parseCpuList(content);
}

QList<int> HelperService::get_cpus_offline()
{
    QString content = readSysfsFile(QStringLiteral("%1/offline").arg(SYS_CPU_PATH));
    return parseCpuList(content);
}

QList<int> HelperService::get_cpus_present()
{
    QString content = readSysfsFile(QStringLiteral("%1/present").arg(SYS_CPU_PATH));
    return parseCpuList(content);
}

QStringList HelperService::get_cpu_governors(int cpu)
{
    if (!isPresent(cpu) || !isOnline(cpu)) {
        return QStringList();
    }
    
    QString content = readSysfsFile(QStringLiteral("%1/%2").arg(cpufreqPath(cpu), SCALING_AVAILABLE_GOV));
    return parseList(content);
}

QStringList HelperService::get_cpu_energy_preferences(int cpu)
{
    if (!isPresent(cpu) || !isOnline(cpu)) {
        return QStringList();
    }
    
    QString content = readSysfsFile(QStringLiteral("%1/%2").arg(cpufreqPath(cpu), ENERGY_PERF_AVAIL));
    return parseList(content);
}

QString HelperService::get_cpu_governor(int cpu)
{
    if (!isPresent(cpu) || !isOnline(cpu)) {
        return QString();
    }
    
    return readSysfsFile(QStringLiteral("%1/%2").arg(cpufreqPath(cpu), SCALING_GOVERNOR)).trimmed();
}

QString HelperService::get_cpu_energy_preference(int cpu)
{
    if (!isPresent(cpu) || !isOnline(cpu)) {
        return QString();
    }
    
    return readSysfsFile(QStringLiteral("%1/%2").arg(cpufreqPath(cpu), ENERGY_PERF_PREF)).trimmed();
}

QList<int> HelperService::get_cpu_frequencies(int cpu)
{
    QList<int> result;
    
    if (!isPresent(cpu) || !isOnline(cpu)) {
        return result << 0 << 0;
    }
    
    QString minStr = readSysfsFile(QStringLiteral("%1/%2").arg(cpufreqPath(cpu), SCALING_MIN_FREQ));
    QString maxStr = readSysfsFile(QStringLiteral("%1/%2").arg(cpufreqPath(cpu), SCALING_MAX_FREQ));
    
    result << minStr.trimmed().toInt() << maxStr.trimmed().toInt();
    return result;
}

QList<int> HelperService::get_cpu_limits(int cpu)
{
    QList<int> result;
    
    if (!isPresent(cpu) || !isOnline(cpu)) {
        return result << 0 << 0;
    }
    
    QString minStr = readSysfsFile(QStringLiteral("%1/%2").arg(cpufreqPath(cpu), CPUINFO_MIN_FREQ));
    QString maxStr = readSysfsFile(QStringLiteral("%1/%2").arg(cpufreqPath(cpu), CPUINFO_MAX_FREQ));
    
    result << minStr.trimmed().toInt() << maxStr.trimmed().toInt();
    return result;
}

int HelperService::cpu_allowed_offline(int cpu)
{
    QString path = QStringLiteral("%1/cpu%2/%3").arg(SYS_CPU_PATH).arg(cpu).arg(ONLINE_FILE);
    return QFile::exists(path) ? 1 : 0;
}

// ============================================================================
// CPU Mutations (require authorization)
// ============================================================================

int HelperService::update_cpu_settings(int cpu, int freq_min, int freq_max)
{
    if (!isAuthorized()) {
        return -1;
    }
    
    if (!isPresent(cpu) || !isOnline(cpu)) {
        return -1;
    }
    
    QString basePath = cpufreqPath(cpu);
    
    // Write min frequency first (to avoid min > max temporarily)
    if (!writeSysfsFile(QStringLiteral("%1/%2").arg(basePath, SCALING_MIN_FREQ), 
                        QString::number(freq_min))) {
        return -13;
    }
    
    if (!writeSysfsFile(QStringLiteral("%1/%2").arg(basePath, SCALING_MAX_FREQ), 
                        QString::number(freq_max))) {
        return -13;
    }
    
    return 0;
}

int HelperService::update_cpu_governor(int cpu, const QString &governor)
{
    if (!isAuthorized()) {
        return -1;
    }
    
    if (!isPresent(cpu) || !isOnline(cpu)) {
        return -1;
    }
    
    QString path = QStringLiteral("%1/%2").arg(cpufreqPath(cpu), SCALING_GOVERNOR);
    
    if (!writeSysfsFile(path, governor)) {
        return -13;
    }
    
    return 0;
}

int HelperService::update_cpu_energy_prefs(int cpu, const QString &pref)
{
    if (!isAuthorized()) {
        return -1;
    }
    
    if (!isPresent(cpu) || !isOnline(cpu)) {
        return -1;
    }
    
    // Check if pref is available
    QStringList available = get_cpu_energy_preferences(cpu);
    if (!available.contains(pref)) {
        return 0; // Not an error, just not available
    }
    
    QString path = QStringLiteral("%1/%2").arg(cpufreqPath(cpu), ENERGY_PERF_PREF);
    
    if (!QFile::exists(path)) {
        return 0;
    }
    
    if (!writeSysfsFile(path, pref)) {
        return -13;
    }
    
    return 0;
}

int HelperService::set_cpu_online(int cpu)
{
    if (!isAuthorized()) {
        return -1;
    }
    
    QString path = QStringLiteral("%1/cpu%2/%3").arg(SYS_CPU_PATH).arg(cpu).arg(ONLINE_FILE);
    
    if (!QFile::exists(path)) {
        return -1; // CPU 0 usually can't be offlined
    }
    
    if (!writeSysfsFile(path, QStringLiteral("1"))) {
        return -13;
    }
    
    return 0;
}

int HelperService::set_cpu_offline(int cpu)
{
    if (!isAuthorized()) {
        return -1;
    }
    
    QString path = QStringLiteral("%1/cpu%2/%3").arg(SYS_CPU_PATH).arg(cpu).arg(ONLINE_FILE);
    
    if (!QFile::exists(path)) {
        return -1;
    }
    
    if (!writeSysfsFile(path, QStringLiteral("0"))) {
        return -13;
    }
    
    return 0;
}

void HelperService::quit()
{
    qInfo() << "Quit requested, shutting down helper service...";
    QCoreApplication::quit();
}

// ============================================================================
// Sysfs helpers
// ============================================================================

QString HelperService::readSysfsFile(const QString &path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    
    QTextStream stream(&file);
    return stream.readAll();
}

bool HelperService::writeSysfsFile(const QString &path, const QString &value)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open for writing:" << path << file.errorString();
        return false;
    }
    
    QTextStream stream(&file);
    stream << value;
    
    return true;
}

QList<int> HelperService::parseCpuList(const QString &content) const
{
    QList<int> result;
    QString trimmed = content.trimmed();
    
    if (trimmed.isEmpty()) {
        return result;
    }
    
    // Format: "0-3,5,7-9" -> [0,1,2,3,5,7,8,9]
    QStringList parts = trimmed.split(QLatin1Char(','), Qt::SkipEmptyParts);
    
    for (const QString &part : parts) {
        if (part.contains(QLatin1Char('-'))) {
            QStringList range = part.split(QLatin1Char('-'));
            if (range.size() == 2) {
                int start = range[0].toInt();
                int end = range[1].toInt();
                for (int i = start; i <= end; ++i) {
                    result.append(i);
                }
            }
        } else {
            result.append(part.toInt());
        }
    }
    
    return result;
}

QStringList HelperService::parseList(const QString &content) const
{
    QString trimmed = content.trimmed();
    if (trimmed.isEmpty()) {
        return QStringList();
    }
    
    return trimmed.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
}

bool HelperService::isOnline(int cpu) const
{
    QList<int> online = const_cast<HelperService*>(this)->get_cpus_online();
    return online.contains(cpu);
}

bool HelperService::isPresent(int cpu) const
{
    QList<int> present = const_cast<HelperService*>(this)->get_cpus_present();
    return present.contains(cpu);
}

QString HelperService::cpuPath(int cpu) const
{
    return QStringLiteral("%1/cpu%2").arg(SYS_CPU_PATH).arg(cpu);
}

QString HelperService::cpufreqPath(int cpu) const
{
    return QStringLiteral("%1/cpu%2/%3").arg(SYS_CPU_PATH).arg(cpu).arg(CPUFREQ_DIR);
}
