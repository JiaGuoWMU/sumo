/****************************************************************************/
/// @file    NBLoadedSUMOTLDef.h
/// @author  Jakob Erdmann
/// @date    Mar 2011
/// @version $Id$
///
// A complete traffic light logic loaded from a sumo-net. (opted to reimplement
// since NBLoadedTLDef is quite vissim specific)
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.sourceforge.net/
// Copyright (C) 2001-2011 DLR (http://www.dlr.de/) and contributors
/****************************************************************************/
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
/****************************************************************************/

// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#include <vector>
#include <set>
#include <cassert>
#include <iterator>
#include <utils/common/MsgHandler.h>
#include <utils/common/ToString.h>
#include <utils/options/OptionsCont.h>
#include "NBTrafficLightLogic.h"
#include "NBTrafficLightDefinition.h"
#include "NBLoadedSUMOTLDef.h"
#include "NBNode.h"

#ifdef CHECK_MEMORY_LEAKS
#include <foreign/nvwa/debug_new.h>
#endif // CHECK_MEMORY_LEAKS

// ===========================================================================
// method definitions
// ===========================================================================

NBLoadedSUMOTLDef::NBLoadedSUMOTLDef(const std::string &id, const std::string &programID, SUMOTime offset) :
        NBTrafficLightDefinition(id, programID),
        myTLLogic(0) 
{
    myTLLogic = new NBTrafficLightLogic(id, programID, 0);
    myTLLogic->setOffset(offset);
}


NBLoadedSUMOTLDef::NBLoadedSUMOTLDef(NBTrafficLightDefinition *def, NBTrafficLightLogic *logic) : 
    NBTrafficLightDefinition(def->getID(), def->getProgramID()),
    myTLLogic(new NBTrafficLightLogic(logic)),
    myOriginalNodes(def->getNodes().begin(), def->getNodes().end())
{ 
    myControlledLinks = def->getControlledLinks();
}


NBLoadedSUMOTLDef::~NBLoadedSUMOTLDef() throw() {
    delete myTLLogic;
}


NBTrafficLightLogic *
NBLoadedSUMOTLDef::myCompute(const NBEdgeCont &ec, unsigned int brakingTime) throw() {
    // @todo what to do with those parameters?
    UNUSED_PARAMETER(ec);
    UNUSED_PARAMETER(brakingTime);
    myTLLogic->closeBuilding();
    return new NBTrafficLightLogic(myTLLogic);
}


void
NBLoadedSUMOTLDef::addConnection(NBEdge *from, NBEdge *to, int fromLane, int toLane, int linkIndex) {
    assert(myTLLogic->getNumLinks() > 0); // logic should be loaded by now
    if (linkIndex >= (int)myTLLogic->getNumLinks()) {
        WRITE_ERROR("Invalid linkIndex " + toString(linkIndex) + " for traffic light '" + getID() + 
                "' with " + toString(myTLLogic->getNumLinks()) + " links.");
        return;
    }
    NBConnection conn(from, fromLane, to, toLane, linkIndex);
    // avoid duplicates
    remove_if(myControlledLinks.begin(), myControlledLinks.end(), connection_equal(conn));
    myControlledLinks.push_back(conn); 
    addNode(from->getToNode());
    addNode(to->getFromNode());
    myOriginalNodes.insert(from->getToNode());
    myOriginalNodes.insert(to->getFromNode());
    // added connections are definitely controlled. make sure none are removed because they lie within the tl
    myControlledInnerEdges.insert(from->getID());
    // set this information now so that it can be used while loading diffs
    from->setControllingTLInformation(conn, getID());
}


void
NBLoadedSUMOTLDef::setTLControllingInformation(const NBEdgeCont &) const throw() {
    setTLControllingInformation();
}


void
NBLoadedSUMOTLDef::setTLControllingInformation() const {
    // set the information about the link's positions within the tl into the
    //  edges the links are starting at, respectively
    for (NBConnectionVector::const_iterator it = myControlledLinks.begin(); it != myControlledLinks.end(); it++) {
        const NBConnection &c = *it;
        assert(c.getTLIndex() < myTLLogic->getNumLinks());
        NBEdge *edge = c.getFrom();
        edge->setControllingTLInformation(c, getID());
    }
}


void
NBLoadedSUMOTLDef::remapRemoved(NBEdge*, const EdgeVector&, const EdgeVector&) throw() {}


void
NBLoadedSUMOTLDef::replaceRemoved(NBEdge*, int, NBEdge*, int) throw() {}


void
NBLoadedSUMOTLDef::addPhase(SUMOTime duration, const std::string &state) {
    myTLLogic->addStep(duration, state);
}


bool 
NBLoadedSUMOTLDef::amInvalid() {
    if (myControlledLinks.size()==0) {
        return true;
    }
    // make sure that myControlledNodes are the original nodes
    if (myControlledNodes.size() != myOriginalNodes.size()) {
        return true;
    }
    for (std::vector<NBNode*>::iterator i=myControlledNodes.begin(); i!=myControlledNodes.end(); i++) {
        if (myOriginalNodes.count(*i) != 1) {
            return true;
        }
    }
    return false;
}


void 
NBLoadedSUMOTLDef::removeConnection(const NBConnection &conn, bool reconstruct) {
    NBConnectionVector::iterator it = find(myControlledLinks.begin(), myControlledLinks.end(), conn);
    if (it == myControlledLinks.end()) {
        throw ProcessError("Attempt to remove nonexistant connection");
    }
    const int removed = conn.getTLIndex();
    // remove the connection 
    myControlledLinks.erase(it);
    if (reconstruct) {
        // updating the edge is only needed for immediate use in NETEDIT. 
        // It may conflict with loading diffs
        conn.getFrom()->setControllingTLInformation(conn, "");
        // shift link numbers down so there is no gap
        for (NBConnectionVector::iterator it = myControlledLinks.begin(); it != myControlledLinks.end(); it++) {
            NBConnection &c = *it;
            if (c.getTLIndex() > removed) {
                c.setTLIndex(c.getTLIndex() - 1);
            }
        }
        // update controlling information with new link numbers
        setTLControllingInformation();
        // rebuild the logic
        const std::vector<NBTrafficLightLogic::PhaseDefinition> phases = myTLLogic->getPhases(); 
        NBTrafficLightLogic *newLogic = new NBTrafficLightLogic(getID(), getProgramID(), 0);
        newLogic->setOffset(myTLLogic->getOffset());
        for (std::vector<NBTrafficLightLogic::PhaseDefinition>::const_iterator it = phases.begin(); it != phases.end(); it++) {
            std::string newState = it->state;
            newState.erase(newState.begin() + removed);
            newLogic->addStep(it->duration, newState);
        }
        delete myTLLogic;
        myTLLogic = newLogic;
    }
}

/****************************************************************************/

