// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import "../components"

Kirigami.ScrollablePage {
    id: settingsPage
    
    title: i18n("CPU Settings")
    
    // Page actions in the header
    actions: [
        Kirigami.Action {
            text: i18n("Apply")
            icon.name: "dialog-ok-apply"
            enabled: app.hasUnsavedChanges
            onTriggered: app.applyChanges()
        },
        Kirigami.Action {
            text: i18n("Reset")
            icon.name: "edit-undo"
            enabled: app.hasUnsavedChanges
            onTriggered: app.resetChanges()
        },
        Kirigami.Action {
            text: i18n("Refresh")
            icon.name: "view-refresh"
            onTriggered: app.refreshCpuInfo()
        },
        Kirigami.Action {
            text: app.allCpusSelected ? i18n("Single CPU") : i18n("All CPUs")
            icon.name: app.allCpusSelected ? "edit-select" : "edit-select-all"
            checkable: true
            checked: app.allCpusSelected
            onTriggered: app.allCpusSelected = !app.allCpusSelected
        }
    ]
    
    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        
        // Profile selection card
        Kirigami.Card {
            Layout.fillWidth: true
            
            header: Kirigami.Heading {
                text: i18n("Frequency Profile")
                level: 3
            }
            
            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing
                
                Controls.ComboBox {
                    id: profileCombo
                    Layout.fillWidth: true
                    
                    model: app.profileModel
                    textRole: "name"
                    
                    onActivated: {
                        if (currentIndex >= 0) {
                            app.applyProfile(currentText)
                        }
                    }
                }
                
                Controls.Label {
                    text: i18n("Select a profile to quickly apply predefined CPU settings")
                    font: Kirigami.Theme.smallFont
                    color: Kirigami.Theme.disabledTextColor
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }
        
        // CPU selection card
        Kirigami.Card {
            Layout.fillWidth: true
            
            header: Kirigami.Heading {
                text: i18n("CPU Selection")
                level: 3
            }
            
            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing
                
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing
                    
                    Controls.Label {
                        text: i18n("CPU:")
                    }
                    
                    Controls.ComboBox {
                        id: cpuCombo
                        Layout.fillWidth: true
                        enabled: !app.allCpusSelected
                        
                        model: app.cpuModel
                        textRole: "display"
                        
                        currentIndex: app.currentCpu
                        
                        onActivated: {
                            app.currentCpu = currentIndex
                        }
                    }
                    
                    Controls.Switch {
                        id: allCpusSwitch
                        text: i18n("All CPUs")
                        checked: app.allCpusSelected
                        onToggled: app.allCpusSelected = checked
                    }
                }
                
                // Online status (only for single CPU)
                RowLayout {
                    Layout.fillWidth: true
                    visible: !app.allCpusSelected && app.currentCpu > 0
                    spacing: Kirigami.Units.smallSpacing
                    
                    Controls.Label {
                        text: i18n("CPU Online:")
                    }
                    
                    Controls.Switch {
                        checked: app.cpuOnline
                        onToggled: app.setCpuOnline(checked)
                    }
                    
                    Item { Layout.fillWidth: true }
                }
            }
        }
        
        // CPU Table overview card
        Kirigami.Card {
            Layout.fillWidth: true
            
            header: Kirigami.Heading {
                text: i18n("CPU Overview")
                level: 3
            }
            
            contentItem: CpuTable {
                id: cpuTableView
                
                // Calculate height based on model count (header + rows)
                implicitHeight: Math.min((app.cpuModel ? app.cpuModel.count : 0) * 40 + 50, 400)
                
                model: app.cpuModel
                selectedCpu: app.currentCpu
                
                onCpuClicked: function(cpu) {
                    app.currentCpu = cpu
                    app.allCpusSelected = false
                }
            }
        }
        
        // Frequency settings card
        Kirigami.Card {
            Layout.fillWidth: true
            
            header: Kirigami.Heading {
                text: i18n("Frequency Settings")
                level: 3
            }
            
            contentItem: ColumnLayout {
                spacing: Kirigami.Units.largeSpacing
                
                // Min frequency slider
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing
                    
                    RowLayout {
                        Layout.fillWidth: true
                        
                        Controls.Label {
                            text: i18n("Minimum Frequency:")
                        }
                        
                        Item { Layout.fillWidth: true }
                        
                        Controls.Label {
                            text: (minFreqSlider.value / 1000).toFixed(0) + " MHz"
                            font.bold: true
                        }
                    }
                    
                    Controls.Slider {
                        id: minFreqSlider
                        Layout.fillWidth: true
                        
                        from: app.hardwareMinFreq
                        to: app.hardwareMaxFreq
                        value: app.currentMinFreq
                        stepSize: 100000 // 100 MHz steps
                        
                        onMoved: app.setMinFrequency(value)
                        
                        // Ensure min doesn't exceed max
                        onValueChanged: {
                            if (value > maxFreqSlider.value) {
                                maxFreqSlider.value = value
                            }
                        }
                    }
                    
                    // Tick marks for min frequency slider
                    Item {
                        id: minFreqTickContainer
                        Layout.fillWidth: true
                        Layout.preferredHeight: appConfig.tickMarksEnabled ? (appConfig.frequencyTicksNumeric ? 24 : 12) : 0
                        visible: appConfig.tickMarksEnabled
                        
                        property var availableFreqs: app.sysfsReader ? app.sysfsReader.availableFrequencies(app.currentCpu) : []
                        property real sliderMin: app.hardwareMinFreq
                        property real sliderMax: app.hardwareMaxFreq
                        property real sliderRange: sliderMax - sliderMin
                        
                        Repeater {
                            model: minFreqTickContainer.availableFreqs.length > 0 ? minFreqTickContainer.availableFreqs : []
                            
                            delegate: Item {
                                property real freq: modelData
                                property real normalizedPos: minFreqTickContainer.sliderRange > 0 ? 
                                    (freq - minFreqTickContainer.sliderMin) / minFreqTickContainer.sliderRange : 0
                                
                                x: normalizedPos * (minFreqTickContainer.width - 2)
                                y: 0
                                width: 2
                                height: parent.height
                                
                                Rectangle {
                                    width: 2
                                    height: 8
                                    color: Kirigami.Theme.disabledTextColor
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                
                                Controls.Label {
                                    visible: appConfig.frequencyTicksNumeric
                                    text: (freq / 1000).toFixed(1)
                                    font.pixelSize: Kirigami.Theme.smallFont.pixelSize - 2
                                    color: Kirigami.Theme.disabledTextColor
                                    anchors.top: parent.top
                                    anchors.topMargin: 8
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    horizontalAlignment: Text.AlignHCenter
                                }
                            }
                        }
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        
                        Controls.Label {
                            text: (app.hardwareMinFreq / 1000).toFixed(0) + " MHz"
                            font: Kirigami.Theme.smallFont
                            color: Kirigami.Theme.disabledTextColor
                        }
                        
                        Item { Layout.fillWidth: true }
                        
                        Controls.Label {
                            text: (app.hardwareMaxFreq / 1000).toFixed(0) + " MHz"
                            font: Kirigami.Theme.smallFont
                            color: Kirigami.Theme.disabledTextColor
                        }
                    }
                }
                
                Kirigami.Separator {
                    Layout.fillWidth: true
                }
                
                // Max frequency slider
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing
                    
                    RowLayout {
                        Layout.fillWidth: true
                        
                        Controls.Label {
                            text: i18n("Maximum Frequency:")
                        }
                        
                        Item { Layout.fillWidth: true }
                        
                        Controls.Label {
                            text: (maxFreqSlider.value / 1000).toFixed(0) + " MHz"
                            font.bold: true
                        }
                    }
                    
                    Controls.Slider {
                        id: maxFreqSlider
                        Layout.fillWidth: true
                        
                        from: app.hardwareMinFreq
                        to: app.hardwareMaxFreq
                        value: app.currentMaxFreq
                        stepSize: 100000 // 100 MHz steps
                        
                        onMoved: app.setMaxFrequency(value)
                        
                        // Ensure max doesn't go below min
                        onValueChanged: {
                            if (value < minFreqSlider.value) {
                                minFreqSlider.value = value
                            }
                        }
                    }
                    
                    // Tick marks for max frequency slider
                    Item {
                        id: maxFreqTickContainer
                        Layout.fillWidth: true
                        Layout.preferredHeight: appConfig.tickMarksEnabled ? (appConfig.frequencyTicksNumeric ? 24 : 12) : 0
                        visible: appConfig.tickMarksEnabled
                        
                        property var availableFreqs: app.sysfsReader ? app.sysfsReader.availableFrequencies(app.currentCpu) : []
                        property real sliderMin: app.hardwareMinFreq
                        property real sliderMax: app.hardwareMaxFreq
                        property real sliderRange: sliderMax - sliderMin
                        
                        Repeater {
                            model: maxFreqTickContainer.availableFreqs.length > 0 ? maxFreqTickContainer.availableFreqs : []
                            
                            delegate: Item {
                                property real freq: modelData
                                property real normalizedPos: maxFreqTickContainer.sliderRange > 0 ? 
                                    (freq - maxFreqTickContainer.sliderMin) / maxFreqTickContainer.sliderRange : 0
                                
                                x: normalizedPos * (maxFreqTickContainer.width - 2)
                                y: 0
                                width: 2
                                height: parent.height
                                
                                Rectangle {
                                    width: 2
                                    height: 8
                                    color: Kirigami.Theme.disabledTextColor
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                
                                Controls.Label {
                                    visible: appConfig.frequencyTicksNumeric
                                    text: (freq / 1000).toFixed(1)
                                    font.pixelSize: Kirigami.Theme.smallFont.pixelSize - 2
                                    color: Kirigami.Theme.disabledTextColor
                                    anchors.top: parent.top
                                    anchors.topMargin: 8
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    horizontalAlignment: Text.AlignHCenter
                                }
                            }
                        }
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        
                        Controls.Label {
                            text: (app.hardwareMinFreq / 1000).toFixed(0) + " MHz"
                            font: Kirigami.Theme.smallFont
                            color: Kirigami.Theme.disabledTextColor
                        }
                        
                        Item { Layout.fillWidth: true }
                        
                        Controls.Label {
                            text: (app.hardwareMaxFreq / 1000).toFixed(0) + " MHz"
                            font: Kirigami.Theme.smallFont
                            color: Kirigami.Theme.disabledTextColor
                        }
                    }
                }
            }
        }
        
        // Power settings card
        Kirigami.Card {
            Layout.fillWidth: true
            
            header: Kirigami.Heading {
                text: i18n("Power Settings")
                level: 3
            }
            
            contentItem: ColumnLayout {
                spacing: Kirigami.Units.largeSpacing
                
                // Governor selection
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing
                    
                    Controls.Label {
                        text: i18n("Governor:")
                    }
                    
                    Controls.ComboBox {
                        id: governorCombo
                        Layout.fillWidth: true
                        
                        model: app.governorModel
                        textRole: "name"
                        
                        // Find and set current index based on current governor
                        Component.onCompleted: updateCurrentIndex()
                        
                        function updateCurrentIndex() {
                            for (var i = 0; i < count; i++) {
                                if (model.data(model.index(i, 0), Qt.DisplayRole) === app.currentGovernor) {
                                    currentIndex = i
                                    return
                                }
                            }
                            currentIndex = -1
                        }
                        
                        Connections {
                            target: app
                            function onCurrentCpuStateChanged() {
                                governorCombo.updateCurrentIndex()
                            }
                        }
                        
                        onActivated: {
                            if (currentIndex >= 0) {
                                app.setGovernor(currentText)
                            }
                        }
                    }
                }
                
                // Energy preference selection (only visible when available)
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing
                    visible: app.energyPrefAvailable
                    
                    Controls.Label {
                        text: i18n("Energy Preference:")
                    }
                    
                    Controls.ComboBox {
                        id: energyPrefCombo
                        Layout.fillWidth: true
                        
                        model: app.energyPrefModel
                        textRole: "name"
                        
                        // Find and set current index based on current pref
                        Component.onCompleted: updateCurrentIndex()
                        
                        function updateCurrentIndex() {
                            for (var i = 0; i < count; i++) {
                                if (model.data(model.index(i, 0), Qt.DisplayRole) === app.currentEnergyPref) {
                                    currentIndex = i
                                    return
                                }
                            }
                            currentIndex = -1
                        }
                        
                        Connections {
                            target: app
                            function onCurrentCpuStateChanged() {
                                energyPrefCombo.updateCurrentIndex()
                            }
                        }
                        
                        onActivated: {
                            if (currentIndex >= 0) {
                                app.setEnergyPref(currentText)
                            }
                        }
                    }
                }
                
                // Help text for energy preference
                Controls.Label {
                    visible: app.energyPrefAvailable
                    text: i18n("Energy performance preference controls the trade-off between performance and power consumption.")
                    font: Kirigami.Theme.smallFont
                    color: Kirigami.Theme.disabledTextColor
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                
                // Warning when energy pref not available
                Kirigami.InlineMessage {
                    Layout.fillWidth: true
                    visible: !app.energyPrefAvailable
                    type: Kirigami.MessageType.Information
                    text: i18n("Energy performance preference is not available on this system. This feature requires Intel P-State or AMD P-State driver.")
                }
            }
        }
        
        // Unsaved changes indicator
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: app.hasUnsavedChanges
            type: Kirigami.MessageType.Warning
            text: i18n("You have unsaved changes. Click 'Apply' to save them.")
            
            actions: [
                Kirigami.Action {
                    text: i18n("Apply")
                    icon.name: "dialog-ok-apply"
                    onTriggered: app.applyChanges()
                },
                Kirigami.Action {
                    text: i18n("Reset")
                    icon.name: "edit-undo"
                    onTriggered: app.resetChanges()
                }
            ]
        }
        
        // Spacer
        Item {
            Layout.fillHeight: true
        }
    }
}
