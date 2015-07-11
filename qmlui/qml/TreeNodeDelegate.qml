/*
  Q Light Controller Plus
  TreeNodeDelegate.qml

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

Rectangle
{
    id: nodeContainer
    width: 350
    height: nodeLabel.height + nodeChildrenView.height

    color: "transparent"

    property string textLabel
    property var folderChildren
    property bool isExpanded: false
    property bool isSelected: false
    property int childrenHeight: 0
    property int variableHeight: 0
    property string nodeIcon: "qrc:/folder.svg"
    property string childrenDelegate: "qrc:/FunctionDelegate.qml"

    signal toggled(bool expanded, int newHeight)
    signal clicked(var qItem)
    signal doubleClicked(int fID, int fType)

    Rectangle
    {
        width: parent.width
        height: 35
        radius: 3
        color: "#0978FF"
        visible: isSelected
    }

    Image
    {
        width: 40
        height: 35
        source: nodeIcon
    }

    RobotoText
    {
        id: nodeLabel
        x: 45
        width: parent.width
        height: 35
        label: textLabel
        fontSize: 11
    }

    MouseArea
    {
        width: parent.width
        height: 35
        onClicked:
        {
            isExpanded = !isExpanded
            nodeContainer.toggled(isExpanded, childrenHeight)
            isSelected = true
            nodeContainer.clicked(nodeContainer)
            //functionManager.selectFunction(-1, nodeContainer, false)
        }
    }

    function nodeToggled(expanded, height)
    {
        if (expanded)
            variableHeight += height;
        else
            variableHeight -= height;
    }

    ListView
    {
        id: nodeChildrenView
        visible: isExpanded
        x: 30
        y: nodeLabel.height
        height: isExpanded ? (childrenHeight + variableHeight) : 0
        model: folderChildren
        delegate:
            Component
            {
                Loader
                {
                    width: nodeContainer.width
                    //height: 35
                    source: hasChildren ? "qrc:/TreeNodeDelegate.qml" : childrenDelegate
                    onLoaded:
                    {
                        item.textLabel = label
                        if (hasChildren)
                        {
                            item.folderChildren = childrenModel
                            item.nodeIcon = nodeContainer.nodeIcon
                            item.childrenDelegate = childrenDelegate
                            item.childrenHeight = (childrenModel.rowCount() * 35)
                        }
                        else
                        {
                            item.cRef = classRef
                        }
                    }
                    Connections
                    {
                         target: item
                         onToggled: nodeToggled(item.isExpanded, item.childrenHeight)
                    }
                    Connections
                    {
                        target: item
                        onClicked: if (hasChildren) nodeContainer.clicked(item)
                    }
                    Connections
                    {
                        target: item
                        onDoubleClicked: nodeContainer.doubleClicked(ID, Type)
                    }
                }
        }
    }
}
