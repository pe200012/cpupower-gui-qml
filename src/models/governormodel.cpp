// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#include "governormodel.h"

GovernorModel::GovernorModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int GovernorModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_governors.count();
}

QVariant GovernorModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_governors.count()) {
        return {};
    }

    const QString &governor = m_governors.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return governor;
    case DisplayNameRole:
        // Capitalize first letter for display
        if (governor.isEmpty()) {
            return governor;
        }
        return governor.at(0).toUpper() + governor.mid(1);
    default:
        return {};
    }
}

QHash<int, QByteArray> GovernorModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {DisplayNameRole, "displayName"}
    };
}

void GovernorModel::setGovernors(const QStringList &govs)
{
    if (m_governors == govs) {
        return;
    }

    beginResetModel();
    m_governors = govs;
    endResetModel();

    emit governorsChanged();
    emit countChanged();
}

QString GovernorModel::governorAt(int index) const
{
    if (index < 0 || index >= m_governors.count()) {
        return {};
    }
    return m_governors.at(index);
}

int GovernorModel::indexOf(const QString &governor) const
{
    return m_governors.indexOf(governor);
}
