/*
  Q Light Controller Plus
  fixturemanager.cpp

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

#include <QQuickItem>
#include <QQmlEngine>
#include <QVariant>
#include <QDebug>
#include <QtMath>

#include "fixturemanager.h"
#include "qlcfixturemode.h"
#include "qlccapability.h"
#include "qlcfixturedef.h"
#include "fixture.h"
#include "doc.h"

FixtureManager::FixtureManager(QQuickView *view, Doc *doc, QObject *parent)
    : QObject(parent)
    , m_view(view)
    , m_doc(doc)
{
    Q_ASSERT(m_doc != NULL);

    qmlRegisterType<QLCCapability>("com.qlcplus.classes", 1, 0, "QLCCapability");

    m_fixtureTree = new TreeModel(this);
    QQmlEngine::setObjectOwnership(m_fixtureTree, QQmlEngine::CppOwnership);
    QStringList treeColumns;
    treeColumns << "classRef";
    m_fixtureTree->setColumnNames(treeColumns);
    m_fixtureTree->enableSorting(true);

    connect(m_doc, SIGNAL(loaded()),
            this, SLOT(slotDocLoaded()));
}

quint32 FixtureManager::invalidFixture()
{
    return Fixture::invalidId();
}

quint32 FixtureManager::fixtureForAddress(quint32 index)
{
    return m_doc->fixtureForAddress(index);
}

bool FixtureManager::addFixture(QString manuf, QString model, QString mode, QString name,
                                int uniIdx, int address, int channels, int quantity, quint32 gap,
                                qreal xPos, qreal yPos)
{
    qDebug() << Q_FUNC_INFO << manuf << model << quantity;

    QLCFixtureDef *fxiDef = m_doc->fixtureDefCache()->fixtureDef(manuf, model);
    Q_ASSERT(fxiDef != NULL);

    QLCFixtureMode *fxiMode = fxiDef->mode(mode);
    Q_ASSERT(fxiMode != NULL);

    for (int i = 0; i < quantity; i++)
    {
        Fixture *fxi = new Fixture(m_doc);

        /* If we're adding more than one fixture,
           append a number to the end of the name */
        if (quantity > 1)
            fxi->setName(QString("%1 #%2").arg(name).arg(i + 1));
        else
            fxi->setName(name);
        fxi->setAddress(address + (i * channels) + (i * gap));
        fxi->setUniverse(uniIdx);
        fxi->setFixtureDefinition(fxiDef, fxiMode);

        m_doc->addFixture(fxi);
        emit newFixtureCreated(fxi->id(), xPos, yPos);
    }
    m_fixtureList.clear();
    m_fixtureList = m_doc->fixtures();
    emit fixturesCountChanged();

    updateFixtureTree();

    return true;
}

QString FixtureManager::channelIcon(quint32 fxID, quint32 chIdx)
{
    Fixture *fixture = m_doc->fixture(fxID);
    if (fixture == NULL)
        return QString();

    const QLCChannel *channel = fixture->channel(chIdx);
    if (channel == NULL)
        return QString();

    QString chIcon = channel->getIconNameFromGroup(channel->group());
    if (chIcon.startsWith("#"))
    {
        if (chIcon == "#FF0000") return "qrc:/red.svg";
        else if (chIcon == "#00FF00") return "qrc:/green.svg";
        else if (chIcon == "#0000FF") return "qrc:/blue.svg";
        else if (chIcon == "#00FFFF") return "qrc:/cyan.svg";
        else if (chIcon == "#FF00FF") return "qrc:/magenta.svg";
        else if (chIcon == "#FFFF00") return "qrc:/yellow.svg";
        else if (chIcon == "#FF7E00") return "qrc:/amber.svg";
        else if (chIcon == "#FFFFFF") return "qrc:/white.svg";
        else if (chIcon == "#9400D3") return "qrc:/uv.svg";
    }
    else
        chIcon.replace(".png", ".svg");

    return "qrc" + chIcon;
}

void FixtureManager::setChannelValue(quint32 fixtureID, quint32 channelIndex, quint8 value)
{
    emit channelValueChanged(fixtureID, channelIndex, value);
}

void FixtureManager::setIntensityValue(quint8 value)
{
    emit channelTypeValueChanged(QLCChannel::Intensity, value);
}

void FixtureManager::setColorValue(quint8 red, quint8 green, quint8 blue,
                                   quint8 white, quint8 amber, quint8 uv)
{
    emit colorChanged(QColor(red, green, blue), QColor(white, amber, uv));
}

void FixtureManager::setPresetValue(int index, quint8 value)
{
    qDebug() << "[FixtureManager] setPresetValue - index:" << index << "value:" << value;
    QList<const QLCChannel*>channels = m_presetsCache.keys();

    if (index < 0 || index >= channels.count())
        return;

    const QLCChannel* ch = channels.at(index);
    emit presetChanged(ch, value);
}

QMultiHash<int, SceneValue> FixtureManager::setFixtureCapabilities(quint32 fxID, bool enable)
{
    int capDelta = 1;
    bool hasDimmer = false, hasColor = false, hasPosition = false;
    bool hasColorWheel = false, hasGobos = false;

    QMultiHash<int, SceneValue> channelsMap;

    Fixture *fixture = m_doc->fixture(fxID);
    if (fixture == NULL)
        return channelsMap;

    if (enable == false)
        capDelta = -1;

    for (quint32 ch = 0; ch < fixture->channels(); ch++)
    {
        const QLCChannel* channel(fixture->channel(ch));
        if(channel == NULL)
            continue;

        int chType = channel->group();

        switch (channel->group())
        {
            case QLCChannel::Intensity:
            {
                QLCChannel::PrimaryColour col = channel->colour();
                switch (col)
                {
                    case QLCChannel::NoColour:
                        hasDimmer = true;
                    break;
                    case QLCChannel::Red:
                    case QLCChannel::Green:
                    case QLCChannel::Blue:
                    case QLCChannel::Cyan:
                    case QLCChannel::Magenta:
                    case QLCChannel::Yellow:
                    case QLCChannel::White:
                        hasColor = true;
                    break;
                    default: break;
                }
                chType = col;
            }
            break;
            case QLCChannel::Pan:
            case QLCChannel::Tilt:
                hasPosition = true;
            break;
            case QLCChannel::Colour:
            {
                hasColorWheel = true;
                if (enable)
                {
                    if (m_presetsCache.contains(channel) == false)
                    {
                        m_presetsCache[channel] = fxID;
                        emit colorWheelChannelsChanged();
                    }
                }
                else
                {
                    m_presetsCache.remove(channel);
                    emit colorWheelChannelsChanged();
                }
            }
            break;
            case QLCChannel::Gobo:
            {
                hasGobos = true;
                if (enable)
                {
                    if (m_presetsCache.contains(channel) == false)
                    {
                        m_presetsCache[channel] = fxID;
                        emit goboChannelsChanged();
                    }
                }
                else
                {
                    m_presetsCache.remove(channel);
                    emit goboChannelsChanged();
                }
            }
            break;
            default:
            break;
        }
        if (hasDimmer)
        {
            QQuickItem *capItem = qobject_cast<QQuickItem*>(m_view->rootObject()->findChild<QObject *>("capIntensity"));
            capItem->setProperty("counter", capItem->property("counter").toInt() + capDelta);
        }
        if (hasColor)
        {
            QQuickItem *capItem = qobject_cast<QQuickItem*>(m_view->rootObject()->findChild<QObject *>("capColor"));
            capItem->setProperty("counter", capItem->property("counter").toInt() + capDelta);
        }
        if (hasPosition)
        {
            QQuickItem *capItem = qobject_cast<QQuickItem*>(m_view->rootObject()->findChild<QObject *>("capPosition"));
            capItem->setProperty("counter", capItem->property("counter").toInt() + capDelta);
        }
        if (hasColorWheel)
        {
            QQuickItem *capItem = qobject_cast<QQuickItem*>(m_view->rootObject()->findChild<QObject *>("capColorWheel"));
            capItem->setProperty("counter", capItem->property("counter").toInt() + capDelta);
        }
        if (hasGobos)
        {
            QQuickItem *capItem = qobject_cast<QQuickItem*>(m_view->rootObject()->findChild<QObject *>("capGobos"));
            capItem->setProperty("counter", capItem->property("counter").toInt() + capDelta);
        }

        channelsMap.insert(chType, SceneValue(fxID, ch));
    }
    return channelsMap;
}

int FixtureManager::fixturesCount()
{
    return m_doc->fixtures().count();
}

QQmlListProperty<Fixture> FixtureManager::fixtures()
{
    m_fixtureList.clear();
    m_fixtureList = m_doc->fixtures();
    return QQmlListProperty<Fixture>(this, m_fixtureList);
}

QVariant FixtureManager::groupsModel()
{
    return QVariant::fromValue(m_fixtureTree);
}

void FixtureManager::addFixturesToNewGroup(QList<quint32> fxList)
{
    FixtureGroup *group = new FixtureGroup(m_doc);
    m_doc->addFixtureGroup(group);
    group->setName(tr("New group %1").arg(group->id() + 1));

    // here we should perform a "smart" grid based
    // on the 2D position of the fixtures and their heads number.
    // For now we use the "old" QLC+ mechanism of calculating an
    // equilateral grid size
    int headsCount = 0;
    foreach(quint32 id, fxList)
    {
        Fixture* fxi = m_doc->fixture(id);
        if (fxi != NULL)
            headsCount += fxi->heads();
    }
    qreal side = qSqrt(headsCount);
    if (side != qFloor(side))
        side += 1; // Fixture number doesn't provide a full square

    group->setSize(QSize(side, side));
    foreach(quint32 id, fxList)
        group->assignFixture(id);

    updateFixtureTree();
}

QVariantList FixtureManager::presetsChannels(QLCChannel::Group group)
{
    QVariantList prList;
    int i = 0;

    foreach(const QLCChannel *ch, m_presetsCache.keys())
    {
        if (ch->group() != group)
        {
            i++;
            continue;
        }
        quint32 fxID = m_presetsCache[ch];
        Fixture *fixture = m_doc->fixture(fxID);
        if (fixture == NULL)
            continue;

        const QLCFixtureDef *def = fixture->fixtureDef();
        if (def != NULL)
        {
            QVariantMap prMap;
            prMap.insert("name", QString("%1 - %2")
                                .arg(def->model())
                                .arg(ch->name()));
            prMap.insert("presetIndex", i);
            prList.append(prMap);
        }
        i++;
    }

    return prList;
}

void FixtureManager::updateFixtureTree()
{
    // create the fixture groups data model
    m_fixtureTree->clear();

    QStringList uniNames = m_doc->inputOutputMap()->universeNames();

    // add the current universes as groups
    foreach(Fixture *fixture, m_doc->fixtures())
    {
        QVariantList params;
        params.append(QVariant::fromValue(fixture));
        m_fixtureTree->addItem(fixture->name(), params, uniNames.at(fixture->universe()));
    }

    // add the actual Fixture Groups
    foreach (FixtureGroup* grp, m_doc->fixtureGroups())
    {
        foreach(quint32 fxID, grp->fixtureList())
        {
            Fixture *fixture = m_doc->fixture(fxID);
            if (fixture == NULL)
                continue;
            QVariantList params;
            params.append(QVariant::fromValue(fixture));
            m_fixtureTree->addItem(fixture->name(), params, grp->name());
        }
    }
    emit groupsModelChanged();
}

QVariantList FixtureManager::goboChannels()
{
    return presetsChannels(QLCChannel::Gobo);
}

QVariantList FixtureManager::colorWheelChannels()
{
    return presetsChannels(QLCChannel::Colour);
}

QVariantList FixtureManager::presetCapabilities(int index)
{
    QList<const QLCChannel*>channels = m_presetsCache.keys();

    qDebug() << "[FixtureManager] Requesting presets at index:" << index << "count:" << channels.count();

    if (index < 0 || index >= channels.count())
        return QVariantList();

    const QLCChannel* ch = channels.at(index);
    qDebug() << "[FixtureManager] Channel requested:" << ch->name() << "count:" << ch->capabilities().count();

    QVariantList var;
    foreach(QLCCapability *cap, ch->capabilities())
        var.append(QVariant::fromValue(cap));

    return var;
}

void FixtureManager::slotDocLoaded()
{
    m_fixtureList.clear();
    m_fixtureList = m_doc->fixtures();
    emit fixturesCountChanged();

    updateFixtureTree();
}
