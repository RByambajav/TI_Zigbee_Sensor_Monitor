//***********************************************************************************
//*         ****
//*         ****
//*         ******o****
//*    ********_///_****        TEXAS INSTRUMENTS
//*     ******/_//_/*****
//*      ** ***(_/*****         NetworkView.cpp
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

#include <QDebug>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QIcon>
#include <QSettings>
#include <QApplication>

#include <math.h>

#include "networkview.h"
#include "edge.h"
#include "node.h"


#ifdef DEBUG
extern QFile debugFile;
#endif


NetworkView::NetworkView(QWidget *pParent)
    : timerId(0)
{
    pParentWindow = pParent;

    pScene = new QGraphicsScene(this);
    pScene->setItemIndexMethod(QGraphicsScene::NoIndex);
    pScene->setSceneRect(-2000, -1600, 4000, 3200);
    setScene(pScene);

	setDragMode(QGraphicsView::ScrollHandDrag);
    setCacheMode(CacheBackground);
    setRenderHint(QPainter::Antialiasing);
    setTransformationAnchor(AnchorUnderMouse);
    setResizeAnchor(AnchorViewCenter);

	// Add Connected sink record to the Node data base.
    // This should always be the first record in the data base
    pNodeDb = new NodeDb;

    NodeData *pData = new NodeData;
    pData->addr = 0x0000;
    pData->type = NodeData::NT_SINK;
    pData->level = 0;
    pData->parent = 0;
    pData->xpos = 0;
    pData->ypos = NodeDb::DB_SINK_DY;
    pData->status = NodeData::DS_NORMAL;
    pNodeDb->addData(pData);

    addNode(0);
    scale(1,1);
	fIsOnline= false;

}

//-----------------------------------------------------------------------------
/** \brief Item has moved
 *
 *  Function called when any node is moved. 
 *  The position should be updated because the other nodes are positioned
 *  relativ to the parent node.
 *
 *  \param[in] addr
 *        Address of the node that has been moved.
 *
 */
//-----------------------------------------------------------------------------
void NetworkView::itemMoved(int addr)
{

    if ((nodeList.size() > 0) && (pNodeDb))
    {
        for (int i = 0; i < nodeList.size(); i++)
        {
            if (addr == nodeList[i]->pNode->getAddr())
            {
                QPointF pos;
                pos = nodeList[i]->pNode->pos();
                pNodeDb->updatePos(i, pos.x(), pos.y());
                break;
            }
        }
    }
}


void NetworkView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Plus:
        scaleView(1.2);
        break;
    case Qt::Key_Minus:
        scaleView(1 / 1.2);
        break;
    default:
        QGraphicsView::keyPressEvent(event);
    }
}


void NetworkView::wheelEvent(QWheelEvent *event)
{
    scaleView(pow((double)2, -event->delta() / 240.0));
}


void NetworkView::drawBackground(QPainter *painter, const QRectF &rect)
{
    Q_UNUSED(rect);

    // Fill background according to application state
    QRectF sceneRect = this->sceneRect();
    QLinearGradient gradient(sceneRect.topLeft(), sceneRect.bottomRight());
	QColor cBottomRight(Qt::white);

	gradient.setColorAt(0, bgColour);
	gradient.setColorAt(1, cBottomRight );

	painter->fillRect(rect.intersect(sceneRect), gradient);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(sceneRect);
    
}


void NetworkView::updateBackground(QColor &newColour)
{
	bgColour= newColour;
	// Force redraw of the background
    scaleView(1/1.1);
    scaleView(1.1);
}


void NetworkView::setOnline(bool f)
{
	fIsOnline= f;
}


bool NetworkView::isOnline(void)
{
	return fIsOnline;
}



void NetworkView::scaleView(qreal scaleFactor)
{
    qreal factor = matrix().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    if (factor < 0.07 || factor > 100)
        return;

    scale(scaleFactor, scaleFactor);
}


//-----------------------------------------------------------------------------
/** \brief Start check alive timer
 *
 *  Start timer that will be used to check if node is still alive.
 */
//-----------------------------------------------------------------------------
void NetworkView::startCheckAliveTimer()
{
    QSettings settings;

    // The check on timeout will be in milli seconds
    aliveTimerInterval = settings.value("NodeTimeout", 20).toInt() * 1000;

    if (timerId)
    {
        killTimer(timerId);
    }

    // Start timer with fixed interval
    timerId = startTimer(CHECK_ALIVE_INTERVAL);
}

//-----------------------------------------------------------------------------
/** \brief Stop check alive timer
 *
 *  Stop timer that will be used to check if node is still alive.
 *
 */
//-----------------------------------------------------------------------------
void NetworkView::stopCheckAliveTimer()
{
    if (timerId)
    {
        killTimer(timerId);
    }
}

//-----------------------------------------------------------------------------
/** \brief Timer event
 *
 *  On timeout of timerId, check timestamp of each node and check if
 *  the alive time has expired.
 *
 *  \param[in] event
 *        Current event. Not used.
 *
 */
//-----------------------------------------------------------------------------
void NetworkView::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);

    QList<int> deletedNodes;
    int i;

    // Node 0 is the connected coordinator and should not be checked.
    for (i = 1; i < nodeList.size(); i++)
    {
        // If elapsed time is more than the given interval
        // the node is probably out of service and should
        // be deleted from the view.
        // One exception is for router nodes with child nodes that are still alive.
        // These nodes should not be deleted, but the node type should be changed.
        if (nodeList[i]->pNode->getElapsedTime() > aliveTimerInterval)
        {
            if (pNodeDb->findChild(0, nodeList[i]->pNode->getAddr()) == 0)
            {
                deleteNode(i);
                // To be sure that the index of the different node lists
                // are kept insane, only one node will be deleted each time.
                break;
            }
            else if ((nodeList[i]->pNode->getType() == NodeData::NT_ROUTER) ||
                     (nodeList[i]->pNode->getType() == NodeData::NT_DUMMY_ROUTER))
            {   
                NodeData *pData;
                pNodeDb->getData(i, &pData);
                pData->type = NodeData::NT_DUMMY_ROUTER;
                nodeList[i]->pNode->updateData(i);
            }
        }
    }

}

//-----------------------------------------------------------------------------
/** \brief Add Node to the network
 *
 *  Add node to the network scene
 *
 *  \param[in] nodeData
 *        Structure with data about the node.
 *
 */
//-----------------------------------------------------------------------------
void NetworkView::addNode(int index)
{
    NodeData *pData;
    pNodeDb->getData(index, &pData);

    Node *pNode = new Node(this);
    pNode->setNodeData(index);

    if(pData->type == NodeData::NT_SINK)
    {
        // Default mode for sink on startup.
        // Add node should only be called ones for the sink node.
        pNode->setMode(NodeData::NM_DISCONNECTED);
        pNode->setToolTip(tr("This is the Sink node connected to the PC\n \
        Solid gray color with black outline indicate that the dongle is not connected or not responding.\n \
        Solid red color with black outline means that the dongle is connected and the COM port is open.\n \
        Solid grey color with red outline indicate the the dongle is connected but the COM port is not open."));
    }
    else if ((pData->type == NodeData::NT_ROUTER)||(pData->type == NodeData::NT_DUMMY_ROUTER))
    {
        pNode->setToolTip(tr("Sensor node defined as Router and Source."));
    }
    else
    {
        pNode->setToolTip(tr("Sensor node defined as a Source."));
    }

    pScene->addItem(pNode);
	// Beep when node is added
	if(pData->type != NodeData::NT_SINK)
	{
		QApplication::beep();
	}


    LIST_DATA *pListData = new LIST_DATA;
    pListData->pNode = pNode;
    pListData->pEdge = NULL;
    nodeList.insert(index, pListData);

}

//-----------------------------------------------------------------------------
/** \brief Delete Node
 *
 *  Delete Node for given index. The index is related to the Node data base
 *  and the node list. All child nodes will also be deleted by calling this
 *  functin recursively.
 *
 *  \param[in] index
 *        Index of the Node DB.
 *
 */
//-----------------------------------------------------------------------------
void NetworkView::deleteNode(int index)
{
    // Check if Node has any Children.
    int start;
    int child;

    NodeData *pData;
    pNodeDb->getData(index, &pData);
    
    start = index + 1;
    do 
    {
        child = pNodeDb->findChild(start, pData->addr);
        if (child > 0) 
        {
            deleteNode(child);
        }
        start = child + 1;
    } while (child > 0);

    // Remove Edge from parent node
    nodeList[pNodeDb->getIndex(pData->parent)]->pNode->removeEdge(pData->addr);

    pScene->removeItem(nodeList[index]->pNode);
    // There is no edge for the coordinator, so start with index 0
    pScene->removeItem(nodeList[index]->pEdge);
    delete nodeList[index]->pNode;
    delete nodeList[index]->pEdge;
    nodeList[index]->pNode = NULL;
    nodeList[index]->pEdge = NULL;
    nodeList.removeAt(index);
    pNodeDb->deleteNode(index);

	// Beep when node is removed
	QApplication::beep();

}

//-----------------------------------------------------------------------------
/** \brief Delete All Nodes
 *
 */
//-----------------------------------------------------------------------------
void NetworkView::deleteAll()
{
    int i;
 
    // Remove all edges from Coordinator node
    nodeList[0]->pNode->removeEdge(0);

    if (nodeList[0]->pEdge) 
    {
        pScene->removeItem(nodeList[0]->pEdge);
        delete nodeList[0]->pEdge;
    }

    for(i = 1; i < nodeList.size();i++)
    {
        pScene->removeItem(nodeList[i]->pNode);
        if (nodeList[i]->pEdge) pScene->removeItem(nodeList[i]->pEdge);
    }

    while(nodeList.size() > 1)
    {
        delete nodeList[1]->pNode;
        if (nodeList[1]->pEdge != NULL) delete nodeList[1]->pEdge;
        nodeList.removeAt(1);
    }
}

//-----------------------------------------------------------------------------
/** \brief Update Node data
 *
 *  \param[in] node
 *        The numerical id of the node.
 *  \param[in] data
 *        Reference to the updated data.
 *
 */
//-----------------------------------------------------------------------------
void NetworkView::updateNode(int index)
{
    nodeList[index]->pNode->updateData(index);
}

//-----------------------------------------------------------------------------
/** \brief Connect nodes
 *
 *  Setting a connector between two nodes.
 *
 *  \param[in] srce
 *        The source node.
 *
 *  \param[in] dest
 *        The destination node.
 *
 */
//-----------------------------------------------------------------------------
void NetworkView::connectNodes(int srce, int dest)
{
    Edge *pEdge = new Edge(nodeList[srce]->pNode, nodeList[dest]->pNode);
    pScene->addItem(pEdge);
    nodeList[srce]->pEdge = pEdge;

}

//-----------------------------------------------------------------------------
/** \brief Set the mode of the node
 *
 *  Setting a connector between two nodes.
 *
 *  \param[in] mode
 *        The mode to be applied.
 *
 *  \param[in] node
 *        The numerical id of the node.
 *
 */
//-----------------------------------------------------------------------------
void NetworkView::setMode(const NodeData::NODE_MODE mode, int node)
{
    nodeList[node]->pNode->setMode(mode);
}

//-----------------------------------------------------------------------------
/** \brief Change Temperatur unit
 *
 *
 */
//-----------------------------------------------------------------------------
void NetworkView::changeTempUnit()
{
    for(int i = 1; i < nodeList.size();i++)
    {
        nodeList[i]->pNode->setTempUnit();
    }
}
