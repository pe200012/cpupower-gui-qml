// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/**
 * CpuTable - A table view showing CPU information
 * 
 * Displays current settings for all CPUs in a tabular format.
 */
ColumnLayout {
    id: cpuTable
    
    // Properties
    property alias model: listView.model
    property int selectedCpu: -1
    
    // Signals
    signal cpuClicked(int cpu)
    
    spacing: 0
    
    // Header row
    Rectangle {
        Layout.fillWidth: true
        height: headerRow.implicitHeight + Kirigami.Units.smallSpacing * 2
        color: Kirigami.Theme.alternateBackgroundColor
        
        RowLayout {
            id: headerRow
            anchors.fill: parent
            anchors.margins: Kirigami.Units.smallSpacing
            spacing: Kirigami.Units.largeSpacing
            
            Controls.Label {
                text: i18n("CPU")
                font.bold: true
                Layout.preferredWidth: 50
            }
            
            Controls.Label {
                text: i18n("Min (MHz)")
                font.bold: true
                Layout.preferredWidth: 80
            }
            
            Controls.Label {
                text: i18n("Max (MHz)")
                font.bold: true
                Layout.preferredWidth: 80
            }
            
            Controls.Label {
                text: i18n("Current (MHz)")
                font.bold: true
                Layout.preferredWidth: 100
            }
            
            Controls.Label {
                text: i18n("Governor")
                font.bold: true
                Layout.fillWidth: true
            }
            
            Controls.Label {
                text: i18n("Online")
                font.bold: true
                Layout.preferredWidth: 60
            }
        }
    }
    
    // CPU list
    ListView {
        id: listView
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        
        delegate: Controls.ItemDelegate {
            id: cpuDelegate
            
            width: listView.width
            
            required property int index
            required property int cpuNumber
            required property real freqMin
            required property real freqMax
            required property real currentFreq
            required property string governor
            required property bool online
            
            highlighted: cpuTable.selectedCpu === cpuDelegate.cpuNumber
            
            onClicked: {
                cpuTable.selectedCpu = cpuDelegate.cpuNumber
                cpuTable.cpuClicked(cpuDelegate.cpuNumber)
            }
            
            contentItem: RowLayout {
                spacing: Kirigami.Units.largeSpacing
                
                Controls.Label {
                    text: cpuDelegate.cpuNumber.toString()
                    Layout.preferredWidth: 50
                }
                
                Controls.Label {
                    text: (cpuDelegate.freqMin / 1000).toFixed(0)
                    Layout.preferredWidth: 80
                }
                
                Controls.Label {
                    text: (cpuDelegate.freqMax / 1000).toFixed(0)
                    Layout.preferredWidth: 80
                }
                
                Controls.Label {
                    text: cpuDelegate.online ? (cpuDelegate.currentFreq / 1000).toFixed(0) : "-"
                    Layout.preferredWidth: 100
                    font.bold: true
                    color: Kirigami.Theme.positiveTextColor
                }
                
                Controls.Label {
                    text: cpuDelegate.governor
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                
                Kirigami.Icon {
                    source: cpuDelegate.online ? "dialog-ok" : "dialog-cancel"
                    Layout.preferredWidth: Kirigami.Units.iconSizes.small
                    Layout.preferredHeight: Kirigami.Units.iconSizes.small
                }
                
                Item { Layout.preferredWidth: 40 }
            }
        }
    }
    
    // Empty state
    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        visible: listView.count === 0
        text: i18n("No CPUs detected")
    }
}
