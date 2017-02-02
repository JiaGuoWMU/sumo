/****************************************************************************/
/// @file    GNERerouterIntervalDialog.cpp
/// @author  Pablo Alvarez Lopez
/// @date    eb 2017
/// @version $Id: GNERerouterIntervalDialog.cpp 22824 2017-02-02 09:51:02Z palcraft $
///
/// Dialog for edit rerouter intervals
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.dlr.de/
// Copyright (C) 2001-2017 DLR (http://www.dlr.de/) and contributors
/****************************************************************************/
//
//   This file is part of SUMO.
//   SUMO is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 3 of the License, or
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

#include <iostream>
#include <utils/gui/windows/GUIAppEnum.h>
#include <utils/gui/images/GUIIconSubSys.h>

#include "GNERerouterIntervalDialog.h"
#include "GNERerouter.h"
#include "GNERerouterInterval.h"

#ifdef CHECK_MEMORY_LEAKS
#include <foreign/nvwa/debug_new.h>
#endif

// ===========================================================================
// FOX callback mapping
// ===========================================================================

FXDEFMAP(GNERerouterIntervalDialog) GNERerouterIntervalDialogMap[] = {
    FXMAPFUNC(SEL_DOUBLECLICKED,    MID_GNE_MODE_ADDITIONALDIALOG_TABLE,    GNERerouterIntervalDialog::onCmdDoubleClicked),
    FXMAPFUNC(SEL_COMMAND,          MID_GNE_MODE_ADDITIONALDIALOG_ACCEPT,   GNERerouterIntervalDialog::onCmdAccept),
    FXMAPFUNC(SEL_COMMAND,          MID_GNE_MODE_ADDITIONALDIALOG_CANCEL,   GNERerouterIntervalDialog::onCmdCancel),
    FXMAPFUNC(SEL_COMMAND,          MID_GNE_MODE_ADDITIONALDIALOG_RESET,    GNERerouterIntervalDialog::onCmdReset),
};

// Object implementation
FXIMPLEMENT(GNERerouterIntervalDialog, FXDialogBox, GNERerouterIntervalDialogMap, ARRAYNUMBER(GNERerouterIntervalDialogMap))

// ===========================================================================
// member method definitions
// ===========================================================================

GNERerouterIntervalDialog::GNERerouterIntervalDialog(GNERerouter* rerouterParent) :
    GNEAdditionalDialog(rerouterParent, 320, 240),
    myRerouterParent(rerouterParent) {
    // Create table, copy intervals and update table
    myIntervalList = new FXTable(myContentFrame, this, MID_GNE_MODE_ADDITIONALDIALOG_TABLE, TABLE_NO_ROWSELECT | TABLE_NO_COLSELECT | LAYOUT_FILL_X | LAYOUT_FILL_Y);
    myIntervalList->setEditable(false);
    copyIntervals();
    updateTable();

    // Execute additional dialog (To make it modal)
    execute();
}


GNERerouterIntervalDialog::~GNERerouterIntervalDialog() {
}


long
GNERerouterIntervalDialog::onCmdAccept(FXObject*, FXSelector, void*) {
    
    // in this point we need to use GNEChange_RerouterInterval to allow undo/redos of rerouterIntervals

    // set new intervals into rerouter
    myRerouterParent->setRerouterIntervals(myRerouterIntervals);
    // Stop Modal
    getApp()->stopModal(this, TRUE);
    return 1;
}


long
GNERerouterIntervalDialog::onCmdCancel(FXObject*, FXSelector, void*) {
    // Clear copied intervals
    for(std::vector<GNERerouterInterval*>::const_iterator i = myRerouterIntervals.begin(); i != myRerouterIntervals.end(); i++) {
        delete (*i);
    }
    myRerouterIntervals.clear();
    // Stop Modal
    getApp()->stopModal(this, TRUE);
    return 1;
}


long
GNERerouterIntervalDialog::onCmdReset(FXObject*, FXSelector, void*) {
    // clear copied/modified intervals
    for(std::vector<GNERerouterInterval*>::const_iterator i = myRerouterIntervals.begin(); i != myRerouterIntervals.end(); i++) {
        delete (*i);
    }
    // Copy original intervals again and update table
    copyIntervals();
    updateTable();
    return 1;
}



long 
GNERerouterIntervalDialog::onCmdDoubleClicked(FXObject*, FXSelector, void*) {
    // First check if
    if(myIntervalList->getNumRows() > 0) {
        // check if add button was pressed
        if(myIntervalList->getItem((int)myRerouterIntervals.size(), 3)->hasFocus()) {
            if(true) {
                std::cout << "add row " << std::endl;
                return 1;
            }
        } else {
            // check if some delete button was pressed
            for(int i = 0; i < (int)myRerouterIntervals.size(); i++) {
                if(myIntervalList->getItem(i, 3)->hasFocus()) {
                    myIntervalList->removeRows(i);
                    myRerouterIntervals.erase(myRerouterIntervals.begin() + i);
                    return 1;
                }
            }
            return 1;
        }
    }
}


void 
GNERerouterIntervalDialog::copyIntervals() {
    myRerouterIntervals.clear();
    for(std::vector<GNERerouterInterval*>::const_iterator i = myRerouterParent->getRerouterIntervals().begin(); i != myRerouterParent->getRerouterIntervals().end(); i++) {
        myRerouterIntervals.push_back(new GNERerouterInterval(*i));
    }
}

void
GNERerouterIntervalDialog::updateTable() {
     // clear table
    myIntervalList->clearItems();
    // set number of rows
    myIntervalList->setTableSize(int(myRerouterIntervals.size()) + 1, 4);
    // Configure list
    myIntervalList->setVisibleColumns(4);
    myIntervalList->setColumnWidth(0, 106);
    myIntervalList->setColumnWidth(1, 106);
    myIntervalList->setColumnWidth(2, 49);
    myIntervalList->setColumnWidth(3, 50);
    myIntervalList->setColumnText(0, "start");
    myIntervalList->setColumnText(1, "end");
    myIntervalList->setColumnText(2, "edit");
    myIntervalList->setColumnText(3, "");
    myIntervalList->getRowHeader()->setWidth(0);
    // Declare index for rows and pointer to FXTableItem
    int indexRow = 0;
    FXTableItem* item = 0;
    // iterate over values
    for (std::vector<GNERerouterInterval*>::iterator i = myRerouterIntervals.begin(); i != myRerouterIntervals.end(); i++) {
        // Set time
        item = new FXTableItem(toString((*i)->getBegin()).c_str());
        myIntervalList->setItem(indexRow, 0, item);
        // Set speed
        item = new FXTableItem(toString((*i)->getEnd()).c_str());
        myIntervalList->setItem(indexRow, 1, item);
        // set remove
        item = new FXTableItem("", GUIIconSubSys::getIcon(ICON_REMOVE));
        item->setJustify(FXTableItem::CENTER_X | FXTableItem::CENTER_Y);
        myIntervalList->setItem(indexRow, 3, item);
        // Update index
        indexRow++;
    }
    // set add
    item = new FXTableItem("", GUIIconSubSys::getIcon(ICON_ADD));
    item->setJustify(FXTableItem::CENTER_X | FXTableItem::CENTER_Y);
    myIntervalList->setItem(indexRow, 3, item);
}

/****************************************************************************/