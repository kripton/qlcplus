/*
  Q Light Controller Plus
  mainview2d.cpp

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

#include <QDebug>
#include <QQuickItem>
#include <QQmlContext>
#include <QQmlComponent>

#include "doc.h"
#include "mainview2d.h"
#include "qlccapability.h"
#include "qlcfixturemode.h"
#include "monitorproperties.h"

MainView2D::MainView2D(QQuickView *view, Doc *doc, QObject *parent)
    : PreviewContext(view, doc, parent)
{
    m_gridSize = QSize(5, 5);
    m_gridScale = 1.0;
    m_cellPixels = 100;
    m_xOffset = 0;
    m_yOffset = 0;
    m_gridUnits = 1000;

    m_view2D = NULL;
    m_contents2D = NULL;

    fixtureComponent = new QQmlComponent(m_view->engine(), QUrl("qrc:/Fixture2DItem.qml"));
    if (fixtureComponent->isError())
        qDebug() << fixtureComponent->errors();

    connect(m_doc, SIGNAL(loaded()),
            this, SLOT(slotRefreshView()));
}

MainView2D::~MainView2D()
{
    resetItems();
}

void MainView2D::enableContext(bool enable)
{
    PreviewContext::enableContext(enable);
    if (enable == true)
        slotRefreshView();
}

void MainView2D::resetItems()
{
    QMapIterator<quint32, QQuickItem*> it(m_itemsMap);
    while(it.hasNext())
    {
        it.next();
        delete it.value();
    }
    m_itemsMap.clear();
}

void MainView2D::initialize2DProperties()
{
    m_view2D = qobject_cast<QQuickItem*>(m_view->rootObject()->findChild<QObject *>("twoDView"));
    m_contents2D = qobject_cast<QQuickItem*>(m_view->rootObject()->findChild<QObject *>("twoDContents"));

    if (m_view2D == NULL || m_contents2D == NULL)
        return;

    m_gridScale = m_view2D->property("gridScale").toReal();
    m_cellPixels = m_view2D->property("baseCellSize").toReal();

    m_xOffset = m_contents2D->property("x").toReal();
    m_yOffset = m_contents2D->property("y").toReal();
}

void MainView2D::createFixtureItem(quint32 fxID, qreal x, qreal y, bool mmCoords)
{
    if (isEnabled() == false)
        return;

    //if (m_view2D == NULL || m_contents2D == NULL)
        initialize2DProperties();

    qDebug() << "[MainView2D] Creating fixture with ID" << fxID << "x:" << x << "y:" << y;

    Fixture *fixture = m_doc->fixture(fxID);
    MonitorProperties *mProps = m_doc->monitorProperties();
    QLCFixtureMode *fxMode = fixture->fixtureMode();
    QRectF fxRect;

    QQuickItem *newFixtureItem = qobject_cast<QQuickItem*>(fixtureComponent->create());

    newFixtureItem->setParentItem(m_contents2D);
    newFixtureItem->setProperty("fixtureID", fxID);

    if (mProps->hasFixturePosition(fxID) == false)
    {
        if (mmCoords == false)
        {
            if (x == 0 && y == 0)
            {
                x = (m_xOffset * m_gridUnits) / m_cellPixels;
                y = (m_yOffset * m_gridUnits) / m_cellPixels;
            }
            else
            {
                x = ((x - m_xOffset) * m_gridUnits) / m_cellPixels;
                y = ((y - m_yOffset) * m_gridUnits) / m_cellPixels;
            }
        }
        fxRect.setX(x);
        fxRect.setY(y);
    }
    else
    {
        QPointF fxOrig = mProps->fixturePosition(fxID);
        fxRect.setX(fxOrig.x());
        fxRect.setY(fxOrig.y());
    }

    if (fxMode != NULL)
    {
        if (fxMode->physical().width() != 0)
            fxRect.setWidth(fxMode->physical().width());
        else
            fxRect.setWidth(300);

        if (fxMode->physical().height() != 0)
            fxRect.setHeight(fxMode->physical().height());
        else
            fxRect.setHeight(300);

        qDebug() << "Current mode fixture heads:" << fxMode->heads().count();
        newFixtureItem->setProperty("headsNumber", fxMode->heads().count());
    }
    else
    {
        // default to 300x300mm
        fxRect.setWidth(300);
        fxRect.setHeight(300);
    }

    newFixtureItem->setProperty("mmWidth", fxRect.width());
    newFixtureItem->setProperty("mmHeight", fxRect.height());

    if (mProps->hasFixturePosition(fxID) == false)
    {
        QPointF availablePos = m_doc->getAvailable2DPosition(fxRect);
        x = availablePos.x();
        y = availablePos.y();
        // add the new fixture to the Doc monitor properties
        mProps->setFixturePosition(fxID, QPointF(x, y));
    }
    else
    {
        QPointF fxOrig = mProps->fixturePosition(fxID);
        x = fxOrig.x();
        y = fxOrig.y();
    }

    newFixtureItem->setProperty("mmXPos", x);
    newFixtureItem->setProperty("mmYPos", y);
    newFixtureItem->setProperty("fixtureName", fixture->name());

    // and finally add the new item to the items map
    m_itemsMap[fxID] = newFixtureItem;

    updateFixture(fixture);
}

QList<quint32> MainView2D::selectFixturesRect(QRectF rect)
{
    QList<quint32>fxList;
    QMapIterator<quint32, QQuickItem *> it(m_itemsMap);
    while (it.hasNext())
    {
        it.next();
        QQuickItem *fxItem = it.value();
        qreal itemXPos = fxItem->property("x").toReal();
        qreal itemYPos = fxItem->property("y").toReal();
        qreal itemWidth = fxItem->property("width").toReal();
        qreal itemHeight = fxItem->property("height").toReal();

        QRectF itemRect(itemXPos, itemYPos, itemWidth, itemHeight);

        qDebug() << "Rect:" << rect << "itemRect:" << itemRect;

        if (rect.contains(itemRect))
        {
            if (fxItem->property("isSelected").toBool() == false)
            {
                fxItem->setProperty("isSelected", true);
                fxList.append(it.key());
            }
        }
    }
    return fxList;
}

void MainView2D::slotRefreshView()
{
    if (isEnabled() == false)
        return;

    initialize2DProperties();

    if (m_view2D == NULL || m_contents2D == NULL)
        return;

    MonitorProperties *mProps = m_doc->monitorProperties();
    QList<quint32> mPropsIDs;
    if (mProps)
    {
        mPropsIDs = mProps->fixtureItemsID();
        m_view2D->setProperty("gridWidth", mProps->gridSize().width());
        m_view2D->setProperty("gridHeight", mProps->gridSize().height());
        if (mProps->gridUnits() == MonitorProperties::Meters)
            m_view2D->setProperty("gridUnits", 1000);
        else
            m_view2D->setProperty("gridUnits", 304.8);
    }

    resetItems();

    foreach(Fixture *fixture, m_doc->fixtures())
    {
        if (mPropsIDs.contains(fixture->id()))
        {
            QPointF fxPos = mProps->fixturePosition(fixture->id());
            createFixtureItem(fixture->id(), fxPos.x(), fxPos.y());
        }
        else
            createFixtureItem(fixture->id(), 0, 0, false);
    }
}

void MainView2D::updateFixture(Fixture *fixture)
{
    if (m_enabled == false || fixture == NULL)
        return;

    if (m_itemsMap.contains(fixture->id()) == false)
        return;

    QQuickItem *fxItem = m_itemsMap[fixture->id()];
    bool colorSet = false;
    bool goboSet = false;

    for (int headIdx = 0; headIdx < fixture->heads(); headIdx++)
    {
        quint32 mdIndex = fixture->masterIntensityChannel(headIdx);
        qDebug() << "Head" << headIdx << "dimmer channel:" << mdIndex;
        qreal intValue = 1.0;
        if (mdIndex != QLCChannel::invalid())
            intValue = (qreal)fixture->channelValueAt(mdIndex) / 255;

        QMetaObject::invokeMethod(fxItem, "setHeadIntensity",
                Q_ARG(QVariant, headIdx),
                Q_ARG(QVariant, intValue));

        QVector <quint32> rgbCh = fixture->rgbChannels(headIdx);
        if (rgbCh.size() == 3)
        {
            quint8 r = 0, g = 0, b = 0;
            r = fixture->channelValueAt(rgbCh.at(0));
            g = fixture->channelValueAt(rgbCh.at(1));
            b = fixture->channelValueAt(rgbCh.at(2));

            QMetaObject::invokeMethod(fxItem, "setHeadColor",
                    Q_ARG(QVariant, headIdx),
                    Q_ARG(QVariant, QColor(r, g, b)));
            colorSet = true;
        }
        QVector <quint32> cmyCh = fixture->cmyChannels(headIdx);
        if (cmyCh.size() == 3)
        {
            quint8 c = 0, m = 0, y = 0;
            c = fixture->channelValueAt(cmyCh.at(0));
            m = fixture->channelValueAt(cmyCh.at(1));
            y = fixture->channelValueAt(cmyCh.at(2));
            QColor col;
            col.setCmyk(c, m, y, 0);
            QMetaObject::invokeMethod(fxItem, "setHeadColor",
                    Q_ARG(QVariant, headIdx),
                    Q_ARG(QVariant, QColor(col.red(), col.green(), col.blue())));
            colorSet = true;
        }
    }
    // now scan all the channels in search for color wheels and gobos
    for (quint32 i = 0; i < fixture->channels(); i++)
    {
        const QLCChannel *ch = fixture->channel(i);
        if (ch == NULL)
            continue;
        uchar value = fixture->channelValueAt(i);

        switch (ch->group())
        {
            case QLCChannel::Colour:
            {
                if(colorSet)
                    break;
                foreach(QLCCapability *cap, ch->capabilities())
                {
                    if (value >= cap->min() && value <= cap->max())
                    {
                        QColor wheelColor = cap->resourceColor1();
                        if (wheelColor.isValid())
                        {
                            QMetaObject::invokeMethod(fxItem, "setHeadColor",
                                    Q_ARG(QVariant, 0),
                                    Q_ARG(QVariant, wheelColor));
                            colorSet = true;
                        }
                    }
                }
            }
            break;
            case QLCChannel::Gobo:
            {
                if (goboSet)
                    break;

                foreach(QLCCapability *cap, ch->capabilities())
                {
                    if (value >= cap->min() && value <= cap->max())
                    {
                        QString resName = cap->resourceName();

                        if(resName.isEmpty() == false && resName.contains("open.png") == false)
                        {
                            QMetaObject::invokeMethod(fxItem, "setGoboPicture",
                                    Q_ARG(QVariant, 0),
                                    Q_ARG(QVariant, resName));
                            // here we don't look for any other gobos, so if a
                            // fixture has more than one gobo wheel, the second
                            // one will be skipped if the first one has been set
                            // to a non-open gobo
                            goboSet = true;
                        }
                    }
                }
            }
            break;
            // ... more preset channels to be added here (Shutter ?)
            default:
            break;
        }
    }
}

