// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#ifndef PROFILEMODEL_H
#define PROFILEMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariantMap>

class ProfileManager;

/**
 * @brief Data structure for profile settings
 */
struct ProfileSettings {
    int cpu;
    int freqMin;    // kHz
    int freqMax;    // kHz
    QString governor;
    bool online;
};

/**
 * @brief List model for CPU profiles
 * 
 * Provides a QAbstractListModel interface for the list of available profiles.
 */
class ProfileModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString currentProfile READ currentProfile NOTIFY currentProfileChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        IsBuiltInRole,
        IsSystemRole,
        IsUserRole,
        CanDeleteRole
    };

    explicit ProfileModel(ProfileManager *manager, QObject *parent = nullptr);
    ~ProfileModel() override = default;

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Properties
    int count() const { return m_profiles.count(); }
    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);
    QString currentProfile() const;

    // Actions
    Q_INVOKABLE QString profileAt(int index) const;
    Q_INVOKABLE QVariantMap getProfileSettings(const QString &name) const;
    Q_INVOKABLE bool isBuiltIn(const QString &name) const;
    Q_INVOKABLE bool isSystem(const QString &name) const;
    Q_INVOKABLE bool canDelete(const QString &name) const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool deleteProfile(const QString &name);
    Q_INVOKABLE bool createProfile(const QString &name, const QVariantMap &settings);

signals:
    void countChanged();
    void currentIndexChanged();
    void currentProfileChanged();
    void profileDeleted(const QString &name);
    void profileCreated(const QString &name);

private:
    void loadProfiles();

    ProfileManager *m_manager;
    QStringList m_profiles;
    int m_currentIndex = -1;  // -1 means "No profile"
};

#endif // PROFILEMODEL_H
