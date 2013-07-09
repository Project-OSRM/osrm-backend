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
#include "typedefs.h"

#include <cstdlib>

#include <iostream>
#include <fstream>
#include <string>

ExtractorCallbacks * extractCallBacks;

int main (int argc, char *argv[]) {
    try {
        double startup_time = get_timestamp();

        if(argc < 2) {
            ERR("usage: \n" << argv[0] << " <file.osm/.osm.bz2/.osm.pbf> [<profile.lua>]");
        }

        /*** Setup Scripting Environment ***/
        ScriptingEnvironment scriptingEnvironment((argc > 2 ? argv[2] : "profile.lua"));

        unsigned number_of_threads = omp_get_num_procs();
        if(testDataFile("extractor.ini")) {
            BaseConfiguration extractorConfig("extractor.ini");
            unsigned rawNumber = stringToInt(extractorConfig.GetParameter("Threads"));
            if( rawNumber != 0 && rawNumber <= number_of_threads) {
                number_of_threads = rawNumber;
            }
        }
        omp_set_num_threads(number_of_threads);

        INFO("extracting data from input file " << argv[1]);
        bool file_has_pbf_format(false);
        std::string output_file_name(argv[1]);
        std::string restrictionsFileName(argv[1]);
        std::string::size_type pos = output_file_name.find(".osm.bz2");
        if(pos==std::string::npos) {
            pos = output_file_name.find(".osm.pbf");
            if(pos!=std::string::npos) {
                file_has_pbf_format = true;
            }
        }
        if(pos!=std::string::npos) {
            output_file_name.replace(pos, 8, ".osrm");
            restrictionsFileName.replace(pos, 8, ".osrm.restrictions");
        } else {
            pos=output_file_name.find(".osm");
            if(pos!=std::string::npos) {
                output_file_name.replace(pos, 5, ".osrm");
                restrictionsFileName.replace(pos, 5, ".osrm.restrictions");
            } else {
                output_file_name.append(".osrm");
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
        BaseParser* parser;
        if(file_has_pbf_format) {
            parser = new PBFParser(argv[1], extractCallBacks, scriptingEnvironment);
        } else {
            parser = new XMLParser(argv[1], extractCallBacks, scriptingEnvironment);
        }

        if(!parser->ReadHeader()) {
            ERR("Parser not initialized!");
        }
        INFO("Parsing in progress..");
        double parsing_start_time = get_timestamp();
        parser->Parse();
        INFO("Parsing finished after " <<
            (get_timestamp() - parsing_start_time) <<
            " seconds"
        );

        externalMemory.PrepareData(output_file_name, restrictionsFileName, amountOfRAM);

        delete parser;
        delete extractCallBacks;

        INFO("extraction finished after " << get_timestamp() - startup_time << "s");

        std::cout << "\nRun:\n"
            << "./osrm-prepare " << output_file_name << " " << restrictionsFileName
            << std::endl;
        return 0;
    } catch(std::exception & e) {
        WARN("unhandled exception: " << e.what());
    }
}
