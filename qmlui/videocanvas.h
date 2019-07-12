/*
  Q Light Controller Plus
  VIDEOCANVAS.h

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

#ifndef VIDEOCANVAS_H
#define VIDEOCANVAS_H

#include <QQuickView>
#include <QObject>
#include <QFont>
#include <QHash>

#include "previewcontext.h"

class QXmlStreamReader;
class QXmlStreamWriter;
class QLCInputSource;
class TreeModel;
class Doc;

#define KXMLQLCVideoCanvas "VideoCanvas"

#define KXMLQLCVideoCanvasProperties "Properties"
#define KXMLQLCVideoCanvasPropertiesSize "Size"
#define KXMLQLCVideoCanvasPropertiesSizeWidth "Width"
#define KXMLQLCVideoCanvasPropertiesSizeHeight "Height"

#define KXMLQLCVideoCanvasPropertiesInput "Input"
#define KXMLQLCVideoCanvasPropertiesInputUniverse "Universe"
#define KXMLQLCVideoCanvasPropertiesInputChannel "Channel"

class VideoCanvas : public PreviewContext
{
    Q_OBJECT

    Q_PROPERTY(bool editMode READ editMode WRITE setEditMode NOTIFY editModeChanged)

public:
    VideoCanvas(QQuickView *view, Doc *doc, QObject *parent = nullptr);

    /** Return the number of pixels in 1mm */
    qreal pixelDensity() const;

    /** Reset the Video Canvas contents to an initial state */
    void resetContents();

    /** Get/Set the VC edit mode flag */
    bool editMode() const;
    void setEditMode(bool editMode);

    enum LoadStatus
    {
        Cleared = 0,
        Loading,
        Loaded
    };

    /** Get the current VC load status */
    LoadStatus loadStatus() const;

signals:
    void editModeChanged(bool editMode);

protected:
    bool m_editMode;

    /** The current VC load status */
    LoadStatus m_loadStatus;


    /*********************************************************************
     * Load & Save
     *********************************************************************/
public:
    /** Load properties and contents from an XML tree */
    bool loadXML(QXmlStreamReader &root);

    /** Load the Virtual Console global properties XML tree */
    bool loadPropertiesXML(QXmlStreamReader &root);

    /** Save properties and contents to an XML document */
    bool saveXML(QXmlStreamWriter *doc);

    /** Do post-load cleanup & checks */
    void postLoad();
};

#endif // VIDEOCANVAS_H
