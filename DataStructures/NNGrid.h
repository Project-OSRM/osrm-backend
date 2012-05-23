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

#ifndef ROUTED
#include <stxxl.h>
#endif

#ifdef _WIN32
#include <math.h>
#endif

#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>

#include "ExtractorStructs.h"
#include "GridEdge.h"
#include "Percent.h"
#include "PhantomNodes.h"
#include "Util.h"
#include "StaticGraph.h"
#include "../Algorithms/Bresenham.h"

namespace NNGrid{

static boost::thread_specific_ptr<std::ifstream> localStream;

template<bool WriteAccess = false>
class NNGrid {
public:
    NNGrid() /*: cellCache(500), fileCache(500)*/ {
        ramIndexTable.resize((1024*1024), ULONG_MAX);
    }

    NNGrid(const char* rif, const char* _i) {
        if(WriteAccess) {
            ERR("Not available in Write mode");
        }
        iif = std::string(_i);
        ramIndexTable.resize((1024*1024), ULONG_MAX);
        ramInFile.open(rif, std::ios::in | std::ios::binary);
    }

    ~NNGrid() {
        if(ramInFile.is_open()) ramInFile.close();

#ifndef ROUTED
        if (WriteAccess) {
            entries.clear();
        }
#endif
        if(localStream.get() && localStream->is_open()) {
            localStream->close();
        }
    }

    void OpenIndexFiles() {
        assert(ramInFile.is_open());
        ramInFile.read((char*)&ramIndexTable[0], sizeof(unsigned long)*1024*1024);
        ramInFile.close();
    }

    template<typename EdgeT>
    inline void ConstructGrid(std::vector<EdgeT> & edgeList, char * ramIndexOut, char * fileIndexOut) {
    	//TODO: Implement this using STXXL-Streams
#ifndef ROUTED
        Percent p(edgeList.size());
        BOOST_FOREACH(EdgeT & edge, edgeList) {
            p.printIncrement();
            if(edge.ignoreInGrid)
                continue;
            int slat = 100000*lat2y(edge.lat1/100000.);
            int slon = edge.lon1;
            int tlat = 100000*lat2y(edge.lat2/100000.);
            int tlon = edge.lon2;
            AddEdge( _GridEdge( edge.id, edge.nameID, edge.weight, _Coordinate(slat, slon), _Coordinate(tlat, tlon) ) );
        }
        double timestamp = get_timestamp();
        //create index file on disk, old one is over written
        indexOutFile.open(fileIndexOut, std::ios::out | std::ios::binary | std::ios::trunc);
        INFO("sorting grid data consisting of " << entries.size() << " edges...");
        //sort entries
        stxxl::sort(entries.begin(), entries.end(), CompareGridEdgeDataByRamIndex(), 1024*1024*1024);
        INFO("finished sorting after " << (get_timestamp() - timestamp) << "s");
        std::vector<GridEntry> entriesInFileWithRAMSameIndex;
        unsigned indexInRamTable = entries.begin()->ramIndex;
        unsigned long lastPositionInIndexFile = 0;
        cout << "writing data ..." << flush;
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
        ofstream ramFile(ramIndexOut, std::ios::out | std::ios::binary | std::ios::trunc);
        //write 4 MB of index Table in RAM
        ramFile.write((char *)&ramIndexTable[0], sizeof(unsigned long)*1024*1024 );
        //close ram index file
        ramFile.close();
#endif
    }

    bool FindPhantomNodeForCoordinate( const _Coordinate & location, PhantomNode & resultNode) {
//        double time1 = get_timestamp();
        bool foundNode = false;
        _Coordinate startCoord(100000*(lat2y(static_cast<double>(location.lat)/100000.)), location.lon);
        /** search for point on edge close to source */
        unsigned fileIndex = GetFileIndexForLatLon(startCoord.lat, startCoord.lon);
        std::vector<_GridEdge> candidates;
        for(int j = -32768; (j < (32768+1)) && (fileIndex != UINT_MAX); j+=32768) {
            for(int i = -1; i < 2; ++i){
                GetContentsOfFileBucketEnumerated(fileIndex+i+j, candidates);
            }
        }
        _GridEdge smallestEdge;
        _Coordinate tmp;
        double dist = numeric_limits<double>::max();
        double r, tmpDist;

        BOOST_FOREACH(_GridEdge candidate, candidates) {
            r = 0.;
            tmpDist = ComputeDistance(startCoord, candidate.startCoord, candidate.targetCoord, tmp, &r);
            if(tmpDist < dist && !DoubleEpsilonCompare(dist, tmpDist)) {
                //INFO("a) " << candidate.edgeBasedNode << ", dist: " << tmpDist);
                dist = tmpDist;
                resultNode.edgeBasedNode = candidate.edgeBasedNode;
                resultNode.nodeBasedEdgeNameID = candidate.nameID;
                resultNode.weight1 = candidate.weight;
                resultNode.weight2 = INT_MAX;
                resultNode.location.lat = tmp.lat;
                resultNode.location.lon = tmp.lon;
                foundNode = true;
                smallestEdge = candidate;
                //}  else if(tmpDist < dist) {
                //INFO("a) ignored " << candidate.edgeBasedNode << " at distance " << std::fabs(dist - tmpDist));
            } else if(DoubleEpsilonCompare(dist, tmpDist) && 1 == std::abs((int)candidate.edgeBasedNode-(int)resultNode.edgeBasedNode)) {
                resultNode.weight2 = candidate.weight;
                //INFO("b) " << candidate.edgeBasedNode << ", dist: " << tmpDist);
            }
        }

        //        INFO("startcoord: " << smallestEdge.startCoord << ", tgtcoord" <<  smallestEdge.targetCoord << "result: " << newEndpoint);
        //        INFO("length of old edge: " << ApproximateDistance(smallestEdge.startCoord, smallestEdge.targetCoord));
        //        INFO("Length of new edge: " << ApproximateDistance(smallestEdge.startCoord, newEndpoint));
        //        assert(!resultNode.isBidirected() || (resultNode.weight1 == resultNode.weight2));
        //        if(resultNode.weight1 != resultNode.weight2) {
        //            INFO("-> Weight1: " << resultNode.weight1 << ", weight2: " << resultNode.weight2);
        //            INFO("-> node: " << resultNode.edgeBasedNode << ", bidir: " << (resultNode.isBidirected() ? "yes" : "no"));
        //        }

//        INFO("startCoord: " << smallestEdge.startCoord << "; targetCoord: " << smallestEdge.targetCoord << "; newEndpoint: " << resultNode.location);
        double ratio = (foundNode ? std::min(1., ApproximateDistance(smallestEdge.startCoord, resultNode.location)/ApproximateDistance(smallestEdge.startCoord, smallestEdge.targetCoord)) : 0);
        resultNode.location.lat = round(100000.*(y2lat(static_cast<double>(resultNode.location.lat)/100000.)));
//        INFO("Length of vector: " << ApproximateDistance(smallestEdge.startCoord, resultNode.location)/ApproximateDistance(smallestEdge.startCoord, smallestEdge.targetCoord));
        //Hack to fix rounding errors and wandering via nodes.
        if(std::abs(location.lon - resultNode.location.lon) == 1)
            resultNode.location.lon = location.lon;
        if(std::abs(location.lat - resultNode.location.lat) == 1)
            resultNode.location.lat = location.lat;

        resultNode.weight1 *= ratio;
        if(INT_MAX != resultNode.weight2) {
            resultNode.weight2 -= resultNode.weight1;
        }
        resultNode.ratio = ratio;
        //        INFO("New weight1: " << resultNode.weight1 << ", new weight2: " << resultNode.weight2 << ", ratio: " << ratio);
        //        INFO("selected node: " << resultNode.edgeBasedNode << ", bidirected: " << (resultNode.isBidirected() ? "yes" : "no") <<  "\n--");
//        double time2 = get_timestamp();
//        INFO("NN-Lookup in " << 1000*(time2-time1) << "ms");
        return foundNode;
    }

    bool FindRoutingStarts(const _Coordinate& start, const _Coordinate& target, PhantomNodes & routingStarts) {
        routingStarts.Reset();
        return (FindPhantomNodeForCoordinate( start, routingStarts.startPhantom) &&
                FindPhantomNodeForCoordinate( target, routingStarts.targetPhantom) );
    }

    bool FindNearestCoordinateOnEdgeInNodeBasedGraph(const _Coordinate& inputCoordinate, _Coordinate& outputCoordinate) {
        bool found = false;
        unsigned fileIndex = GetFileIndexForLatLon(100000*(lat2y(static_cast<double>(inputCoordinate.lat)/100000.)), inputCoordinate.lon);
        std::vector<_GridEdge> candidates;
        boost::unordered_map< unsigned, unsigned > cellMap;
        for(int j = -32768; j < (32768+1); j+=32768) {
            for(int i = -1; i < 2; ++i) {
                GetContentsOfFileBucket(fileIndex+i+j, candidates, cellMap);
            }
        }
        _Coordinate tmp;
        double dist = (std::numeric_limits<double>::max)();
        BOOST_FOREACH(_GridEdge candidate, candidates) {
            double r = 0.;
            double tmpDist = ComputeDistance(inputCoordinate, candidate.startCoord, candidate.targetCoord, tmp, &r);
            if(tmpDist < dist) {
                found = true;
                dist = tmpDist;
                outputCoordinate = tmp;
            }
        }
        outputCoordinate.lat = 100000*(y2lat(static_cast<double>(outputCoordinate.lat)/100000.));
        return found;
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
        BOOST_FOREACH(_GridEdge candidate, candidates) {
            double r = 0.;
            double tmpDist = ComputeDistance(startCoord, candidate.startCoord, candidate.targetCoord, tmp, &r);
            if(tmpDist < dist) {
                dist = tmpDist;
                outputCoordinate.lat = round(100000*(y2lat(static_cast<double>(tmp.lat)/100000.)));
                outputCoordinate.lon = tmp.lon;
            }
        }
    }


private:
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

    inline bool DoubleEpsilonCompare(const double d1, const double d2) {
        return (std::fabs(d1 - d2) < 0.0001);
    }

    inline unsigned FillCell(std::vector<GridEntry>& entriesWithSameRAMIndex, const unsigned long fileOffset, boost::unordered_map< unsigned, unsigned > & cellMap ) {
        std::vector<char> tmpBuffer(32*32*4096,0);
        unsigned long indexIntoTmpBuffer = 0;
        unsigned numberOfWrittenBytes = 0;
        assert(indexOutFile.is_open());

        std::vector<unsigned long> cellIndex(32*32,ULONG_MAX);

        for(unsigned i = 0; i < entriesWithSameRAMIndex.size() -1; ++i) {
            assert(entriesWithSameRAMIndex[i].ramIndex== entriesWithSameRAMIndex[i+1].ramIndex);
        }

        //sort & unique
        std::sort(entriesWithSameRAMIndex.begin(), entriesWithSameRAMIndex.end(), CompareGridEdgeDataByFileIndex());
        entriesWithSameRAMIndex.erase(std::unique(entriesWithSameRAMIndex.begin(), entriesWithSameRAMIndex.end()), entriesWithSameRAMIndex.end());

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
        indexOutFile.write((char *)&cellIndex[0],32*32*sizeof(unsigned long));
        numberOfWrittenBytes += 32*32*sizeof(unsigned long);

        //write contents of tmpbuffer to disk
        indexOutFile.write(&tmpBuffer[0], indexIntoTmpBuffer*sizeof(char));
        numberOfWrittenBytes += indexIntoTmpBuffer*sizeof(char);

        return numberOfWrittenBytes;
    }

    inline unsigned FlushEntriesWithSameFileIndexToBuffer( std::vector<GridEntry> &vectorWithSameFileIndex, std::vector<char> & tmpBuffer, const unsigned long index) const {
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
        memcpy((char*)&(tmpBuffer[index+counter]), (char *)&lengthOfBucket, sizeof(lengthOfBucket));
        counter += sizeof(lengthOfBucket);

        BOOST_FOREACH(const GridEntry & entry, vectorWithSameFileIndex) {
            char * data = (char *)&(entry.edge);
            memcpy((char*)&(tmpBuffer[index+counter]), data, sizeof(entry.edge));
            counter += sizeof(entry.edge);
        }
        //Freeing data
        vectorWithSameFileIndex.clear();
        return counter;
    }

    inline void GetContentsOfFileBucketEnumerated(const unsigned fileIndex, std::vector<_GridEdge>& result) const {
        unsigned ramIndex = GetRAMIndexFromFileIndex(fileIndex);
        unsigned long startIndexInFile = ramIndexTable[ramIndex];
        if(startIndexInFile == ULONG_MAX) {
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
        localStream->seekg(startIndexInFile+(enumeratedIndex*sizeof(unsigned long)));
        unsigned long fetchedIndex = 0;
        localStream->read((char*) &fetchedIndex, sizeof(unsigned long));

        if(fetchedIndex == ULONG_MAX) {
            return;
        }
        const unsigned long position = fetchedIndex + 32*32*sizeof(unsigned long) ;

        unsigned lengthOfBucket;
        unsigned currentSizeOfResult = result.size();
        localStream->seekg(position);
        localStream->read((char *)&(lengthOfBucket), sizeof(unsigned));
        result.resize(currentSizeOfResult+lengthOfBucket);
        localStream->read((char *)&result[currentSizeOfResult], lengthOfBucket*sizeof(_GridEdge));
    }


    inline void GetContentsOfFileBucket(const unsigned fileIndex, std::vector<_GridEdge>& result, boost::unordered_map< unsigned, unsigned> & cellMap) {
        unsigned ramIndex = GetRAMIndexFromFileIndex(fileIndex);
        unsigned long startIndexInFile = ramIndexTable[ramIndex];
        if(startIndexInFile == ULONG_MAX) {
            return;
        }

        unsigned long cellIndex[32*32];

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
        localStream->read((char*) cellIndex, 32*32*sizeof(unsigned long));
        assert(cellMap.find(fileIndex) != cellMap.end());
        if(cellIndex[cellMap[fileIndex]] == ULONG_MAX) {
            return;
        }
        const unsigned long position = cellIndex[cellMap[fileIndex]] + 32*32*sizeof(unsigned long) ;

        unsigned lengthOfBucket;
        unsigned currentSizeOfResult = result.size();
        localStream->seekg(position);
        localStream->read((char *)&(lengthOfBucket), sizeof(unsigned));
        result.resize(currentSizeOfResult+lengthOfBucket);
        localStream->read((char *)&result[currentSizeOfResult], lengthOfBucket*sizeof(_GridEdge));
    }

    inline void AddEdge(const _GridEdge & edge) {
#ifndef ROUTED
        std::vector<BresenhamPixel> indexList;
        GetListOfIndexesForEdgeAndGridSize(edge.startCoord, edge.targetCoord, indexList);
        for(unsigned i = 0; i < indexList.size(); ++i) {
            entries.push_back(GridEntry(edge, indexList[i].first, indexList[i].second));
        }
#endif
    }

    inline double ComputeDistance(const _Coordinate& inputPoint, const _Coordinate& source, const _Coordinate& target, _Coordinate& nearest, double *r) {

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
            p = ((x + (m*y)) + (m*m*a - m*b))/(1. + m*m);
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

    inline unsigned GetFileIndexForLatLon(const int lt, const int ln) {
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

    const static unsigned long END_OF_BUCKET_DELIMITER = UINT_MAX;

    std::ofstream indexOutFile;
    std::ifstream ramInFile;
#ifndef ROUTED
    stxxl::vector<GridEntry> entries;
#endif
    std::vector<unsigned long> ramIndexTable; //8 MB for first level index in RAM
    std::string iif;
    //    LRUCache<int,std::vector<unsigned> > cellCache;
    //    LRUCache<int,std::vector<_Edge> > fileCache;
};
}

typedef NNGrid::NNGrid<false> ReadOnlyGrid;
typedef NNGrid::NNGrid<true > WritableGrid;

#endif /* NNGRID_H_ */
