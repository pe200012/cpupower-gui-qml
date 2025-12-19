// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#include "energyprefmodel.h"

EnergyPrefModel::EnergyPrefModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int EnergyPrefModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_preferences.count();
}

QVariant EnergyPrefModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_preferences.count()) {
        return {};
    }

    const QString &pref = m_preferences.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return pref;
    case DisplayNameRole: {
        // Convert underscore-separated names to title case
        // e.g., "balance_performance" -> "Balance Performance"
        QString display = pref;
        display.replace(QLatin1Char('_'), QLatin1Char(' '));
        if (!display.isEmpty()) {
            display[0] = display[0].toUpper();
            for (int i = 1; i < display.length(); ++i) {
                if (display[i - 1] == QLatin1Char(' ')) {
                    display[i] = display[i].toUpper();
                }
            }
        }
        return display;
    }
    default:
        return {};
    }
}

QHash<int, QByteArray> EnergyPrefModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {DisplayNameRole, "displayName"}
    };
}

void EnergyPrefModel::setPreferences(const QStringList &prefs)
{
    if (m_preferences == prefs) {
        return;
    }

    beginResetModel();
    m_preferences = prefs;
    endResetModel();

    emit preferencesChanged();
    emit countChanged();
}

QString EnergyPrefModel::preferenceAt(int index) const
{
    if (index < 0 || index >= m_preferences.count()) {
        return {};
    }
    return m_preferences.at(index);
}

int EnergyPrefModel::indexOf(const QString &pref) const
{
    return m_preferences.indexOf(pref);
}
