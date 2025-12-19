// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/**
 * FrequencySlider - A reusable component for adjusting CPU frequencies
 * 
 * Displays a labeled slider with min/max bounds and current value display.
 * Values are in kHz internally but displayed in MHz.
 */
ColumnLayout {
    id: frequencySlider
    
    // Properties
    property string label: i18n("Frequency")
    property real value: 0          // Current value in kHz
    property real from: 0           // Minimum value in kHz
    property real to: 0             // Maximum value in kHz
    property real stepSize: 100000  // Step size in kHz (default 100 MHz)
    property bool showBounds: true  // Show min/max labels
    
    // Signals
    signal moved(real newValue)
    
    spacing: Kirigami.Units.smallSpacing
    
    // Header row with label and value
    RowLayout {
        Layout.fillWidth: true
        
        Controls.Label {
            text: frequencySlider.label
        }
        
        Item { Layout.fillWidth: true }
        
        Controls.Label {
            text: (slider.value / 1000).toFixed(0) + " MHz"
            font.bold: true
        }
    }
    
    // Slider
    Controls.Slider {
        id: slider
        Layout.fillWidth: true
        
        from: frequencySlider.from
        to: frequencySlider.to
        value: frequencySlider.value
        stepSize: frequencySlider.stepSize
        
        onMoved: frequencySlider.moved(value)
        
        // Update external value when slider changes
        onValueChanged: {
            if (frequencySlider.value !== value) {
                frequencySlider.value = value
            }
        }
    }
    
    // Bounds row
    RowLayout {
        Layout.fillWidth: true
        visible: frequencySlider.showBounds
        
        Controls.Label {
            text: (frequencySlider.from / 1000).toFixed(0) + " MHz"
            font: Kirigami.Theme.smallFont
            color: Kirigami.Theme.disabledTextColor
        }
        
        Item { Layout.fillWidth: true }
        
        Controls.Label {
            text: (frequencySlider.to / 1000).toFixed(0) + " MHz"
            font: Kirigami.Theme.smallFont
            color: Kirigami.Theme.disabledTextColor
        }
    }
    
    // Sync external value changes to slider
    onValueChanged: {
        if (slider.value !== value) {
            slider.value = value
        }
    }
}
