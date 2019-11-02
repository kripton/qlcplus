/*
  Q Light Controller Plus
  dmxovernow.cpp

  Copyright (C) Massimo Callegari

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

#include "dmxovernow.h"

#include <QDebug>
#include <QDir>

DMXoverNOW::DMXoverNOW(DMXInterface *interface, quint32 outputLine)
    : DMXUSBWidget(interface, outputLine, DEFAULT_OUTPUT_FREQUENCY)
    , m_running(false)
{
}

DMXoverNOW::~DMXoverNOW()
{
    stop();

    if (m_file.isOpen() == true)
        m_file.close();
}

DMXUSBWidget::Type DMXoverNOW::type() const
{
    return DMXUSBWidget::DMXoverNow;
}

bool DMXoverNOW::checkReply(uint8_t request)
{
    QByteArray reply = m_file.readAll();

    qDebug() << Q_FUNC_INFO << "Reply: " << QString::number(reply[0], 16) << QString::number(reply[1], 16);

    // TODO: Base64 decode and check second byte's first bit

    return true;
}

QString DMXoverNOW::getDeviceName()
{
    QDir sysfsDriverDir("/sys/bus/usb/drivers/cp210x");
    QStringList devDirs = sysfsDriverDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    if (devDirs.length() != 1)
    {
        qWarning() << Q_FUNC_INFO << "Number of devices found:" << devDirs.length() << ". Expected: 1";
    }

    // 1- scan all the devices in the driver folder
    foreach (QString dir, devDirs)
    {
        if (dir.contains(".") &&
            dir.contains(":"))
        {
            // 2 - find all folders of this device device
            QStringList devSubDirs = QDir("/sys/bus/usb/drivers/cp210x/" + dir).entryList(QDir::Dirs | QDir::NoDotAndDotDot);

            foreach (QString subDir, devSubDirs)
            {
                if (subDir.startsWith("tty"))
                {
                    qDebug() << "Found dev's TTY:" << subDir;
                    return "/dev/" + subDir;
                }
            }
        }
    }
    return QString();
}

/****************************************************************************
 * Open & Close
 ****************************************************************************/
bool DMXoverNOW::open(quint32 line, bool input)
{
    Q_UNUSED(line)
    Q_UNUSED(input)

    QString ttyName = getDeviceName();

    if (ttyName.isEmpty())
        m_file.setFileName("/dev/ttyUSB0");
    else
        m_file.setFileName(ttyName);

    // Could we set the baudrate here?

    m_file.unsetError();
    if (m_file.open(QIODevice::ReadWrite | QIODevice::Unbuffered) == false)
    {
        qWarning() << "DMXoverNOW output" << m_file.fileName()
                   << "cannot be opened:" << m_file.errorString();
        return false;
    }

    struct termios2 tio;

    ioctl(m_file.handle(), TCGETS2, &tio);
    tio.c_cflag &= ~CBAUD;
    tio.c_cflag |= BOTHER;
    tio.c_ispeed = 921600;
    tio.c_ospeed = 921600;
    ioctl(m_file.handle(), TCSETS2, &tio);

    QByteArray initSequence;
    QByteArray b64encoded;

    /* Check connection */
    initSequence.append((char)1);
    initSequence.append((char)0);
    initSequence.append("QLC+ ");
    initSequence.append(APPVERSION);
    initSequence.append((char)0);
    b64encoded = initSequence.toBase64();
    b64encoded.append((char)0);
    if (m_file.write(b64encoded) == true)
    {
        if (checkReply(1) == false)
            qWarning() << Q_FUNC_INFO << name() << "Initialization failed";
    }
    else
        qWarning() << Q_FUNC_INFO << name() << "Initialization failed";

    // start the output thread
    start();

    return true;
}

bool DMXoverNOW::close(quint32 line, bool input)
{
    Q_UNUSED(line)
    Q_UNUSED(input)

    stop();

    if (m_file.isOpen() == true)
        m_file.close();

    return true;
}

QString DMXoverNOW::uniqueName(ushort line, bool input) const
{
    Q_UNUSED(line)
    Q_UNUSED(input)
    return QString("%1").arg(name());
}

/****************************************************************************
 * Name & Serial
 ****************************************************************************/

QString DMXoverNOW::additionalInfo() const
{
    QString info;

    info += QString("<P>");
    info += QString("<B>%1:</B> %2 (%3)").arg(QObject::tr("Protocol"))
                                         .arg("DMXoverNOW")
                                         .arg(QObject::tr("Output"));
    info += QString("<BR>");
    info += QString("<B>%1:</B> %2").arg(QObject::tr("Manufacturer"))
                                         .arg(vendor());
    info += QString("<BR>");
    info += QString("<B>%1:</B> %2").arg(QObject::tr("Serial number"))
                                                 .arg(serial());
    info += QString("</P>");

    return info;
}

/****************************************************************************
 * Write universe data
 ****************************************************************************/

bool DMXoverNOW::writeUniverse(quint32 universe, quint32 output, const QByteArray& data)
{
    Q_UNUSED(universe)
    Q_UNUSED(output)

    if (m_file.isOpen() == false)
        return false;

    //qDebug() << "Writing universe...";

    /*
    QByteArray sendData;
    sendData.append((char)0x21); // command
    sendData.append((char)0); // universeID
    sendData.append(data);

    QByteArray b64encoded;
    b64encoded.append(sendData.toBase64());
    b64encoded.append((char)0);

    if (m_file.write(sendData.toBase64()) <= 0)
    {
        qWarning() << Q_FUNC_INFO << name() << "will not accept DMX data";
    }
    */

    if (m_outputLines[0].m_universeData.size() == 0)
        m_outputLines[0].m_universeData.append(data);
    else
        m_outputLines[0].m_universeData.replace(0, data.size(), data);

    return true;
}

void DMXoverNOW::stop()
{
    if (isRunning() == true)
    {
        m_running = false;
        wait();
    }
}

void DMXoverNOW::run()
{
    qDebug() << "OUTPUT thread started";

    QElapsedTimer timer;

    m_running = true;

    if (m_outputLines[0].m_compareData.size() == 0)
        m_outputLines[0].m_compareData.fill(0, 512);

    // Wait for device to settle in case the device was opened just recently
    usleep(1000);

    while (m_running == true)
    {
        timer.restart();

        QByteArray sendData;
        sendData.append((char)0x21); // command
        sendData.append((char)0); // universeID
        sendData.append(m_outputLines[0].m_universeData);

        QByteArray b64encoded;
        b64encoded.append(sendData.toBase64());
        b64encoded.append((char)0);

        if (m_file.write(sendData.toBase64()) <= 0)
        {
            qWarning() << Q_FUNC_INFO << name() << "will not accept DMX data";
            continue;
        }

        int timetoSleep = m_frameTimeUs - (timer.nsecsElapsed() / 1000);
        if (timetoSleep < 0)
            qWarning() << "DMX output is running late !";
        else
            usleep(timetoSleep);
    }
}
