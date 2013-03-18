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

#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstring>

#include <algorithm>
#include <fstream>
#include <limits>
#include <vector>

#ifndef ROUTED
#include <stxxl.h>
#endif

#ifdef _WIN32
#include <math.h>
#endif

#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>

#include "DeallocatingVector.h"
#include "GridEdge.h"
#include "Percent.h"
#include "PhantomNodes.h"
#include "MercatorUtil.h"
#include "StaticGraph.h"
#include "TimingUtil.h"
#include "../Algorithms/Bresenham.h"
#include "QueryEdge.h"

class NodeInformationHelpDesk;
class QueryGraph;

static boost::thread_specific_ptr<std::ifstream> localStream;


class NNGrid {
public:
    NNGrid() : nodeHelpDesk(NULL) {}
    
    
    NNGrid(const char* rif, const char* _i, NodeInformationHelpDesk* _nodeHelpDesk, StaticGraph<QueryEdge::EdgeData>* g) : nodeHelpDesk(_nodeHelpDesk), graph(g) {
        nodeHelpDesk = _nodeHelpDesk;
        iif = std::string(_i);
        
        ramIndexTable.resize((1024*1024), std::numeric_limits<uint64_t>::max());
        ramInFile.open(rif, std::ios::in | std::ios::binary);
        if(!ramInFile) { ERR(rif <<  " not found"); }
    }
    
    ~NNGrid() {
        if(ramInFile.is_open()) ramInFile.close();

        if(localStream.get() && localStream->is_open()) {
            localStream->close();
        }
    }

    void OpenIndexFiles() {
        assert(ramInFile.is_open());
        ramInFile.read(static_cast<char*>(static_cast<void*>(&ramIndexTable[0]) ), sizeof(uint64_t)*1024*1024);
        ramInFile.close();
    }

#ifndef ROUTED
    template<typename EdgeT>
    inline void ConstructGrid(DeallocatingVector<EdgeT> & edgeList, const char * ramIndexOut, const char * fileIndexOut) {
    	//TODO: Implement this using STXXL-Streams
        Percent p(edgeList.size());
        BOOST_FOREACH(EdgeT & edge, edgeList) {
            p.printIncrement();
            if(edge.ignoreInGrid)
                continue;
            int slat = 100000*lat2y(edge.lat1/100000.);
            int slon = edge.lon1;
            int tlat = 100000*lat2y(edge.lat2/100000.);
            int tlon = edge.lon2;
            AddEdge( _GridEdge( edge.id, edge.nameID, edge.weight, _Coordinate(slat, slon), _Coordinate(tlat, tlon), edge.belongsToTinyComponent, edge.mode ) );
        }
        if( 0 == entries.size() ) {
        	ERR("No viable edges for nearest neighbor index. Aborting");
        }
        double timestamp = get_timestamp();
        //create index file on disk, old one is over written
        indexOutFile.open(fileIndexOut, std::ios::out | std::ios::binary | std::ios::trunc);
        //sort entries
        stxxl::sort(entries.begin(), entries.end(), CompareGridEdgeDataByRamIndex(), 1024*1024*1024);
        INFO("finished sorting after " << (get_timestamp() - timestamp) << "s");
        std::vector<GridEntry> entriesInFileWithRAMSameIndex;
        unsigned indexInRamTable = entries.begin()->ramIndex;
        uint64_t lastPositionInIndexFile = 0;
        std::cout << "writing data ..." << std::flush;
        p.reinit(entries.size());
        boost::unordered_map< unsigned, unsigned > cellMap(1024);
        BOOST_FOREACH(GridEntry & gridEntry, entries) {
            p.printIncrement();
            if(gridEntry.ramIndex != indexInRamTable) {
                cellMap.clear();
                BuildCellIndexToFileIndexMap(indexInRamTable, cellMap);

                unsigned numberOfBytesInCell = FillCell(entriesInFileWithRAMSameIndex, lastPositionInIndexFile, cellMap);
                ramIndexTable[indexInRamTable] = lastPositionInIndexFile;
                lastPositionInIndexFile += numberOfBytesInCell;
                entriesInFileWithRAMSameIndex.clear();
                indexInRamTable = gridEntry.ramIndex;
            }
            entriesInFileWithRAMSameIndex.push_back(gridEntry);
        }
        cellMap.clear();
        BuildCellIndexToFileIndexMap(indexInRamTable, cellMap);
        /*unsigned numberOfBytesInCell = */FillCell(entriesInFileWithRAMSameIndex, lastPositionInIndexFile, cellMap);
        ramIndexTable[indexInRamTable] = lastPositionInIndexFile;
        entriesInFileWithRAMSameIndex.clear();
        std::vector<GridEntry>().swap(entriesInFileWithRAMSameIndex);
        assert(entriesInFileWithRAMSameIndex.size() == 0);
        //close index file
        indexOutFile.close();

        //Serialize RAM Index
        std::ofstream ramFile(ramIndexOut, std::ios::out | std::ios::binary | std::ios::trunc);
        //write 4 MB of index Table in RAM
        ramFile.write((char *)&ramIndexTable[0], sizeof(uint64_t)*1024*1024 );
        //close ram index file
        ramFile.close();
    }
#endif
    inline bool CoordinatesAreEquivalent(const _Coordinate & a, const _Coordinate & b, const _Coordinate & c, const _Coordinate & d) const {
        return (a == b && c == d) || (a == c && b == d) || (a == d && b == c);
    }

    bool FindPhantomNodeForCoordinate( const _Coordinate & location, PhantomNode & resultNode, const unsigned zoomLevel);

    bool FindRoutingStarts(const _Coordinate& start, const _Coordinate& target, PhantomNodes & routingStarts, unsigned zoomLevel) {
        routingStarts.Reset();
        return (FindPhantomNodeForCoordinate( start, routingStarts.startPhantom, zoomLevel) &&
                FindPhantomNodeForCoordinate( target, routingStarts.targetPhantom, zoomLevel) );
    }

    bool FindNearestCoordinateOnEdgeInNodeBasedGraph(const _Coordinate& inputCoordinate, _Coordinate& outputCoordinate, unsigned zoomLevel = 18) {
        PhantomNode resultNode;
        bool foundNode = FindPhantomNodeForCoordinate(inputCoordinate, resultNode, zoomLevel);
        outputCoordinate = resultNode.location;
        return foundNode;
    }

    void FindNearestPointOnEdge(const _Coordinate& inputCoordinate, _Coordinate& outputCoordinate) {
        _Coordinate startCoord(100000*(lat2y(static_cast<double>(inputCoordinate.lat)/100000.)), inputCoordinate.lon);
        unsigned fileIndex = GetFileIndexForLatLon(startCoord.lat, startCoord.lon);

        std::vector<_GridEdge> candidates;
        boost::unordered_map< unsigned, unsigned > cellMap;
        for(int j = -32768; j < (32768+1); j+=32768) {
            for(int i = -1; i < 2; ++i) {
                GetContentsOfFileBucket(fileIndex+i+j, candidates, cellMap);
            }
        }
        _Coordinate tmp;
        double dist = (std::numeric_limits<double>::max)();
        BOOST_FOREACH(const _GridEdge & candidate, candidates) {
            double r = 0.;
            double tmpDist = ComputeDistance(startCoord, candidate.startCoord, candidate.targetCoord, tmp, &r);
            if(tmpDist < dist) {
                dist = tmpDist;
                outputCoordinate.lat = round(100000*(y2lat(static_cast<double>(tmp.lat)/100000.)));
                outputCoordinate.lon = tmp.lon;
            }
        }
    }


protected:
    inline unsigned GetCellIndexFromRAMAndFileIndex(const unsigned ramIndex, const unsigned fileIndex) const {
        unsigned lineBase = ramIndex/1024;
        lineBase = lineBase*32*32768;
        unsigned columnBase = ramIndex%1024;
        columnBase=columnBase*32;
        for (int i = 0;i < 32;++i) {
            for (int j = 0;j < 32;++j) {
                const unsigned localFileIndex = lineBase + i * 32768 + columnBase + j;
                if(localFileIndex == fileIndex) {
                    unsigned cellIndex = i * 32 + j;
                    return cellIndex;
                }
            }
        }
        return UINT_MAX;
    }

    inline void BuildCellIndexToFileIndexMap(const unsigned ramIndex, boost::unordered_map<unsigned, unsigned >& cellMap){
        unsigned lineBase = ramIndex/1024;
        lineBase = lineBase*32*32768;
        unsigned columnBase = ramIndex%1024;
        columnBase=columnBase*32;
        std::vector<std::pair<unsigned, unsigned> >insertionVector(1024);
        for (int i = 0;i < 32;++i) {
            for (int j = 0;j < 32;++j) {
                unsigned fileIndex = lineBase + i * 32768 + columnBase + j;
                unsigned cellIndex = i * 32 + j;
                insertionVector[i * 32 + j] = std::make_pair(fileIndex, cellIndex);
            }
        }
        cellMap.insert(insertionVector.begin(), insertionVector.end());
    }

    inline bool DoubleEpsilonCompare(const double d1, const double d2) const {
        return (std::fabs(d1 - d2) < FLT_EPSILON);
    }

#ifndef ROUTED
    inline unsigned FillCell(std::vector<GridEntry>& entriesWithSameRAMIndex, const uint64_t fileOffset, boost::unordered_map< unsigned, unsigned > & cellMap ) {
        std::vector<char> tmpBuffer(32*32*4096,0);
        uint64_t indexIntoTmpBuffer = 0;
        unsigned numberOfWrittenBytes = 0;
        assert(indexOutFile.is_open());

        std::vector<uint64_t> cellIndex(32*32,std::numeric_limits<uint64_t>::max());

        for(unsigned i = 0; i < entriesWithSameRAMIndex.size() -1; ++i) {
            assert(entriesWithSameRAMIndex[i].ramIndex== entriesWithSameRAMIndex[i+1].ramIndex);
        }

        //sort & unique
        std::sort(entriesWithSameRAMIndex.begin(), entriesWithSameRAMIndex.end(), CompareGridEdgeDataByFileIndex());
//        entriesWithSameRAMIndex.erase(std::unique(entriesWithSameRAMIndex.begin(), entriesWithSameRAMIndex.end()), entriesWithSameRAMIndex.end());

        //traverse each file bucket and write its contents to disk
        std::vector<GridEntry> entriesWithSameFileIndex;
        unsigned fileIndex = entriesWithSameRAMIndex.begin()->fileIndex;

        BOOST_FOREACH(GridEntry & gridEntry, entriesWithSameRAMIndex) {
            assert(cellMap.find(gridEntry.fileIndex) != cellMap.end() ); //asserting that file index belongs to cell index
            if(gridEntry.fileIndex != fileIndex) {
                // start in cellIndex vermerken
                int localFileIndex = entriesWithSameFileIndex.begin()->fileIndex;
                int localCellIndex = cellMap.find(localFileIndex)->second;
                assert(cellMap.find(entriesWithSameFileIndex.begin()->fileIndex) != cellMap.end());

                cellIndex[localCellIndex] = indexIntoTmpBuffer + fileOffset;
                indexIntoTmpBuffer += FlushEntriesWithSameFileIndexToBuffer(entriesWithSameFileIndex, tmpBuffer, indexIntoTmpBuffer);
                fileIndex = gridEntry.fileIndex;
            }
            entriesWithSameFileIndex.push_back(gridEntry);
        }
        assert(cellMap.find(entriesWithSameFileIndex.begin()->fileIndex) != cellMap.end());
        int localFileIndex = entriesWithSameFileIndex.begin()->fileIndex;
        int localCellIndex = cellMap.find(localFileIndex)->second;

        cellIndex[localCellIndex] = indexIntoTmpBuffer + fileOffset;
        indexIntoTmpBuffer += FlushEntriesWithSameFileIndexToBuffer(entriesWithSameFileIndex, tmpBuffer, indexIntoTmpBuffer);

        assert(entriesWithSameFileIndex.size() == 0);
        indexOutFile.write(static_cast<char*>(static_cast<void*>(&cellIndex[0])),32*32*sizeof(uint64_t));
        numberOfWrittenBytes += 32*32*sizeof(uint64_t);

        //write contents of tmpbuffer to disk
        indexOutFile.write(&tmpBuffer[0], indexIntoTmpBuffer*sizeof(char));
        numberOfWrittenBytes += indexIntoTmpBuffer*sizeof(char);

        return numberOfWrittenBytes;
    }

    inline unsigned FlushEntriesWithSameFileIndexToBuffer( std::vector<GridEntry> &vectorWithSameFileIndex, std::vector<char> & tmpBuffer, const uint64_t index) const {
        sort( vectorWithSameFileIndex.begin(), vectorWithSameFileIndex.end() );
        vectorWithSameFileIndex.erase(unique(vectorWithSameFileIndex.begin(), vectorWithSameFileIndex.end()), vectorWithSameFileIndex.end());
        const unsigned lengthOfBucket = vectorWithSameFileIndex.size();
        tmpBuffer.resize(tmpBuffer.size()+(sizeof(_GridEdge)*lengthOfBucket) + sizeof(unsigned) );
        unsigned counter = 0;

        for(unsigned i = 0; i < vectorWithSameFileIndex.size()-1; ++i) {
            assert( vectorWithSameFileIndex[i].fileIndex == vectorWithSameFileIndex[i+1].fileIndex );
            assert( vectorWithSameFileIndex[i].ramIndex == vectorWithSameFileIndex[i+1].ramIndex );
        }

        //write length of bucket
        memcpy((char*)&(tmpBuffer[index+counter]), (char*)&lengthOfBucket, sizeof(lengthOfBucket));
        counter += sizeof(lengthOfBucket);

        BOOST_FOREACH(const GridEntry & entry, vectorWithSameFileIndex) {
            char * data = (char*)&(entry.edge);
            memcpy(static_cast<char*>(static_cast<void*>(&(tmpBuffer[index+counter]) )), data, sizeof(entry.edge));
            counter += sizeof(entry.edge);
        }
        //Freeing data
        vectorWithSameFileIndex.clear();
        return counter;
    }
#endif

    inline void GetContentsOfFileBucketEnumerated(const unsigned fileIndex, std::vector<_GridEdge>& result) const {
        unsigned ramIndex = GetRAMIndexFromFileIndex(fileIndex);
        uint64_t startIndexInFile = ramIndexTable[ramIndex];
        if(startIndexInFile == std::numeric_limits<uint64_t>::max()) {
            return;
        }
        unsigned enumeratedIndex = GetCellIndexFromRAMAndFileIndex(ramIndex, fileIndex);

        if(!localStream.get() || !localStream->is_open()) {
            localStream.reset(new std::ifstream(iif.c_str(), std::ios::in | std::ios::binary));
        }
        if(!localStream->good()) {
            localStream->clear(std::ios::goodbit);
            DEBUG("Resetting stale filestream");
        }

        //only read the single necessary cell index
        localStream->seekg(startIndexInFile+(enumeratedIndex*sizeof(uint64_t)));
        uint64_t fetchedIndex = 0;
        localStream->read(static_cast<char*>( static_cast<void*>(&fetchedIndex)), sizeof(uint64_t));

        if(fetchedIndex == std::numeric_limits<uint64_t>::max()) {
            return;
        }
        const uint64_t position = fetchedIndex + 32*32*sizeof(uint64_t) ;

        unsigned lengthOfBucket;
        unsigned currentSizeOfResult = result.size();
        localStream->seekg(position);
        localStream->read(static_cast<char*>( static_cast<void*>(&(lengthOfBucket))), sizeof(unsigned));
        result.resize(currentSizeOfResult+lengthOfBucket);
        localStream->read(static_cast<char*>( static_cast<void*>(&result[currentSizeOfResult])), lengthOfBucket*sizeof(_GridEdge));
    }

    inline void GetContentsOfFileBucket(const unsigned fileIndex, std::vector<_GridEdge>& result, boost::unordered_map< unsigned, unsigned> & cellMap) {
        unsigned ramIndex = GetRAMIndexFromFileIndex(fileIndex);
        uint64_t startIndexInFile = ramIndexTable[ramIndex];
        if(startIndexInFile == std::numeric_limits<uint64_t>::max()) {
            return;
        }

        uint64_t cellIndex[32*32];

        cellMap.clear();
        BuildCellIndexToFileIndexMap(ramIndex,  cellMap);
        if(!localStream.get() || !localStream->is_open()) {
            localStream.reset(new std::ifstream(iif.c_str(), std::ios::in | std::ios::binary));
        }
        if(!localStream->good()) {
            localStream->clear(std::ios::goodbit);
            DEBUG("Resetting stale filestream");
        }

        localStream->seekg(startIndexInFile);
        localStream->read(static_cast<char*>(static_cast<void*>( cellIndex)), 32*32*sizeof(uint64_t));
        assert(cellMap.find(fileIndex) != cellMap.end());
        if(cellIndex[cellMap[fileIndex]] == std::numeric_limits<uint64_t>::max()) {
            return;
        }
        const uint64_t position = cellIndex[cellMap[fileIndex]] + 32*32*sizeof(uint64_t) ;

        unsigned lengthOfBucket;
        unsigned currentSizeOfResult = result.size();
        localStream->seekg(position);
        localStream->read(static_cast<char*>(static_cast<void*>(&(lengthOfBucket))), sizeof(unsigned));
        result.resize(currentSizeOfResult+lengthOfBucket);
        localStream->read(static_cast<char*>(static_cast<void*>(&result[currentSizeOfResult])), lengthOfBucket*sizeof(_GridEdge));
    }

#ifndef ROUTED
    inline void AddEdge(const _GridEdge & edge) {
        std::vector<BresenhamPixel> indexList;
        GetListOfIndexesForEdgeAndGridSize(edge.startCoord, edge.targetCoord, indexList);
        for(unsigned i = 0; i < indexList.size(); ++i) {
            entries.push_back(GridEntry(edge, indexList[i].first, indexList[i].second));
        }
    }
#endif

    inline double ComputeDistance(const _Coordinate& inputPoint, const _Coordinate& source, const _Coordinate& target, _Coordinate& nearest, double *r) {
//        INFO("comparing point " << inputPoint << " to edge [" << source << "-" << target << "]");
        const double x = static_cast<double>(inputPoint.lat);
        const double y = static_cast<double>(inputPoint.lon);
        const double a = static_cast<double>(source.lat);
        const double b = static_cast<double>(source.lon);
        const double c = static_cast<double>(target.lat);
        const double d = static_cast<double>(target.lon);
        double p,q,mX,nY;
//        INFO("x=" << x << ", y=" << y << ", a=" << a << ", b=" << b << ", c=" << c << ", d=" << d);
        if(fabs(a-c) > FLT_EPSILON){
            const double m = (d-b)/(c-a); // slope
            // Projection of (x,y) on line joining (a,b) and (c,d)
            p = ((x + (m*y)) + (m*m*a - m*b))/(1. + m*m);
            q = b + m*(p - a);
        }
        else{
            p = c;
            q = y;
        }
        nY = (d*p - c*q)/(a*d - b*c);
        mX = (p - nY*a)/c;// These values are actually n/m+n and m/m+n , we neednot calculate the values of m an n as we are just interested in the ratio
//        INFO("p=" << p << ", q=" << q << ", nY=" << nY << ", mX=" << mX);
        if(std::isnan(mX)) {
            *r = (target == inputPoint) ? 1. : 0.;
        } else {
            *r = mX;
        }
//        INFO("r=" << *r);
        if(*r<=0.){
            nearest.lat = source.lat;
            nearest.lon = source.lon;
//            INFO("a returning distance " << ((b - y)*(b - y) + (a - x)*(a - x)))
            return ((b - y)*(b - y) + (a - x)*(a - x));
        }
        else if(*r >= 1.){
            nearest.lat = target.lat;
            nearest.lon = target.lon;
//            INFO("b returning distance " << ((d - y)*(d - y) + (c - x)*(c - x)))

            return ((d - y)*(d - y) + (c - x)*(c - x));
        }
        // point lies in between
        nearest.lat = p;
        nearest.lon = q;
//        INFO("c returning distance " << (p-x)*(p-x) + (q-y)*(q-y))

        return (p-x)*(p-x) + (q-y)*(q-y);
    }

    inline void GetListOfIndexesForEdgeAndGridSize(const _Coordinate& start, const _Coordinate& target, std::vector<BresenhamPixel> &indexList) const {
        double lat1 = start.lat/100000.;
        double lon1 = start.lon/100000.;

        double x1 = ( lon1 + 180.0 ) / 360.0;
        double y1 = ( lat1 + 180.0 ) / 360.0;

        double lat2 = target.lat/100000.;
        double lon2 = target.lon/100000.;

        double x2 = ( lon2 + 180.0 ) / 360.0;
        double y2 = ( lat2 + 180.0 ) / 360.0;

        Bresenham(x1*32768, y1*32768, x2*32768, y2*32768, indexList);
        BOOST_FOREACH(BresenhamPixel & pixel, indexList) {
            int fileIndex = (pixel.second-1)*32768 + pixel.first;
            int ramIndex = GetRAMIndexFromFileIndex(fileIndex);
            pixel.first = fileIndex;
            pixel.second = ramIndex;
        }
    }

    inline unsigned GetFileIndexForLatLon(const int lt, const int ln) const {
        double lat = lt/100000.;
        double lon = ln/100000.;

        double x = ( lon + 180.0 ) / 360.0;
        double y = ( lat + 180.0 ) / 360.0;

        if( x>1.0 || x < 0.)
            return UINT_MAX;
        if( y>1.0 || y < 0.)
            return UINT_MAX;

        unsigned line = (32768 * (32768-1))*y;
        line = line - (line % 32768);
        assert(line % 32768 == 0);
        unsigned column = 32768.*x;
        unsigned fileIndex = line+column;
        return fileIndex;
    }

    inline unsigned GetRAMIndexFromFileIndex(const int fileIndex) const {
        unsigned fileLine = fileIndex / 32768;
        fileLine = fileLine / 32;
        fileLine = fileLine * 1024;
        unsigned fileColumn = (fileIndex % 32768);
        fileColumn = fileColumn / 32;
        unsigned ramIndex = fileLine + fileColumn;
        assert(ramIndex < 1024*1024);
        return ramIndex;
    }

    const static uint64_t END_OF_BUCKET_DELIMITER = boost::integer_traits<uint64_t>::const_max;

    std::ifstream ramInFile;
#ifndef ROUTED
    std::ofstream indexOutFile;
    stxxl::vector<GridEntry> entries;
#endif
    NodeInformationHelpDesk* nodeHelpDesk;
    std::vector<uint64_t> ramIndexTable; //8 MB for first level index in RAM
    std::string iif;
    bool writeAccess;
    StaticGraph<QueryEdge::EdgeData>* graph;
    //    LRUCache<int,std::vector<unsigned> > cellCache;
    //    LRUCache<int,std::vector<_Edge> > fileCache;
};

class ReadOnlyGrid : public NNGrid {
public:
    ReadOnlyGrid(const char* rif, const char* _i, NodeInformationHelpDesk* _nodeHelpDesk, StaticGraph<QueryEdge::EdgeData>* g) : NNGrid(rif,_i,_nodeHelpDesk,g) {}
};

class WritableGrid : public NNGrid {
public:
    WritableGrid() {
        ramIndexTable.resize((1024*1024), std::numeric_limits<uint64_t>::max());
    }
    ~WritableGrid() {
        #ifndef ROUTED
            entries.clear();
        #endif
    }
};

#endif /* NNGRID_H_ */
