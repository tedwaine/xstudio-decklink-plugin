// SPDX-License-Identifier: Apache-2.0
import QtQuick 2.12
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2
import Qt.labs.qmlmodels 1.0

import xStudio 1.0
import xstudio.qml.models 1.0
import xstudio.qml.helpers 1.0

Item {

	id: bmd_settings_dialog

    // this property is REQUIRED to set the docking widget size
    property var dockWidgetSize: 130

    // this is REQUIRED to ensure correct scaling
    anchors.fill: parent

    XsGradientRectangle {
        anchors.fill: bmd_settings_dialog
    }

    XsModuleData {

        id: decklink_settings
        modelDataName: "Decklink Settings"
    }

    XsAttributeValue {
        id: __decklinkEnabled
        attributeTitle: "Enabled"
        model: decklink_settings
    }
    property alias is_running: __decklinkEnabled.value

    XsAttributeValue {
        id: __status
        attributeTitle: "Status"
        model: decklink_settings
    }
    property alias statusMessage: __status.value

    XsAttributeValue {
        id: __startStop
        attributeTitle: "Start Stop"
        model: decklink_settings
    }
    property alias startStop: __startStop.value

    XsAttributeValue {
        id: __trackViewport
        attributeTitle: "Track Viewport"
        model: decklink_settings
    }
    property alias trackViewport: __trackViewport.value

    XsAttributeValue {
        id: __audioDelay
        attributeTitle: "Audio Sync Delay (milliseconds)"
        model: decklink_settings
    }
    property alias audioDelay: __audioDelay.value

    property var maxBoxWidth: 30

    ListModel{ 
        id: bmd_settings_attr_model
        ListElement{
            label_text: "SDI Output Resolution"
            model_name: "Decklink Settings"
            attr_name: "Output Resolution"        
            disable_when_running: true  
        }
        ListElement{
            label_text: "SDI Refresh Rate"
            model_name: "Decklink Settings"
            attr_name: "Refresh Rate"
            disable_when_running: true  
        }

        ListElement{
            label_text: "Pixel Format"
            model_name: "Decklink Settings"
            attr_name: "Pixel Format" 
            disable_when_running: true  
        }

        ListElement{
            label_text: "OCIO Display"
            model_name: "decklink_viewport_toolbar"
            attr_name: "Display"  
            disable_when_running: false  
        }

        ListElement{
            label_text: "Image Fit Mode"
            model_name: "decklink_viewport_toolbar"
            attr_name: "Fit (F)"
            disable_when_running: false
        }

    }

    property real button_radius: 3

    ColumnLayout {

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 5
        spacing: 10

        XsText {
            text: "SDI Controls"
            font.weight: Font.Bold
            font.pixelSize: XsStyleSheet.fontSize
            font.family: XsStyleSheet.fontFamily
            Layout.alignment: Qt.AlignHCenter
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "black"
        }

        Item {
            height: 5
        }

        XsText {
            text: "SDI Output"
            font.pixelSize: XsStyleSheet.fontSize
            font.family: XsStyleSheet.fontFamily
            Layout.alignment: Qt.AlignLeft
        }

        Item {

            id: startStopButton
            Layout.preferredHeight: 20
            Layout.preferredWidth: 80
            Layout.alignment: Qt.AlignHCenter

            Rectangle {
                height: parent.height
                width: parent.width/2
                color: "#900"
                radius: button_radius
                XsText {
                    text: "Off"
                    anchors.centerIn: parent
                    font.weight: Font.Bold
                    font.pixelSize: 10
                }
            }
    
            Rectangle {
                height: parent.height
                width: 10
                color: is_running ? "#090" : "#900"
                x: parent.width/2-button_radius
            }

            Rectangle {
                height: parent.height
                width: parent.width/2
                x: width
                color: "#090"
                radius: button_radius
                XsText {
                    text: "On"
                    anchors.centerIn: parent
                    font.weight: Font.Bold
                    font.pixelSize: 10
                }
            }

            Rectangle {
                height: parent.height
                width: parent.width/2
                x: is_running ? 0 : width
                color: "#333"
                radius: button_radius
                border.color: "#888"
                border.width: 2
                Behavior on x {NumberAnimation {duration: 50}}
            }

            Rectangle {
                anchors.fill: parent
                color: "transparent"
                radius: button_radius
                border.color: mouseArea2.containsMouse ? XsStyleSheet.highlightColor : "transparent"
                border.width: 1
            }

            MouseArea {
                id: mouseArea2
                hoverEnabled: true
                anchors.fill: startStopButton
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    // toggling the start stop value to get a callback on the C++
                    // side to toggle SDI output. That actual value of startStop
                    // is not important!
                    startStop = !startStop
                }
            }
        }

        Repeater {
            model: bmd_settings_attr_model
            DecklinkMultichoiceSetting {
                Layout.fillWidth: true
            }
        }

    }

}