// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    id: preferencesPage
    
    title: i18n("Preferences")
    
    // Page actions
    actions: [
        Kirigami.Action {
            text: i18n("Save")
            icon.name: "document-save"
            onTriggered: {
                appConfig.save()
                showPassiveNotification(i18n("Preferences saved"))
            }
        },
        Kirigami.Action {
            text: i18n("Reset")
            icon.name: "edit-undo"
            onTriggered: {
                appConfig.reload()
                showPassiveNotification(i18n("Preferences reset to saved values"))
            }
        }
    ]
    
    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        
        // Default Profile card
        Kirigami.Card {
            Layout.fillWidth: true
            
            header: Kirigami.Heading {
                text: i18n("Default Profile")
                level: 3
            }
            
            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing
                
                Controls.ComboBox {
                    id: defaultProfileCombo
                    Layout.fillWidth: true
                    
                    model: app.profileModel
                    textRole: "name"
                    
                    Component.onCompleted: updateCurrentIndex()
                    
                    function updateCurrentIndex() {
                        for (var i = 0; i < count; i++) {
                            if (model.data(model.index(i, 0), Qt.DisplayRole) === appConfig.defaultProfile) {
                                currentIndex = i
                                return
                            }
                        }
                        currentIndex = -1
                    }
                    
                    Connections {
                        target: appConfig
                        function onDefaultProfileChanged() {
                            defaultProfileCombo.updateCurrentIndex()
                        }
                    }
                    
                    onActivated: {
                        if (currentIndex >= 0) {
                            appConfig.defaultProfile = currentText
                        }
                    }
                }
                
                Controls.Label {
                    text: i18n("The default profile is applied automatically when the application starts or when 'Restore Defaults' is used.")
                    font: Kirigami.Theme.smallFont
                    color: Kirigami.Theme.disabledTextColor
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }
        
        // GUI Behavior card
        Kirigami.Card {
            Layout.fillWidth: true
            
            header: Kirigami.Heading {
                text: i18n("Behavior")
                level: 3
            }
            
            contentItem: ColumnLayout {
                spacing: Kirigami.Units.largeSpacing
                
                // All CPUs default
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing
                    
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        
                        Controls.Label {
                            text: i18n("Default to All CPUs")
                        }
                        
                        Controls.Label {
                            text: i18n("Apply settings to all CPUs by default instead of a single CPU")
                            font: Kirigami.Theme.smallFont
                            color: Kirigami.Theme.disabledTextColor
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                    
                    Controls.Switch {
                        checked: appConfig.allCpusDefault
                        onToggled: appConfig.allCpusDefault = checked
                    }
                }
                
                Kirigami.Separator {
                    Layout.fillWidth: true
                }
                
                // Minimize to tray
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing
                    
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        
                        Controls.Label {
                            text: i18n("Minimize to System Tray")
                        }
                        
                        Controls.Label {
                            text: i18n("Keep running in the system tray when the window is closed")
                            font: Kirigami.Theme.smallFont
                            color: Kirigami.Theme.disabledTextColor
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                    
                    Controls.Switch {
                        checked: appConfig.minimizeToTray
                        onToggled: appConfig.minimizeToTray = checked
                    }
                }
                
                Kirigami.Separator {
                    Layout.fillWidth: true
                }
                
                // Start minimized
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing
                    
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        
                        Controls.Label {
                            text: i18n("Start Minimized")
                        }
                        
                        Controls.Label {
                            text: i18n("Start the application minimized to the system tray")
                            font: Kirigami.Theme.smallFont
                            color: Kirigami.Theme.disabledTextColor
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                    
                    Controls.Switch {
                        checked: appConfig.startMinimized
                        onToggled: appConfig.startMinimized = checked
                        enabled: appConfig.minimizeToTray
                    }
                }
            }
        }
        
        // Display Settings card
        Kirigami.Card {
            Layout.fillWidth: true
            
            header: Kirigami.Heading {
                text: i18n("Display Settings")
                level: 3
            }
            
            contentItem: ColumnLayout {
                spacing: Kirigami.Units.largeSpacing
                
                // Tick marks on sliders
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing
                    
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        
                        Controls.Label {
                            text: i18n("Show Frequency Tick Marks")
                        }
                        
                        Controls.Label {
                            text: i18n("Display tick marks on frequency sliders at available frequency steps")
                            font: Kirigami.Theme.smallFont
                            color: Kirigami.Theme.disabledTextColor
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                    
                    Controls.Switch {
                        checked: appConfig.tickMarksEnabled
                        onToggled: appConfig.tickMarksEnabled = checked
                    }
                }
                
                Kirigami.Separator {
                    Layout.fillWidth: true
                }
                
                // Show frequency numbers
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing
                    
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        
                        Controls.Label {
                            text: i18n("Show Numeric Frequency Labels")
                        }
                        
                        Controls.Label {
                            text: i18n("Display frequency values on tick marks (may be cluttered on some systems)")
                            font: Kirigami.Theme.smallFont
                            color: Kirigami.Theme.disabledTextColor
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                    
                    Controls.Switch {
                        checked: appConfig.frequencyTicksNumeric
                        onToggled: appConfig.frequencyTicksNumeric = checked
                        enabled: appConfig.tickMarksEnabled
                    }
                }
            }
        }
        
        // Energy Preference Settings card
        Kirigami.Card {
            Layout.fillWidth: true
            
            header: Kirigami.Heading {
                text: i18n("Energy Preference")
                level: 3
            }
            
            contentItem: ColumnLayout {
                spacing: Kirigami.Units.largeSpacing
                
                // Per-CPU energy preference
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing
                    
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        
                        Controls.Label {
                            text: i18n("Per-CPU Energy Preference")
                        }
                        
                        Controls.Label {
                            text: i18n("Allow setting different energy preferences for each CPU instead of a global setting")
                            font: Kirigami.Theme.smallFont
                            color: Kirigami.Theme.disabledTextColor
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                    
                    Controls.Switch {
                        checked: appConfig.energyPrefPerCpu
                        onToggled: appConfig.energyPrefPerCpu = checked
                    }
                }
            }
        }
        
        // Spacer
        Item {
            Layout.fillHeight: true
        }
    }
}
