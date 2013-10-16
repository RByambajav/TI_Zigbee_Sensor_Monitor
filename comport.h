#ifndef COMPORT_H
#define COMPORT_H
//***********************************************************************************
//*         ****
//*         ****
//*         ******o****
//*    ********_///_****        TEXAS INSTRUMENTS
//*     ******/_//_/*****
//*      ** ***(_/*****         ComPort.h
//*          *********
//*           *****
//*            ***
//*
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
#include <QTimer>
#include <QCloseEvent>

#include "windows.h"
#include "setupapi.h"
#include "comportthread.h"
#include "nodedb.h"


class ComPort : public QObject
{
    Q_OBJECT

public:
	ComPort(QObject *pParent);
    int Enum(QStringList &portList);
	bool Open(const QString &qsPortName, unsigned int baudRate);
    bool Read();
    int write(unsigned char *pBuffer, int length);
    bool Close();
    ComPortThread *getThread(){return &comPortThread; }

    enum {CP_ERROR, CP_SUCCESS};


signals:
    void dataReceived();
    void noPingResponse();
    void pingReceived();
    void emptyPacket();
    void unknownMessage();

protected:

    int EnumSetupapi(QStringList &portList);
    int EnumWmi(QStringList &portList);
    bool IsNumeric(LPCTSTR pszString, BOOL bIgnoreColon);

    /// Comport Thread used to read data.
    ComPortThread comPortThread;

};

#endif
