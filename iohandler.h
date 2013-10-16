#ifndef IOHANDLER_H
#define IOHANDLER_H
//***********************************************************************************
//*         ****
//*         ****
//*         ******o****
//*    ********_///_****        TEXAS INSTRUMENTS
//*     ******/_//_/*****
//*      ** ***(_/*****         IoHandler.h
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

#include <QStringList>
#include <QMessageBox>
#include <QMainWindow>

#include "comport.h"
#include "loghandler.h"

/** \brief Handler of all communication with connected device.
 *
 * This class provides methods to give a list of connected devices that are applicable
 * for this application and the methods to communicate with the selected device. 
 *
 * This class is implemented with platform independent functionality but for the actual access
 * to the underlaying hardware the ComPort class is implemented with all the platform dependent functionality.
 *
 *
 */
class IoHandler : public QObject
{
    Q_OBJECT

public:
	IoHandler(QObject *pParent = NULL);
    virtual IoHandler::~IoHandler();

	int RefreshComPort();
    QStringList &getComPortList();
    int openComPort(const QString &portName);
    int closeComPort();
    void sendPing();
	void setLogHandler(LogHandler *pLog);

    enum{IO_ERROR, IO_SUCCESS};
    enum {TIMER_PING = 1000};
    enum {SYS_PING_RESPONSE = 0x0161, ZB_RECEIVE_DATA_INDICATION = 0x8746};
	enum {APP_ZASA = 0x0036, APP_Z2007 = 0x0136, APP_ZPRO = 0x0236 };

signals:
    void dataReceived();
    void pingReceived();
    void unknownMessage();
    void emptyPacket();
    void noPingResponse();

private slots:
    void handlePacket();
    void checkPing();
  
protected:
    
	void handleData(QByteArray *pPacket);
    unsigned char CreateCrc(int length, unsigned char *pData);

	// Housekeeping
    ComPort     *comPort;
    QMainWindow *pMyParent;
    bool        m_bComPortListOk;
    QStringList m_lstComPort;

    /// Mutex to protect the Node database to make it thread safe.
    QMutex mutex;
    /// Pointer to the node data base
    NodeDb *pNodeDb;
    /// Boolean used to check if ping has been answered within the timeframe.
    bool pingAnswered;
    /// Timer used for the ping timeframe.
    QTimer *pingTimer;

private:
	LogHandler *pLog;

};

#endif

