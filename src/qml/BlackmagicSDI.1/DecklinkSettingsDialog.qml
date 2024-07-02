// SPDX-License-Identifier: Apache-2.0
import QtQuick 2.12
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2
import Qt.labs.qmlmodels 1.0

import xStudio 1.1
import xstudio.qml.models 1.0
import xstudio.qml.helpers 1.0

XsWindow {

	id: bmd_settings_dialog
	width: 400
	height: 500
    title: "Blackmagic Designs Decklink Output"
    centerOnOpen: true

    XsModuleData {
        id: decklink_settings
        modelDataName: "Decklink Settings"
        onJsonChanged: {
            running.index = search_recursive("Enabled", "title")
            __statusMessage.index = search_recursive("Status", "title")
            __startStop.index = search_recursive("Start Stop", "title")
            __trackViewport.index = search_recursive("Track Viewport", "title")
            __audioDelay.index = search_recursive("Audio Sync Delay (milliseconds)", "title")
        }
    }

    XsModelProperty {
        id: running
        role: "value"
        index: decklink_settings.search_recursive("Enable", "title")
    }
    property alias is_running: running.value

    property var foo: is_running

    XsModelProperty {
        id: __statusMessage
        role: "value"
        index: decklink_settings.search_recursive("Status", "title")
    }
    property alias statusMessage: __statusMessage.value

    XsModelProperty {
        id: __startStop
        role: "value"
        index: decklink_settings.search_recursive("Start Stop", "title")
    }
    property alias startStop: __startStop.value

    XsModelProperty {
        id: __trackViewport
        role: "value"
        index: decklink_settings.search_recursive("Track Viewport", "title")
    }
    property alias trackViewport: __trackViewport.value

    XsModelProperty {
        id: __audioDelay
        role: "value"
        index: decklink_settings.search_recursive("Audio Sync Delay (milliseconds)", "title")
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

    ColumnLayout {

        anchors.fill: parent
        anchors.margins: 20
        spacing: 10

        RowLayout {

            Layout.alignment: Qt.AlignHCenter
            spacing: 20

            Image {
                id: decklink_image
                source: "qrc:/bmd_icons/decklink_image.png"
                Layout.maximumWidth: 100
                Layout.maximumHeight: 100
            }

            XsText {
                text: "Blackmagic Designs\nDecklink SDI Output"
                font.pixelSize: 30
                lineHeight: 1.2
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter // Text.AlignLeft // Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        GridLayout {

            id: grid
            Layout.alignment: Qt.AlignHCenter
            columns: 2
            rowSpacing: 10
            columnSpacing: 10

            Text {
                Layout.alignment: Qt.AlignRight
                text: "SDI Output"
                color: XsStyle.controlColor
                font.family: XsStyle.controlTitleFontFamily
                font.pixelSize: XsStyle.popupControlFontSize
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
            }            
            
            Item {

                id: startStopButton
                Layout.preferredHeight: 30
                Layout.preferredWidth: 120

                Rectangle {
                    height: parent.height
                    width: parent.width/2
                    color: "#900"
                    radius: 5
                    XsText {
                        text: "Off"
                        anchors.centerIn: parent
                        font.weight: Font.Bold
                        font.pixelSize: 15
                    }
                }
        
                Rectangle {
                    height: parent.height
                    width: 10
                    color: is_running ? "#090" : "#900"
                    x: parent.width/2-5
                }

                Rectangle {
                    height: parent.height
                    width: parent.width/2
                    x: width
                    color: "#090"
                    radius: 5
                    XsText {
                        text: "On"
                        anchors.centerIn: parent
                        font.weight: Font.Bold
                        font.pixelSize: 15
                    }
                }

                Rectangle {
                    height: parent.height
                    width: parent.width/2
                    x: is_running ? 0 : width
                    color: "#333"
                    radius: 5
                    border.color: "#888"
                    border.width: 3
                    Behavior on x {NumberAnimation {duration: 50}}
                }

                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    radius: 5
                    border.color: mouseArea2.containsMouse ? XsStyle.highlightColor : "transparent"
                    border.width: 2
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

                Item {
                    Component.onCompleted: {
                        while (children.length) { children[0].parent = grid; }
                    }
        
                    Text {
                        Layout.alignment: Qt.AlignRight
                        Layout.column: 0
                        Layout.row: index+1
                        text: label_text
                        color: XsStyle.controlColor
                        font.family: XsStyle.controlTitleFontFamily
                        font.pixelSize: XsStyle.popupControlFontSize
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                    }
        
                    DecklinkMultichoiceSetting {
        
                        Layout.row: index+1
                        Layout.column: 1
                        Layout.preferredHeight: 24
                        Layout.preferredWidth: maxBoxWidth
                        onModelWidthChanged: {
                            maxBoxWidth = Math.max(maxBoxWidth, modelWidth)
                        }
                        enabled: disable_when_running ? !is_running : true
                    }    
                }
            }

            Text {

                Layout.row: 6
                Layout.column: 0
                Layout.alignment: Qt.AlignRight
                text: "Follow Main Viewport Pan/Zoom"
                color: XsStyle.controlColor
                font.family: XsStyle.controlTitleFontFamily
                font.pixelSize: XsStyle.popupControlFontSize
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
            }

            XsCheckbox {
                Layout.row: 6
                Layout.column: 1
                Layout.alignment: Qt.AlignLeft
                Layout.preferredHeight: 24
                id: bakeColorBox
                checked: trackViewport != undefined ? trackViewport : false
                onClicked: {
                    trackViewport = !trackViewport
                }
            }

            Text {

                Layout.row: 7
                Layout.column: 0
                Layout.alignment: Qt.AlignRight
                text: "Audio Delay (millisecs)"
                color: XsStyle.controlColor
                font.family: XsStyle.controlTitleFontFamily
                font.pixelSize: XsStyle.popupControlFontSize
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
            }

            Rectangle {

                color: XsStyle.mediaInfoBarOffsetBgColor
                border.width: 1
                border.color: ma3.containsMouse ? XsStyle.hoverColor : XsStyle.mainColor
                width: audioLatencyInput.font.pixelSize*6
                height: audioLatencyInput.font.pixelSize*1.4
                id: audioLatencyInputBox

                MouseArea {
                    id: ma3
                    anchors.fill: parent
                    hoverEnabled: true
                }

                TextInput {

                    anchors.fill: parent
                    id: audioLatencyInput
                    text: "" + audioDelay
                    width: font.pixelSize*2
                    color: enabled ? XsStyle.controlColor : XsStyle.controlColorDisabled
                    selectByMouse: true
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter

                    font {
                        family: XsStyle.fontFamily
                    }

                    onEditingFinished: {
                        focus = false
                        audioDelay = parseInt(text)
                    }
                }
            }

            Text {
                Layout.row: 8
                Layout.column: 0
                Layout.alignment: Qt.AlignRight | Qt.AlignTop
                
                text: "Status"
                color: XsStyle.controlColor
                font.family: XsStyle.controlTitleFontFamily
                font.pixelSize: XsStyle.popupControlFontSize
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
            }

            Rectangle {

                color: "#333"
                Layout.row: 8
                Layout.column: 1
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                Layout.preferredWidth: maxBoxWidth + 50
                Layout.preferredHeight: 40                

                Text {
                    anchors.fill: parent
                    anchors.margins: 10
                    text: statusMessage != undefined ? statusMessage : ""
                    color: XsStyle.controlColor
                    font.family: XsStyle.controlTitleFontFamily
                    font.pixelSize: XsStyle.popupControlFontSize
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignTop
                }
            }
        }

        Item {
            Layout.fillHeight: true
        }

    }

    RoundButton {
        id: btnOK
        text: qsTr("Close")
        width: 60
        height: 24
        radius: 5
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 5
        DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        background: Rectangle {
            radius: 5
//                color: XsStyle.highlightColor//mouseArea.containsMouse?:XsStyle.controlBackground
            color: mouseArea.containsMouse?XsStyle.primaryColor:XsStyle.controlBackground
            gradient:mouseArea.containsMouse?styleGradient.accent_gradient:Gradient.gradient
            anchors.fill: parent
        }
        contentItem: Text {
            text: btnOK.text
            color: XsStyle.hoverColor//:XsStyle.mainColor
            font.family: XsStyle.fontFamily
            font.hintingPreference: Font.PreferNoHinting
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        MouseArea {
            id: mouseArea
            hoverEnabled: true
            anchors.fill: btnOK
            cursorShape: Qt.PointingHandCursor
            onClicked: bmd_settings_dialog.close()
        }
    }

}