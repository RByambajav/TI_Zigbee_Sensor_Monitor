//***********************************************************************************
//*         ****
//*         ****
//*         ******o****
//*    ********_///_****        TEXAS INSTRUMENTS
//*     ******/_//_/*****
//*      ** ***(_/*****         LogHandler.cpp
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

#include "loghandler.h"
#include "iohandler.h"


LogHandler::LogHandler(QObject *pParent)
{
    pMyParent = (QMainWindow *)pParent;
}


LogHandler::~LogHandler()
{
	closeLog();
}


//-----------------------------------------------------------------------------
/** \brief Open logging
 *
 *  Open COM port and start reading. 
 *
 *  \param[in] fileName
 *          Name of the file to log to.
 *
 */
//-----------------------------------------------------------------------------
void LogHandler::openLog(const QString &fileName)
{
	QIODevice::OpenMode openMode= QFile::WriteOnly;

	if (fileName!=NULL) {
		logFile.setFileName(fileName);
		openMode|= QFile::Truncate;
	} else {
		openMode|= QFile::Append;
	}

	if (!logFile.isOpen()) {
		logFile.open(openMode);

		// Date stamp
		QTextStream log(&logFile);

		timeStamp= QDateTime::currentDateTime();
		qsTime= "# Logging started: ";
		qsTime+= timeStamp.toString(Qt::TextDate);
		log << qsTime + "\n";
	}

}


void LogHandler::closeLog(void)
{
	if (logFile.isOpen()) {
		// Date stamp
		QTextStream log(&logFile);

		timeStamp= QDateTime::currentDateTime();
		qsTime= "# Logging stopped: ";
		qsTime+= timeStamp.toString(Qt::TextDate);
		log << qsTime + "\n";
		logFile.close();
	}
}

//-----------------------------------------------------------------------------
/** \brief	Save a packet in the log file
 *
 *  \param[in] pPacket
 *         Pointer to packet buffer
 */
//-----------------------------------------------------------------------------
void LogHandler::enterPacket(QByteArray *pPacket)
{
	if (!logFile.isOpen())
		return;

	// Save time stamp
	QTime timeStamp =QTime::currentTime();
	QString qsTime;

	qsTime= timeStamp.toString("hh:mm:ss ");

	unsigned short cmd;  // Command ID
	cmd = MAKE_WORD_LE(pPacket,1);

	switch (cmd)
	{
	case IoHandler::SYS_PING_RESPONSE:
		// LEN(0,1)+CMD(1,2)+FCS(3,1) 
		logRawPacket(qsTime,pPacket);
		break;
	case IoHandler::ZB_RECEIVE_DATA_INDICATION:
		// LEN(0,1)+CMD(1,2)+ADDR(3,2)+COMMAND(5,2)+LEN(7,2)+TEMP(9,1)+VOLTAGE(10,1)+FCS(11,1) 
		logRawPacket(qsTime,pPacket);
		break;
	default:
		logRawPacket(qsTime,pPacket);
		break;
	}
}



void LogHandler::logRawPacket(QString &qsTime, QByteArray *pPacket)
{
	QTextStream log(&logFile);
	QString qs;
	int blen= pPacket->size();

	for (int i=0; i<blen; i++) {
		QString qsc;

		qsc.sprintf("%02x ", (uchar)pPacket->at(i) );
		qs.append(qsc);
	}
	log << qsTime;
	log << qs;
	log << "\n";
}
