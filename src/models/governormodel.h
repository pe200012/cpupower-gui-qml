// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#ifndef GOVERNORMODEL_H
#define GOVERNORMODEL_H

#include <QAbstractListModel>
#include <QStringList>

/**
 * @brief Simple string list model for governors
 * 
 * Wraps a QStringList of available governors for use in QML ComboBox.
 */
class GovernorModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QStringList governors READ governors WRITE setGovernors NOTIFY governorsChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        DisplayNameRole  // Capitalized version
    };

    explicit GovernorModel(QObject *parent = nullptr);
    ~GovernorModel() override = default;

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Properties
    int count() const { return m_governors.count(); }
    QStringList governors() const { return m_governors; }
    void setGovernors(const QStringList &govs);

    // Helper
    Q_INVOKABLE QString governorAt(int index) const;
    Q_INVOKABLE int indexOf(const QString &governor) const;

signals:
    void countChanged();
    void governorsChanged();

private:
    QStringList m_governors;
};

#endif // GOVERNORMODEL_H
