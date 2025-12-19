// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#ifndef ENERGYPREFMODEL_H
#define ENERGYPREFMODEL_H

#include <QAbstractListModel>
#include <QStringList>

class EnergyPrefModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QStringList preferences READ preferences WRITE setPreferences NOTIFY preferencesChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        DisplayNameRole
    };

    explicit EnergyPrefModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_preferences.count(); }
    QStringList preferences() const { return m_preferences; }
    void setPreferences(const QStringList &prefs);

    Q_INVOKABLE QString preferenceAt(int index) const;
    Q_INVOKABLE int indexOf(const QString &pref) const;

signals:
    void countChanged();
    void preferencesChanged();

private:
    QStringList m_preferences;
};

#endif // ENERGYPREFMODEL_H
