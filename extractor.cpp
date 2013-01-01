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

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>

#include "typedefs.h"
#include "Extractor/ExtractorCallbacks.h"
#include "Extractor/ExtractionContainers.h"
#include "Extractor/ScriptingEnvironment.h"
#include "Extractor/PBFParser.h"
#include "Extractor/XMLParser.h"
#include "Util/BaseConfiguration.h"
#include "Util/InputFileUtil.h"
#include "Util/MachineInfo.h"
#include "Util/OpenMPWrapper.h"
#include "Util/StringUtil.h"

typedef BaseConfiguration ExtractorConfiguration;

ExtractorCallbacks * extractCallBacks;

int main (int argc, char *argv[]) {
    double earliestTime = get_timestamp();

    if(argc < 2) {
        ERR("usage: \n" << argv[0] << " <file.osm/.osm.bz2/.osm.pbf> [<profile.lua>]");
    }

    /*** Setup Scripting Environment ***/
    ScriptingEnvironment scriptingEnvironment((argc > 2 ? argv[2] : "profile.lua"));

    unsigned numberOfThreads = omp_get_num_procs();
    if(testDataFile("extractor.ini")) {
        ExtractorConfiguration extractorConfig("extractor.ini");
        unsigned rawNumber = stringToInt(extractorConfig.GetParameter("Threads"));
        if( rawNumber != 0 && rawNumber <= numberOfThreads)
            numberOfThreads = rawNumber;
    }
    omp_set_num_threads(numberOfThreads);


    INFO("extracting data from input file " << argv[1]);
    bool isPBF(false);
    std::string outputFileName(argv[1]);
    std::string restrictionsFileName(argv[1]);
    std::string::size_type pos = outputFileName.find(".osm.bz2");
    if(pos==std::string::npos) {
        pos = outputFileName.find(".osm.pbf");
        if(pos!=std::string::npos) {
            isPBF = true;
        }
    }
    if(pos!=std::string::npos) {
        outputFileName.replace(pos, 8, ".osrm");
        restrictionsFileName.replace(pos, 8, ".osrm.restrictions");
    } else {
        pos=outputFileName.find(".osm");
        if(pos!=std::string::npos) {
            outputFileName.replace(pos, 5, ".osrm");
            restrictionsFileName.replace(pos, 5, ".osrm.restrictions");
        } else {
            outputFileName.append(".osrm");
            restrictionsFileName.append(".osrm.restrictions");
        }
    }

    unsigned amountOfRAM = 1;
    unsigned installedRAM = GetPhysicalmemory(); 
    if(installedRAM < 2048264) {
        WARN("Machine has less than 2GB RAM.");
    }
	
    StringMap stringMap;
    ExtractionContainers externalMemory;


    stringMap[""] = 0;
    extractCallBacks = new ExtractorCallbacks(&externalMemory, &stringMap);
    BaseParser<ExtractorCallbacks, _Node, _RawRestrictionContainer, _Way> * parser;
    if(isPBF) {
        parser = new PBFParser(argv[1]);
    } else {
        parser = new XMLParser(argv[1]);
    }
    parser->RegisterCallbacks(extractCallBacks);
    parser->RegisterScriptingEnvironment(scriptingEnvironment);

    if(!parser->Init())
        ERR("Parser not initialized!");
    double time = get_timestamp();
    parser->Parse();
    INFO("parsing finished after " << get_timestamp() - time << " seconds");

    externalMemory.PrepareData(outputFileName, restrictionsFileName, amountOfRAM);

    stringMap.clear();
    delete parser;
    delete extractCallBacks;
    INFO("finished after " << get_timestamp() - earliestTime << "s");

    std::cout << "\nRun:\n"
                   "./osrm-prepare " << outputFileName << " " << restrictionsFileName << std::endl;
    return 0;
}
