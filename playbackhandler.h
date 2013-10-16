#ifndef PLAYBACKHANDLER_H
#define PLAYBACKHANDLER_H
//***********************************************************************************
//*         ****
//*         ****
//*         ******o****
//*    ********_///_****        TEXAS INSTRUMENTS
//*     ******/_//_/*****
//*      ** ***(_/*****         PlaybackHandler.h
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

#include <QMainWindow>
#include <QThread>
#include <QMutex>
#include <QString>
#include <QFile>
#include <QTime>
#include <QSemaphore>

#include "nodedb.h"


/** \brief Handler of all file playback.
 */
class PlaybackHandler : public QThread
{
    Q_OBJECT

public:

	PlaybackHandler(QObject *pParent = NULL);
    virtual PlaybackHandler::~PlaybackHandler();
	void fileOpen(const QString &fileName);
	void run();
	void stop(void);
	void next(void);
	bool isPlaying(void) { return fPlaying; }

signals:
    void dataReceived();
    void pingReceived();
    void unknownMessage();
    void emptyPacket();
	void packetReady();
    void noPingResponse();
	void playbackStopped();

private slots:
    void handlePacket();
    void checkPing();

protected:

    void handleData(QByteArray *pPacket);

    /// Mutex to protect the Node database to make it thread safe.
    QMutex mutex;
    /// Pointer to the node data base
    NodeDb *pNodeDb;
	/// Boolean used to check if ping has been answered within the timeframe.
    bool pingAnswered;
    /// Timer used for the ping timeframe.
    QTimer *pingTimer;

private:
	int getPacket(QByteArray **pPacket);

	QMainWindow *pMyParent;
	bool fPlaying;
	QFile playbackFile;
    /// packet FIFO queue where all the packets will be stored.
    QQueue<QByteArray *> packets;
	QSemaphore *pSync;
	QTime timeStamp;

};

#endif

