// SPDX-License-Identifier: Apache-2.0
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.12

import xStudio 1.0

import xstudio.qml.models 1.0
import xstudio.qml.helpers 1.0

import "."

XsTrayButton {
    // prototype: true

    anchors.fill: parent
    text: "Draw"
    source: "qrc:/bmd_icons/sdi-logo.svg"
    tooltip: "Open the Colour Correction Panel.  Apply SOP and LGG colour offsets to selected Media."
    buttonPadding: pad
    toggled_on: is_running != undefined ? is_running : false
    onClicked: {
        settings_dlg.visible = !settings_dlg.visible
    }

    XsModuleData {
        id: decklink_settings
        modelDataName: "Decklink Settings"
        onJsonChanged: {
            running.index = search_recursive("Enabled", "title")
        }
    }
    XsModelProperty {
        id: running
        role: "value"
        index: decklink_settings.search_recursive("Enabled", "title")
    }
    property alias is_running: running.value

    DecklinkSettingsDialog {
        id: settings_dlg
        visible: false
    }

}