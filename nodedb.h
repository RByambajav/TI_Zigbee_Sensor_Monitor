#ifndef NODEDB_H
#define NODEDB_H
//***********************************************************************************
//*         ****
//*         ****
//*         ******o****
//*    ********_///_****        TEXAS INSTRUMENTS
//*     ******/_//_/*****
//*      ** ***(_/*****         NodeDb.h
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

#include <QQueue>
#include <QMutex>
#include <QFile>

#include "nodedata.h"
#include "profile.h"


class NodeDb
{
public:
    NodeDb();
    virtual ~NodeDb();
    bool deleteNode(int index);
    bool deleteAll();
    int addData(NodeData *pData, bool bAddParents = true);
    bool getData(int index, NodeData **pData);
    bool updatePos(int index, int xPos, int yPos);
    int findChild(int startIndex, int parent);
    int getIndex(int addr);
    bool setType(int index, NodeData::NODE_TYPE type);

	Profile &getProfile(void);
	void setProfileID(unsigned short id);

    enum{DB_SUCCESS, DB_ERROR, DB_SINK_DY = -200, DB_DX = 300, DB_DY = 300};

protected:
    bool getCskipParam(unsigned short addr, NodeData::NODE_TYPE *pType, int *pDepth, QList<int> &lParents);
	bool getNodeParam(NodeData *pData, QList<int> &lParents);
    void calculateXYpos(NodeData *pData);
    void addParents(QList<int> &lParents);

    enum{MAX_DEPTH = 5, MAX_ROUTERS = 6, MAX_CHILDREN = 20, SIZE_SKIP = 6};
    ///Cskip table filled by the constructor
    unsigned short Cskip[SIZE_SKIP];

private:
	static Profile profile;
	bool fUseCskip;
    bool fShowTemp;

};

#endif
