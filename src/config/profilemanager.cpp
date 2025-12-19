// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#include "profilemanager.h"
#include "core/sysfsreader.h"

#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDebug>

ProfileManager::ProfileManager(SysfsReader *sysfs, QObject *parent)
    : QObject(parent)
    , m_sysfs(sysfs)
{
    loadProfiles();
}

QStringList ProfileManager::profileNames() const
{
    QStringList names = m_profiles.keys();
    names.sort();
    return names;
}

bool ProfileManager::hasProfile(const QString &name) const
{
    return m_profiles.contains(name);
}

bool ProfileManager::isSystemProfile(const QString &name) const
{
    auto it = m_profiles.find(name);
    if (it != m_profiles.end()) {
        return it->isSystem;
    }
    return false;
}

bool ProfileManager::isBuiltinProfile(const QString &name) const
{
    auto it = m_profiles.find(name);
    if (it != m_profiles.end()) {
        return it->isBuiltin;
    }
    return false;
}

bool ProfileManager::canDeleteProfile(const QString &name) const
{
    auto it = m_profiles.find(name);
    if (it != m_profiles.end()) {
        return it->canDelete();
    }
    return false;
}

QVariantMap ProfileManager::getProfileSettings(const QString &name) const
{
    QVariantMap result;
    auto it = m_profiles.find(name);
    if (it == m_profiles.end()) {
        return result;
    }

    const Profile &prof = *it;
    result[QStringLiteral("name")] = prof.name;
    result[QStringLiteral("isSystem")] = prof.isSystem;
    result[QStringLiteral("isBuiltin")] = prof.isBuiltin;
    result[QStringLiteral("canDelete")] = prof.canDelete();

    QVariantList cpuSettings;
    for (auto cpuIt = prof.settings.constBegin(); cpuIt != prof.settings.constEnd(); ++cpuIt) {
        QVariantMap cpuMap;
        cpuMap[QStringLiteral("cpu")] = cpuIt->cpu;
        cpuMap[QStringLiteral("freqMin")] = cpuIt->freqMin;
        cpuMap[QStringLiteral("freqMax")] = cpuIt->freqMax;
        cpuMap[QStringLiteral("governor")] = cpuIt->governor;
        cpuMap[QStringLiteral("online")] = cpuIt->online;
        cpuMap[QStringLiteral("energyPref")] = cpuIt->energyPref;
        cpuSettings.append(cpuMap);
    }
    result[QStringLiteral("cpuSettings")] = cpuSettings;

    return result;
}

bool ProfileManager::createProfile(const QString &name, const QVariantMap &settings)
{
    if (name.isEmpty()) {
        emit error(tr("Profile name cannot be empty"));
        return false;
    }

    // Don't overwrite system/builtin profiles
    if (hasProfile(name) && !canDeleteProfile(name)) {
        emit error(tr("Cannot overwrite system profile: %1").arg(name));
        return false;
    }

    Profile profile;
    profile.name = name;
    profile.isSystem = false;
    profile.isBuiltin = false;

    // Generate filename
    QString safeName = name;
    safeName.replace(QLatin1Char(' '), QLatin1Char('-'));
    profile.filePath = userProfileDir() + QStringLiteral("/cpg-%1.profile").arg(safeName);

    // Parse settings
    const QVariantList cpuSettings = settings.value(QStringLiteral("cpuSettings")).toList();
    for (const QVariant &v : cpuSettings) {
        const QVariantMap cpuMap = v.toMap();
        CpuProfileEntry entry;
        entry.cpu = cpuMap.value(QStringLiteral("cpu")).toInt();
        entry.freqMin = cpuMap.value(QStringLiteral("freqMin")).toLongLong();
        entry.freqMax = cpuMap.value(QStringLiteral("freqMax")).toLongLong();
        entry.governor = cpuMap.value(QStringLiteral("governor")).toString();
        entry.online = cpuMap.value(QStringLiteral("online"), true).toBool();
        entry.energyPref = cpuMap.value(QStringLiteral("energyPref")).toString();
        profile.settings[entry.cpu] = entry;
    }

    if (!writeProfileFile(profile)) {
        emit error(tr("Failed to write profile file"));
        return false;
    }

    m_profiles[name] = profile;
    emit profilesChanged();
    emit profileCreated(name);
    return true;
}

bool ProfileManager::deleteProfile(const QString &name)
{
    if (!canDeleteProfile(name)) {
        emit error(tr("Cannot delete profile: %1").arg(name));
        return false;
    }

    auto it = m_profiles.find(name);
    if (it == m_profiles.end()) {
        return false;
    }

    // Delete the file
    if (!it->filePath.isEmpty() && QFile::exists(it->filePath)) {
        if (!QFile::remove(it->filePath)) {
            emit error(tr("Failed to delete profile file"));
            return false;
        }
    }

    m_profiles.erase(it);
    emit profilesChanged();
    emit profileDeleted(name);
    return true;
}

void ProfileManager::reload()
{
    m_profiles.clear();
    loadProfiles();
    emit profilesChanged();
}

const Profile *ProfileManager::profile(const QString &name) const
{
    auto it = m_profiles.find(name);
    if (it != m_profiles.end()) {
        return &(*it);
    }
    return nullptr;
}

QString ProfileManager::systemProfileDir()
{
    return QStringLiteral("/etc/cpupower_gui.d");
}

QString ProfileManager::userProfileDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + QStringLiteral("/cpupower_gui");
}

void ProfileManager::loadProfiles()
{
    // Generate default profiles first
    generateDefaultProfiles();

    // Load system profiles (can override defaults)
    loadProfilesFromDir(systemProfileDir(), true);

    // Load user profiles (can override system)
    loadProfilesFromDir(userProfileDir(), false);
}

void ProfileManager::loadProfilesFromDir(const QString &dirPath, bool isSystem)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        return;
    }

    const QStringList profileFiles = dir.entryList({QStringLiteral("*.profile")}, QDir::Files, QDir::Name);
    for (const QString &file : profileFiles) {
        const QString filePath = dir.absoluteFilePath(file);
        Profile prof = parseProfileFile(filePath, isSystem);
        if (prof.isValid()) {
            m_profiles[prof.name] = prof;
        }
    }
}

void ProfileManager::generateDefaultProfiles()
{
    if (!m_sysfs) {
        return;
    }

    const QStringList governors = m_sysfs->availableGovernors(0);
    if (governors.isEmpty()) {
        return;
    }

    const QList<int> cpus = m_sysfs->availableCpus();

    // Generate "Balanced" profile
    QString balancedGov;
    if (governors.contains(QStringLiteral("schedutil"))) {
        balancedGov = QStringLiteral("schedutil");
    } else if (governors.contains(QStringLiteral("ondemand"))) {
        balancedGov = QStringLiteral("ondemand");
    } else if (governors.contains(QStringLiteral("powersave"))) {
        balancedGov = QStringLiteral("powersave");
    }

    if (!balancedGov.isEmpty()) {
        Profile balanced;
        balanced.name = QStringLiteral("Balanced");
        balanced.isBuiltin = true;
        for (int cpu : cpus) {
            CpuProfileEntry entry;
            entry.cpu = cpu;
            entry.freqMin = m_sysfs->minFrequencyHardware(cpu);
            entry.freqMax = m_sysfs->maxFrequencyHardware(cpu);
            entry.governor = balancedGov;
            entry.online = true;
            balanced.settings[cpu] = entry;
        }
        m_profiles[balanced.name] = balanced;
    }

    // Generate profiles for each available governor (except userspace)
    for (const QString &gov : governors) {
        if (gov == QStringLiteral("userspace")) {
            continue;
        }

        QString name = gov;
        name[0] = name[0].toUpper();  // Capitalize first letter

        // Skip if we already have this profile (e.g., Balanced)
        if (m_profiles.contains(name)) {
            continue;
        }

        Profile profile;
        profile.name = name;
        profile.isBuiltin = true;
        for (int cpu : cpus) {
            CpuProfileEntry entry;
            entry.cpu = cpu;
            entry.freqMin = m_sysfs->minFrequencyHardware(cpu);
            entry.freqMax = m_sysfs->maxFrequencyHardware(cpu);
            entry.governor = gov;
            entry.online = true;
            profile.settings[cpu] = entry;
        }
        m_profiles[name] = profile;
    }
}

Profile ProfileManager::parseProfileFile(const QString &filePath, bool isSystem) const
{
    Profile profile;
    profile.filePath = filePath;
    profile.isSystem = isSystem;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return profile;
    }

    QTextStream in(&file);
    bool firstLine = true;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        // Skip empty lines
        if (line.isEmpty()) {
            continue;
        }

        // Parse name from first line: "# name: ProfileName"
        if (firstLine) {
            firstLine = false;
            if (line.startsWith(QStringLiteral("# name:"))) {
                profile.name = line.mid(7).trimmed();
                continue;
            } else if (line.startsWith(QStringLiteral("#"))) {
                // Comment but not name, use filename
                QFileInfo fi(filePath);
                profile.name = fi.baseName();
                continue;
            }
        }

        // Skip comments
        if (line.startsWith(QLatin1Char('#'))) {
            continue;
        }

        // Parse CPU settings: "cpu  fmin  fmax  governor  online"
        static const QRegularExpression whitespace(QStringLiteral("\\s+"));
        const QStringList parts = line.split(whitespace, Qt::SkipEmptyParts);
        if (parts.size() < 4) {
            continue;
        }

        // Parse CPU range (e.g., "0-3" or "0,2,4")
        const QString cpuSpec = parts[0];
        QList<int> cpuList;

        // Handle ranges like "0-3"
        if (cpuSpec.contains(QLatin1Char('-')) && !cpuSpec.contains(QLatin1Char(','))) {
            const QStringList range = cpuSpec.split(QLatin1Char('-'));
            if (range.size() == 2) {
                int start = range[0].toInt();
                int end = range[1].toInt();
                for (int i = start; i <= end; ++i) {
                    cpuList.append(i);
                }
            }
        }
        // Handle comma-separated list "0,2,4"
        else if (cpuSpec.contains(QLatin1Char(','))) {
            const QStringList cpuParts = cpuSpec.split(QLatin1Char(','));
            for (const QString &p : cpuParts) {
                if (p.contains(QLatin1Char('-'))) {
                    const QStringList range = p.split(QLatin1Char('-'));
                    if (range.size() == 2) {
                        int start = range[0].toInt();
                        int end = range[1].toInt();
                        for (int i = start; i <= end; ++i) {
                            cpuList.append(i);
                        }
                    }
                } else {
                    cpuList.append(p.toInt());
                }
            }
        }
        // Single CPU
        else {
            cpuList.append(cpuSpec.toInt());
        }

        // Parse frequency values (in MHz in file, convert to kHz)
        qint64 fmin = 0;
        qint64 fmax = 0;
        const QString fminStr = parts[1];
        const QString fmaxStr = parts[2];

        if (fminStr != QStringLiteral("-") && fminStr.toLongLong() > 0) {
            fmin = fminStr.toLongLong() * 1000;  // MHz to kHz
        }
        if (fmaxStr != QStringLiteral("-") && fmaxStr.toLongLong() > 0) {
            fmax = fmaxStr.toLongLong() * 1000;  // MHz to kHz
        }

        const QString governor = parts[3];
        bool online = true;
        if (parts.size() > 4) {
            const QString onlineStr = parts[4].toLower();
            online = (onlineStr == QStringLiteral("y") || 
                      onlineStr == QStringLiteral("yes") ||
                      onlineStr == QStringLiteral("1") ||
                      onlineStr == QStringLiteral("true"));
        }

        // Apply to all CPUs in the list
        for (int cpu : cpuList) {
            CpuProfileEntry entry;
            entry.cpu = cpu;

            // Use hardware limits if not specified
            if (m_sysfs) {
                entry.freqMin = (fmin > 0) ? fmin : m_sysfs->minFrequencyHardware(cpu);
                entry.freqMax = (fmax > 0) ? fmax : m_sysfs->maxFrequencyHardware(cpu);
            } else {
                entry.freqMin = fmin;
                entry.freqMax = fmax;
            }

            entry.governor = (governor != QStringLiteral("-")) ? governor : QString();
            entry.online = online;
            profile.settings[cpu] = entry;
        }
    }

    // Fallback name from filename if not found
    if (profile.name.isEmpty()) {
        QFileInfo fi(filePath);
        profile.name = fi.baseName();
    }

    return profile;
}

bool ProfileManager::writeProfileFile(const Profile &profile) const
{
    // Ensure directory exists
    const QString dirPath = userProfileDir();
    QDir dir(dirPath);
    if (!dir.exists()) {
        if (!dir.mkpath(dirPath)) {
            return false;
        }
    }

    QFile file(profile.filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << "# name: " << profile.name << "\n\n";
    out << "# CPU\tMin\tMax\tGovernor\tOnline\n";

    for (auto it = profile.settings.constBegin(); it != profile.settings.constEnd(); ++it) {
        const CpuProfileEntry &entry = *it;
        out << entry.cpu << "\t"
            << (entry.freqMin / 1000) << "\t"  // kHz to MHz
            << (entry.freqMax / 1000) << "\t"  // kHz to MHz
            << entry.governor << "\t"
            << (entry.online ? "y" : "n") << "\n";
    }

    return true;
}
