#ifndef POLYLINECOMPRESSOR_H_
#define POLYLINECOMPRESSOR_H_

struct SegmentInformation;

#include "osrm/coordinate.hpp"

#include <string>
#include <vector>

class PolylineCompressor
{
  private:
    std::string encode_vector(std::vector<int> &numbers) const;

    std::string encode_number(const int number_to_encode) const;

  public:
    std::string get_encoded_string(const std::vector<SegmentInformation> &polyline) const;

    std::vector<FixedPointCoordinate> decode_string(const std::string &geometry_string) const;
};

#endif /* POLYLINECOMPRESSOR_H_ */
