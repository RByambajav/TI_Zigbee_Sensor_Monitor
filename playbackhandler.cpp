//***********************************************************************************
//*         ****
//*         ****
//*         ******o****
//*    ********_///_****        TEXAS INSTRUMENTS
//*     ******/_//_/*****
//*      ** ***(_/*****         PlaybackHandler.cpp
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

#include "playbackhandler.h"
#include "iohandler.h"


#define MAX_PKT_LENGTH 48


PlaybackHandler::PlaybackHandler(QObject *pParent)
{
    pMyParent = (QMainWindow *)pParent;

	connect(this, SIGNAL(packetReady()), this, SLOT(handlePacket()));
	connect(this, SIGNAL(dataReceived()), pParent, SLOT(dataReceived()));
    connect(this, SIGNAL(pingReceived()), pParent, SLOT(pingReceived()));
    connect(this, SIGNAL(noPingResponse()), pParent, SLOT(noPingResponse()));
	connect(this, SIGNAL(playbackStopped()), pParent, SLOT(stopPlayback()));

    // Create instance to Node Database.
    pNodeDb = new NodeDb;
	pingAnswered= false;

	// Initialise sync semaphore
	pSync= new QSemaphore(1);
}


PlaybackHandler::~PlaybackHandler()
{
	delete pSync;
	delete pNodeDb;
}


//-----------------------------------------------------------------------------
/** \brief Thread loop
 *
 *  Read log file entries, keep timming.
 */
//-----------------------------------------------------------------------------
void PlaybackHandler::run(void)
{
	unsigned char pPacketBuffer[MAX_PKT_LENGTH];
	QTextStream log(stdout);
	QTime prevTime;

    playbackFile.open(QFile::ReadOnly);
	QTextStream input(&playbackFile);
	ulong iDiff= ~0;
	fPlaying= true;
	pSync->acquire(1);

	while (fPlaying) {

		int nLength;

		// Read the packet from the file
		QString entry;
		entry= input.readLine();
		if (input.atEnd()) {
			fPlaying= false;
			break;
		}

		// Comment ?
		if (entry.at(0)=='#')
			continue;

		// Analyse time stamp
		QString qsTime;
		QStringList el= entry.split(' ');
		qsTime= el.at(0);

		QTime thisTime= QTime::fromString(qsTime,"hh:mm:ss");
		if (iDiff==~0) {
			prevTime= thisTime;
			iDiff= 0;
		} else {
			iDiff= prevTime.secsTo(thisTime);
		}
		prevTime= thisTime;

		log << qsTime + "\n";
		log.flush();

		// Sleep the required period
		pSync->tryAcquire(1,iDiff*1000);

		if (fPlaying) {

			// Get the rest of the packet
			QString qsLen= el.at(1);
			nLength= qsLen.toInt(0,16);
			QByteArray *packet = new QByteArray((char *)pPacketBuffer, nLength+3);

			// Save the packet on the Queue
			mutex.lock();
			timeStamp= thisTime;
			packet->clear();
			for (int i=0; i<nLength+3; i++) {
				QString qs= el.at(i+1);
				uchar v= qs.toInt(0,16);
				packet->append(v);
			}
			packets.enqueue(packet);
			mutex.unlock();

			// Start packet processing by receiving thread
			emit packetReady();

		}
	}

	pSync->release();
	playbackFile.close();
	emit playbackStopped();
}


//-----------------------------------------------------------------------------
/** \brief Open file
 *
 *  Get packet and analyze content.
 *
 *  \return
 *         \b bool. TRUE if file opens successfully, else FALSE.
 */
//-----------------------------------------------------------------------------
void PlaybackHandler::fileOpen(const QString &fileName)
{
	playbackFile.setFileName(fileName);
}


//-----------------------------------------------------------------------------
/** \brief Jump to next item
*/
//-----------------------------------------------------------------------------
void PlaybackHandler::next(void)
{
	pSync->release();
}


//-----------------------------------------------------------------------------
/** \brief Stop playback
 *
 *  Get packet and analyze content.
 *
*/
//-----------------------------------------------------------------------------
void PlaybackHandler::stop(void)
{
	fPlaying= false;
	pSync->release();
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
int PlaybackHandler::getPacket(QByteArray **pPacket)
{
    int length;

    //Read the packet from the Queue
    mutex.lock();

    if (!packets.isEmpty())
    {
        length = packets.at(0)->count();
        *pPacket = packets.dequeue();
    }
    else
    {
        length = 0;
    }

    mutex.unlock();

    return length;
}


//-----------------------------------------------------------------------------
/** \brief	Check if Ping has been received.
 *
 *	This functin will be called after timeout of ping timer.
 *
 */
//-----------------------------------------------------------------------------
void PlaybackHandler::checkPing() 
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
void PlaybackHandler::handlePacket() 
{
    QByteArray *pPacket;

    if (getPacket(&pPacket) > 0)
    {
        unsigned short cmdID;  // Command ID
        //LENGTH(1)-CMD(2)-DATA(....)-FCS(1)
        cmdID = MAKE_WORD_LE(pPacket,1);

		switch (cmdID)
		{
		case IoHandler::SYS_PING_RESPONSE:
			unsigned short profileID;
			profileID= MAKE_WORD_LE(pPacket,3);
			pNodeDb->setProfileID(profileID);

			pingAnswered = true;
			emit pingReceived();
			break;
		case IoHandler::ZB_RECEIVE_DATA_INDICATION:
			handleData(pPacket);
			break;
		default:
			emit unknownMessage();
		}

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
void PlaybackHandler::handleData(QByteArray *pPacket)
{
    NodeData *pData = new NodeData;
	pData->UpdateData(pPacket);
	pData->timeStamp= timeStamp;
	pNodeDb->addData(pData);

    emit dataReceived();
}


