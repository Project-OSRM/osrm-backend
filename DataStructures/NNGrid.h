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

#ifndef NNGRID_H_
#define NNGRID_H_

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <limits>
#include <vector>
#include <stxxl.h>

#include <boost/thread.hpp>
#include <google/dense_hash_map>

#include "ExtractorStructs.h"
#include "GridEdge.h"
#include "Percent.h"
#include "PhantomNodes.h"
#include "Util.h"
#include "StaticGraph.h"

static const unsigned MAX_CACHE_ELEMENTS = 1000;

namespace NNGrid{
static unsigned GetFileIndexForLatLon(const int lt, const int ln) {
    double lat = lt/100000.;
    double lon = ln/100000.;

    double x = ( lon + 180.0 ) / 360.0;
    double y = ( lat + 180.0 ) / 360.0;

    assert( x<=1.0 && x >= 0);
    assert( y<=1.0 && y >= 0);

    unsigned line = 1073741824.0*y;
    line = line - (line % 32768);
    assert(line % 32768 == 0);
    unsigned column = 32768.*x;
    unsigned fileIndex = line+column;
    return fileIndex;
}

static unsigned GetRAMIndexFromFileIndex(const int fileIndex) {
    unsigned fileLine = fileIndex / 32768;
    fileLine = fileLine / 32;
    fileLine = fileLine * 1024;
    unsigned fileColumn = (fileIndex % 32768);
    fileColumn = fileColumn / 32;
    unsigned ramIndex = fileLine + fileColumn;
    assert(ramIndex < 1024*1024);
    return ramIndex;
}

static inline int signum(int x){
    return (x > 0) ? 1 : (x < 0) ? -1 : 0;
}

static void GetIndicesByBresenhamsAlgorithm(int xstart,int ystart,int xend,int yend, std::vector<std::pair<unsigned, unsigned> > &indexList) {
    int x, y, t, dx, dy, incx, incy, pdx, pdy, ddx, ddy, es, el, err;

    dx = xend - xstart;
    dy = yend - ystart;

    incx = signum(dx);
    incy = signum(dy);
    if(dx<0) dx = -dx;
    if(dy<0) dy = -dy;

    if (dx>dy) {
        pdx=incx; pdy=0;
        ddx=incx; ddy=incy;
        es =dy;   el =dx;
    } else {
        pdx=0;    pdy=incy;
        ddx=incx; ddy=incy;
        es =dx;   el =dy;
    }
    x = xstart;
    y = ystart;
    err = el/2;
    {
        int fileIndex = (y-1)*32768 + x;
        int ramIndex = GetRAMIndexFromFileIndex(fileIndex);
        indexList.push_back(std::make_pair(fileIndex, ramIndex));
    }

    for(t=0; t<el; ++t) {
        err -= es;
        if(err<0) {
            err += el;
            x += ddx;
            y += ddy;
        } else {
            x += pdx;
            y += pdy;
        }
        {
            int fileIndex = (y-1)*32768 + x;
            int ramIndex = GetRAMIndexFromFileIndex(fileIndex);
            indexList.push_back(std::make_pair(fileIndex, ramIndex));
        }
    }
}

static void GetListOfIndexesForEdgeAndGridSize(_Coordinate& start, _Coordinate& target, std::vector<std::pair<unsigned, unsigned> > &indexList) {
    double lat1 = start.lat/100000.;
    double lon1 = start.lon/100000.;

    double x1 = ( lon1 + 180.0 ) / 360.0;
    double y1 = ( lat1 + 180.0 ) / 360.0;

    double lat2 = target.lat/100000.;
    double lon2 = target.lon/100000.;

    double x2 = ( lon2 + 180.0 ) / 360.0;
    double y2 = ( lat2 + 180.0 ) / 360.0;

    GetIndicesByBresenhamsAlgorithm(x1*32768, y1*32768, x2*32768, y2*32768, indexList);
}

template<bool WriteAccess = false>
class NNGrid {
public:
    NNGrid() { ramIndexTable.resize((1024*1024), UINT_MAX); if( WriteAccess) { entries = new stxxl::vector<GridEntry>(); }}

    NNGrid(const char* rif, const char* _i, unsigned numberOfThreads = omp_get_num_procs()) {
        iif = _i;
        ramIndexTable.resize((1024*1024), UINT_MAX);
        ramInFile.open(rif, std::ios::in | std::ios::binary);
    }

    ~NNGrid() {
        if(ramInFile.is_open()) ramInFile.close();

        if (WriteAccess) {
            delete entries;
        }
    }

    void OpenIndexFiles() {
        assert(ramInFile.is_open());

        for(int i = 0; i < 1024*1024; i++) {
            unsigned temp;
            ramInFile.read((char*)&temp, sizeof(unsigned));
            ramIndexTable[i] = temp;
        }
        ramInFile.close();
    }

    template<typename EdgeT, typename NodeInfoT>
    void ConstructGrid(std::vector<EdgeT> & edgeList, vector<NodeInfoT> * int2ExtNodeMap, char * ramIndexOut, char * fileIndexOut) {
        Percent p(edgeList.size());
        for(NodeID i = 0; i < edgeList.size(); i++) {
            p.printIncrement();
            if( edgeList[i].isLocatable() == false )
                continue;
            EdgeT edge = edgeList[i];

            int slat = 100000*lat2y(static_cast<double>(int2ExtNodeMap->at(edge.source()).lat)/100000.);
            int slon = int2ExtNodeMap->at(edge.source()).lon;
            int tlat = 100000*lat2y(static_cast<double>(int2ExtNodeMap->at(edge.target()).lat)/100000.);
            int tlon = int2ExtNodeMap->at(edge.target()).lon;
            AddEdge( _GridEdge(
                    edgeList[i].source(),
                    edgeList[i].target(),
                    _Coordinate(slat, slon),
                    _Coordinate(tlat, tlon) )
            );
        }
        double timestamp = get_timestamp();
        //create index file on disk, old one is over written
        indexOutFile.open(fileIndexOut, std::ios::out | std::ios::binary | std::ios::trunc);
        cout << "sorting grid data consisting of " << entries->size() << " edges..." << flush;
        //sort entries
        stxxl::sort(entries->begin(), entries->end(), CompareGridEdgeDataByRamIndex(), 1024*1024*1024);
        cout << "ok in " << (get_timestamp() - timestamp) << "s" << endl;
        std::vector<GridEntry> entriesInFileWithRAMSameIndex;
        unsigned indexInRamTable = entries->begin()->ramIndex;
        unsigned lastPositionInIndexFile = 0;
        unsigned numberOfUsedCells = 0;
        unsigned maxNumberOfRAMCellElements = 0;
        cout << "writing data ..." << flush;
        p.reinit(entries->size());
        for(stxxl::vector<GridEntry>::iterator vt = entries->begin(); vt != entries->end(); vt++) {
            p.printIncrement();
            if(vt->ramIndex != indexInRamTable) {
                unsigned numberOfBytesInCell = FillCell(entriesInFileWithRAMSameIndex, lastPositionInIndexFile);
                if(entriesInFileWithRAMSameIndex.size() > maxNumberOfRAMCellElements)
                    maxNumberOfRAMCellElements = entriesInFileWithRAMSameIndex.size();

                ramIndexTable[indexInRamTable] = lastPositionInIndexFile;
                lastPositionInIndexFile += numberOfBytesInCell;
                entriesInFileWithRAMSameIndex.clear();
                indexInRamTable = vt->ramIndex;
                numberOfUsedCells++;
            }
            entriesInFileWithRAMSameIndex.push_back(*vt);
        }
        /*unsigned numberOfBytesInCell = */FillCell(entriesInFileWithRAMSameIndex, lastPositionInIndexFile);
        ramIndexTable[indexInRamTable] = lastPositionInIndexFile;
        numberOfUsedCells++;
        entriesInFileWithRAMSameIndex.clear();

        assert(entriesInFileWithRAMSameIndex.size() == 0);

        for(int i = 0; i < 1024*1024; i++) {
            if(ramIndexTable[i] != UINT_MAX) {
                numberOfUsedCells--;
            }
        }
        assert(numberOfUsedCells == 0);

        //close index file
        indexOutFile.close();
        //Serialize RAM Index
        ofstream ramFile(ramIndexOut, std::ios::out | std::ios::binary | std::ios::trunc);
        //write 4 MB of index Table in RAM
        for(int i = 0; i < 1024*1024; i++)
            ramFile.write((char *)&ramIndexTable[i], sizeof(unsigned) );
        //close ram index file
        ramFile.close();
    }

    bool GetStartAndDestNodesOfEdge(const _Coordinate& coord, NodesOfEdge& nodesOfEdge) {
        _Coordinate startCoord(100000*(lat2y(static_cast<double>(coord.lat)/100000.)), coord.lon);
        /** search for point on edge next to source */
        unsigned fileIndex = GetFileIndexForLatLon(startCoord.lat, startCoord.lon);
        std::vector<_Edge> candidates;
        double timestamp = get_timestamp();

        for(int j = -32768; j < (32768+1); j+=32768) {
            for(int i = -1; i < 2; i++){
                GetContentsOfFileBucket(fileIndex+i+j, candidates);
            }
        }
        _Coordinate tmp;
        double dist = numeric_limits<double>::max();
        timestamp = get_timestamp();
        for(std::vector<_Edge>::iterator it = candidates.begin(); it != candidates.end(); it++) {
            double r = 0.;
            double tmpDist = ComputeDistance(startCoord, it->startCoord, it->targetCoord, tmp, &r);
            if(tmpDist < dist) {
                //              std::cout << "[debug] start distance " << (it - candidates.begin()) << " " << tmpDist << std::endl;
                nodesOfEdge.startID = it->start;
                nodesOfEdge.destID = it->target;
                nodesOfEdge.ratio = r;
                dist = tmpDist;
                nodesOfEdge.projectedPoint.lat = round(100000*(y2lat(static_cast<double>(tmp.lat)/100000.)));
                nodesOfEdge.projectedPoint.lon = tmp.lon;
            }
        }
        if(dist != numeric_limits<double>::max()) {
            return true;
        }
        return false;
    }

    bool FindPhantomNodeForCoordinate( const _Coordinate & location, PhantomNode & resultNode) {
        bool foundNode = false;
        _Coordinate startCoord(100000*(lat2y(static_cast<double>(location.lat)/100000.)), location.lon);
        /** search for point on edge close to source */
        unsigned fileIndex = GetFileIndexForLatLon(startCoord.lat, startCoord.lon);
        std::vector<_Edge> candidates;
        double timestamp = get_timestamp();

        for(int j = -32768; j < (32768+1); j+=32768) {
            for(int i = -1; i < 2; i++){
                GetContentsOfFileBucket(fileIndex+i+j, candidates);
            }
        }

        _Coordinate tmp;
        double dist = numeric_limits<double>::max();
        timestamp = get_timestamp();
        for(std::vector<_Edge>::iterator it = candidates.begin(); it != candidates.end(); it++) {
            double r = 0.;
            double tmpDist = ComputeDistance(startCoord, it->startCoord, it->targetCoord, tmp, &r);
            if(tmpDist < dist) {
                //              std::cout << "[debug] start distance " << (it - candidates.begin()) << " " << tmpDist << std::endl;
                resultNode.startNode = it->start;
                resultNode.targetNode = it->target;
                resultNode.ratio = r;
                dist = tmpDist;
                resultNode.location.lat = round(100000*(y2lat(static_cast<double>(tmp.lat)/100000.)));
                resultNode.location.lon = tmp.lon;
                foundNode = true;
            }
        }
        return foundNode;
    }

    bool FindRoutingStarts(const _Coordinate& start, const _Coordinate& target, PhantomNodes * routingStarts) {
        routingStarts->Reset();
        return (FindPhantomNodeForCoordinate( start, routingStarts->startPhantom) &&
                FindPhantomNodeForCoordinate( target, routingStarts->targetPhantom) );
    }

    void FindNearestNodeInGraph(const _Coordinate& inputCoordinate, _Coordinate& outputCoordinate) {
        unsigned fileIndex = GetFileIndexForLatLon(100000*(lat2y(static_cast<double>(inputCoordinate.lat)/100000.)), inputCoordinate.lon);
        std::vector<_Edge> candidates;
        double timestamp = get_timestamp();
        for(int j = -32768; j < (32768+1); j+=32768) {
            for(int i = -1; i < 2; i++) {
                GetContentsOfFileBucket(fileIndex+i+j, candidates);
            }
        }
        _Coordinate tmp;
        double dist = numeric_limits<double>::max();
        timestamp = get_timestamp();
        for(std::vector<_Edge>::iterator it = candidates.begin(); it != candidates.end(); it++) {
            double r = 0.;
            double tmpDist = ComputeDistance(inputCoordinate, it->startCoord, it->targetCoord, tmp, &r);
            if(tmpDist < dist) {
                dist = tmpDist;
                outputCoordinate = tmp;
            }
        }
        outputCoordinate.lat = 100000*(y2lat(static_cast<double>(outputCoordinate.lat)/100000.));
    }

    void FindNearestPointOnEdge(const _Coordinate& inputCoordinate, _Coordinate& outputCoordinate) {
        _Coordinate startCoord(100000*(lat2y(static_cast<double>(inputCoordinate.lat)/100000.)), inputCoordinate.lon);
        unsigned fileIndex = GetFileIndexForLatLon(startCoord.lat, startCoord.lon);

        std::vector<_Edge> candidates;
        for(int j = -32768; j < (32768+1); j+=32768) {
            for(int i = -1; i < 2; i++) {
                GetContentsOfFileBucket(fileIndex+i+j, candidates);
            }
        }
        _Coordinate tmp;
        double dist = numeric_limits<double>::max();
        for(std::vector<_Edge>::iterator it = candidates.begin(); it != candidates.end(); it++) {
            double r = 0.;
            double tmpDist = ComputeDistance(startCoord, it->startCoord, it->targetCoord, tmp, &r);
            if(tmpDist < dist) {
                dist = tmpDist;
                outputCoordinate.lat = round(100000*(y2lat(static_cast<double>(tmp.lat)/100000.)));
                outputCoordinate.lon = tmp.lon;
            }
        }
    }


private:

    unsigned FillCell(std::vector<GridEntry>& entriesWithSameRAMIndex, unsigned fileOffset )
    {
        vector<char> * tmpBuffer = new vector<char>();
        tmpBuffer->resize(32*32*4096,0);
        unsigned indexIntoTmpBuffer = 0;
        unsigned numberOfWrittenBytes = 0;
        assert(indexOutFile.is_open());

        vector<unsigned> cellIndex;
        cellIndex.resize(32*32,UINT_MAX);
        google::dense_hash_map< unsigned, unsigned > * cellMap = new google::dense_hash_map< unsigned, unsigned >(1024);
        cellMap->set_empty_key(UINT_MAX);

        unsigned ramIndex = entriesWithSameRAMIndex.begin()->ramIndex;
        unsigned lineBase = ramIndex/1024;
        lineBase = lineBase*32*32768;
        unsigned columnBase = ramIndex%1024;
        columnBase=columnBase*32;

        for(int i = 0; i < 32; i++)
        {
            for(int j = 0; j < 32; j++)
            {
                unsigned fileIndex = lineBase + i*32768 + columnBase+j;
                unsigned cellIndex = i*32+j;
                cellMap->insert(std::make_pair(fileIndex, cellIndex));
            }
        }

        for(unsigned i = 0; i < entriesWithSameRAMIndex.size() -1; i++)
        {
            assert(entriesWithSameRAMIndex[i].ramIndex== entriesWithSameRAMIndex[i+1].ramIndex);
        }

        //sort & unique
        std::sort(entriesWithSameRAMIndex.begin(), entriesWithSameRAMIndex.end(), CompareGridEdgeDataByFileIndex());
        std::vector<GridEntry>::iterator uniqueEnd = std::unique(entriesWithSameRAMIndex.begin(), entriesWithSameRAMIndex.end());

        //traverse each file bucket and write its contents to disk
        std::vector<GridEntry> entriesWithSameFileIndex;
        unsigned fileIndex = entriesWithSameRAMIndex.begin()->fileIndex;

        for(std::vector<GridEntry>::iterator it = entriesWithSameRAMIndex.begin(); it != uniqueEnd; it++)
        {
            assert(cellMap->find(it->fileIndex) != cellMap->end() ); //asserting that file index belongs to cell index
            if(it->fileIndex != fileIndex)
            {
                // start in cellIndex vermerken
                int localFileIndex = entriesWithSameFileIndex.begin()->fileIndex;
                int localCellIndex = cellMap->find(localFileIndex)->second;
                /*int localRamIndex = */GetRAMIndexFromFileIndex(localFileIndex);
                assert(cellMap->find(entriesWithSameFileIndex.begin()->fileIndex) != cellMap->end());

                cellIndex[localCellIndex] = indexIntoTmpBuffer + fileOffset;
                indexIntoTmpBuffer += FlushEntriesWithSameFileIndexToBuffer(entriesWithSameFileIndex, tmpBuffer, indexIntoTmpBuffer);
                entriesWithSameFileIndex.clear(); //todo: in flushEntries erledigen.
            }
            GridEntry data = *it;
            entriesWithSameFileIndex.push_back(data);
            fileIndex = it->fileIndex;
        }
        assert(cellMap->find(entriesWithSameFileIndex.begin()->fileIndex) != cellMap->end());
        int localFileIndex = entriesWithSameFileIndex.begin()->fileIndex;
        int localCellIndex = cellMap->find(localFileIndex)->second;
        /*int localRamIndex = */GetRAMIndexFromFileIndex(localFileIndex);

        cellIndex[localCellIndex] = indexIntoTmpBuffer + fileOffset;
        indexIntoTmpBuffer += FlushEntriesWithSameFileIndexToBuffer(entriesWithSameFileIndex, tmpBuffer, indexIntoTmpBuffer);
        entriesWithSameFileIndex.clear(); //todo: in flushEntries erledigen.

        assert(entriesWithSameFileIndex.size() == 0);

        for(int i = 0; i < 32*32; i++)
        {
            indexOutFile.write((char *)&cellIndex[i], sizeof(unsigned));
            numberOfWrittenBytes += sizeof(unsigned);
        }

        //write contents of tmpbuffer to disk
        for(unsigned i = 0; i < indexIntoTmpBuffer; i++)
        {
            indexOutFile.write(&(tmpBuffer->at(i)), sizeof(char));
            numberOfWrittenBytes += sizeof(char);
        }

        delete tmpBuffer;
        delete cellMap;
        return numberOfWrittenBytes;
    }

    unsigned FlushEntriesWithSameFileIndexToBuffer( std::vector<GridEntry> &vectorWithSameFileIndex, vector<char> * tmpBuffer, const unsigned index)
    {
        tmpBuffer->resize(tmpBuffer->size()+(sizeof(NodeID)+sizeof(NodeID)+4*sizeof(int)+sizeof(unsigned))*vectorWithSameFileIndex.size() );
        unsigned counter = 0;
        unsigned max = UINT_MAX;

        for(unsigned i = 0; i < vectorWithSameFileIndex.size()-1; i++)
        {
            assert( vectorWithSameFileIndex[i].fileIndex == vectorWithSameFileIndex[i+1].fileIndex );
            assert( vectorWithSameFileIndex[i].ramIndex == vectorWithSameFileIndex[i+1].ramIndex );
        }

        sort( vectorWithSameFileIndex.begin(), vectorWithSameFileIndex.end() );
        std::vector<GridEntry>::const_iterator newEnd = unique(vectorWithSameFileIndex.begin(), vectorWithSameFileIndex.end());
        for(std::vector<GridEntry>::const_iterator et = vectorWithSameFileIndex.begin(); et != newEnd; et++)
        {
            char * start = (char *)&et->edge.start;
            for(unsigned i = 0; i < sizeof(NodeID); i++)
            {
                tmpBuffer->at(index+counter) = start[i];
                counter++;
            }
            char * target = (char *)&et->edge.target;
            for(unsigned i = 0; i < sizeof(NodeID); i++)
            {
                tmpBuffer->at(index+counter) = target[i];
                counter++;
            }
            char * slat = (char *) &(et->edge.startCoord.lat);
            for(unsigned i = 0; i < sizeof(int); i++)
            {
                tmpBuffer->at(index+counter) = slat[i];
                counter++;
            }
            char * slon = (char *) &(et->edge.startCoord.lon);
            for(unsigned i = 0; i < sizeof(int); i++)
            {
                tmpBuffer->at(index+counter) = slon[i];
                counter++;
            }
            char * tlat = (char *) &(et->edge.targetCoord.lat);
            for(unsigned i = 0; i < sizeof(int); i++)
            {
                tmpBuffer->at(index+counter) = tlat[i];
                counter++;
            }
            char * tlon = (char *) &(et->edge.targetCoord.lon);
            for(unsigned i = 0; i < sizeof(int); i++)
            {
                tmpBuffer->at(index+counter) = tlon[i];
                counter++;
            }
        }
        char * umax = (char *) &max;
        for(unsigned i = 0; i < sizeof(unsigned); i++)
        {
            tmpBuffer->at(index+counter) = umax[i];
            counter++;
        }
        return counter;
    }

    void GetContentsOfFileBucket(const unsigned fileIndex, std::vector<_Edge>& result) {
        unsigned ramIndex = GetRAMIndexFromFileIndex(fileIndex);
        unsigned startIndexInFile = ramIndexTable[ramIndex];
        if(startIndexInFile == UINT_MAX) {
            return;
        }

        std::vector<unsigned> cellIndex;
        cellIndex.resize(32*32);
        google::dense_hash_map< unsigned, unsigned > * cellMap = new google::dense_hash_map< unsigned, unsigned >(1024);
        cellMap->set_empty_key(UINT_MAX);

        unsigned lineBase = ramIndex/1024;
        lineBase = lineBase*32*32768;
        unsigned columnBase = ramIndex%1024;
        columnBase=columnBase*32;

        for(int i = 0; i < 32; i++) {
            for(int j = 0; j < 32; j++) {
                assert(cellMap->size() >= 0);
                unsigned fileIndex = lineBase + i*32768 + columnBase+j;
                unsigned cellIndex = i*32+j;
                cellMap->insert(std::make_pair(fileIndex, cellIndex));
            }
        }
        {
//        if(cellCache.Holds(startIndexInFile)) {
//            cellIndex = cellCache.Find(startIndexInFile);
//        } else {
            std::ifstream localStream(iif, std::ios::in | std::ios::binary);
            localStream.seekg(startIndexInFile);
            unsigned numOfElementsInCell = 0;
            for(int i = 0; i < 32*32; i++) {
                localStream.read((char *)&cellIndex[i], sizeof(unsigned));
                numOfElementsInCell += cellIndex[i];
            }
            localStream.close();
            assert(cellMap->find(fileIndex) != cellMap->end());
            if(cellIndex[cellMap->find(fileIndex)->second] == UINT_MAX) {
                delete cellMap;
                return;
//            }
//            if(cellCache.Size() > MAX_CACHE_ELEMENTS) {
//                std::cout << "fixme: cell cache full" << std::endl;
//            }
//            std::cout << "Adding cache entry for cell position: " << startIndexInFile << std::endl;
//#pragma omp critical
//            {
//                cellCache.Add(startIndexInFile, cellIndex);
            }
        }
        unsigned position = cellIndex[cellMap->find(fileIndex)->second] + 32*32*sizeof(unsigned) ;
//        if(fileCache.Find(position).size() > 0) {
//            result = fileCache.Find(position);
//        } else {
            std::ifstream localStream(iif, std::ios::in | std::ios::binary);
            localStream.seekg(position);
            unsigned numberOfEdgesInFileBucket = 0;
            NodeID start, target; int slat, slon, tlat, tlon;
            do {
                localStream.read((char *)&(start), sizeof(NodeID));
                if(localStream.eof() || start == UINT_MAX)
                    break;
                localStream.read((char *)&(target), sizeof(NodeID));
                localStream.read((char *)&(slat), sizeof(int));
                localStream.read((char *)&(slon), sizeof(int));
                localStream.read((char *)&(tlat), sizeof(int));
                localStream.read((char *)&(tlon), sizeof(int));

                _Edge e(start, target);
                e.startCoord.lat = slat;
                e.startCoord.lon = slon;
                e.targetCoord.lat = tlat;
                e.targetCoord.lon = tlon;

                result.push_back(e);
                numberOfEdgesInFileBucket++;
            } while(true);
            localStream.close();

//            if(fileCache.Size() > MAX_CACHE_ELEMENTS) {
//                std::cout << "fixme: file cache full" << std::endl;
//            }
//            std::cout << "Adding cache entry for file position: " << position << std::endl;
//#pragma omp critical
//            {
//                fileCache.Add(position, result);
//            }
//        }
        delete cellMap;
    }

    void AddEdge(_GridEdge edge) {
        std::vector<std::pair<unsigned, unsigned> > indexList;
        GetListOfIndexesForEdgeAndGridSize(edge.startCoord, edge.targetCoord, indexList);
        for(unsigned i = 0; i < indexList.size(); i++) {
            entries->push_back(GridEntry(edge, indexList[i].first, indexList[i].second));
        }
    }

    double ComputeDistance(const _Coordinate& inputPoint, const _Coordinate& source, const _Coordinate& target, _Coordinate& nearest, double *r) {

        const double x = (double)inputPoint.lat;
        const double y = (double)inputPoint.lon;
        const double a = (double)source.lat;
        const double b = (double)source.lon;
        const double c = (double)target.lat;
        const double d = (double)target.lon;
        double p,q,mX,nY;
        if(c != a){
            const double m = (d-b)/(c-a); // slope
            // Projection of (x,y) on line joining (a,b) and (c,d)
            p = ((x + (m*y)) + (m*m*a - m*b))/(1 + m*m);
            q = b + m*(p - a);
        }
        else{
            p = c;
            q = y;
        }
        nY = (d*p - c*q)/(a*d - b*c);
        mX = (p - nY*a)/c;// These values are actually n/m+n and m/m+n , we neednot calculate the values of m an n as we are just interested in the ratio
        *r = mX;
        if(*r<=0){
            nearest.lat = source.lat;
            nearest.lon = source.lon;
            return ((b - y)*(b - y) + (a - x)*(a - x));
        }
        else if(*r >= 1){
            nearest.lat = target.lat;
            nearest.lon = target.lon;
            return ((d - y)*(d - y) + (c - x)*(c - x));

        }
        // point lies in between
        nearest.lat = p;
        nearest.lon = q;
        return (p-x)*(p-x) + (q-y)*(q-y);
    }

    ofstream indexOutFile;
    ifstream ramInFile;
    stxxl::vector<GridEntry> * entries;
    std::vector<unsigned> ramIndexTable; //4 MB for first level index in RAM
    const char * iif;
//    HashTable<unsigned, std::vector<_Edge> > fileCache;
//    HashTable<unsigned, std::vector<unsigned> > cellCache;
};
}

typedef NNGrid::NNGrid<false> ReadOnlyGrid;
typedef NNGrid::NNGrid<true > WritableGrid;

#endif /* NNGRID_H_ */
