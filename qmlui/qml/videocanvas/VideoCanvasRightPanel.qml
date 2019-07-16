/*
  Q Light Controller Plus
  VideoCanvasRightPanel.qml

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

import QtQuick 2.0
import QtQuick.Controls 2.1

import org.qlcplus.classes 1.0
import "."

SidePanel
{
    Rectangle
    {
        id: sideBar
        width: collapseWidth
        height: parent.height
        color: "transparent"
        z: 2

        Column
        {
            anchors.horizontalCenter: parent.horizontalCenter
            width: iconSize
            spacing: 3

            ButtonGroup { id: videocanvasButtonsGroup }

            IconButton
            {
                id: editModeButton
                z: 2
                width: iconSize
                height: iconSize
                imgSource: "qrc:/edit.svg"
                checkable: true
                checked: videoCanvas.editMode
                ButtonGroup.group: videocanvasButtonsGroup
                tooltip: qsTr("Display the VideoCanvas properties")

                onCheckedChanged:
                {
                    videoCanvas.editMode = checked
                    if (checked == true)
                        // TODO: videoCanvasGeneralPropertis. Later: Per widget
                        loaderSource = "qrc:/VideoCanvasWidgetProperties.qml"
                    else
                        border.color = "#1D1D1D"
                    animatePanel(checked)
                }

                SequentialAnimation on border.color
                {
                    PropertyAnimation { to: "red"; duration: 1000 }
                    PropertyAnimation { to: "white"; duration: 1000 }
                    running: editModeButton.checked
                    loops: Animation.Infinite
                }
            }
            IconButton
            {
                id: funcEditor
                z: 2
                width: iconSize
                height: iconSize
                imgSource: "qrc:/functions.svg"
                tooltip: qsTr("Function Manager")
                checkable: true
                ButtonGroup.group: videocanvasButtonsGroup
                onToggled:
                {
                    if (checked == true)
                        loaderSource = "qrc:/FunctionManager.qml"
                    animatePanel(checked)
                }
            }

        }
    }
}

