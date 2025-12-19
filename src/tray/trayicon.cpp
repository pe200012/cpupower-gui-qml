// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#include "trayicon.h"
#include "config/profilemanager.h"

#include <QMenu>
#include <QAction>
#include <KLocalizedString>

TrayIcon::TrayIcon(QObject *parent)
    : QObject(parent)
{
    m_sni = new KStatusNotifierItem(QStringLiteral("cpupower-gui"), this);
    m_sni->setCategory(KStatusNotifierItem::Hardware);
    m_sni->setStatus(KStatusNotifierItem::Active);
    m_sni->setIconByName(QStringLiteral("cpu"));
    m_sni->setTitle(i18n("CPU Power GUI"));
    m_sni->setToolTipTitle(i18n("CPU Power GUI"));
    m_sni->setToolTipSubTitle(i18n("Manage CPU frequency and governor"));

    // Connect signals
    connect(m_sni, &KStatusNotifierItem::activateRequested, this, &TrayIcon::onActivateRequested);
    connect(m_sni, &KStatusNotifierItem::secondaryActivateRequested, this, &TrayIcon::onSecondaryActivateRequested);

    setupMenu();
}

TrayIcon::~TrayIcon() = default;

bool TrayIcon::isVisible() const
{
    return m_sni && m_sni->status() != KStatusNotifierItem::Passive;
}

void TrayIcon::setVisible(bool visible)
{
    if (!m_sni) {
        return;
    }

    const auto newStatus = visible ? KStatusNotifierItem::Active : KStatusNotifierItem::Passive;
    if (m_sni->status() != newStatus) {
        m_sni->setStatus(newStatus);
        emit visibleChanged();
    }
}

void TrayIcon::setProfileManager(ProfileManager *manager)
{
    m_profileManager = manager;
    if (m_profileManager) {
        connect(m_profileManager, &ProfileManager::profilesChanged, this, &TrayIcon::updateMenu);
    }
    updateMenu();
}

void TrayIcon::updateMenu()
{
    if (!m_sni) {
        return;
    }

    QMenu *menu = m_sni->contextMenu();
    if (!menu) {
        return;
    }

    // Clear existing profile actions (keep separator and quit)
    const QList<QAction *> actions = menu->actions();
    for (QAction *action : actions) {
        if (action->data().isValid()) {  // Profile actions have data
            menu->removeAction(action);
            delete action;
        }
    }

    // Add profile actions
    if (m_profileManager) {
        const QStringList profiles = m_profileManager->profileNames();

        // Insert before the separator
        QAction *separator = nullptr;
        for (QAction *action : menu->actions()) {
            if (action->isSeparator()) {
                separator = action;
                break;
            }
        }

        for (const QString &profile : profiles) {
            QAction *action = new QAction(profile, menu);
            action->setData(profile);
            connect(action, &QAction::triggered, this, &TrayIcon::onProfileAction);

            if (separator) {
                menu->insertAction(separator, action);
            } else {
                menu->addAction(action);
            }
        }
    }
}

void TrayIcon::show()
{
    setVisible(true);
}

void TrayIcon::hide()
{
    setVisible(false);
}

void TrayIcon::onActivateRequested(bool active, const QPoint &pos)
{
    Q_UNUSED(active)
    Q_UNUSED(pos)
    emit activateRequested();
}

void TrayIcon::onSecondaryActivateRequested(const QPoint &pos)
{
    Q_UNUSED(pos)
    // Middle-click - could be used for quick toggle or refresh
}

void TrayIcon::onProfileAction()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        const QString profileName = action->data().toString();
        emit profileSelected(profileName);
    }
}

void TrayIcon::setupMenu()
{
    if (!m_sni) {
        return;
    }

    QMenu *menu = new QMenu();

    // Profiles will be added dynamically via updateMenu()

    menu->addSeparator();

    // Quit action
    QAction *quitAction = new QAction(i18n("Quit"), menu);
    quitAction->setIcon(QIcon::fromTheme(QStringLiteral("application-exit")));
    connect(quitAction, &QAction::triggered, this, &TrayIcon::quitRequested);
    menu->addAction(quitAction);

    m_sni->setContextMenu(menu);
}
