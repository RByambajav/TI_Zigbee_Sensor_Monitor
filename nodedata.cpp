//***********************************************************************************
//*         ****
//*         ****
//*         ******o****
//*    ********_///_****        TEXAS INSTRUMENTS
//*     ******/_//_/*****
//*      ** ***(_/*****         NodeData.cpp
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

#include "nodedata.h"
#include "nodedb.h"

NodeData::NodeData()
{
	temp= 0;
	voltage=0;
	parentAddr= 0x0000;
	fNoData= false;
	pNodeDb= new NodeDb();
}


void NodeData::UpdateData(QByteArray *pPacket)
{
	// Assuming the MT protocol is used
	// LEN(0,1)+CMD(1,2)+ADDR(3,2)+COMMAND(5,2)+LEN(7,2)+TEMP(9,1)+VOLTAGE(10,1)+parentAddr(11,2) + FCS(13,1) 

	addr = ((unsigned short)(pPacket->at(4)<<8)) | ((unsigned short)(pPacket->at(3) & 0xFF));
    type = NodeData::NT_DUMMY;

	temp= pPacket->at(9);
	voltage= pPacket->at(10);
	Profile::Id id;

	id= pNodeDb->getProfile().getId();
	if (id==Profile::Z_2007_SA || id==Profile::Z_PRO_SA || id==Profile::Z_PRO_SA2) {
		parentAddr= MAKE_WORD_LE(pPacket,11);
	}
}


int NodeData::GetDataItem(int index, QString *p)
{
	int r= 0;

	if (index>=2 && fNoData)
		return -1;

	switch (index)
	{
	case 0:				// Address 
		p->sprintf("0x%04X", addr);
		break;
	case 1:				// Type
		break;
	case 2:				// Temperature Celsius
        if (temp < 255) 
		    p->sprintf("%02d°C", temp);
		break;
	case 3:				// Temperature Fahrenheit
        if (temp < 255) 
		    p->sprintf("%02d°F", (int)((temp * 1.8) + 32));
		break;
	case 4:				// Voltage (for ZASA only)
		if (pNodeDb->getProfile().getId()==Profile::ZASA)
		{
			float v= (float)voltage/10;
			p->sprintf("%.1fV", v);
		} else {
			r= -1;
		}
		break;
	default:
		r= -1;
	};
	return r;
}
