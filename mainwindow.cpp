//***********************************************************************************
//*         ****
//*         ****
//*         ******o****
//*    ********_///_****        TEXAS INSTRUMENTS
//*     ******/_//_/*****
//*      ** ***(_/*****         MainWindow.cpp
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

#include <QtGui>
#include <QStyle>
#include <QMotifStyle>
#include <QPlastiqueStyle>
#include <QCDEStyle>
#include <QCleanlooksStyle>

#include "windows.h"			// Dbt.h requires this file			
#include <Dbt.h>

#include "MainWindow.h"

// Static pointer to main window. Needed in order to post event
// to main window from static event filter function
static QObject *pMainWindow;



///////////////////////////// Defines / Locals ////////////////////////////////
#define APP_VERSION		"1.3.2"



MainWindow::MainWindow()
{
 
	setMinimumSize(800,600);
	setFont(QFont("Arial", 10, QFont::Normal));

	QApplication::setStyle(new QPlastiqueStyle);

    networkView = new NetworkView(this);
	profile.setId(Profile::UNKNOWN);

    setCentralWidget(networkView);

	helpShortcut = new QShortcut(QKeySequence(tr("F1")), this);
	connect(helpShortcut, SIGNAL(activated()), this, SLOT(showHelpInfo()));

	esc = new QShortcut(QKeySequence(tr("ESC")), this);
	connect(esc, SIGNAL(activated()), this, SLOT(escapeToggle()));

	altEnter = new QShortcut(QKeySequence(tr("ALT+RETURN")),this);
	connect(altEnter, SIGNAL(activated()), this, SLOT(toggleFullscreen()));
	
    setWindowIcon(QIcon(":/ti.ico"));

	QString title(profile.getTitle());
    title += " (" APP_VERSION ")";
    setWindowTitle(title);

    //Create IO Handler 
    ioHandler = new IoHandler(this);
	// Create log handler
	logHandler = new LogHandler(this);
	// Create playback handler
	pbHandler = new PlaybackHandler(this);

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    /* Get the Dispatcher instace */
    m_EventDispatcher = QAbstractEventDispatcher::instance(0);
    m_EventDispatcher->setEventFilter((QAbstractEventDispatcher::EventFilter)MainWindow::wmEventFilter);

    pMainWindow = this;

    installEventFilter(this);

    bDevChanged = false;
    bComPortOpen = false;
	nPackets = 0;
	fLogActive = false;
	fPlaybackFileOpen = false;
	appState= INIT;

    // Populate Combobox for COM port.
    if (comPortComboBox.SetComPortList(ioHandler->getComPortList()))
    {
        applDeviceConnected = true;
        // If applicable device found, we can start reading from the port.
        startCapture();

    }
    else
    {
        applDeviceConnected = false;
    }

    comPortComboBox.setToolTip(tr("Select applicable COM port."));

    // Create instance to Node Database.
    pNodeDb = new NodeDb;

    bDeleteNodes = false;

    // Information used to store data in windows registery.
    QCoreApplication::setOrganizationName("Texas Instruments");
    QCoreApplication::setOrganizationDomain("ti.com");
	QCoreApplication::setApplicationName("ZigBee Sensor Monitor");

}

void MainWindow::escapeToggle()
{
	if(isFullScreen())
		toggleFullscreen();
}

void MainWindow::toggleFullscreen()
{
	if(!isFullScreen())
	{
		setWindowState(Qt::WindowFullScreen);
	}
	else
	{
		setWindowState(Qt::WindowNoState);
	}
}



/** \brief Show settings dialog
 *
 */
void MainWindow::showSettings()
{
    SettingsWindow settingsWindow;
    QSettings settings;

    bool celsius = settings.value("TempInCelsius", true).toBool();
    if(settingsWindow.exec() == SettingsWindow::BTN_OK)
    {
        networkView->startCheckAliveTimer();
        if (celsius != settings.value("TempInCelsius", true).toBool())
        {
            networkView->changeTempUnit();
        }
    }
}



/** \brief Create Actions
 *
 */
void MainWindow::createActions()
{
    captureAction = new QAction(tr("&Capture"), this);
    captureAction->setIcon(QIcon(":/images/play_128.png"));
    captureAction->setShortcut(tr("F5"));
    captureAction->setToolTip(tr("Start capturing data from COM port. (F5)"));
    captureAction->setDisabled(FALSE);
    connect(captureAction, SIGNAL(triggered()), this, SLOT(onStartCapture()));

    pauseAction  = new QAction(tr("&Pause"), this);
    pauseAction->setIcon(QIcon(":/images/pause_128.png"));
    pauseAction->setShortcut(tr("F6"));
    pauseAction->setToolTip(tr("Pause capturing data from the COM port. (F6)"));
    pauseAction->setDisabled(TRUE);
    connect(pauseAction, SIGNAL(triggered()), this, SLOT(onPauseCapture()));

    stopAction = new QAction(tr("&Stop"), this);
    stopAction->setIcon(QIcon(":/images/stop_128.png"));
    stopAction->setShortcut(tr("F4"));
    stopAction->setToolTip(tr("Stop data capture/playback. (F4)"));
    stopAction->setDisabled(TRUE);
    connect(stopAction, SIGNAL(triggered()), this, SLOT(onStopCapture()));


    refreshAction = new QAction(tr("&Refresh"), this);
    refreshAction->setIcon(QIcon(":/images/refresh_128.png"));
    refreshAction->setShortcut(tr("F7"));
    refreshAction->setToolTip(tr("Refresh list of serial ports. (F7)"));
    connect(refreshAction, SIGNAL(triggered()), this, SLOT(refreshComPort()));

	fileOpenAction = new QAction(tr("&File open"), this);
    fileOpenAction->setIcon(QIcon(":/images/open_128.png"));
    fileOpenAction->setShortcut(tr("F11"));
    fileOpenAction->setToolTip(tr("Open log file for playback (F11)"));
    connect(fileOpenAction, SIGNAL(triggered()), this, SLOT(showPlaybackDialog()));

	logAction = new QAction(tr("&Log"), this);
    logAction->setIcon(QIcon(":/images/save_128.png"));
    logAction->setToolTip(tr("Start logging to file"));
    connect(logAction, SIGNAL(triggered()), this, SLOT(showLogDialog()));
	logAction->setDisabled(FALSE);

	playAction = new QAction(tr("&Play"), this);
    playAction->setIcon(QIcon(":/images/play_128.png"));
    playAction->setShortcut(tr("F10"));
    playAction->setToolTip(tr("Start playback of log file (F10)"));
    connect(playAction, SIGNAL(triggered()), this, SLOT(startPlayback()));
	playAction->setDisabled(TRUE);

	settingsAction = new QAction(tr("&Settings"), this);
    settingsAction->setIcon(QIcon(":/images/settings_128.png"));
    settingsAction->setShortcut(tr("F8"));
    settingsAction->setToolTip(tr("Configuration (F8)"));
    connect(settingsAction, SIGNAL(triggered()), this, SLOT(showSettings()));

    helpAction = new QAction(tr("&Help"), this);
    helpAction->setIcon(QIcon(":/images/help_128.png"));
    helpAction->setShortcut(tr("F8"));
    helpAction->setToolTip(tr("Help information (F8)"));
    connect(helpAction, SIGNAL(triggered()), this, SLOT(showHelpInfo()));

    aboutAction = new QAction(tr("&About"), this);
    aboutAction->setIcon(QIcon(":/images/information_128.png"));
    aboutAction->setShortcut(tr("F9"));
    aboutAction->setToolTip(tr("About information (F10)"));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(showAbout()));
}

void MainWindow::createMenus()
{
    // For the moment we don't have eny menu items.
}


void MainWindow::showAbout()
{
    QString aboutString;
    QImage tiLogo(":/images/ti_logo_red.png");
    QMessageBox mb;

    mb.setWindowTitle(tr("About ZigBee Sensor Monitor"));

    aboutString = QMessageBox::tr("<font size='10' >Texas</font><br>"
                                  "<font size='10' >Instruments</font><br><br>"
                                  "<h3>ZigBee Sensor Monitor Ver. " APP_VERSION "</h3>"
                                  "<p>This software is developed by Texas Instruments to the courtesy of our customers."
                                  "Texas Instruments is a world-wide distributor of integrated radio tranceiver chips."
                                  "For further information on the products from Texas Instruments, please contact us or visit our web site:<br>"
                                  "<a href=\"http://www.ti.com/lprf\">www.ti.com/lprf</a>"
                                  "<h3>Distribution and disclaimer.</h3>"
                                  //"<p>This program may be distributed freely under the condition that no profit is gained from its distribution, nor from any other program distributed in the same package. "
                                  //"All files that are part of this package have to be distributed together and none of them may be changed in any way other than archiving or crunching.</p>"
                                  "<p>This program is distributed as freeware (and giftware). This package is provided \"as is\" without warranty of any kind. The author assumes no responsibility or liability whatsoever for any damage or loss of data caused by using this package.</p>"
                                  "<p>Copyright (c) Texas Instruments Norway AS</p>"
                                  );

    QPixmap pm = QPixmap::fromImage(tiLogo);
    if (!pm.isNull()) mb.setIconPixmap(pm);
    mb.setText(aboutString);
    mb.addButton(QMessageBox::Ok);
    mb.exec();
   
}

void MainWindow::createToolBars()
{
    captureToolBar = addToolBar(tr("&Capture"));
    captureToolBar->addAction(captureAction);
    captureToolBar->addAction(pauseAction);
    captureToolBar->addAction(stopAction);

    comPortToolBar = addToolBar(tr("&ComPort"));
    comPortToolBar->addWidget(&comPortComboBox);
    comPortToolBar->addAction(refreshAction);

    fileToolbar = addToolBar(tr("&Playback"));
	fileToolbar->addAction(logAction);
	fileToolbar->addAction(fileOpenAction);
    fileToolbar->addAction(playAction);

	helpToolBar = addToolBar(tr("&Help"));
    helpToolBar->addAction(settingsAction);
    helpToolBar->addAction(helpAction);
    helpToolBar->addAction(aboutAction);
}

void MainWindow::createStatusBar()
{
	QLabel **pLabel[]= { &qlAppState, &qlCommStatus, &qlTrafficStats, &qlTime, &qlProtocol };

	for (int i=0; i<5; i++) {
		QLabel **ql= pLabel[i];

	    *ql = new QLabel(tr("-"));
		(*ql)->setAlignment(Qt::AlignCenter);
		(*ql)->setMargin(3);
		statusBar()->addWidget(*ql,1);
	}
}

//-----------------------------------------------------------------------------
/** \brief Update window according to application state
 *
 */
//-----------------------------------------------------------------------------
void MainWindow::update()
{
	bool fStopped;

	fStopped= appState==STOPPED;

	// Update widget states
	playAction->setEnabled(fPlaybackFileOpen && fStopped);
	captureAction->setEnabled(fStopped);
	pauseAction->setEnabled(appState==ONLINE);
	stopAction->setDisabled(fStopped);
	comPortComboBox.setEnabled(fStopped);
	refreshAction->setEnabled(fStopped);
	fileOpenAction->setEnabled(fStopped);
	logAction->setEnabled(fStopped);

	// Network view state
	networkView->setOnline(appState==ONLINE);

	// Update status bar and Network View background
	switch (appState) {
		case ONLINE:
			qlAppState->setText(tr("<p style=\"color:green; font-weight:bold\">ONLINE</p>"));
			networkView->updateBackground(QColor(Qt::darkCyan));
			break;
		case PLAYBACK:
			qlAppState->setText(tr("<p style=\"color:blue; font-weight:bold\">PLAYBACK</p>"));
			networkView->updateBackground(QColor(Qt::darkBlue));
			break;
		case STOPPED:
			qlAppState->setText(tr("<p>STOPPED</p>"));
			qlCommStatus->setText("-");
			networkView->updateBackground(QColor(Qt::gray));
			break;
		default:
			break;
	}

}


//-----------------------------------------------------------------------------
/** \brief Show playback dialog
 *
 */
//-----------------------------------------------------------------------------
void MainWindow::showPlaybackDialog()
{
	QString fileName = QFileDialog::getOpenFileName(this,
         tr("Open log file"), ".", tr("Log Files (*.log)"));
	if (!fileName.isEmpty()) {
		pbHandler->fileOpen(fileName);
		fPlaybackFileOpen= true;
		update();
	}
}


//-----------------------------------------------------------------------------
/** \brief Show log dialog
 *
 */
//-----------------------------------------------------------------------------
void MainWindow::showLogDialog()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Open log file"), 
		"ZSensorMonitor.log", tr("Log Files (*.log)"));

	if (!fileName.isEmpty()) {
		logHandler->openLog(fileName);
		fLogActive= true;
		update();
	}
}


//-----------------------------------------------------------------------------
/** \brief On Start capturing packets
 *
 *  Called when user click button on toolbar.
 */
//-----------------------------------------------------------------------------
void MainWindow::onStartCapture()
{
    startCapture();
}


//-----------------------------------------------------------------------------
/** \brief Start capturing packets
 *
 *  Call function in IO Handler to start reading from file.
 */
//-----------------------------------------------------------------------------
void MainWindow::startPlayback(void)
{
	if (bDeleteNodes) removeAllNodes();
	pbHandler->start();
	qlAppState->setText(tr("Playback"));
	nPackets= 0;
	appState= PLAYBACK;
	update();

}


//-----------------------------------------------------------------------------
/** \brief Stop playback
 *
 *  Call function in playback Handler to stop reading from file.
 */
//-----------------------------------------------------------------------------
void MainWindow::stopPlayback(void)
{
	pbHandler->stop();
    qlAppState->setText(tr("Stopped"));
	appState= STOPPED;

	update();
}



//-----------------------------------------------------------------------------
/** \brief Start capturing packets
 *
 *  Call function in IO Handler to start reading from COM port.
 */
//-----------------------------------------------------------------------------
void MainWindow::startCapture(bool devChangeEvent)
{
    QString qsComPort;
    qsComPort= comPortComboBox.currentText();
	nPackets = 0;

    int s, e;
    s = qsComPort.indexOf("COM");
    int i = s + 3;
    while (qsComPort[i++].isDigit());
    e = i - 1;

	ioHandler->setLogHandler(fLogActive ? logHandler : NULL);

    if (ioHandler->openComPort(qsComPort.mid(s,e - s)) == IoHandler::IO_SUCCESS)
    {
        bComPortOpen = true;
        if(devChangeEvent)
        {
            bDevChanged = true;
        }
        qlAppState->setText(tr("Wait for ping response... "));
        networkView->startCheckAliveTimer();
		appState= ONLINE;

		if (fLogActive) {
			logHandler->openLog();
		}

		update();
    }
    else
    {
        QString qsTemp;
        bComPortOpen = false;
        qsTemp = "Not able to open " + qsComPort.mid(s,e - s);
        QMessageBox::warning(this, tr("Open COM Port"), qsTemp, QMessageBox::AcceptRole, QMessageBox::AcceptRole);

        refreshComPort();
    }
}

//-----------------------------------------------------------------------------
/** \brief On Pause capturing 
 *
 *  Called when user click Pause button on toolbar.
 */
//-----------------------------------------------------------------------------
void MainWindow::onPauseCapture()
{
    stopCapture();
    bDeleteNodes = false;
}

//-----------------------------------------------------------------------------
/** \brief On Stop capturing 
 *
 *  Called when user click button on toolbar.
 */
//-----------------------------------------------------------------------------
void MainWindow::onStopCapture()
{
    stopCapture();
	pbHandler->stop();
    bDeleteNodes = true;
}

//-----------------------------------------------------------------------------
/** \brief Stop capturing packets
 *
 *  Call function in IO handler to close COM port.
 */
//-----------------------------------------------------------------------------
void MainWindow::stopCapture(bool devChangeEvent)
{
    networkView->stopCheckAliveTimer();

    if (bComPortOpen)
    {
        if (ioHandler->closeComPort() == IoHandler::IO_ERROR)
        {
            QMessageBox::warning(this, tr("Close COM Port"), tr("Not able to close COM port"));
        }
    }

    bComPortOpen = false;

    if (applDeviceConnected)
    {
        qlCommStatus->setText(tr("Connected and idle"));
        networkView->setMode(NodeData::NM_CONNECTED_IDLE, 0);
    }
    else
    {
        networkView->setMode(NodeData::NM_DISCONNECTED, 0);
        qlCommStatus->setText(tr("Disconnected"));
    }

    if (devChangeEvent)
    {
        bDevChanged = true;
    }

	pbHandler->stop();
	logHandler->closeLog();
	appState= STOPPED;
	update();

    
}

//-----------------------------------------------------------------------------
/** \brief Remove All Nodes
 *
 *  Both the Node Data base and the graphical view will be cleared.
 *  The Coordinator node(sink) will not be removed.
 */
//-----------------------------------------------------------------------------
void MainWindow::removeAllNodes()
{
    networkView->deleteAll();

    pNodeDb->deleteAll();
}

//-----------------------------------------------------------------------------
/** \brief Refresh COM port
 *
 *  Call function in IO handler to refresh com ports.
 *  get new list of COM ports.
 */
//-----------------------------------------------------------------------------
void MainWindow::refreshComPort()
{
    if (ioHandler->RefreshComPort() == IoHandler::IO_SUCCESS)
    {
        comPortComboBox.clear();
        if (comPortComboBox.SetComPortList(ioHandler->getComPortList()))
        {
            // Applicable COM port found. change status of connected node
            // to not responding. This will be changed if ping received.
            applDeviceConnected = true;
        }
        else
        {
            // No applicable COM port found. Dongle not connected.
            applDeviceConnected = false;
            networkView->setMode(NodeData::NM_DISCONNECTED, 0);
            qlCommStatus->setText(tr("Disconnected"));
        }
    }
    else
    {
        applDeviceConnected = false;
        networkView->setMode(NodeData::NM_DISCONNECTED, 0);
        qlCommStatus->setText(tr("Disconnected"));
        QMessageBox::warning(this, tr("Refresh COM port"), tr("Not able to refresh COM Port")); 
    }

}



//-----------------------------------------------------------------------------
/** \brief Ping response received.
 *
 *  Ping received and connection established
 *
 */
//-----------------------------------------------------------------------------
void MainWindow::pingReceived()
{
	QString qs;
	nPackets++;
	qs.sprintf("Packets: %d", nPackets);
	qlTrafficStats->setText(qs);

	qtCaptureTimer.start();
    qlCommStatus->setText("Collecting data");

	// Display profile information in the statusbar
	qs= tr("Profile: <b>");
	qs+= pNodeDb->getProfile().getName() + "</b>";
	qlProtocol->setText(qs);

    // Change mode for the dongle. The dongle will always be node 0.
    networkView->setMode(NodeData::NM_CONNECTED, 0);
	updateStats(qtCaptureTimer);
	update();
}



//-----------------------------------------------------------------------------
/** \brief Data Received
 *
 *  Called when packet with Node information received.
 *  Read Node data base and check for new information.
 *
 */
//-----------------------------------------------------------------------------
void MainWindow::dataReceived()
{
    int i = 0;

    NodeData *pData;
    // Check each node in the data base and check if updates are needed.
    while(pNodeDb->getData(i, &pData))
    {
        // Check if status changed
        switch (pData->status)
        {
            case NodeData::DS_NORMAL :
                // Do nothing. Put first in the switch because this will be the most used status.            
                break;
            case NodeData::DS_UPDATED :
            case NodeData::DS_MOVE :
                networkView->updateNode(i);
                pData->status = NodeData::DS_NORMAL;
                break;
            case NodeData::DS_NEW :
                networkView->addNode(i);
                networkView->connectNodes(i, pNodeDb->getIndex(pData->parent));
                pData->status = NodeData::DS_NORMAL;
                break;
            default :
                // Do nothing
                ;
        }

		updateStats(pData->timeStamp);

        i++;
    }

	// Do a new round to see if any of nodes are to be deleted
	i=0;
	while(pNodeDb->getData(i, &pData))
    {
		if(pData->status == NodeData::DS_DELETE)
		{
			networkView->deleteNode(i);
			// Do not incrememt index since list indexes are now decreased by 1
			i--;
		}
		i++;
	}
}


void MainWindow::updateStats(QTime &timeStamp)
{
	QString qs;
	nPackets++;
	qs.sprintf("Packets: %d", nPackets);
	qlTrafficStats->setText(qs);

	qs= timeStamp.toString("'Last packet:' hh:mm:ss");
	qlTime->setText(qs);

}



//-----------------------------------------------------------------------------
/** \brief No Ping response.
 *
 *  No Ping response received, connection lost?
 *
 */
//-----------------------------------------------------------------------------
void MainWindow::noPingResponse()
{
    if (applDeviceConnected)
    {
        stopCapture();
        qlCommStatus->setText("Connected but no response");
        networkView->setMode(NodeData::NM_NOT_RSP, 0);
    }
    else
    {
        stopCapture();
        networkView->setMode(NodeData::NM_DISCONNECTED, 0);
        qlCommStatus->setText(tr("Disconnected"));
    }
}

//-----------------------------------------------------------------------------
/** \brief	Close event
 *
 *  \param[in] event
 */
//-----------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *event)
{
    // Make sure that the COM port is closed before ending the application.
    if (event->type() == QEvent::Close)
    {
        ioHandler->closeComPort();
    }
	pbHandler->stop();

}

//-----------------------------------------------------------------------------
/** \brief	Windows Event Filter
 *
 *   Catch any window message.
 *   vmEventFilter must be made known by the main application loop on beforehand.
 *   See consturctor of this class.
 *   Since this is a static function we have to post an event to tell
 *   the main window about it.
 *
 *  \param[in] message
 *        Window message.
 */
//-----------------------------------------------------------------------------
bool MainWindow::wmEventFilter(void *message)
{
    MSG *msg;

    msg = (MSG*)message;
    if (msg->message == WM_DEVICECHANGE && msg->wParam == DBT_DEVNODES_CHANGED) 
    {

        DeviceChangeEvent *deviceChangeEvent = new DeviceChangeEvent();

        QCoreApplication::postEvent(pMainWindow, deviceChangeEvent);

    }

    return false;
}

 //-----------------------------------------------------------------------------
/** \brief	Event Filter
 *
 *   Catch events that need special handling by the main window.
 *
 *  \param[in] obj
 *           The watched object.
 *  \param[in] ev
 *           Event.
 */
//-----------------------------------------------------------------------------
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
 {
    if (obj == this)
    {
        // bDevChanged will by default be false. If this event occure and either
        // COM port is opened or closed, bDevChanged will be set true to avoid
        // repeated calls to open/close COM port. Plug/unplug of USB device seems 
        // to create several VW_DEVICECHANGE events. To make sure that the bDevChanged
        // is reset to default value even if open/close should not be successfull, there
        // is a timer set and on timeout the value will be set false.
        if (!bDevChanged && event->type() == DeviceChangeEvent::DeviceChanged)
        {

            QTimer::singleShot(TIMER_DEV_CHANGE, pMainWindow, SLOT(timeOutDevChange()));

            deviceChange();
            return true;
        }
    }

     // If event not already handled, we have to pass it to the parent class.
     return QMainWindow::eventFilter(obj, event);
 }
//-----------------------------------------------------------------------------
/** \brief	Device Change event
 *
 *   Called when device changes has occured. E.G. USB device plugged/removed.
 *
 *  \param[in] ev
 *           Event.
 */
//-----------------------------------------------------------------------------

void MainWindow::deviceChange()
{
	refreshComPort();
   
    if (applDeviceConnected && appState==INIT)
    {
        // COM port was not open
        startCapture(true);
    }
    
	if (!applDeviceConnected && appState==ONLINE) 
    {
        // Applicable device not connected and COM port was open
        // this means that the device was removed.
        stopCapture(true);
    }
    
}

//-----------------------------------------------------------------------------
/** \brief	Called when timeout of Device Change event timer expired.
 *
 *  The timer and the flag is used to avoid handling of the same event
 *  more than ones. It seems that when the USB dongle is plugged/unplugged, 
 *  several WM_DEVICECHANGED messages are generated by the system.
 *
 */
//-----------------------------------------------------------------------------
void MainWindow::timeOutDevChange()
{
    bDevChanged = false;
}

//-----------------------------------------------------------------------------
/** \brief	Set Application path
 *
 *  Should be called in the main procedure to set current path of the application
 *
 *  \param[in] path
 *        Complete path including the name of the exe file to start application.  
 */
//-----------------------------------------------------------------------------
void MainWindow::setAppPath(char *path)
{
    QString temp = path;

    appPath = temp.left(temp.lastIndexOf('\\'));

}

//-----------------------------------------------------------------------------
/** \brief	Show help information
 *
 *  Open pdf file with application help information.
 */
//-----------------------------------------------------------------------------
void MainWindow::showHelpInfo()
{
    QString temp;
	QString qsUserGuide(profile.getUserManual());
    temp = appPath + "\\..\\" + qsUserGuide;
	if (QFile::exists (temp)) {
		ShellExecute(0, TEXT("open"), temp.utf16(), NULL, NULL, SW_SHOWMAXIMIZED);
	} else  {
		QMessageBox msgbox;

		msgbox.setText("<h3>File not found!</h3><p>" + temp + "</p>");
		msgbox.setStandardButtons(QMessageBox::Cancel);
		msgbox.setIcon(QMessageBox::Warning);
		msgbox.exec();

	}
}

