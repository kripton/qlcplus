/*
  Q Light Controller Plus
  videocanvas.cpp

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

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QQmlEngine>
#include <QQuickItem>

#include "videocanvas.h"
#include "doc.h"
#include "app.h"

VideoCanvas::VideoCanvas(QQuickView *view, Doc *doc, QObject *parent)
    : PreviewContext(view, doc, "VIDEOCANVAS", parent)
    , m_editMode(false)
    , m_loadStatus(Cleared)
{
    Q_ASSERT(doc != nullptr);

    setContextResource("qrc:/VideoCanvas");
    setContextTitle(tr("Video Canvas"));
}

qreal VideoCanvas::pixelDensity() const
{
    App *app = qobject_cast<App *>(m_view);
    return app->pixelDensity();
}

void VideoCanvas::resetContents()
{
    m_loadStatus = Cleared;
}

bool VideoCanvas::editMode() const
{
    return m_editMode;
}

void VideoCanvas::setEditMode(bool editMode)
{
    if (m_editMode == editMode)
        return;

    m_editMode = editMode;
    emit editModeChanged(editMode);
}

VideoCanvas::LoadStatus VideoCanvas::loadStatus() const
{
    return m_loadStatus;
}

/*****************************************************************************
 * Load & Save
 *****************************************************************************/

bool VideoCanvas::loadXML(QXmlStreamReader &root)
{
    if (root.name() != KXMLQLCVideoCanvas)
    {
        qWarning() << Q_FUNC_INFO << "Video Canvas node not found";
        return false;
    }

    m_loadStatus = Loading;

    while (root.readNextStartElement())
    {
        //qDebug() << "VC tag:" << root.name();
        if (root.name() == KXMLQLCVideoCanvasProperties)
        {
            loadPropertiesXML(root);
        }
        else
        {
            qWarning() << Q_FUNC_INFO << "Unknown Virtual Console tag"
                       << root.name().toString();
            root.skipCurrentElement();
        }
    }

    m_loadStatus = Loaded;

    return true;
}

bool VideoCanvas::loadPropertiesXML(QXmlStreamReader &root)
{
    if (root.name() != KXMLQLCVideoCanvasProperties)
    {
        qWarning() << Q_FUNC_INFO << "Video Canvas properties node not found";
        return false;
    }

    QString str;
    while (root.readNextStartElement())
    {
        /** This is a legacy property, converted into
         *  VCFrame "WindowState" tag */
        if (root.name() == KXMLQLCVideoCanvasPropertiesSize)
        {
            QSize sz;

            /* Width */
            str = root.attributes().value(KXMLQLCVideoCanvasPropertiesSizeWidth).toString();
            if (str.isEmpty() == false)
                sz.setWidth(str.toInt());

            /* Height */
            str = root.attributes().value(KXMLQLCVideoCanvasPropertiesSizeHeight).toString();
            if (str.isEmpty() == false)
                sz.setHeight(str.toInt());

            root.skipCurrentElement();
        }
        else
        {
            qWarning() << Q_FUNC_INFO << "Unknown Video Canvas property tag:"
                       << root.name().toString();
            root.skipCurrentElement();
        }
    }

    return true;
}

bool VideoCanvas::saveXML(QXmlStreamWriter *doc)
{
    Q_ASSERT(doc != nullptr);

    /* Virtual Console entry */
    doc->writeStartElement(KXMLQLCVideoCanvas);

    /* Properties */
    //m_properties.saveXML(doc);

    /* End the <VideoCanvas> tag */
    doc->writeEndElement();

    return true;
}

void VideoCanvas::postLoad()
{

}
