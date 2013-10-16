//***********************************************************************************
//*         ****
//*         ****
//*         ******o****
//*    ********_///_****        TEXAS INSTRUMENTS
//*     ******/_//_/*****
//*      ** ***(_/*****         ComPortCombo.cpp
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

#include "comportcombo.h"



ComPortCombo::ComPortCombo(QWidget *parent)
    : QComboBox(parent)
{
    
}

/** \brief Set COM Port list
 *
 *  The applicable device will be selected automatically if found.
 *  NB! From Version 1.2.0 non-applicable ports are filtered out in "IoHandler"
 *      at enumeration time. 
 *
 *  \param[in] pList
 *           List of COM ports that should be put in the Combobox.
 */
bool ComPortCombo::SetComPortList(const QStringList &pList)
{
	// Add items to Combo Box
	for (int i = 0; i < pList.size(); i++)
    {
        addItem(pList[i]);
    }

    // Set index to first COM-port
	if (pList.size()>0)
		setCurrentIndex(0);

	return pList.size()>0;
}
