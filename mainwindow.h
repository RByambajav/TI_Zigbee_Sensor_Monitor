#ifndef MAINWINDOW_H
#define MAINWINDOW_H
//***********************************************************************************
//*         ****
//*         ****
//*         ******o****
//*    ********_///_****        TEXAS INSTRUMENTS
//*     ******/_//_/*****
//*      ** ***(_/*****         MainWindow.h
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
#include <QLabel>
#include <QMenu>
#include <QTextEdit>
#include <QComboBox>
#include <QAbstractEventDispatcher>
#include <QFileDialog>

#include "iohandler.h"
#include "playbackhandler.h"
#include "loghandler.h"
#include "comportcombo.h"
#include "networkview.h"
#include "settingswindow.h"

class NetworkView;
class DeviceChange;


/** \brief The Main Window
 *
 *
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

	static bool wmEventFilter(void *message);
    void setAppPath(char *path);

	typedef enum {INIT,STOPPED, ONLINE, PLAYBACK} State;
    enum{TIMER_DEV_CHANGE = 2000};

    NetworkView *networkView;


protected:
    void deviceChange();

    /// Pointer to the node data base
    NodeDb *pNodeDb;
    /// The application path is stored on startup.
    QString appPath;

private slots:
	void toggleFullscreen();
	void escapeToggle();
	void showAbout();
    void showSettings();
    void removeAllNodes();
    void onStartCapture();
    void startCapture(bool devChangeEvent = false);
    void onPauseCapture();
    void onStopCapture();
    void stopCapture(bool devChangeEvent = false);
    void refreshComPort();
    void pingReceived();
    void noPingResponse();
    void dataReceived(/*int index*/);
    void timeOutDevChange();
    void showHelpInfo();

	void showPlaybackDialog();
	void showLogDialog();
	void startPlayback(void);
	void stopPlayback();

private:
    void createActions();
	void createMenus();
    void createToolBars();
    void createStatusBar();
	void update();
	void updateStats(QTime &timeStamp);
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);

    // Toolbar for start and stop
    QToolBar *captureToolBar;
    // Toolbar for the combobox.
    QToolBar *comPortToolBar;
    // Toolbar for the file functions
    QToolBar *fileToolbar;
    // Toolbar for the help functions.
    QToolBar *helpToolBar;
    // Put the combobox with COM ports on the toolbar.
    ComPortCombo comPortComboBox;
    /// Flag to indicate if applicable device connected
    bool applDeviceConnected;
    /// Flag to indicate that device change event has occured.
    bool bDevChanged;
    /// Flag to indicate if COM port is open or not
    bool bComPortOpen;
    /// Flag used to control if nodes should be deleted or not after pause or stop.
    bool bDeleteNodes;
    /// Flag to control logging
	bool fLogActive;
	/// True if a file is open for playback
	bool fPlaybackFileOpen;
    
	/// Event handler
    QAbstractEventDispatcher *m_EventDispatcher;

	// Profile 
	Profile profile;

	// Keyboard shortcuts
	QShortcut *helpShortcut;
	QShortcut *esc;
	QShortcut *altEnter;
    QAction *fullscreen;

	// Actions
    QAction *stopAction;
    QAction *captureAction;
    QAction *pauseAction;
    QAction *refreshAction;
    QAction *playAction;
    QAction *logAction;
    QAction *fileOpenAction;
    QAction *settingsAction;
    QAction *helpAction;
    QAction *aboutAction;

	// Status bar items
    QLabel *qlAppState;
	QLabel *qlCommStatus;
	QLabel *qlTrafficStats;
	QLabel *qlTime;
	QLabel *qlProtocol;

	// Data stream handlers
    IoHandler *ioHandler;
	LogHandler *logHandler;
	PlaybackHandler *pbHandler;

	// Auxiliary dialogs
	SettingsWindow *settingsWindow;
	QFileDialog *pFileDialog;
	QFileDialog *pLogDialog;

	// Statistics/properties
	QTime qtCaptureTimer;
	int nPackets;
	State appState;
};

/** \brief Device Change Event Class
 *
 *  This class is needed to define a user event
 *  that will be used to fetch the WM_DEVICECHANGE message
 *  when a USB device has been connected/disconnected.
 *
 */
class DeviceChangeEvent : public QEvent
{
public :
    enum{DeviceChanged = 1050};
    DeviceChangeEvent(): QEvent((QEvent::Type)DeviceChanged) {}
};
#endif