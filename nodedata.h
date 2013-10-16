#ifndef NODEDATA_H
#define NODEDATA_H
//***********************************************************************************
//*         ****
//*         ****
//*         ******o****
//*    ********_///_****        TEXAS INSTRUMENTS
//*     ******/_//_/*****
//*      ** ***(_/*****         NodeData.h
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
#include <QTime>

class NodeDb;

class NodeData
{
public:
	NodeData();
	void UpdateData(QByteArray *pPacket);
	int GetDataItem(int index,QString *p);

    typedef enum {DS_NORMAL, DS_NEW, DS_UPDATED, DS_MOVE, DS_DELETE} DB_STATUS;
    typedef enum {NT_DUMMY, NT_SINK, NT_ROUTER, NT_DUMMY_ROUTER, NT_SOURCE} NODE_TYPE;
    typedef enum {NM_DUMMY, NM_DISCONNECTED, NM_CONNECTED, NM_CONNECTED_IDLE, NM_NOT_RSP, }NODE_MODE;

	int             level;
	int             parent;
	int             seq;
	int             addr;
	NODE_TYPE       type;
	NODE_MODE       mode;
	unsigned char   temp;		
	unsigned char   voltage; // For ez-cc2480 only
	int             xpos;
	int             ypos;
	DB_STATUS       status;
	QTime           timeStamp;
	bool			fNoData;

	unsigned short	parentAddr;
private:
    NodeDb          *pNodeDb;

};

#endif
