// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#ifndef PROFILEMANAGER_H
#define PROFILEMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariantMap>
#include <QDir>

class SysfsReader;

/**
 * @brief Represents a single CPU profile
 * 
 * A profile contains settings for each CPU:
 * - Minimum/maximum frequencies
 * - Governor
 * - Online state
 * - Energy performance preference (if available)
 */
struct CpuProfileEntry {
    int cpu{0};
    qint64 freqMin{0};   // in kHz
    qint64 freqMax{0};   // in kHz
    QString governor;
    bool online{true};
    QString energyPref;
};

/**
 * @brief Profile data container
 */
class Profile
{
public:
    QString name;
    QString filePath;
    bool isSystem{false};      // From /etc/cpupower_gui.d/
    bool isBuiltin{false};     // Generated default profile
    QMap<int, CpuProfileEntry> settings;  // cpu -> settings

    bool isValid() const { return !name.isEmpty(); }
    bool isCustom() const { return !isBuiltin && !isSystem; }
    bool canDelete() const { return isCustom(); }
};

/**
 * @brief Manages CPU power profiles
 * 
 * Handles:
 * - Loading profiles from system and user directories
 * - Generating default profiles (Balanced, Performance, etc.)
 * - Creating/deleting user profiles
 * - Applying profiles via D-Bus helper
 */
class ProfileManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList profileNames READ profileNames NOTIFY profilesChanged)
    Q_PROPERTY(int profileCount READ profileCount NOTIFY profilesChanged)

public:
    explicit ProfileManager(SysfsReader *sysfs, QObject *parent = nullptr);
    ~ProfileManager() override = default;

    // Profile list
    QStringList profileNames() const;
    int profileCount() const { return m_profiles.count(); }

    // Profile access
    Q_INVOKABLE bool hasProfile(const QString &name) const;
    Q_INVOKABLE bool isSystemProfile(const QString &name) const;
    Q_INVOKABLE bool isBuiltinProfile(const QString &name) const;
    Q_INVOKABLE bool canDeleteProfile(const QString &name) const;

    // Profile operations
    Q_INVOKABLE QVariantMap getProfileSettings(const QString &name) const;
    Q_INVOKABLE bool createProfile(const QString &name, const QVariantMap &settings);
    Q_INVOKABLE bool deleteProfile(const QString &name);
    Q_INVOKABLE void reload();

    // Get profile object (for internal use)
    const Profile *profile(const QString &name) const;

    // Static paths
    static QString systemProfileDir();
    static QString userProfileDir();

signals:
    void profilesChanged();
    void profileCreated(const QString &name);
    void profileDeleted(const QString &name);
    void error(const QString &message);

private:
    void loadProfiles();
    void loadProfilesFromDir(const QString &dirPath, bool isSystem);
    void generateDefaultProfiles();
    Profile parseProfileFile(const QString &filePath, bool isSystem) const;
    bool writeProfileFile(const Profile &profile) const;

    SysfsReader *m_sysfs;
    QMap<QString, Profile> m_profiles;
};

#endif // PROFILEMANAGER_H
