// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#include "helperservice.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
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
    // Setup idle timer
    m_idleTimer.setSingleShot(true);
    connect(&m_idleTimer, &QTimer::timeout, this, &HelperService::onIdleTimeout);
}

void HelperService::setIdleTimeout(int seconds)
{
    m_idleTimeoutSecs = seconds;
    if (m_idleTimeoutSecs > 0) {
        resetIdleTimer();
    } else {
        m_idleTimer.stop();
    }
}

void HelperService::resetIdleTimer()
{
    if (m_idleTimeoutSecs > 0) {
        m_idleTimer.start(m_idleTimeoutSecs * 1000);
    }
}

void HelperService::onIdleTimeout()
{
    qInfo() << "Idle timeout reached, shutting down helper service";
    QCoreApplication::quit();
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
    
    // Start idle timer after successful registration
    resetIdleTimer();
    
    return true;
}

// ============================================================================
// Authorization
// ============================================================================

int HelperService::isauthorized()
{
    resetIdleTimer();
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
    
    QDBusConnection systemBus = QDBusConnection::systemBus();
    
    // Build the CheckAuthorization call manually using QDBusMessage
    QDBusMessage msg = QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.PolicyKit1"),
        QStringLiteral("/org/freedesktop/PolicyKit1/Authority"),
        QStringLiteral("org.freedesktop.PolicyKit1.Authority"),
        QStringLiteral("CheckAuthorization")
    );
    
    // Build the subject structure: (sa{sv})
    QDBusArgument subjectArg;
    subjectArg.beginStructure();
    subjectArg << QStringLiteral("system-bus-name");
    subjectArg.beginMap(QMetaType::QString, qMetaTypeId<QDBusVariant>());
    subjectArg.beginMapEntry();
    subjectArg << QStringLiteral("name") << QDBusVariant(sender);
    subjectArg.endMapEntry();
    subjectArg.endMap();
    subjectArg.endStructure();
    
    // Build empty details map as a{ss}
    QDBusArgument detailsArg;
    detailsArg.beginMap(QMetaType::QString, QMetaType::QString);
    detailsArg.endMap();
    
    // Flags: AllowUserInteraction = 1
    quint32 flags = 1;
    
    // Set arguments: subject, action_id, details, flags, cancellation_id
    msg.setArguments({
        QVariant::fromValue(subjectArg),
        actionId,
        QVariant::fromValue(detailsArg),
        flags,
        QString()  // empty cancellation_id
    });
    
    // Make the call (blocking, with timeout for user interaction)
    QDBusMessage reply = systemBus.call(msg, QDBus::Block, 120000);  // 2 minute timeout
    
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "PolicyKit authorization failed:" << reply.errorMessage();
        return false;
    }
    
    if (reply.arguments().isEmpty()) {
        qWarning() << "PolicyKit returned empty response";
        return false;
    }
    
    // Parse result: (bba{ss})
    // We need to extract just the first boolean (is_authorized)
    QVariant resultVariant = reply.arguments().at(0);
    
    // The result is a struct, extract it via QDBusArgument
    const QDBusArgument resultArg = resultVariant.value<QDBusArgument>();
    
    bool isAuthorized = false;
    bool isChallenge = false;
    
    // Read the struct
    resultArg.beginStructure();
    resultArg >> isAuthorized;
    resultArg >> isChallenge;
    // Must read the a{ss} map to fully consume the struct
    resultArg.beginMap();
    while (!resultArg.atEnd()) {
        resultArg.beginMapEntry();
        QString key, value;
        resultArg >> key >> value;
        resultArg.endMapEntry();
    }
    resultArg.endMap();
    resultArg.endStructure();
    
    qDebug() << "PolicyKit authorization result: authorized=" << isAuthorized << "challenge=" << isChallenge;
    
    // Cache result (only if authorized without challenge)
    if (isAuthorized && !isChallenge) {
        m_authorizedSenders[cacheKey] = true;
    }
    
    return isAuthorized;
}

// ============================================================================
// CPU Queries (read-only)
// ============================================================================

QList<int> HelperService::get_cpus_available()
{
    resetIdleTimer();
    // Available CPUs = Present CPUs that can be brought online
    return get_cpus_present();
}

QList<int> HelperService::get_cpus_online()
{
    resetIdleTimer();
    QString content = readSysfsFile(QStringLiteral("%1/online").arg(SYS_CPU_PATH));
    return parseCpuList(content);
}

QList<int> HelperService::get_cpus_offline()
{
    resetIdleTimer();
    QString content = readSysfsFile(QStringLiteral("%1/offline").arg(SYS_CPU_PATH));
    return parseCpuList(content);
}

QList<int> HelperService::get_cpus_present()
{
    resetIdleTimer();
    QString content = readSysfsFile(QStringLiteral("%1/present").arg(SYS_CPU_PATH));
    return parseCpuList(content);
}

QStringList HelperService::get_cpu_governors(int cpu)
{
    resetIdleTimer();
    if (!isPresent(cpu) || !isOnline(cpu)) {
        return QStringList();
    }
    
    QString content = readSysfsFile(QStringLiteral("%1/%2").arg(cpufreqPath(cpu), SCALING_AVAILABLE_GOV));
    return parseList(content);
}

QStringList HelperService::get_cpu_energy_preferences(int cpu)
{
    resetIdleTimer();
    if (!isPresent(cpu) || !isOnline(cpu)) {
        return QStringList();
    }
    
    QString content = readSysfsFile(QStringLiteral("%1/%2").arg(cpufreqPath(cpu), ENERGY_PERF_AVAIL));
    return parseList(content);
}

QString HelperService::get_cpu_governor(int cpu)
{
    resetIdleTimer();
    if (!isPresent(cpu) || !isOnline(cpu)) {
        return QString();
    }
    
    return readSysfsFile(QStringLiteral("%1/%2").arg(cpufreqPath(cpu), SCALING_GOVERNOR)).trimmed();
}

QString HelperService::get_cpu_energy_preference(int cpu)
{
    resetIdleTimer();
    if (!isPresent(cpu) || !isOnline(cpu)) {
        return QString();
    }
    
    return readSysfsFile(QStringLiteral("%1/%2").arg(cpufreqPath(cpu), ENERGY_PERF_PREF)).trimmed();
}

QList<int> HelperService::get_cpu_frequencies(int cpu)
{
    resetIdleTimer();
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
    resetIdleTimer();
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
    resetIdleTimer();
    QString path = QStringLiteral("%1/cpu%2/%3").arg(SYS_CPU_PATH).arg(cpu).arg(ONLINE_FILE);
    return QFile::exists(path) ? 1 : 0;
}

// ============================================================================
// CPU Mutations (require authorization)
// ============================================================================

int HelperService::update_cpu_settings(int cpu, int freq_min, int freq_max)
{
    resetIdleTimer();
    qDebug() << "update_cpu_settings called: cpu=" << cpu << "freq_min=" << freq_min << "freq_max=" << freq_max;
    
    if (!isAuthorized()) {
        qWarning() << "Not authorized";
        return -1;
    }
    
    if (!isPresent(cpu) || !isOnline(cpu)) {
        qWarning() << "CPU" << cpu << "not present or not online";
        return -1;
    }
    
    QString basePath = cpufreqPath(cpu);
    
    // Read current values to determine write order
    QString curMinStr = readSysfsFile(QStringLiteral("%1/%2").arg(basePath, SCALING_MIN_FREQ)).trimmed();
    QString curMaxStr = readSysfsFile(QStringLiteral("%1/%2").arg(basePath, SCALING_MAX_FREQ)).trimmed();
    int curMin = curMinStr.toInt();
    int curMax = curMaxStr.toInt();
    
    qDebug() << "Current values: min=" << curMin << "max=" << curMax;
    qDebug() << "Target values: min=" << freq_min << "max=" << freq_max;
    
    // Determine the correct order to avoid temporary invalid states
    // Rule: min <= max must always be true
    // If new_max < cur_min, we must lower min first
    // If new_min > cur_max, we must raise max first
    
    bool success = true;
    
    if (freq_max < curMin) {
        // New max is lower than current min - must lower min first
        qDebug() << "Lowering min first (new max < current min)";
        if (!writeSysfsFile(QStringLiteral("%1/%2").arg(basePath, SCALING_MIN_FREQ), 
                            QString::number(freq_min))) {
            qWarning() << "Failed to write min frequency";
            success = false;
        }
        if (!writeSysfsFile(QStringLiteral("%1/%2").arg(basePath, SCALING_MAX_FREQ), 
                            QString::number(freq_max))) {
            qWarning() << "Failed to write max frequency";
            success = false;
        }
    } else if (freq_min > curMax) {
        // New min is higher than current max - must raise max first
        qDebug() << "Raising max first (new min > current max)";
        if (!writeSysfsFile(QStringLiteral("%1/%2").arg(basePath, SCALING_MAX_FREQ), 
                            QString::number(freq_max))) {
            qWarning() << "Failed to write max frequency";
            success = false;
        }
        if (!writeSysfsFile(QStringLiteral("%1/%2").arg(basePath, SCALING_MIN_FREQ), 
                            QString::number(freq_min))) {
            qWarning() << "Failed to write min frequency";
            success = false;
        }
    } else {
        // No conflict - write in standard order (min first, then max)
        qDebug() << "Standard order (no conflict)";
        if (!writeSysfsFile(QStringLiteral("%1/%2").arg(basePath, SCALING_MIN_FREQ), 
                            QString::number(freq_min))) {
            qWarning() << "Failed to write min frequency";
            success = false;
        }
        if (!writeSysfsFile(QStringLiteral("%1/%2").arg(basePath, SCALING_MAX_FREQ), 
                            QString::number(freq_max))) {
            qWarning() << "Failed to write max frequency";
            success = false;
        }
    }
    
    // Verify the result
    QString newMinStr = readSysfsFile(QStringLiteral("%1/%2").arg(basePath, SCALING_MIN_FREQ)).trimmed();
    QString newMaxStr = readSysfsFile(QStringLiteral("%1/%2").arg(basePath, SCALING_MAX_FREQ)).trimmed();
    qDebug() << "After write: min=" << newMinStr << "max=" << newMaxStr;
    
    return success ? 0 : -13;
}

int HelperService::update_cpu_governor(int cpu, const QString &governor)
{
    resetIdleTimer();
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
    resetIdleTimer();
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
    resetIdleTimer();
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
    resetIdleTimer();
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
