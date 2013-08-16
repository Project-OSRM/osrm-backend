/*
    open source routing machine
    Copyright (C) Dennis Luxen, others 2010

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU AFFERO General Public License as published by
the Free Software Foundation; either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
or see http://www.gnu.org/licenses/agpl.txt.
 */

#include "SearchEngineData.h"

void SearchEngineData::InitializeOrClearFirstThreadLocalStorage() {
    if(!forwardHeap.get()) {
        forwardHeap.reset(new QueryHeap(nodeHelpDesk->GetNumberOfNodes()));
    } else {
        forwardHeap->Clear();
    }
    if(!backwardHeap.get()) {
        backwardHeap.reset(new QueryHeap(nodeHelpDesk->GetNumberOfNodes()));
    } else {
        backwardHeap->Clear();
    }
}

void SearchEngineData::InitializeOrClearSecondThreadLocalStorage() {
    if(!forwardHeap2.get()) {
        forwardHeap2.reset(new QueryHeap(nodeHelpDesk->GetNumberOfNodes()));
    } else {
        forwardHeap2->Clear();
    }
    if(!backwardHeap2.get()) {
        backwardHeap2.reset(new QueryHeap(nodeHelpDesk->GetNumberOfNodes()));
     } else {
        backwardHeap2->Clear();
    }
}

void SearchEngineData::InitializeOrClearThirdThreadLocalStorage() {
    if(!forwardHeap3.get()) {
        forwardHeap3.reset(new QueryHeap(nodeHelpDesk->GetNumberOfNodes()));
    } else {
        forwardHeap3->Clear();
    }
    if(!backwardHeap3.get()) {
        backwardHeap3.reset(new QueryHeap(nodeHelpDesk->GetNumberOfNodes()));
    } else {
        backwardHeap3->Clear();
    }
}
