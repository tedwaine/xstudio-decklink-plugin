// SPDX-License-Identifier: Apache-2.0
import QtQuick 2.12
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.3
import Qt.labs.qmlmodels 1.0

import xStudio 1.0
import xstudio.qml.models 1.0
import xstudio.qml.helpers 1.0

ColumnLayout {

    property var toggle_attr_name
    property var display_name

    XsAttributeValue {
        id: the_attr
        attributeTitle: toggle_attr_name
        model: decklink_settings
    }
    property alias attr_value: the_attr.value

    XsLabel {
        text: display_name
        Layout.alignment: Qt.AlignLeft
    }

    XsCheckBox {
        Layout.alignment: Qt.AlignLeft
        checked: attr_value
        onClicked: attr_value = !attr_value
    }

}