// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/**
 * ProfileSelector - A ComboBox for selecting and applying profiles
 * 
 * Provides profile selection with an optional apply button.
 */
RowLayout {
    id: profileSelector
    
    // Properties
    property alias model: profileCombo.model
    property string currentProfile: ""
    property bool showApplyButton: true
    
    // Signals
    signal profileSelected(string profileName)
    signal applyClicked(string profileName)
    
    spacing: Kirigami.Units.smallSpacing
    
    Controls.Label {
        text: i18n("Profile:")
    }
    
    Controls.ComboBox {
        id: profileCombo
        Layout.fillWidth: true
        
        textRole: "name"
        
        onActivated: {
            profileSelector.currentProfile = currentText
            profileSelector.profileSelected(currentText)
        }
    }
    
    Controls.Button {
        visible: profileSelector.showApplyButton
        text: i18n("Apply")
        icon.name: "dialog-ok-apply"
        enabled: profileCombo.currentIndex >= 0
        
        onClicked: {
            if (profileCombo.currentIndex >= 0) {
                profileSelector.applyClicked(profileCombo.currentText)
            }
        }
    }
    
    // Find and select profile by name
    function selectProfile(name) {
        for (var i = 0; i < profileCombo.count; i++) {
            if (profileCombo.model.data(profileCombo.model.index(i, 0), Qt.DisplayRole) === name) {
                profileCombo.currentIndex = i
                currentProfile = name
                return true
            }
        }
        return false
    }
}
