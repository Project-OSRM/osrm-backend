# encoding: utf-8

##
# This file is auto-generated. DO NOT EDIT!
#
require 'protobuf'
require 'protobuf/message'

module Protobuffer_response

  ##
  # Message Classes
  #
  class Coordinate < ::Protobuf::Message; end
  class Route_summary < ::Protobuf::Message; end
  class Route_instructions < ::Protobuf::Message; end
  class Route < ::Protobuf::Message; end
  class Hint < ::Protobuf::Message; end
  class Route_response < ::Protobuf::Message; end
  class Matrix_row < ::Protobuf::Message; end
  class Distance_matrix < ::Protobuf::Message; end
  class Named_location < ::Protobuf::Message; end
  class Locate_response < ::Protobuf::Message; end
  class Nearest_response < ::Protobuf::Message; end


  ##
  # Message Fields
  #
  class Coordinate
    required :float, :lat, 1
    required :float, :lon, 2
  end

  class Route_summary
    required :uint32, :total_distance, 1
    required :uint32, :total_time, 2
    required :string, :start_point, 3
    required :string, :end_point, 4
  end

  class Route_instructions
    required :string, :instruction_id, 1
    required :string, :street_name, 2
    required :int32, :length, 3
    required :int32, :position, 4
    required :int32, :time, 5
    required :string, :length_str, 6
    required :string, :earth_direction, 7
    required :int32, :azimuth, 8
    optional :int32, :travel_mode, 9
  end

  class Route
    optional ::Protobuffer_response::Route_summary, :route_summary, 1
    optional :string, :route_geometry, 2
    repeated ::Protobuffer_response::Coordinate, :via_points, 3
    repeated :int32, :via_indices, 4
    repeated :string, :route_name, 5
    repeated ::Protobuffer_response::Route_instructions, :route_instructions, 6
  end

  class Hint
    required :int32, :check_sum, 1
    repeated :string, :location, 2
  end

  class Route_response
    required :int32, :status, 1
    required :string, :status_message, 2
    optional ::Protobuffer_response::Route, :main_route, 3
    optional ::Protobuffer_response::Route, :alternative_route, 4
    optional ::Protobuffer_response::Hint, :hint, 5
  end

  class Matrix_row
    repeated :uint32, :entry, 1
  end

  class Distance_matrix
    repeated ::Protobuffer_response::Matrix_row, :row, 1
  end

  class Named_location
    required ::Protobuffer_response::Coordinate, :mapped_coordinate, 1
    required :string, :name, 2
  end

  class Locate_response
    required :uint32, :status, 1
    required ::Protobuffer_response::Coordinate, :mapped_coordinate, 2
  end

  class Nearest_response
    required :uint32, :status, 1
    repeated ::Protobuffer_response::Named_location, :location, 2
  end

end

