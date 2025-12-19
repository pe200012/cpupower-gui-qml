// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/**
 * CpuSelector - A reusable component for selecting CPUs
 * 
 * Provides a ComboBox for selecting individual CPUs with an 
 * optional "All CPUs" toggle switch.
 */
RowLayout {
    id: cpuSelector
    
    // Properties
    property alias model: cpuCombo.model
    property int currentCpu: 0
    property bool allCpusSelected: false
    property bool showAllCpusToggle: true
    
    // Signals
    signal cpuSelected(int cpu)
    signal allCpusToggled(bool all)
    
    spacing: Kirigami.Units.smallSpacing
    
    Controls.Label {
        text: i18n("CPU:")
    }
    
    Controls.ComboBox {
        id: cpuCombo
        Layout.fillWidth: true
        enabled: !cpuSelector.allCpusSelected
        
        textRole: "display"
        currentIndex: cpuSelector.currentCpu
        
        onActivated: {
            cpuSelector.currentCpu = currentIndex
            cpuSelector.cpuSelected(currentIndex)
        }
    }
    
    Controls.Switch {
        visible: cpuSelector.showAllCpusToggle
        text: i18n("All CPUs")
        checked: cpuSelector.allCpusSelected
        
        onToggled: {
            cpuSelector.allCpusSelected = checked
            cpuSelector.allCpusToggled(checked)
        }
    }
    
    // Sync external changes
    onCurrentCpuChanged: {
        if (cpuCombo.currentIndex !== currentCpu) {
            cpuCombo.currentIndex = currentCpu
        }
    }
}
