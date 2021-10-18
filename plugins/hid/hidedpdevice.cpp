/*
  Q Light Controller Plus
  hiddmxdevice.cpp

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

#include <errno.h>
#if !defined(WIN32) && !defined(Q_OS_WIN)
  #include <unistd.h>
#endif

#include <QApplication>
#include <QMessageBox>
#include <QByteArray>
#include <QObject>
#include <QString>
#include <QDebug>
#include <QFile>

#include "hidedpdevice.h"
#include "qlcmacros.h"
#include "hidapi.h"
#include "hidplugin.h"

// Efficient Dmx Protocol Commands
enum Edp_Commands : uint8_t {
    Ping                      = 0x00, // Payload: Serial number of requester
    Pong                      = 0x01, // Payload: Serial number of responder
    DmxDataAllZero            = 0x10, // Followed by 1 byte (universeId), no chunk header, no packet header
    DmxData                   = 0x11, // One command for compressed and uncompressed data, sent in chunks
    DmxDataRequest            = 0x12, // Poll the content of a universe
    DiscoveryRequest          = 0x20,
    DiscoveryRespone          = 0x21,
    DiscoveryMute             = 0x22,
    DiscoveryUnMuteAll        = 0x23,
};

// The smallest chunk size this is designed to work on is 32 bytes (RF24 max payload length)
// However, we need to transfer at most 512 byte (One DMX frame). How many chunks do we need?
// 1 byte COMMAND
// 1 byte "DmxData" chunk header (= universe & chunk counter)
//     = 30 byte DMX data per packet maximum
//       512 byte + 4 byte DmxData packet header (crc + universe + full/partial + partialOffset)
//       = 516 Byte DmxData Playload
//     516/30 = 18 packets MAX (= 540 byte)  => 5 bit required for the chunk counter => 32 possible values
// => since we could now count 31 chunks, we could also use even smaller chunk sizes (~18 byte)

// Special values for the chunk counter
// actually only 5 bit => 0-31
enum Edp_DmxData_ChunkCounter : uint8_t {
    FirstPacket               = 0
};

// Should occupy one byte
struct Edp_DmxData_ChunkHeader {
    uint8_t                   RESERVED0    : 2; // Reserved for future use ;)
    Edp_DmxData_ChunkCounter  chunkCounter : 5;
    bool                      lastChunk    : 1; // 0 = first or middle chunk, 1 = last chunk
};

// 4 byte
struct Edp_DmxData_PacketHeader {
    uint16_t              crc;
    uint8_t               compressed   : 1; // 0 = raw, 1 = compressed
    uint8_t               partial      : 1; // 0 = full frame, 1 = partial
    uint8_t               universeId   : 6; // Universe Id (64 possibilities)
    uint8_t               partialOffset;    // If partial: Position the frame starts at
};

HIDEDPDevice::HIDEDPDevice(HIDPlugin* parent, quint32 line, const QString &name, const QString& path)
    : HIDDevice(parent, line, name, path)
{
    m_capabilities = QLCIOPlugin::Output;
    init();
}

HIDEDPDevice::~HIDEDPDevice()
{
    closeInput();
    closeOutput();
    hid_close(m_handle);
}

void HIDEDPDevice::init()
{
    /* Device name */
    m_handle = hid_open_path(path().toUtf8().constData());

    if (!m_handle)
    {
        QMessageBox::warning(NULL, (tr("HID EDP Interface Error")),
            (tr("Unable to open %1. Make sure the udev rule is installed.").arg(name())),
             QMessageBox::AcceptRole, QMessageBox::AcceptRole);
        return;
    }

    /** Reset channels when opening the interface: */
    m_dmx_cmp.fill(0, 512);
    m_dmx_in_cmp.fill(0, 512);
    outputDMX(m_dmx_cmp, true);

    maxSendChunkSize = 64;
}

/*****************************************************************************
 * File operations
 *****************************************************************************/

bool HIDEDPDevice::openInput()
{
    return true;
}

void HIDEDPDevice::closeInput()
{
    //m_mode &= ~DMX_MODE_INPUT;
    //updateMode();
}

bool HIDEDPDevice::openOutput()
{
    //m_mode |= DMX_MODE_OUTPUT;
    //updateMode();

    return true;
}

void HIDEDPDevice::closeOutput()
{
    //m_mode &= ~DMX_MODE_OUTPUT;
    //updateMode();
}

QString HIDEDPDevice::path() const
{
    return m_file.fileName();
}

bool HIDEDPDevice::readEvent()
{
    return true;
}

/*****************************************************************************
 * Device info
 *****************************************************************************/

QString HIDEDPDevice::infoText()
{
    QString info;

    info += QString("<B>%1</B><P>").arg(m_name);

    return info;
}

/*****************************************************************************
 * Input data
 *****************************************************************************/

void HIDEDPDevice::feedBack(quint32 channel, uchar value)
{
    /* HID devices don't support feedback (yet) */
    Q_UNUSED(channel);
    Q_UNUSED(value);
}

void HIDEDPDevice::run()
{
    while(m_running == true)
    {
        /*
        unsigned char buffer[35];
        int size;

        size = hid_read_timeout(m_handle, buffer, 33, HID_DMX_READ_TIMEOUT);
        */
        /**
        * Protocol: 33 bytes in buffer[33]
        * [0]      = chunk, which is the offset by which the channel is calculated
        *            from, the nth chunk starts at address n * 32
        * [1]-[32] = channel values, where the nth value is the offset + n
        */
           /*
        while(size > 0)
        {
            if(size == 33)
            {
                unsigned short startOff = buffer[0] * 32;
                if (buffer[0] < 16)
                {
                    for (int i = 0; i < 32; i++)
                    
                        unsigned short channel = startOff + i;
                        unsigned char value = buffer[i + 1];
                        if ((unsigned char)m_dmx_in_cmp.at(channel) != value)
                        {
                            emit valueChanged(UINT_MAX, m_line, channel, value);
                            m_dmx_in_cmp[channel] = value;
                        }
                    }
                }
            }

            size = hid_read_timeout(m_handle, buffer, 33, HID_DMX_READ_TIMEOUT);
        }
        */
    }
}

/*****************************************************************************
 * Output data
 *****************************************************************************/

void HIDEDPDevice::outputDMX(const QByteArray &universe, bool forceWrite)
{
    Q_UNUSED(forceWrite)

    bool callAgain = false;
    uint16_t thisChunkSize = 0;

    int bytesWritten = 0;

    memset(inData, 0x00, 512);
    memcpy(inData, universe.data(), universe.size());

    callAgain = false;
    prepareDmxData(0, 512, &thisChunkSize, &callAgain);
    bytesWritten = hid_write(m_handle, (const unsigned char *)outData, thisChunkSize);
    while(callAgain) {
        prepareDmxData(0, 0, &thisChunkSize, &callAgain);
        bytesWritten = hid_write(m_handle, (const unsigned char *)outData, thisChunkSize);
    }
}


// Take data from inData, prepare the complete packet in scratch
// Then, chop it into chunks and store them in outData, one per call
bool HIDEDPDevice::prepareDmxData(uint8_t universeId, uint16_t inDataSize, uint16_t* thisChunkSize, bool* callAgain) {
    uint16_t limitedInDataSize;
    uint16_t partialSize;
    uint8_t* destination;
    uint16_t firstUsedChannel;
    uint16_t lastUsedChannel;

    struct Edp_DmxData_ChunkHeader* chunkHeader = (struct Edp_DmxData_ChunkHeader*)(outData + sizeof(Edp_Commands));
    struct Edp_DmxData_PacketHeader* packetHeader = (struct Edp_DmxData_PacketHeader*)(outData + sizeof(Edp_Commands) + sizeof(Edp_DmxData_ChunkHeader));

    if (inDataSize != 0) {
        // Start a new packet, discard existing data and chunks

        memset(outData, 0x00, 600);

        // Loop over the input data so we know:
        //   - if it's all empty / zero
        //   - what the first and last used channels are so can send a partial frame
        firstUsedChannel = 600;  // Some invalid value so we can detect if NO channel is in use
        lastUsedChannel = 600;   // Some invalid value so we can detect if NO channel is in use
        for (uint16_t i = 0; i < inDataSize; i++) {
            if ((firstUsedChannel == 600) && (inData[i] != 0)) {
                firstUsedChannel = i;
            }
            if (inData[i] != 0) {
                lastUsedChannel = i;
            }
        }

        // Special case: allZero packet
        if (lastUsedChannel == 600) {
            outData[0] = Edp_Commands::DmxDataAllZero;
            outData[1] = universeId;
            *thisChunkSize = 2;
            *callAgain = false;
            return true;
        }

        outData[0] = Edp_Commands::DmxData;
        packetHeader->universeId = universeId;

        limitedInDataSize = MIN(inDataSize, 512);

        prepareDmxData_chunkOffset = maxSendChunkSize;

        // IF NOT SUPPORT PARTIAL
        packetHeader->partial = 0;
        packetHeader->partialOffset = 0;
        partialSize = 512;
        // ELSE
        packetHeader->partial = 1;
        packetHeader->partialOffset = MIN(firstUsedChannel, 255);
        partialSize = MIN(lastUsedChannel, 511) - packetHeader->partialOffset + 1;
        //qDebug() << "prepareDMX: firstUsedChannel: " << firstUsedChannel << "lastUsedChannel: " << lastUsedChannel << "partialOffset: " <<  packetHeader->partialOffset << "partialSize: " << partialSize;

        // Compress inData to outData. If it's larger than the input, it will be overwritten later
        prepareDmxData_sizeOfDataToBeSent = 600 - sizeof(Edp_Commands) - sizeof(Edp_DmxData_ChunkHeader) - sizeof(Edp_DmxData_PacketHeader);
        destination = outData + sizeof(Edp_Commands) + sizeof(struct Edp_DmxData_ChunkHeader) + sizeof(Edp_DmxData_PacketHeader);
//        snappy::RawCompress((const char *)inData + packetHeader->partialOffset, partialSize, (char*)destination, &prepareDmxData_sizeOfDataToBeSent);

//        if (prepareDmxData_sizeOfDataToBeSent >= partialSize) {
//            LOG("Compressed size: %d (inSize: %d) => SENDING UNCOMPRESSED!", prepareDmxData_sizeOfDataToBeSent, partialSize);
            packetHeader->compressed = 0;
            memcpy(destination, inData + packetHeader->partialOffset, partialSize);
            prepareDmxData_sizeOfDataToBeSent = partialSize;
//         } else {
//            packetHeader->compressed = 1;
//         }

        // Calculate a CRC so the receivers know if they got all the correct chunks
        // CRC is over the complete "payload" = without the PacketHeader
        packetHeader->crc = qChecksum((const char*)(outData + sizeof(Edp_Commands) + sizeof(struct Edp_DmxData_ChunkHeader) + sizeof(Edp_DmxData_PacketHeader)), prepareDmxData_sizeOfDataToBeSent);

        // Increase the size of the packet by the prepended header
        prepareDmxData_sizeOfDataToBeSent += sizeof(struct Edp_DmxData_PacketHeader);

//        LOG("Size with packetHeader: %u", prepareDmxData_sizeOfDataToBeSent);

        // Make chunk 0 ready
        chunkHeader->chunkCounter = Edp_DmxData_ChunkCounter::FirstPacket;
        if ((prepareDmxData_sizeOfDataToBeSent + sizeof(Edp_Commands) + sizeof (struct Edp_DmxData_ChunkHeader)) <= maxSendChunkSize) {
            // Yay, only one chunk needed :D
            chunkHeader->lastChunk = true;
            *thisChunkSize = prepareDmxData_sizeOfDataToBeSent + sizeof(Edp_Commands) + sizeof (struct Edp_DmxData_ChunkHeader);
            *callAgain = false;
//            LOG("Only one chunk is needed :D Size: %u", prepareDmxData_sizeOfDataToBeSent + sizeof(Edp_Commands) + sizeof (struct Edp_DmxData_ChunkHeader));
            return true;
        }

        chunkHeader->lastChunk = false;

        *thisChunkSize = maxSendChunkSize;
        *callAgain = true;

//        LOG("Chunk 0 is ready! :D Size: %u", maxSendChunkSize);

        return true;

    } else {
        // Next chunk if available, otherwise return false
        // TODO: Check if there actually is a next chunk or if this was accidentally
        //       called without inDataSize

        // ChunkOffset points to the OLD chunk's data

        destination = outData + sizeof(Edp_Commands) + sizeof(struct Edp_DmxData_ChunkHeader);
        memcpy(destination, outData + prepareDmxData_chunkOffset, maxSendChunkSize - sizeof(Edp_Commands) - sizeof(struct Edp_DmxData_ChunkHeader));

        chunkHeader->chunkCounter = (Edp_DmxData_ChunkCounter)(chunkHeader->chunkCounter + 1);

//        LOG("Chunk %u is ready! chunkOffset: %u, maxSendChunkSize: %u, prepareDmxData_sizeOfDataToBeSent: %u",
//            chunkHeader->chunkCounter,
//            prepareDmxData_chunkOffset,
//            maxSendChunkSize,
//            prepareDmxData_sizeOfDataToBeSent);

        if (prepareDmxData_chunkOffset + maxSendChunkSize >= prepareDmxData_sizeOfDataToBeSent + sizeof(struct Edp_DmxData_PacketHeader)) {
            chunkHeader->lastChunk = true;
            *thisChunkSize = prepareDmxData_sizeOfDataToBeSent - prepareDmxData_chunkOffset + sizeof(struct Edp_DmxData_PacketHeader);
            *callAgain = false;
//            LOG("It's the last chunk! Size: %u %04x", *thisChunkSize, *thisChunkSize);
            return true;
        }

        prepareDmxData_chunkOffset = prepareDmxData_chunkOffset + (maxSendChunkSize - sizeof(Edp_Commands) - sizeof(Edp_DmxData_ChunkHeader));
        *callAgain = true;

        return true;
    }
}