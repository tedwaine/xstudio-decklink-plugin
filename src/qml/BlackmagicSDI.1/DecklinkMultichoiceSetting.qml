// SPDX-License-Identifier: Apache-2.0
import QtQuick 2.12
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.3
import Qt.labs.qmlmodels 1.0

import xStudio 1.0
import xstudio.qml.models 1.0
import xstudio.qml.helpers 1.0

ColumnLayout {

    id: combo_box

    property var attr_name
    property var label_text
    property var attrs_model

    XsAttributeValue {
        id: __value
        attributeTitle: attr_name
        model: attrs_model
    }
    property alias value: __value.value

    XsAttributeValue {
        id: __choices
        attributeTitle: attr_name
        model: attrs_model
        role: "combo_box_options"
    }
    property alias choices: __choices.value

    XsLabel {
        text: label_text
        Layout.alignment: Qt.AlignLeft
    }

    XsComboBox {
        id: resolutionChoice
        model: choices
        Layout.fillWidth: true
        Layout.preferredHeight: 24
        property var value_: value ? value : null
        onValue_Changed: {
            currentIndex = indexOfValue(value_)
        }
        Component.onCompleted: currentIndex = indexOfValue(value_)
        onCurrentValueChanged: {
            if (value != currentValue) {
                value = currentValue;
            }
        }
    }

}