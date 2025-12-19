// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "pages"
import "components"

Kirigami.ApplicationWindow {
    id: root
    
    title: i18n("CPU Power GUI")
    
    minimumWidth: Kirigami.Units.gridUnit * 25
    minimumHeight: Kirigami.Units.gridUnit * 30
    width: Kirigami.Units.gridUnit * 30
    height: Kirigami.Units.gridUnit * 35
    
    // Global drawer for navigation
    globalDrawer: Kirigami.GlobalDrawer {
        id: globalDrawer
        
        title: i18n("CPU Power GUI")
        titleIcon: "io.github.cpupower_gui.qt"
        
        isMenu: true
        
        actions: [
            Kirigami.Action {
                text: i18n("Settings")
                icon.name: "configure"
                onTriggered: {
                    pageStack.clear()
                    pageStack.push(settingsPage)
                }
            },
            Kirigami.Action {
                text: i18n("Profiles")
                icon.name: "view-list-icons"
                onTriggered: {
                    pageStack.clear()
                    pageStack.push(profilesPage)
                }
            },
            Kirigami.Action {
                text: i18n("Preferences")
                icon.name: "preferences-system"
                onTriggered: {
                    pageStack.clear()
                    pageStack.push(preferencesPage)
                }
            },
            Kirigami.Action {
                separator: true
            },
            Kirigami.Action {
                text: i18n("About CPU Power GUI")
                icon.name: "help-about"
                onTriggered: {
                    pageStack.pushDialogLayer(aboutPage)
                }
            },
            Kirigami.Action {
                text: i18n("Quit")
                icon.name: "application-exit"
                shortcut: StandardKey.Quit
                onTriggered: Qt.quit()
            }
        ]
    }
    
    // Context drawer for per-page actions  
    contextDrawer: Kirigami.ContextDrawer {
        id: contextDrawer
    }
    
    // Initial page
    pageStack.initialPage: settingsPage
    
    // Page components
    Component {
        id: settingsPage
        SettingsPage {}
    }
    
    Component {
        id: profilesPage
        ProfilesPage {}
    }
    
    Component {
        id: preferencesPage
        PreferencesPage {}
    }
    
    Component {
        id: aboutPage
        Kirigami.AboutPage {
            aboutData: {
                "displayName": i18n("CPU Power GUI"),
                "productName": "cpupower-gui",
                "componentName": "cpupower-gui",
                "shortDescription": i18n("GUI utility to change the CPU frequency and governor"),
                "homepage": "https://github.com/vagnum08/cpupower-gui",
                "bugAddress": "https://github.com/vagnum08/cpupower-gui/issues",
                "version": Qt.application.version,
                "licenses": [
                    {
                        "name": "GPL v3",
                        "text": "",
                        "spdx": "GPL-3.0-or-later"
                    }
                ],
                "copyrightStatement": i18n("Copyright (C) 2017-2024 [RnD]²"),
                "desktopFileName": "io.github.cpupower_gui.qt",
                "authors": [
                    {
                        "name": i18n("Evangelos Rigas"),
                        "task": i18n("Original Author"),
                        "emailAddress": "erigas@rnd2.org"
                    }
                ]
            }
        }
    }
    
    // Status bar message
    footer: Controls.ToolBar {
        visible: app.statusMessage.length > 0
        
        Controls.Label {
            anchors.centerIn: parent
            text: app.statusMessage
        }
    }
    
    // Handle errors from the backend
    Connections {
        target: app
        
        function onErrorOccurred(message) {
            errorOverlay.text = message
            errorOverlay.visible = true
        }
        
        function onApplySuccess() {
            showPassiveNotification(i18n("Settings applied successfully"))
        }
        
        function onApplyFailed(reason) {
            showPassiveNotification(i18n("Failed to apply settings: %1", reason), "long")
        }
    }
    
    // Error overlay
    Kirigami.InlineMessage {
        id: errorOverlay
        
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: Kirigami.Units.largeSpacing
        }
        
        type: Kirigami.MessageType.Error
        visible: false
        showCloseButton: true
        
        actions: [
            Kirigami.Action {
                text: i18n("Retry")
                icon.name: "view-refresh"
                onTriggered: {
                    app.refreshCpuInfo()
                    errorOverlay.visible = false
                }
            }
        ]
    }
}
