// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import org.kde.kirigami as Kirigami
import "../components"

Kirigami.ScrollablePage {
    id: profilesPage
    
    title: i18n("Profiles")
    
    actions: [
        Kirigami.Action {
            text: i18n("New Profile")
            icon.name: "list-add"
            onTriggered: newProfileDialog.open()
        },
        Kirigami.Action {
            text: i18n("Refresh")
            icon.name: "view-refresh"
            onTriggered: app.profileManager.reload()
        }
    ]
    
    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        
        // Profile list
        Kirigami.Card {
            Layout.fillWidth: true
            
            header: Kirigami.Heading {
                text: i18n("Available Profiles")
                level: 3
            }
            
            contentItem: ColumnLayout {
                spacing: 0
                
                Repeater {
                    model: app.profileModel
                    
                    delegate: Controls.ItemDelegate {
                        id: profileDelegate
                        
                        Layout.fillWidth: true
                        
                        required property int index
                        required property string name
                        required property bool isSystem
                        required property bool isBuiltin
                        
                        contentItem: RowLayout {
                            spacing: Kirigami.Units.smallSpacing
                            
                            Kirigami.Icon {
                                source: profileDelegate.isBuiltin ? "bookmark" : 
                                        profileDelegate.isSystem ? "system-lock-screen" : "document-save"
                                Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                                Layout.preferredHeight: Kirigami.Units.iconSizes.medium
                            }
                            
                            ColumnLayout {
                                spacing: Kirigami.Units.smallSpacing
                                Layout.fillWidth: true
                                
                                Controls.Label {
                                    text: profileDelegate.name
                                    font.bold: true
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                                
                                Controls.Label {
                                    text: profileDelegate.isBuiltin ? i18n("Built-in profile") :
                                          profileDelegate.isSystem ? i18n("System profile") :
                                          i18n("User profile")
                                    font: Kirigami.Theme.smallFont
                                    color: Kirigami.Theme.disabledTextColor
                                    Layout.fillWidth: true
                                }
                            }
                            
                            Controls.Button {
                                text: i18n("Apply")
                                icon.name: "dialog-ok-apply"
                                onClicked: {
                                    app.applyProfile(profileDelegate.name)
                                    applicationWindow().showPassiveNotification(
                                        i18n("Profile '%1' applied", profileDelegate.name)
                                    )
                                }
                            }
                            
                            Controls.Button {
                                icon.name: "edit-delete"
                                visible: !profileDelegate.isBuiltin && !profileDelegate.isSystem
                                enabled: visible
                                
                                Controls.ToolTip.text: i18n("Delete profile")
                                Controls.ToolTip.visible: hovered
                                
                                onClicked: {
                                    deleteConfirmDialog.profileName = profileDelegate.name
                                    deleteConfirmDialog.open()
                                }
                            }
                        }
                        
                        onClicked: {
                            profileDetailsDialog.profileName = profileDelegate.name
                            profileDetailsDialog.open()
                        }
                    }
                }
                
                // Empty state
                Kirigami.PlaceholderMessage {
                    Layout.fillWidth: true
                    visible: app.profileModel.rowCount() === 0
                    text: i18n("No profiles available")
                    explanation: i18n("Create a new profile to save your current CPU settings.")
                    
                    helpfulAction: Kirigami.Action {
                        text: i18n("New Profile")
                        icon.name: "list-add"
                        onTriggered: newProfileDialog.open()
                    }
                }
            }
        }
        
        // Help card
        Kirigami.Card {
            Layout.fillWidth: true
            
            header: Kirigami.Heading {
                text: i18n("About Profiles")
                level: 3
            }
            
            contentItem: Controls.Label {
                text: i18n("Profiles allow you to save and quickly restore CPU power settings.\n\n" +
                           "- Built-in profiles: Default profiles generated based on your hardware.\n" +
                           "- System profiles: Located in /etc/cpupower_gui.d/ (read-only).\n" +
                           "- User profiles: Located in ~/.config/cpupower_gui/ (can be modified).")
                wrapMode: Text.WordWrap
            }
        }
        
        Item {
            Layout.fillHeight: true
        }
    }
    
    // New profile dialog
    Kirigami.Dialog {
        id: newProfileDialog
        
        title: i18n("Create New Profile")
        standardButtons: Kirigami.Dialog.Ok | Kirigami.Dialog.Cancel
        
        onAccepted: {
            if (profileNameField.text.trim().length === 0) {
                applicationWindow().showPassiveNotification(
                    i18n("Please enter a profile name"), "short"
                )
                return
            }
            
            // Get current settings and create profile
            var settings = {}
            var cpuCount = app.cpuModel.rowCount()
            
            for (var cpu = 0; cpu < cpuCount; cpu++) {
                settings[cpu] = {
                    "freqMin": app.currentMinFreq,
                    "freqMax": app.currentMaxFreq,
                    "governor": app.currentGovernor,
                    "online": true,
                    "energyPref": app.currentEnergyPref
                }
            }
            
            if (app.profileManager.createProfile(profileNameField.text.trim(), settings)) {
                applicationWindow().showPassiveNotification(
                    i18n("Profile '%1' created", profileNameField.text.trim())
                )
                profileNameField.text = ""
            } else {
                applicationWindow().showPassiveNotification(
                    i18n("Failed to create profile"), "short"
                )
            }
        }
        
        ColumnLayout {
            spacing: Kirigami.Units.largeSpacing
            
            Controls.Label {
                text: i18n("Save the current CPU settings as a new profile.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            
            Controls.TextField {
                id: profileNameField
                Layout.fillWidth: true
                placeholderText: i18n("Profile name")
            }
            
            Kirigami.InlineMessage {
                Layout.fillWidth: true
                type: Kirigami.MessageType.Information
                text: i18n("The profile will be saved with the current frequency and governor settings for all CPUs.")
            }
        }
    }
    
    // Delete confirmation dialog
    Kirigami.PromptDialog {
        id: deleteConfirmDialog
        
        property string profileName: ""
        
        title: i18n("Delete Profile")
        subtitle: i18n("Are you sure you want to delete the profile '%1'?", profileName)
        standardButtons: Kirigami.Dialog.Yes | Kirigami.Dialog.No
        
        onAccepted: {
            if (app.profileManager.deleteProfile(profileName)) {
                applicationWindow().showPassiveNotification(
                    i18n("Profile '%1' deleted", profileName)
                )
            } else {
                applicationWindow().showPassiveNotification(
                    i18n("Failed to delete profile"), "short"
                )
            }
        }
    }
    
    // Profile details dialog
    Kirigami.Dialog {
        id: profileDetailsDialog
        
        property string profileName: ""
        property var settings: app.profileManager.getProfileSettings(profileName)
        
        title: i18n("Profile: %1", profileName)
        standardButtons: Kirigami.Dialog.Close
        
        preferredWidth: Kirigami.Units.gridUnit * 25
        
        onProfileNameChanged: {
            if (profileName.length > 0) {
                settings = app.profileManager.getProfileSettings(profileName)
            }
        }
        
        ColumnLayout {
            spacing: Kirigami.Units.largeSpacing
            
            Kirigami.FormLayout {
                Layout.fillWidth: true
                
                Controls.Label {
                    Kirigami.FormData.label: i18n("Type:")
                    text: app.profileManager.isBuiltinProfile(profileDetailsDialog.profileName) ? i18n("Built-in") :
                          app.profileManager.isSystemProfile(profileDetailsDialog.profileName) ? i18n("System") :
                          i18n("User")
                }
                
                Controls.Label {
                    Kirigami.FormData.label: i18n("Can Delete:")
                    text: app.profileManager.canDeleteProfile(profileDetailsDialog.profileName) ? i18n("Yes") : i18n("No")
                }
            }
            
            Kirigami.Separator {
                Layout.fillWidth: true
            }
            
            Controls.Label {
                text: i18n("CPU Settings:")
                font.bold: true
            }
            
            // CPU settings list
            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: Kirigami.Units.gridUnit * 10
                clip: true
                
                model: Object.keys(profileDetailsDialog.settings || {})
                
                delegate: Controls.ItemDelegate {
                    width: parent ? parent.width : 0
                    
                    required property string modelData
                    
                    contentItem: ColumnLayout {
                        spacing: Kirigami.Units.smallSpacing
                        
                        Controls.Label {
                            text: i18n("CPU %1", modelData)
                            font.bold: true
                        }
                        
                        GridLayout {
                            columns: 2
                            columnSpacing: Kirigami.Units.largeSpacing
                            rowSpacing: Kirigami.Units.smallSpacing
                            
                            Controls.Label { text: i18n("Min Freq:") }
                            Controls.Label { 
                                text: profileDetailsDialog.settings && profileDetailsDialog.settings[modelData] ? 
                                      (profileDetailsDialog.settings[modelData].freqMin / 1000).toFixed(0) + " MHz" : "N/A"
                            }
                            
                            Controls.Label { text: i18n("Max Freq:") }
                            Controls.Label { 
                                text: profileDetailsDialog.settings && profileDetailsDialog.settings[modelData] ? 
                                      (profileDetailsDialog.settings[modelData].freqMax / 1000).toFixed(0) + " MHz" : "N/A"
                            }
                            
                            Controls.Label { text: i18n("Governor:") }
                            Controls.Label { 
                                text: profileDetailsDialog.settings && profileDetailsDialog.settings[modelData] ? 
                                      profileDetailsDialog.settings[modelData].governor : "N/A"
                            }
                        }
                    }
                }
            }
        }
    }
}
