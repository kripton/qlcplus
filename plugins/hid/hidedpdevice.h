/*
  Q Light Controller Plus
  hiddmxdevice.h

  Copyright (c) Massimo Callegari
                Florian Euchner
                Stefan Krupop

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

#ifndef HIDEDPDEVICE_H
#define HIDEDPDEVICE_H

#include <QObject>

#include "hiddevice.h"
#include "hidapi.h"

// Vendor IDs provided by http://www.linux-usb.org/usb.ids

#define HID_EDP_INTERFACE_VENDOR_ID    0x1209  // http://pid.codes
#define HID_EDP_INTERFACE_PRODUCT_ID   0x2040  // rp2040-usbdmx dongle

#define HID_DMX_READ_TIMEOUT 100

class HIDPlugin;

/*****************************************************************************
 * HIDEventDevice
 *****************************************************************************/

class HIDEDPDevice : public HIDDevice
{
    Q_OBJECT

public:
    HIDEDPDevice(HIDPlugin* parent, quint32 line, const QString& name, const QString& path);
    virtual ~HIDEDPDevice();

protected:
    /** Initialize the device, find out its capabilities etc. */
    void init();

    /** @reimp */
    bool hasInput() { return true; }

    /** @reimp */
    bool hasOutput() { return true; }

    /*********************************************************************
     * File operations
     *********************************************************************/
public:
    /** @reimp */
    bool openInput();

    /** @reimp */
    void closeInput();

    /** @reimp */
    bool openOutput();

    /** @reimp */
    void closeOutput();

    /** @reimp */
    QString path() const;

    /** @reimp */
    bool readEvent();

    /*********************************************************************
     * Device info
     *********************************************************************/
public:
    /** @reimp */
    QString infoText();

    /*********************************************************************
     * Input data
     *********************************************************************/
public:
    /** @reimp */
    void feedBack(quint32 channel, uchar value);

private:
    /** @reimp */
    void run();

    /*********************************************************************
     * Output data
     *********************************************************************/
public:
    /** @reimp */
    void outputDMX(const QByteArray &data, bool forceWrite = false);

     /*********************************************************************
     * EDP - specific functions and device handle
     *********************************************************************/
private:
    uint8_t inData[600];
    uint8_t outData[600];
    uint16_t maxSendChunkSize;
    size_t prepareDmxData_sizeOfDataToBeSent;  // Packetheader + payload length
    uint16_t prepareDmxData_chunkOffset;

    bool prepareDmxData(uint8_t universeId, uint16_t inDataSize, uint16_t* thisChunkSize, bool* callAgain);

    /** Last universe data that has been received */
    QByteArray m_dmx_in_cmp;

    /** Last universe data that has been output */
    QByteArray m_dmx_cmp;

    /** device handle for the interface */
    hid_device *m_handle;
};

#endif
