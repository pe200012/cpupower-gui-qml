// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#ifndef DBUSHELPER_H
#define DBUSHELPER_H

#include <QObject>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QList>
#include <QString>
#include <QStringList>

/**
 * @brief D-Bus helper class for communicating with cpupower-gui-helper service
 * 
 * This class wraps all D-Bus communication with the io.github.cpupower_gui.qt.helper
 * service which runs as root and performs privileged operations on CPU settings.
 */
class DbusHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool authorized READ isAuthorized NOTIFY authorizedChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)

public:
    explicit DbusHelper(QObject *parent = nullptr);
    ~DbusHelper() override;

    // Connection status
    bool isConnected() const;
    bool isAuthorized();

    // CPU queries
    Q_INVOKABLE QList<int> cpusAvailable();
    Q_INVOKABLE QList<int> cpusOnline();
    Q_INVOKABLE QList<int> cpusOffline();
    Q_INVOKABLE QList<int> cpusPresent();
    Q_INVOKABLE QStringList getCpuGovernors(int cpu);
    Q_INVOKABLE bool cpuAllowedOffline(int cpu);

    // CPU mutations (return 0 on success, negative on error)
    Q_INVOKABLE int updateCpuSettings(int cpu, int fmin, int fmax);
    Q_INVOKABLE int updateCpuGovernor(int cpu, const QString &governor);
    Q_INVOKABLE int updateCpuEnergyPrefs(int cpu, const QString &pref);
    Q_INVOKABLE int setCpuOnline(int cpu);
    Q_INVOKABLE int setCpuOffline(int cpu);

signals:
    void authorizedChanged();
    void connectedChanged();
    void operationFailed(const QString &error);
    void helperReady(bool ready);
    void errorOccurred(const QString &error);

private:
    void connectToService();
    QVariant callMethod(const QString &method, const QVariantList &args = {});

    QDBusInterface *m_interface = nullptr;
    bool m_connected = false;

    static constexpr const char *SERVICE_NAME = "io.github.cpupower_gui.qt.helper";
    static constexpr const char *OBJECT_PATH = "/io/github/cpupower_gui/qt/helper";
    static constexpr const char *INTERFACE_NAME = "io.github.cpupower_gui.qt.helper";
};

#endif // DBUSHELPER_H
