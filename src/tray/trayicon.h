// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#ifndef TRAYICON_H
#define TRAYICON_H

#include <QObject>
#include <KStatusNotifierItem>

class Application;
class ProfileManager;

/**
 * @brief System tray icon using KStatusNotifierItem
 * 
 * Provides:
 * - Tray icon visibility
 * - Context menu with profile switching
 * - Quick actions
 */
class TrayIcon : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)

public:
    explicit TrayIcon(QObject *parent = nullptr);
    ~TrayIcon() override;

    bool isVisible() const;
    void setVisible(bool visible);

    void setProfileManager(ProfileManager *manager);

public slots:
    void updateMenu();
    void show();
    void hide();

signals:
    void visibleChanged();
    void activateRequested();
    void profileSelected(const QString &profileName);
    void quitRequested();

private slots:
    void onActivateRequested(bool active, const QPoint &pos);
    void onSecondaryActivateRequested(const QPoint &pos);
    void onProfileAction();

private:
    void setupMenu();

    KStatusNotifierItem *m_sni{nullptr};
    ProfileManager *m_profileManager{nullptr};
};

#endif // TRAYICON_H
