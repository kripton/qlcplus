/*
  Q Light Controller Plus
  IconButton.qml

  Copyright (c) Massimo Callegari

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.1
import QtQuick.Controls.Private 1.0

Rectangle {
    id: baseIconButton
    width: 38
    height: 38
    visible: counter ? true : false

    property color bgColor: "#5F5F5F"
    property color hoverColor: "#B6B6B6"
    property color pressColor: "#054A9E"
    property color checkedColor: "#0978FF"

    property bool checkable: false
    property bool checked: false
    property int counter: 1

    property ExclusiveGroup exclusiveGroup: null

    property string imgSource: ""

    property string tooltip: ""

    signal clicked
    signal toggled

    color: bgColor
    radius: 5
    border.color: "#1D1D1D"
    border.width: 2

    onExclusiveGroupChanged: {
        if (exclusiveGroup)
            exclusiveGroup.bindCheckable(baseIconButton)
    }
    onCheckedChanged: {
        if (checked == true) {
            baseIconButton.color = checkedColor
        }
        else {
            baseIconButton.color = bgColor
        }
    }

    Image {
        id: btnIcon
        anchors.fill: parent
        anchors.margins: 4
        source: imgSource
        sourceSize: Qt.size(width, height)
    }

    MouseArea {
        id: mouseArea1
        anchors.fill: parent
        hoverEnabled: true
        onEntered: { if (checked == false) baseIconButton.color = hoverColor }
        onExited: { if (checked == false) baseIconButton.color = bgColor; Tooltip.hideText() }
        onReleased: {
            if (checkable == true)
            {
                checked = !checked
                baseIconButton.toggled(checked);
            }
            else
                baseIconButton.clicked();
        }

        onCanceled: Tooltip.hideText()

        Timer {
           interval: 1000
           running: mouseArea1.containsMouse && tooltip.length
           onTriggered: Tooltip.showText(mouseArea1, Qt.point(mouseArea1.mouseX, mouseArea1.mouseY), tooltip)
        }

    }
}
