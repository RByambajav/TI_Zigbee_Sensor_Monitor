//***********************************************************************************
//*         ****
//*         ****
//*         ******o****
//*    ********_///_****        TEXAS INSTRUMENTS
//*     ******/_//_/*****
//*      ** ***(_/*****         IoHandler.cpp
//*          *********
//*           *****
//*            ***
//*
//* ZSensor Monitor
//*
//* Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/ 
//* 
//* 
//*  Redistribution and use in source and binary forms, with or without 
//*  modification, are permitted provided that the following conditions 
//*  are met:
//*
//*    Redistributions of source code must retain the above copyright 
//*    notice, this list of conditions and the following disclaimer.
//*
//*    Redistributions in binary form must reproduce the above copyright
//*    notice, this list of conditions and the following disclaimer in the 
//*    documentation and/or other materials provided with the   
//*    distribution.
//*
//*    Neither the name of Texas Instruments Incorporated nor the names of
//*    its contributors may be used to endorse or promote products derived
//*    from this software without specific prior written permission.
//*
//*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//***********************************************************************************
//* Compiler: MSVC++ 2005/2008 Express
//* Target platform: Windows+QT
//***********************************************************************************

#include <QTextStream>
#include <QStringListIterator>

#include "iohandler.h"


IoHandler::IoHandler(QObject *pParent)
{
    pMyParent = (QMainWindow *)pParent;

    comPort = new ComPort(this);

    RefreshComPort();

    connect(this, SIGNAL(dataReceived(/*int index*/)), pParent, SLOT(dataReceived(/*int index*/)));
    connect(this, SIGNAL(pingReceived()), pParent, SLOT(pingReceived()));
    connect(this, SIGNAL(noPingResponse()), pParent, SLOT(noPingResponse()));

    // Create instance to Node Database.
    pNodeDb = new NodeDb;

	pLog= NULL;
}


IoHandler::~IoHandler()
{
	if (pLog)
		delete pLog;
}


void IoHandler::setLogHandler(LogHandler *p)
{
	pLog= p;
}

//-----------------------------------------------------------------------------
/** \brief Open COM Port
 *
 *  Open COM port and start reading. 
 *
 *  \param[in] portName
 *          Name of the COM port. COMx
 *
 */
//-----------------------------------------------------------------------------
int IoHandler::openComPort(const QString &portName)
{
	unsigned int baudrate;

	// Determine baudrate; the MSP430 Virtual UART only supports 9600
	baudrate= portName.contains("MSP430 Application UART") ? 9600: 38400;

    if (comPort->Open(portName,baudrate))
    {
        if (comPort->Read())
        {
            // Check that the connection is established with a ping.
            // Start a one shot timer to check if ping response has been received.
            sendPing();
            pingAnswered = false; 
            QTimer::singleShot(TIMER_PING, this, SLOT(checkPing()));
            return IO_SUCCESS;
        }
        else
        {
            return IO_ERROR;
        }
    }
    else
    {
        return IO_ERROR;
    }
}

//-----------------------------------------------------------------------------
/** \brief Close COM Port
 *
 *  Close COM port 
 *
 */
//-----------------------------------------------------------------------------
int IoHandler::closeComPort()
{
    if (comPort->Close())
    {
        return IO_SUCCESS;
    }
    else
    {
        return IO_ERROR;
    }
}


//-----------------------------------------------------------------------------
/** \brief Refresh list of COM Port
 *
 *
 */
//-----------------------------------------------------------------------------
int IoHandler::RefreshComPort()
{
    QStringList lst;

    if (comPort->Enum(lst) == ComPort::CP_SUCCESS)
    {
        m_bComPortListOk = TRUE;
		//The filter has been removed for the moment. It will be a problem to forsee
		//all names used for drivers of USB to serial adapters.
		//QRegExp qre("^MSP430 Application UART.*|^Communications Port.*$");
		//m_lstComPort= lst.filter(qre);
		m_lstComPort = lst;

		return IO_SUCCESS;
    } else {

        m_bComPortListOk = FALSE;
        return IO_ERROR;
    }
}

//-----------------------------------------------------------------------------
/** \brief Get list of available COM ports.
 *
 *
 */
//-----------------------------------------------------------------------------
QStringList &IoHandler::getComPortList()
{
    return m_lstComPort;
}

//-----------------------------------------------------------------------------
/** \brief	Send Ping
 *
 *		Setup buffer with Ping message and write buffer to COM port.
 *
 *  \return
 *         \b bool. TRUE if COM-port openend successfully, else FALSE
 */
//-----------------------------------------------------------------------------
void IoHandler::sendPing() 
{
    unsigned char pBuffer[5];

    // When ping is answered this will be set true.
    // Will be checked when ping timout.
    pingAnswered = false;
    pBuffer[0] = CPT_SOP; // SOP
    pBuffer[1] = 0x00;    // MSB Command
    pBuffer[2] = 0x21;    // LSB Command
    pBuffer[3] = 01;      // Message length; 
    pBuffer[4] = CreateCrc(3, &pBuffer[1]);

    if (comPort->write(pBuffer, sizeof(pBuffer)) != sizeof(pBuffer))
    {
        QMessageBox::warning(pMyParent, tr("Ping"),tr("Not able to send Ping\nCheck that dongle is connected."),
            QMessageBox::AcceptRole, QMessageBox::AcceptRole);
    }
    
}

//-----------------------------------------------------------------------------
/** \brief	Check if Ping has been received.
 *
 *	This functin will be called after timeout of ping timer.
 *
 */
//-----------------------------------------------------------------------------
void IoHandler::checkPing() 
{
    if (!pingAnswered)
    {
        emit noPingResponse();
    }
}


//-----------------------------------------------------------------------------
/** \brief	Handle packet
 *
 *  Get packet and analyze content.
 *
 *  \return
 *         \b bool. TRUE if COM-port openend successfully, else FALSE
 */
//-----------------------------------------------------------------------------
void IoHandler::handlePacket() 
{
    QByteArray *pPacket;

    if (comPort->getThread()->getPacket(&pPacket) > 0)
    {
        unsigned short cmdID;  // Command ID
        //LENGTH(1)-CMD(2)-DATA(....)-FCS(1)
        cmdID = MAKE_WORD_LE(pPacket,1);

        switch (cmdID)
        {
            case SYS_PING_RESPONSE:
				unsigned short profileID;
				profileID= MAKE_WORD_LE(pPacket,3);
				pNodeDb->setProfileID(profileID);
                pingAnswered = true;
                emit pingReceived();
                break;
            case ZB_RECEIVE_DATA_INDICATION:
                handleData(pPacket);
                break;
            default:
                emit unknownMessage();
        }

		if (pLog)
			pLog->enterPacket(pPacket);
        
    }
    else
    {
        emit emptyPacket();
    }
    
}

//-----------------------------------------------------------------------------
/** \brief	Handle data
 *
 *  Parse packet and store data in Node data base.
 *  emit signal that data base has changed.
 *
 *  \param[in] pPacket
 *         Pointer to packet buffer
 */
//-----------------------------------------------------------------------------
void IoHandler::handleData(QByteArray *pPacket)
{
    NodeData *pData = new NodeData;
	pData->UpdateData(pPacket);
	pData->timeStamp= QTime::currentTime();

    pNodeDb->addData(pData);

    emit dataReceived();
}


//-----------------------------------------------------------------------------
/** \brief	Create CRC
 *
 *  Calculate the crc value
 *
 *  \param[in] length
 *         Length of current buffer.
 *  \param[in] pData
 *         Pointer to current buffer.
 *
 *  \return
 *         \b unsigned \b char. The calculated CRC value.
 */
//-----------------------------------------------------------------------------
unsigned char IoHandler::CreateCrc(int length, unsigned char *pData)
{
	int crc = 0;

	for(int i = 0; i < length; i++){
		crc ^= pData[i];
	}

	return crc;
}
