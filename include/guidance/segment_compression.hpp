#ifndef ENGINE_GUIDANCE_PROCESSING_SEGMENT_COMPRESSION_HPP_
#define ENGINE_GUIDANCE_PROCESSING_SEGMENT_COMPRESSION_HPP_

#include "engine/segment_inforamtion.hpp"

#include <vector>

namespace osrm
{
namespace guidance
{

/*
  Simplify turn instructions
  Input :
    10. Turn left on B 36 for 20 km
    11. Continue on B 35; B 36 for 2 km
    12. Continue on B 36 for 13 km

  Output:
   10. Turn left on B 36 for 35 km
*/

inline void CombineSimilarSegments(std::vector<SegmentInformation> &segments)
{
    // TODO: rework to check only end and start of string.
    //      stl string is way to expensive
    //    unsigned lastTurn = 0;
    //    for(unsigned i = 1; i < path_description.size(); ++i) {
    //        string1 = sEngine.GetEscapedNameForNameID(path_description[i].name_id);
    //        if(TurnInstruction::GoStraight == path_description[i].turn_instruction) {
    //            if(std::string::npos != string0.find(string1+";")
    //                  || std::string::npos != string0.find(";"+string1)
    //                  || std::string::npos != string0.find(string1+" ;")
    //                    || std::string::npos != string0.find("; "+string1)
    //                    ){
    //                SimpleLogger().Write() << "->next correct: " << string0 << " contains " <<
    //                string1;
    //                for(; lastTurn != i; ++lastTurn)
    //                    path_description[lastTurn].name_id = path_description[i].name_id;
    //                path_description[i].turn_instruction = TurnInstruction::NoTurn;
    //            } else if(std::string::npos != string1.find(string0+";")
    //                  || std::string::npos != string1.find(";"+string0)
    //                    || std::string::npos != string1.find(string0+" ;")
    //                    || std::string::npos != string1.find("; "+string0)
    //                    ){
    //                SimpleLogger().Write() << "->prev correct: " << string1 << " contains " <<
    //                string0;
    //                path_description[i].name_id = path_description[i-1].name_id;
    //                path_description[i].turn_instruction = TurnInstruction::NoTurn;
    //            }
    //        }
    //        if (TurnInstruction::NoTurn != path_description[i].turn_instruction) {
    //            lastTurn = i;
    //        }
    //        string0 = string1;
    //    }
    //
}
} // namespace guidance
} // namespace osrm

#endif // ENGINE_GUIDANCE_PROCESSING_SEGMENT_COMPRESSION_HPP_
