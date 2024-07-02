// SPDX-License-Identifier: Apache-2.0
import QtQuick 2.12
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2
import Qt.labs.qmlmodels 1.0

import xStudio 1.1
import xstudio.qml.models 1.0
import xstudio.qml.helpers 1.0

XsComboBox {

    id: combo_box

    XsModuleData {
        id: model_data
        modelDataName: model_name
        onJsonChanged: {
            __value.index = search_recursive(attr_name, "title")
            __choices.index = search_recursive(attr_name, "title")
        }
    }
    XsModelProperty {
        id: __value
        role: "value"
        index: model_data.search_recursive(attr_name, "title")
    }
    property alias value: __value.value

    XsModelProperty {
        id: __choices
        role: "combo_box_options"
        index: model_data.search_recursive(attr_name, "title")
    }
    property alias choices: __choices.value

    model: choices

    onActivated: {
        if (value != choices[index]) {
            value = choices[index]
        }
    }

    onValueChanged: {
        if (choices != undefined && value != currentText) {
            currentIndex = choices.indexOf(value)
        }
    }

    property real modelWidth: 20.0

    TextMetrics {
        id: textMetrics
        font: combo_box.font
    }        

    onModelChanged: {
        if (currentIndex != model.indexOf(value) &&
            model.indexOf(value) != -1) {
            currentIndex = model.indexOf(value)
        }
        for(var i = 0; i < model.length; i++){
            textMetrics.text = model[i]
            modelWidth = Math.max(textMetrics.width, modelWidth)
        }
    }

}