/*****************************************************************************

  Example program for vtzero library.

  vtzero-check - Check vector tiles for validity

*****************************************************************************/

#include "utils.hpp"

#include <vtzero/vector_tile.hpp>

#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

class result {

    int m_return_code = 0;

public:

    void has_warning() noexcept {
        if (m_return_code < 1) {
            m_return_code = 1;
        }
    }

    void has_error() noexcept {
        if (m_return_code < 2) {
            m_return_code = 2;
        }
    }

    void has_fatal_error() noexcept {
        if (m_return_code < 3) {
            m_return_code = 3;
        }
    }

    int return_code() const noexcept {
        return m_return_code;
    }

} result;

class CheckGeomHandler {

    vtzero::point m_prev_point{};
    int m_layer_num;
    int m_feature_num;
    int64_t m_extent;
    bool m_is_first_point = false;
    int m_count = 0;

    void print_context() const {
        std::cerr << " in layer " << m_layer_num
                  << " in feature " << m_feature_num
                  << " in geometry " << m_count
                  << ": ";
    }

    void print_error(const char* message) const {
        result.has_error();
        std::cerr << "Error";
        print_context();
        std::cerr << message << '\n';
    }

    void print_warning(const char* message) const {
        result.has_warning();
        std::cerr << "Warning";
        print_context();
        std::cerr << message << '\n';
    }

    void check_point_location(const vtzero::point point) const {
        if (point.x < -m_extent ||
            point.y < -m_extent ||
            point.x > 2 * m_extent ||
            point.y > 2 * m_extent) {
            print_warning("point waaaay beyond the extent");
        }
    }

public:

    CheckGeomHandler(uint32_t extent, int layer_num, int feature_num) :
        m_layer_num(layer_num),
        m_feature_num(feature_num),
        m_extent(static_cast<int64_t>(extent)) {
    }

    // ----------------------------------------------------------------------

    void points_begin(const uint32_t /*count*/) const {
    }

    void points_point(const vtzero::point point) const {
        check_point_location(point);
    }

    void points_end() const {
    }

    // ----------------------------------------------------------------------

    void linestring_begin(const uint32_t count) {
        if (count < 2) {
            print_error("Not enough points in linestring");
        }
        m_is_first_point = true;
    }

    void linestring_point(const vtzero::point point) {
        if (m_is_first_point) {
            m_is_first_point = false;
        } else {
            if (point == m_prev_point) {
                print_error("Duplicate point in linestring");
            }
        }
        m_prev_point = point;

        check_point_location(point);
    }

    void linestring_end() {
        ++m_count;
    }

    // ----------------------------------------------------------------------

    void ring_begin(const uint32_t count) {
        if (count < 4) {
            print_error("Not enough points in ring");
        }
        m_is_first_point = true;
    }

    void ring_point(const vtzero::point point) {
        if (m_is_first_point) {
            m_is_first_point = false;
        } else {
            if (point == m_prev_point) {
                print_error("Duplicate point in ring");
            }
        }
        m_prev_point = point;

        check_point_location(point);
    }

    void ring_end(const vtzero::ring_type rt) {
        if (rt == vtzero::ring_type::invalid) {
            print_error("Invalid ring with area 0");
        }
        if (m_count == 0 && rt != vtzero::ring_type::outer) {
            print_error("First ring isn't an outer ring");
        }
        ++m_count;
    }

}; // class CheckGeomHandler

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " TILE\n";
        return 1;
    }

    std::string input_file{argv[1]};
    const auto data = read_file(input_file);

    std::set<std::string> layer_names;

    vtzero::vector_tile tile{data};

    int layer_num = 0;
    int feature_num = -1;
    try {
        while (auto layer = tile.next_layer()) {
            if (layer.name().empty()) {
                std::cerr << "Error in layer " << layer_num << ": name is empty (spec 4.1)\n";
                result.has_error();
            }

            std::string name(layer.name());
            if (layer_names.count(name) > 0) {
                std::cerr << "Error in layer " << layer_num << ": name is duplicate of previous layer ('" << name << "') (spec 4.1)\n";
                result.has_error();
            }

            layer_names.insert(name);

            feature_num = 0;
            while (auto feature = layer.next_feature()) {
                CheckGeomHandler handler{layer.extent(), layer_num, feature_num};
                vtzero::decode_geometry(feature.geometry(), handler);
                ++feature_num;
            }
            if (feature_num == 0) {
                std::cerr << "Warning: No features in layer " << layer_num << " (spec 4.1)\n";
                result.has_warning();
            }
            feature_num = -1;
            ++layer_num;
        }
        if (layer_num == 0) {
            std::cerr << "Warning: No layers in vector tile (spec 4.1)\n";
            result.has_warning();
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error in layer " << layer_num;
        if (feature_num >= 0) {
            std::cerr << " in feature " << feature_num;
        }
        std::cerr << ": " << e.what() << '\n';
        result.has_fatal_error();
    }

    return result.return_code();
}

