//***********************************************************************************
//*         ****
//*         ****
//*         ******o****
//*    ********_///_****        TEXAS INSTRUMENTS
//*     ******/_//_/*****
//*      ** ***(_/*****         NodeDb.cpp
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

#include "nodedb.h"


// The data base is defined static in order to be the 
// same for all instances of the NodeDb class.
//static QQueue<NodeDb::NodeData *> nodeData;
static QList<NodeData *> nodeData;

Profile NodeDb::profile;

NodeDb::NodeDb()
{
	fUseCskip= true;
    fShowTemp= false;

    // Calculate skip table. Start with last depth
    for(int i = SIZE_SKIP - 1;i >= 0; i--)
    {
        if (i >= MAX_DEPTH)
        {
            Cskip[i] = 0;
        }
        else if (i == (MAX_DEPTH - 1))
        {
            Cskip[i] = 1;
        }
        else
        {
            Cskip[i] = Cskip[i+1]*MAX_ROUTERS + 1 + MAX_CHILDREN - MAX_ROUTERS;
        }
    }

}

NodeDb::~NodeDb()
{
}


void NodeDb::setProfileID(unsigned short id)
{
	profile.setId((Profile::Id)id);
	if (id!=Profile::ZASA)
		fUseCskip= false;

    if (id == Profile::Z_PRO_SA2)
        fShowTemp= true;
}


Profile &NodeDb::getProfile(void)
{
	return profile;
}


//-----------------------------------------------------------------------------
/** \brief Add Node data
 *
 *  Check if node already exist based on the source address.
 *  If existing: 
 *        Update temperature and voltage information and set status = updated.
 *  If not existing:
 *        Calculate depth, parent and sequence depending on the address.
 *        With the information above, the xpos and ypos should be calculated.
 *        Indicate record as new.
 *
 *  \param[in] pData
 *        Pointer to data.
 *  \param[in] baddParents
 *        Flag to indicate if parents should be added or not. This function
 *        will be called recursively to add parents and to avoid parents
 *        beeing added several times, this flag should be set false when
 *        it is known that the node is parent.
 *
 *  \return int
 *        List index of updated/new item
 */
//-----------------------------------------------------------------------------
int NodeDb::addData(NodeData *pData, bool bAddParents)
{
    int i;

    // The Coordinator should be added first
    if (pData->addr == 0x0000)
    {
        nodeData.insert(0, pData);
        return 0;
    }

    // Search to see if existing.
    if (!nodeData.isEmpty())
    {
        for (i = 0; i < nodeData.size(); i++)
        {
            if ( (nodeData[i]->addr == pData->addr) && ((nodeData[i]->parentAddr == pData->parentAddr) || (pData->type == NodeData::NT_DUMMY_ROUTER)) )
            {
                // If type is Dummy router the
                // the temperature and voltage should not be changed.
                // This is to avoid that the temperatur of the router change to 0 everytime an 
                // update of the child device is requested.
                if (pData->type != NodeData::NT_DUMMY_ROUTER)
                {
                    nodeData[i]->voltage = pData->voltage;
                    nodeData[i]->temp = pData->temp;
					nodeData[i]->parentAddr= pData->parentAddr;
				
					if ((nodeData[i]->type == NodeData::NT_DUMMY_ROUTER) && (pData->temp > 0))
					{
						// If the node was previously defined as Dummy router, it should
						// now be changed to normal router.
						nodeData[i]->type = NodeData::NT_ROUTER;
						if((!fUseCskip) && (!fShowTemp)){
							nodeData[i]->fNoData= 1;
						}
					}
					nodeData[i]->status = NodeData::DS_UPDATED;
				}
                return i;
            }
			// If parent address has changed, delete old entry
			else if((nodeData[i]->addr == pData->addr) && (nodeData[i]->parentAddr != pData->parentAddr))
			{
				nodeData[i]->status = NodeData::DS_DELETE;
			}
        }
    }

    // Getting here means that we have to add a new record in the list.
    QList<int> lParents;
	if (fUseCskip)
		getCskipParam(pData->addr, &pData->type, &pData->level, lParents);
	else
		getNodeParam(pData, lParents);


    //If more than one parent in the list, the device is connected via a router.
    if (bAddParents)
    {
        addParents(lParents);
    }
    
	pData->status = NodeData::DS_NEW;

    pData->parent = lParents.last();
    calculateXYpos(pData);

    // Find position to insert new record.
    for (i = 0; i < nodeData.size(); i++)
    {
        if (pData->level == nodeData[i]->level)
        {
            int j = i;
            while((j < nodeData.size()) &&
                  (nodeData[j]->level == pData->level)&&
                  (nodeData[j]->parent <= lParents.last()))
            {
                j++;
            }
            i = j;
            break;
        }
    }

    nodeData.insert(i, pData);
    
    return i;
}

//-----------------------------------------------------------------------------
/** \brief Add parent nodes
 *
 *  For each parent except the first in the list, add the data to the
 *  node data base. The first node will be the coordinator.
 *
 *  \param[in] lParents
 *        List of parents
 */
//-----------------------------------------------------------------------------
void NodeDb::addParents(QList<int> &lParents)
{
    for(int i = 1; i < lParents.size(); i++)
    {
        NodeData *pData = new NodeData;

        pData->addr = lParents[i];
        pData->type = NodeData::NT_DUMMY_ROUTER;
        addData(pData, false);
    }
}
//-----------------------------------------------------------------------------
/** \brief Get Node Data
 *
 *  \param[in] index
 *        Index of required list item
 *  \param[out] pData
 *        Pointer to data.
 *  \return bool
 *        true if item found, else false
 */
//-----------------------------------------------------------------------------
bool NodeDb::getData(int index, NodeData **pData)
{
    if (index >= nodeData.size()) return false;

    if (nodeData.isEmpty()) return false;

    *pData = nodeData[index];

    return true;
}

//-----------------------------------------------------------------------------
/** \brief Update position of Node
 *
 *  \param[in] index
 *        Index of required list item
 *  \param[out] xPos
 *        X position
 *  \param[out] yPos
 *        Y position
 *  \return bool
 *        true if valid index, else false
 */
//-----------------------------------------------------------------------------
bool NodeDb::updatePos(int index, int xPos, int yPos)
{
    if (index >= nodeData.size()) return false;

    if (nodeData.isEmpty()) return false;

    nodeData[index]->xpos = xPos;
    nodeData[index]->ypos = yPos;

    return true;
}

//-----------------------------------------------------------------------------
/** \brief Set Type of Node
 *
 *  \param[in] index
 *         Index of node
 *
 *  \return bool
 *         true if node could be found, else false.
 */
//-----------------------------------------------------------------------------
bool NodeDb::setType(int index, NodeData::NODE_TYPE type)
{
    if (index >= nodeData.size()) return false;

    if (nodeData.isEmpty()) return false;

    nodeData[index]->type = type;

    return true;
}

//-----------------------------------------------------------------------------
/** \brief Delete data for one node
 *
 *  \param[in] index
 *         Index of node to be deleted.
 *
 *  \return bool
 *         true if node could be deleted, else false.
 */
//-----------------------------------------------------------------------------
bool NodeDb::deleteNode(int index)
{
    if (index > 0)
    {
        delete nodeData[index];
        nodeData.removeAt(index);
        return true;
    }
    else
    {
        return false;
    }
}

//-----------------------------------------------------------------------------
/** \brief Delete All data
 *
 */
//-----------------------------------------------------------------------------
bool NodeDb::deleteAll()
{
    while (nodeData.size() > 1)
    {
        delete nodeData[1];
        nodeData.removeAt(1);
    }

    return true;
}

//-----------------------------------------------------------------------------
/** \brief Get Type and Depth
 *
 *  Use the CSkip address assignment rule to find depth and type of node.
 *  The rule is based on the following definitions
 *
 *  \param[in] addr
 *        Current address to check
 *  \param[out] pType
 *        Type of node. Router or end device
 *  \param[out] pDepth
 *        Depth of Node in Tree structure of address assignment
 *  \param[out] pParent
 *        Parent Node in Tree structure of address assignment
 *  \return bool
 *        true if valid address, else false
 */
//-----------------------------------------------------------------------------
bool NodeDb::getCskipParam(unsigned short addr, NodeData::NODE_TYPE *pType, int *pLevel, QList<int> &lParents)
{
    // Address router
    int Ar;
    // Address parent
    int Ap;
    // depth used in Cskip tree structure
    int depth;
    // flag to indicate if parameters found
    bool found;
    // general counter
    int i;

    // Start with coordinator as parent address
    Ap = 0x0000;
    lParents.append(Ap);
    // The first router address below the coordinator.
    Ar = 0x0001;

    // Search all depth levels
    found = false;
    for (depth = 0; depth < MAX_DEPTH; depth++)
    {
        // Find applicable sub block of addresses at current depth
        i = 0;
        while ((addr > Ar) && (i < MAX_ROUTERS))
        {
            Ar += Cskip[depth];
            i++;
        }

        if (i == MAX_ROUTERS)
        {
            // if i equal max routers the address is a child
            // at previous level.
            // If type is dummy router, the node has been added as result of a message
            // coming from a child. In this case the type NT_DUMMY_ROUTER should be kept.
            if (*pType != NodeData::NT_DUMMY_ROUTER) *pType = NodeData::NT_SOURCE;

            *pLevel = depth + 1;
            found = true;
            break;
        }
        else if (addr == Ar)
        {
            // if equal to a router address, we have a router.
            if (*pType != NodeData::NT_DUMMY_ROUTER) *pType = NodeData::NT_ROUTER;

            *pLevel = depth + 1;
            found = true;
            break;
        }
        else
        {
            // The address was not at current level, move on to next level
            // and set parent address at current level. Add parent address to the
            // list in order to add this to the node data base if necessary.
            Ap = Ar - Cskip[depth];
            lParents.append(Ap);
            // First router address at next level
            Ar = Ap + 1;
        }

    }

    return found;   
}

//-----------------------------------------------------------------------------
/** \brief Get Type and Depth
 *
 *  Use node address info to find info about place in the tree.
 *
 *  \param[in] addr
 *        Current address to check
 *  \param[out] pType
 *        Type of node. Router or end device
 *  \param[out] pDepth
 *        Depth of Node in Tree structure of address assignment
 *  \param[out] pParent
 *        Parent Node in Tree structure of address assignment
 *  \return bool
 *        true if valid address, else false
 */
//-----------------------------------------------------------------------------
bool NodeDb::getNodeParam(NodeData *pData, QList<int> &lParents)
{
    // Address parent
    int Ap;
    // depth counter
    int depth;
    // general counter
    int i;
	// stop condition
	bool fStop;
	// parent found
	bool bParentFound;

    // Start with parent address
    Ap = pData->parentAddr;
    depth= 0;
	fStop= false;
	bParentFound = false;

	do {
		for (i = 0; i < nodeData.size() && !fStop; i++) {
			NodeData *pNd= nodeData.at(i);

			if (pNd->addr==Ap) {
				bParentFound = true;
				lParents.prepend(Ap);
				if ((depth==0) && (pData->type!= NodeData::NT_DUMMY_ROUTER)) {
					// Using data fields of 0xFF to indicate router
					if (pData->voltage==0xff && pData->temp==0xff) {
						pData->type= NodeData::NT_ROUTER;
                        if (!fShowTemp)
						    pData->fNoData= 1;
					}
					else
						pData->type= NodeData::NT_SOURCE;
				}
				if (Ap!=0x0000) {
					Ap= pNd->parentAddr;
					depth++;
				} else {
					fStop= true;
				}
			}
		}
		// If parent was not found, insert the parent as dummy router connected to Coordinator
		if(!bParentFound) {
			depth=1;
			lParents.prepend(pData->parentAddr);
			lParents.prepend(0);
			// Using data fields of 0xFF to indicate router
			if (pData->voltage==0xff && pData->temp==0xff) {
				pData->type= NodeData::NT_ROUTER;
                if (!fShowTemp)
				    pData->fNoData= 1;
			}
			else {
				pData->type= NodeData::NT_SOURCE;
			}
			fStop = true;
		}
	} while (!fStop && depth<MAX_DEPTH);

	pData->level= depth + 1;

	return depth<MAX_DEPTH; // found
}



//-----------------------------------------------------------------------------
/** \brief Calculate XY position
 * 
 */
//-----------------------------------------------------------------------------
void NodeDb::calculateXYpos(NodeData *pData)
{
    // In the first version there will be a simplified algorithm.
    // Count all the children of a parent and distribute the nodes
    // equally over the x axis. The child is put one length unit(DB_DY)
    // below the parent node.

    // Find parent index
    int parent;
    parent = getIndex(pData->parent);

    // Start with counting of the child nodes of current parent and level.
    int nbrOfChildren = 0;
    int i;
    for (i = 0; i < nodeData.size(); i++)
    {
        if ((nodeData[i]->level == pData->level) &&
            (nodeData[i]->parent == pData->parent))
        {
            nbrOfChildren++;
        }
    }

    // Add one for the new node
    nbrOfChildren++;


    // When we know the number of childrens we have to adjust all the childrens xpos
    int xpos = 0xFFFFFFFF;
    if (nbrOfChildren > 1)
    {
        for (i = 0; i < nodeData.size(); i++)
        {
            if ((nodeData[i]->level == pData->level) &&
                (nodeData[i]->parent == pData->parent))
            {
                if (xpos == 0xFFFFFFFF)
                {
                    xpos = nodeData[parent]->xpos - ((nbrOfChildren - 1)*DB_DX/2);
                }

                nodeData[i]->xpos = xpos;
                xpos += DB_DX;
				if (nodeData[i]->status != NodeData::DS_NEW)
                {
					nodeData[i]->status = NodeData::DS_MOVE;
                }
            }
        }
    }
    else
    {
        xpos = nodeData[parent]->xpos;
    }
    
    // And finally we set xpos of the new node.
    pData->xpos = xpos;
    pData->ypos = nodeData[parent]->ypos + DB_DY;
}

//-----------------------------------------------------------------------------
/** \brief Find Child Node
 * 
 *  \param[in] index
 *        Index of parent node
 */
//-----------------------------------------------------------------------------
int NodeDb::findChild(int startIndex, int parent)
{
    int child = 0;
    for (int i = startIndex; i < nodeData.size(); i++)
    {
        if (nodeData[i]->parent == parent)
        {
            child = i;
            break;
        }
    }

    return child;
}

//-----------------------------------------------------------------------------
/** \brief Get Index
 * 
 *  Search data base for given address and return index
 *
 *  \param[in] addr
 *        address to search
 */
//-----------------------------------------------------------------------------
int NodeDb::getIndex(int addr)
{
    int index = 0;
    for (int i = 0; i < nodeData.size(); i++)
    {
        if (nodeData[i]->addr == addr)
        {
            index = i;
            break;
        }
    }

    return index;
}