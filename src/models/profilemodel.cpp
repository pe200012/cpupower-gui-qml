// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#include "profilemodel.h"
#include "config/profilemanager.h"

ProfileModel::ProfileModel(ProfileManager *manager, QObject *parent)
    : QAbstractListModel(parent)
    , m_manager(manager)
{
    loadProfiles();

    connect(m_manager, &ProfileManager::profilesChanged, this, &ProfileModel::refresh);
}

void ProfileModel::loadProfiles()
{
    beginResetModel();
    m_profiles = m_manager->profileNames();
    endResetModel();
    Q_EMIT countChanged();
}

int ProfileModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_profiles.count();
}

QVariant ProfileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_profiles.count()) {
        return QVariant();
    }

    const QString &name = m_profiles.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return name;
    case IsBuiltInRole:
        return isBuiltIn(name);
    case IsSystemRole:
        return isSystem(name);
    case IsUserRole:
        return !isBuiltIn(name) && !isSystem(name);
    case CanDeleteRole:
        return canDelete(name);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ProfileModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {IsBuiltInRole, "isBuiltIn"},
        {IsSystemRole, "isSystem"},
        {IsUserRole, "isUser"},
        {CanDeleteRole, "canDelete"}
    };
}

void ProfileModel::setCurrentIndex(int index)
{
    if (index != m_currentIndex) {
        m_currentIndex = index;
        Q_EMIT currentIndexChanged();
        Q_EMIT currentProfileChanged();
    }
}

QString ProfileModel::currentProfile() const
{
    if (m_currentIndex >= 0 && m_currentIndex < m_profiles.count()) {
        return m_profiles.at(m_currentIndex);
    }
    return QString();
}

QString ProfileModel::profileAt(int index) const
{
    if (index >= 0 && index < m_profiles.count()) {
        return m_profiles.at(index);
    }
    return QString();
}

QVariantMap ProfileModel::getProfileSettings(const QString &name) const
{
    return m_manager->getProfileSettings(name);
}

bool ProfileModel::isBuiltIn(const QString &name) const
{
    return m_manager->isBuiltinProfile(name);
}

bool ProfileModel::isSystem(const QString &name) const
{
    return m_manager->isSystemProfile(name);
}

bool ProfileModel::canDelete(const QString &name) const
{
    return m_manager->canDeleteProfile(name);
}

void ProfileModel::refresh()
{
    loadProfiles();
}

bool ProfileModel::deleteProfile(const QString &name)
{
    if (!canDelete(name)) {
        return false;
    }

    bool success = m_manager->deleteProfile(name);
    if (success) {
        loadProfiles();
        Q_EMIT profileDeleted(name);
    }
    return success;
}

bool ProfileModel::createProfile(const QString &name, const QVariantMap &settings)
{
    bool success = m_manager->createProfile(name, settings);
    if (success) {
        loadProfiles();
        Q_EMIT profileCreated(name);
    }
    return success;
}
