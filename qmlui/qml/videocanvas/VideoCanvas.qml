/*
  Q Light Controller Plus
  VideoCanvas.qml

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
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1

import org.qlcplus.classes 1.0
import "."

Rectangle
{
    id: videoCanvasContainer
    anchors.fill: parent
    color: "transparent"
    objectName: "videoCanvas"

    property string contextName: "VIDEOCANVAS"
    property bool docLoaded: qlcplus.docLoaded

    function enableContext(ctx, setChecked)
    {
        console.log("VIDEOCANVAS enable context " + ctx)
    }

    VideoCanvasRightPanel
    {
        id: rightSidePanel
        visible: qlcplus.accessMask & App.AC_VideoCanvas
        x: parent.width - width
        z: 5
        height: parent.height
    }

    Rectangle
    {
        id: videoCanvasView
        width: parent.width - rightSidePanel.width
        height: parent.height
        color: "transparent"
        clip: true

        Rectangle
        {
            id: videoCanvasToolbar
            width: parent.width
            height: UISettings.iconSizeMedium
            z: 10
            gradient: Gradient
            {
                id: vcTbGradient
                GradientStop { position: 0; color: UISettings.toolbarStartSub }
                GradientStop { position: 1; color: UISettings.toolbarEnd }
            }

            RowLayout
            {
                id: rowLayout1
                anchors.fill: parent
                spacing: 5
                ButtonGroup { id: videoCanvasToolbarGroup }

                // filler
                Rectangle { Layout.fillWidth: true }

                ZoomItem
                {
                    width: UISettings.iconSizeMedium * 2
                    implicitHeight: videoCanvasToolbar.height - 2
                    fontColor: UISettings.bgStrong
                    onZoomOutClicked: { virtualConsole.setPageScale(-0.1) }
                    onZoomInClicked: { virtualConsole.setPageScale(0.1) }
                }

                // TODO: "1:1" and "zoomToFit"
            }
        }

    }
}
