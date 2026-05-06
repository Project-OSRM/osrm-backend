from collections.abc import Iterator, Sequence
from typing import overload

class EngineConfig:
    @overload
    def __init__(self) -> None: ...
    @overload
    def __init__(self, **kwargs) -> None: ...
    def IsValid(self) -> bool: ...
    def SetStorageConfig(self, arg: str, /) -> None: ...
    @property
    def max_locations_trip(self) -> int: ...
    @max_locations_trip.setter
    def max_locations_trip(self, arg: int, /) -> None: ...
    @property
    def max_locations_viaroute(self) -> int: ...
    @max_locations_viaroute.setter
    def max_locations_viaroute(self, arg: int, /) -> None: ...
    @property
    def max_locations_distance_table(self) -> int: ...
    @max_locations_distance_table.setter
    def max_locations_distance_table(self, arg: int, /) -> None: ...
    @property
    def max_locations_map_matching(self) -> int: ...
    @max_locations_map_matching.setter
    def max_locations_map_matching(self, arg: int, /) -> None: ...
    @property
    def max_radius_map_matching(self) -> float: ...
    @max_radius_map_matching.setter
    def max_radius_map_matching(self, arg: float, /) -> None: ...
    @property
    def max_results_nearest(self) -> int: ...
    @max_results_nearest.setter
    def max_results_nearest(self, arg: int, /) -> None: ...
    @property
    def default_radius(self) -> float: ...
    @default_radius.setter
    def default_radius(self, arg: float, /) -> None: ...
    @property
    def max_alternatives(self) -> int: ...
    @max_alternatives.setter
    def max_alternatives(self, arg: int, /) -> None: ...
    @property
    def use_shared_memory(self) -> bool: ...
    @use_shared_memory.setter
    def use_shared_memory(self, arg: bool, /) -> None: ...
    @property
    def memory_file(self) -> str: ...
    @memory_file.setter
    def memory_file(self, arg: str, /) -> None: ...
    @property
    def use_mmap(self) -> bool: ...
    @use_mmap.setter
    def use_mmap(self, arg: bool, /) -> None: ...
    @property
    def algorithm(self) -> Algorithm: ...
    @algorithm.setter
    def algorithm(self, arg: Algorithm, /) -> None: ...
    @property
    def verbosity(self) -> str: ...
    @verbosity.setter
    def verbosity(self, arg: str, /) -> None: ...
    @property
    def dataset_name(self) -> str: ...
    @dataset_name.setter
    def dataset_name(self, arg: str, /) -> None: ...

class Algorithm:
    def __init__(self, arg: str, /) -> None: ...
    def __repr__(self) -> str: ...

class Approach:
    def __init__(self, arg: str, /) -> None: ...
    def __repr__(self) -> str: ...

class Bearing:
    @overload
    def __init__(self) -> None: ...
    @overload
    def __init__(self, arg: tuple[int, int], /) -> None: ...
    @property
    def bearing(self) -> int: ...
    @bearing.setter
    def bearing(self, arg: int, /) -> None: ...
    @property
    def range(self) -> int: ...
    @range.setter
    def range(self, arg: int, /) -> None: ...
    def IsValid(self) -> bool: ...
    def __eq__(self, arg: Bearing, /) -> bool: ...
    def __ne__(self, arg: Bearing, /) -> bool: ...

class Coordinate:
    @overload
    def __init__(self) -> None: ...
    @overload
    def __init__(self, coordinate: Coordinate) -> None: ...
    @overload
    def __init__(self, arg: tuple[float, float], /) -> None: ...
    @property
    def lon(self) -> float: ...
    @lon.setter
    def lon(self, arg: float, /) -> None: ...
    @property
    def lat(self) -> float: ...
    @lat.setter
    def lat(self, arg: float, /) -> None: ...
    def IsValid(self) -> bool: ...
    def __repr__(self) -> str: ...
    def __eq__(self, arg: Coordinate, /) -> bool: ...
    def __ne__(self, arg: Coordinate, /) -> bool: ...

class Object:
    def __init__(self) -> None: ...
    def __len__(self) -> int: ...
    def __bool__(self) -> bool: ...
    def __repr__(self) -> str: ...
    def __getitem__(self, arg: str, /) -> object: ...
    def __contains__(self, arg: str, /) -> bool: ...
    def __iter__(self) -> Iterator[str]: ...

class Array:
    def __init__(self) -> None: ...
    def __len__(self) -> int: ...
    def __bool__(self) -> bool: ...
    def __repr__(self) -> str: ...
    def __getitem__(self, arg: int, /) -> object: ...
    def __iter__(self) -> Iterator: ...

class String:
    def __init__(self, arg: str, /) -> None: ...

class Number:
    def __init__(self, arg: float, /) -> None: ...

class BaseParameters:
    def __init__(self) -> None:
        r"""
        Instantiates an instance of BaseParameters.

        Note:
                        This is the parent class to many parameter classes, and not intended to be used on its own.

        Args:
                        coordinates (list of floats pairs): Pairs of Longitude and Latitude Coordinates. (default [])
                        hints (list): Hint from previous request to derive position in street network. (default [])
                        radiuses (list of floats): Limits the search to given radius in meters. (default [])
                        bearings (list of int pairs): Limits the search to segments with given bearing in degrees towards true north in clockwise direction. (default [])
                        approaches (list): Keep waypoints on curb side. (default [])
                        generate_hints (bool): Adds a hint to the response which can be used in subsequent requests. (default True)
                        exclude (list of strings): Additive list of classes to avoid. (default [])
                        snapping (string 'default' | 'any'): 'default' snapping avoids is_startpoint edges, 'any' will snap to any edge in the graph. (default \'\')

        Returns:
                        __init__ (osrm.osrm_ext.BaseParameters): A BaseParameter object, that is the parent object to many other Parameter objects.
                        IsValid (bool): A bool value denoting validity of parameter values.

        Attributes:
                        coordinates (list of floats pairs): Pairs of longitude & latitude coordinates.
                        hints (list): Hint from previous request to derive position in street network.
                        radiuses (list of floats): Limits the search to given radius in meters.
                        bearings (list of int pairs): Limits the search to segments with given bearing in degrees towards true north in clockwise direction.
                        approaches (list): Keep waypoints on curb side.
                        exclude (list of strings): Additive list of classes to avoid, order does not matter.
                        format (string): Specifies response type - currently only 'json' is supported.
                        generate_hints (bool): Adds a hint to the response which can be used in subsequent requests.
                        skip_waypoints (list): Removes waypoints from the response.
                        snapping (string): 'default' snapping avoids is_startpoint edges, 'any' will snap to any edge in the graph.
        """

    @property
    def coordinates(self) -> list[Coordinate]: ...
    @coordinates.setter
    def coordinates(self, arg: Sequence[Coordinate], /) -> None: ...
    @property
    def hints(self) -> list: ...
    @hints.setter
    def hints(self, arg: list, /) -> None: ...
    @property
    def radiuses(self) -> list[float | None]: ...
    @radiuses.setter
    def radiuses(self, arg: Sequence[float | None], /) -> None: ...
    @property
    def bearings(self) -> list[Bearing | None]: ...
    @bearings.setter
    def bearings(self, arg: Sequence[Bearing | None], /) -> None: ...
    @property
    def approaches(self) -> list[Approach | None]: ...
    @approaches.setter
    def approaches(self, arg: Sequence[Approach | None], /) -> None: ...
    @property
    def exclude(self) -> list[str]: ...
    @exclude.setter
    def exclude(self, arg: Sequence[str], /) -> None: ...
    @property
    def format(self) -> OutputFormatType | None: ...
    @format.setter
    def format(self, arg: OutputFormatType | None) -> None: ...
    @property
    def generate_hints(self) -> bool: ...
    @generate_hints.setter
    def generate_hints(self, arg: bool, /) -> None: ...
    @property
    def skip_waypoints(self) -> bool: ...
    @skip_waypoints.setter
    def skip_waypoints(self, arg: bool, /) -> None: ...
    @property
    def snapping(self) -> SnappingType: ...
    @snapping.setter
    def snapping(self, arg: SnappingType, /) -> None: ...
    def IsValid(self) -> bool: ...

class SnappingType:
    def __init__(self, arg: str, /) -> None:
        """Instantiates a SnappingType based on provided String value."""

    def __repr__(self) -> str:
        """Return a String based on SnappingType value."""

class OutputFormatType:
    def __init__(self, arg: str, /) -> None:
        """Instantiates a OutputFormatType based on provided String value."""

    def __repr__(self) -> str:
        """Return a String based on OutputFormatType value."""

class NearestParameters(BaseParameters):
    @overload
    def __init__(self) -> None:
        """
        Instantiates an instance of NearestParameters.

        Examples:
                        >>> nearest_params = osrm.NearestParameters(
                                coordinates = [(7.41337, 43.72956)],
                                exclude = ['motorway']
                            )
                        >>> nearest_params.IsValid()
                        True

        Args:
                        BaseParameters (osrm.osrm_ext.BaseParameters): Keyword arguments from parent class.

        Returns:
                        __init__ (osrm.NearestParameters): A NearestParameters object, for usage in osrm.OSRM.Nearest.
                        IsValid (bool): A bool value denoting validity of parameter values.

        Attributes:
                        number_of_results (unsigned int): Number of nearest segments that should be returned.
                        BaseParameters (osrm.osrm_ext.BaseParameters): Attributes from parent class.
        """

    @overload
    def __init__(
        self,
        coordinates: Sequence[Coordinate] = [],
        hints: Sequence[str | None] = [],
        radiuses: Sequence[float | None] = [],
        bearings: Sequence[Bearing | None] = [],
        approaches: Sequence[Approach | None] = [],
        generate_hints: bool = True,
        exclude: Sequence[str] = [],
        snapping: SnappingType = "",
    ) -> None: ...
    @property
    def number_of_results(self) -> int: ...
    @number_of_results.setter
    def number_of_results(self, arg: int, /) -> None: ...
    def IsValid(self) -> bool: ...

class TableParameters(BaseParameters):
    @overload
    def __init__(self) -> None:
        r"""
        Instantiates an instance of TableParameters.

        Examples:
                        >>> table_params = osrm.TableParameters(
                                coordinates = [(7.41337, 43.72956), (7.41546, 43.73077)],
                                sources = [0],
                                destinations = [1],
                                annotations = ['duration'],
                                fallback_speed = 1,
                                fallback_coordinate_type = 'input',
                                scale_factor = 0.9
                            )
                        >>> table_params.IsValid()
                        True

        Args:
                        sources (list of int): Use location with given index as source. (default [])
                        destinations (list of int): Use location with given index as destination. (default [])
                        annotations (list of 'none' | 'duration' | 'distance' | 'all'):                     Returns additional metadata for each coordinate along the route geometry. (default [])
                        fallback_speed (float): If no route found between a source/destination pair, calculate the as-the-crow-flies distance,                     then use this speed to estimate duration. (default INVALID_FALLBACK_SPEED)
                        fallback_coordinate_type (string 'input' | 'snapped'): When using a fallback_speed, use the user-supplied coordinate (input),                     or the snapped location (snapped) for calculating distances. (default \'\')
                        scale_factor: Scales the table duration values by this number (use in conjunction with annotations=durations). (default 1.0)
                        BaseParameters (osrm.osrm_ext.BaseParameters): Keyword arguments from parent class.

        Returns:
                        __init__ (osrm.TableParameters): A TableParameters object, for usage in Table.
                        IsValid (bool): A bool value denoting validity of parameter values.

        Attributes:
                        sources (list of int): Use location with given index as source.
                        destinations (list of int): Use location with given index as destination.
                        annotations (string): Returns additional metadata for each coordinate along the route geometry.
                        fallback_speed (float): If no route found between a source/destination pair, calculate the as-the-crow-flies distance,                     then use this speed to estimate duration.
                        fallback_coordinate_type (string): When using a fallback_speed, use the user-supplied coordinate (input),                     or the snapped location (snapped) for calculating distances.
                        scale_factor: Scales the table duration values by this number (use in conjunction with annotations=durations).
                        BaseParameters (osrm.osrm_ext.BaseParameters): Attributes from parent class.
        """

    @overload
    def __init__(
        self,
        sources: Sequence[int] = [],
        destinations: Sequence[int] = [],
        annotations: Sequence[TableAnnotationsType] = [],
        fallback_speed: float = 3.4028234663852886e38,
        fallback_coordinate_type: TableFallbackCoordinateType = "",
        scale_factor: float = 1.0,
        coordinates: Sequence[Coordinate] = [],
        hints: Sequence[str | None] = [],
        radiuses: Sequence[float | None] = [],
        bearings: Sequence[Bearing | None] = [],
        approaches: Sequence[Approach | None] = [],
        generate_hints: bool = True,
        exclude: Sequence[str] = [],
        snapping: SnappingType = "",
    ) -> None: ...
    @property
    def sources(self) -> list[int]: ...
    @sources.setter
    def sources(self, arg: Sequence[int], /) -> None: ...
    @property
    def destinations(self) -> list[int]: ...
    @destinations.setter
    def destinations(self, arg: Sequence[int], /) -> None: ...
    @property
    def fallback_speed(self) -> float: ...
    @fallback_speed.setter
    def fallback_speed(self, arg: float, /) -> None: ...
    @property
    def fallback_coordinate_type(self) -> TableFallbackCoordinateType: ...
    @fallback_coordinate_type.setter
    def fallback_coordinate_type(self, arg: TableFallbackCoordinateType, /) -> None: ...
    @property
    def annotations(self) -> TableAnnotationsType: ...
    @annotations.setter
    def annotations(self, arg: TableAnnotationsType, /) -> None: ...
    @property
    def scale_factor(self) -> float: ...
    @scale_factor.setter
    def scale_factor(self, arg: float, /) -> None: ...
    def IsValid(self) -> bool: ...

class TableFallbackCoordinateType:
    def __init__(self, arg: str, /) -> None:
        """Instantiates a FallbackCoordinateType based on provided String value."""

    def __repr__(self) -> str:
        """Return a String based on FallbackCoordinateType value."""

class TableAnnotationsType:
    def __init__(self, arg: str, /) -> None:
        """Instantiates a AnnotationsType based on provided String value."""

    def __repr__(self) -> str:
        """Return a String based on AnnotationsType value."""

    def __and__(self, arg: TableAnnotationsType, /) -> bool:
        """Return the bitwise AND result of two AnnotationsTypes."""

    def __or__(self, arg: TableAnnotationsType, /) -> TableAnnotationsType:
        """Return the bitwise OR result of two AnnotationsTypes."""

    def __ior__(self, arg: TableAnnotationsType, /) -> TableAnnotationsType:
        """Add the bitwise OR value of another AnnotationsType."""

class RouteParameters(BaseParameters):
    @overload
    def __init__(self) -> None:
        r"""
        Instantiates an instance of RouteParameters.

        Examples:
                        >>> route_params = osrm.RouteParameters(
                                coordinates = [(7.41337, 43.72956), (7.41546, 43.73077)],
                                steps = True,
                                number_of_alternatives = 3,
                                annotations = ['speed'],
                                geometries = 'polyline',
                                overview = 'simplified',
                                continue_straight = False,
                                waypoints = [0, 1],
                                radiuses = [4.07, 4.07],
                                bearings = [(200, 180), (250, 180)],
                                # approaches = ['unrestricted', 'unrestricted'],
                                generate_hints = False,
                                exclude = ['motorway'],
                                snapping = 'any'
                            )
                        >>> route_params.IsValid()
                        True

        Args:
                        steps (bool): Return route steps for each route leg. (default False)
                        number_of_alternatives (int): Search for n alternative routes. (default 0)
                        annotations (list of 'none' | 'duration' |  'nodes' | 'distance' | 'weight' | 'datasources'                     | 'speed' | 'all'): Returns additional metadata for each coordinate along the route geometry. (default [])
                        geometries (string 'polyline' | 'polyline6' | 'geojson'): Returned route geometry format - influences overview and per step. (default )
                        overview (string 'simplified' | 'full' | 'false'): Add overview geometry either full, simplified. (default \'\')
                        continue_straight (bool): Forces the route to keep going straight at waypoints, constraining u-turns. (default {})
                        waypoints (list of int): Treats input coordinates indicated by given indices as waypoints in returned Match object. (default [])
                        BaseParameters (osrm.osrm_ext.BaseParameters): Keyword arguments from parent class.

        Returns:
                        __init__ (osrm.RouteParameters): A RouteParameters object, for usage in Route.
                        IsValid (bool): A bool value denoting validity of parameter values.

        Attributes:
                        steps (bool): Return route steps for each route leg.
                        alternatives (bool): Search for alternative routes.
                        number_of_alternatives (int): Search for n alternative routes.
                        annotations_type (string): Returns additional metadata for each coordinate along the route geometry.
                        geometries (string): Returned route geometry format - influences overview and per step.
                        overview (string): Add overview geometry either full, simplified.
                        continue_straight (bool): Forces the route to keep going straight at waypoints, constraining u-turns.
                        BaseParameters (osrm.osrm_ext.BaseParameters): Attributes from parent class.
        """

    @overload
    def __init__(
        self,
        steps: bool = False,
        number_of_alternatives: int = 0,
        annotations: Sequence[RouteAnnotationsType] = [],
        geometries: RouteGeometriesType = "",
        overview: RouteOverviewType = "",
        continue_straight: bool | None = None,
        waypoints: Sequence[int] = [],
        coordinates: Sequence[Coordinate] = [],
        hints: Sequence[str | None] = [],
        radiuses: Sequence[float | None] = [],
        bearings: Sequence[Bearing | None] = [],
        approaches: Sequence[Approach | None] = [],
        generate_hints: bool = True,
        exclude: Sequence[str] = [],
        snapping: SnappingType = "",
    ) -> None: ...
    @property
    def steps(self) -> bool: ...
    @steps.setter
    def steps(self, arg: bool, /) -> None: ...
    @property
    def alternatives(self) -> bool: ...
    @alternatives.setter
    def alternatives(self, arg: bool, /) -> None: ...
    @property
    def number_of_alternatives(self) -> int: ...
    @number_of_alternatives.setter
    def number_of_alternatives(self, arg: int, /) -> None: ...
    @property
    def annotations_type(self) -> RouteAnnotationsType: ...
    @annotations_type.setter
    def annotations_type(self, arg: RouteAnnotationsType, /) -> None: ...
    @property
    def geometries(self) -> RouteGeometriesType: ...
    @geometries.setter
    def geometries(self, arg: RouteGeometriesType, /) -> None: ...
    @property
    def overview(self) -> RouteOverviewType: ...
    @overview.setter
    def overview(self, arg: RouteOverviewType, /) -> None: ...
    @property
    def continue_straight(self) -> bool | None: ...
    @continue_straight.setter
    def continue_straight(self, arg: bool | None) -> None: ...
    def IsValid(self) -> bool: ...

class RouteGeometriesType:
    def __init__(self, arg: str, /) -> None:
        """Instantiates a GeometriesType based on provided String value."""

    def __repr__(self) -> str:
        """Return a String based on GeometriesType value."""

class RouteOverviewType:
    def __init__(self, arg: str, /) -> None:
        """Instantiates a OverviewType based on provided String value."""

    def __repr__(self) -> str:
        """Return a String based on OverviewType value."""

class RouteAnnotationsType:
    def __init__(self, arg: str, /) -> None:
        """Instantiates a AnnotationsType based on provided String value."""

    def __repr__(self) -> str:
        """Return a String based on AnnotationsType value."""

    def __and__(self, arg: RouteAnnotationsType, /) -> bool:
        """Return the bitwise AND result of two AnnotationsTypes."""

    def __or__(self, arg: RouteAnnotationsType, /) -> RouteAnnotationsType:
        """Return the bitwise OR result of two AnnotationsTypes."""

    def __ior__(self, arg: RouteAnnotationsType, /) -> RouteAnnotationsType:
        """Add the bitwise OR value of another AnnotationsType."""

class MatchParameters(RouteParameters):
    @overload
    def __init__(self) -> None:
        """
        Instantiates an instance of MatchParameters.

        Examples:
                        >>> match_params = osrm.MatchParameters(
                                coordinates = [(7.41337, 43.72956), (7.41546, 43.73077), (7.41862, 43.73216)],
                                timestamps = [1424684612, 1424684616, 1424684620],
                                gaps = 'split',
                                tidy = True
                            )
                        >>> match_params.IsValid()
                        True

        Args:
                        timestamps (list of unsigned int): Timestamps for the input locations in seconds since UNIX epoch. (default [])
                        gaps (list of 'split' | 'ignore'): Allows the input track splitting based on huge timestamp gaps between points. (default [])
                        tidy (bool): Allows the input track modification to obtain better matching quality for noisy tracks. (default False)
                        RouteParameters (osrm.RouteParameters): Keyword arguments from parent class.

        Returns:
                        __init__ (osrm.MatchParameters): A MatchParameters object, for usage in Match.
                        IsValid (bool): A bool value denoting validity of parameter values.

        Attributes:
                        timestamps (list of unsigned int): Timestamps for the input locations in seconds since UNIX epoch.
                        gaps (string): Allows the input track splitting based on huge timestamp gaps between points.
                        tidy (bool): Allows the input track modification to obtain better matching quality for noisy tracks.
                        RouteParameters (osrm.RouteParameters): Attributes from parent class.
        """

    @overload
    def __init__(
        self,
        timestamps: Sequence[int] = [],
        gaps: MatchGapsType = "",
        tidy: bool = False,
        steps: bool = False,
        number_of_alternatives: int = 0,
        annotations: Sequence[RouteAnnotationsType] = [],
        geometries: RouteGeometriesType = "",
        overview: RouteOverviewType = "",
        continue_straight: bool | None = None,
        waypoints: Sequence[int] = [],
        coordinates: Sequence[Coordinate] = [],
        hints: Sequence[str | None] = [],
        radiuses: Sequence[float | None] = [],
        bearings: Sequence[Bearing | None] = [],
        approaches: Sequence[Approach | None] = [],
        generate_hints: bool = True,
        exclude: Sequence[str] = [],
        snapping: SnappingType = "",
    ) -> None: ...
    @property
    def timestamps(self) -> list[int]: ...
    @timestamps.setter
    def timestamps(self, arg: Sequence[int], /) -> None: ...
    @property
    def gaps(self) -> MatchGapsType: ...
    @gaps.setter
    def gaps(self, arg: MatchGapsType, /) -> None: ...
    @property
    def tidy(self) -> bool: ...
    @tidy.setter
    def tidy(self, arg: bool, /) -> None: ...
    def IsValid(self) -> bool: ...

class MatchGapsType:
    def __init__(self, arg: str, /) -> None:
        """Instantiates a GapsType based on provided String value."""

    def __repr__(self) -> str:
        """Return a String based on GapsType value."""

class TripParameters(RouteParameters):
    @overload
    def __init__(self) -> None:
        r"""
        Instantiates an instance of TripParameters.

        Examples:
                        >>> trip_params = osrm.TripParameters(
                                coordinates = [(7.41337, 43.72956), (7.41546, 43.73077)],
                                source = 'any',
                                destination = 'last',
                                roundtrip = False
                            )
                        >>> trip_params.IsValid()
                        True

        Args:
                        source (string 'any' | 'first'): Returned route starts at 'any' or 'first' coordinate. (default \'\')
                        destination (string 'any' | 'last'): Returned route ends at 'any' or 'last' coordinate. (default \'\')
                        roundtrip (bool): Returned route is a roundtrip (route returns to first location). (default True)
                        RouteParameters (osrm.RouteParameters): Keyword arguments from parent class.

        Returns:
                        __init__ (osrm.TripParameters): A TripParameters object, for usage in Trip.
                        IsValid (bool): A bool value denoting validity of parameter values.

        Attributes:
                        source (string): Returned route starts at 'any' or 'first' coordinate.
                        destination (string): Returned route ends at 'any' or 'last' coordinate.
                        roundtrip (bool): Returned route is a roundtrip (route returns to first location).
                        RouteParameters (osrm.RouteParameters): Attributes from parent class.
        """

    @overload
    def __init__(
        self,
        source: TripSourceType = "",
        destination: TripDestinationType = "",
        roundtrip: bool = True,
        steps: bool = False,
        alternatives: int = 0,
        annotations: Sequence[RouteAnnotationsType] = [],
        geometries: RouteGeometriesType = "",
        overview: RouteOverviewType = "",
        continue_straight: bool | None = None,
        waypoints: Sequence[int] = [],
        coordinates: Sequence[Coordinate] = [],
        hints: Sequence[str | None] = [],
        radiuses: Sequence[float | None] = [],
        bearings: Sequence[Bearing | None] = [],
        approaches: Sequence[Approach | None] = [],
        generate_hints: bool = True,
        exclude: Sequence[str] = [],
        snapping: SnappingType = "",
    ) -> None: ...
    @property
    def source(self) -> TripSourceType: ...
    @source.setter
    def source(self, arg: TripSourceType, /) -> None: ...
    @property
    def destination(self) -> TripDestinationType: ...
    @destination.setter
    def destination(self, arg: TripDestinationType, /) -> None: ...
    @property
    def roundtrip(self) -> bool: ...
    @roundtrip.setter
    def roundtrip(self, arg: bool, /) -> None: ...
    def IsValid(self) -> bool: ...

class TripSourceType:
    def __init__(self, arg: str, /) -> None:
        """Instantiates a SourceType based on provided String value."""

    def __repr__(self) -> str:
        """Return a String based on SourceType value."""

class TripDestinationType:
    def __init__(self, arg: str, /) -> None:
        """Instantiates a DestinationType based on provided String value."""

    def __repr__(self) -> str:
        """Return a String based on DestinationType value."""

class TileParameters:
    @overload
    def __init__(self) -> None:
        """
        Instantiates an instance of TileParameters.

        Examples:
                        >>> tile_params = osrm.TileParameters([17059, 11948, 15])
                        >>> tile_params = osrm.TileParameters(
                                x = 17059,
                                y = 11948,
                                z = 15
                            )
                        >>> tile_params.IsValid()
                        True

        Args:
                        list (list of int): Instantiates an instance of TileParameters using an array [x, y, z].
                        x (int): x value.
                        y (int): y value.
                        z (int): z value.

        Returns:
                        __init__ (osrm.TileParameters): A TileParameters object, for usage in Tile.
                        IsValid (bool): A bool value denoting validity of parameter values.

        Attributes:
                        x (int): x value.
                        y (int): y value.
                        z (int): z value.
        """

    @overload
    def __init__(self, arg0: int, arg1: int, arg2: int, /) -> None: ...
    @overload
    def __init__(self, arg: Sequence[int], /) -> None: ...
    @property
    def x(self) -> int: ...
    @x.setter
    def x(self, arg: int, /) -> None: ...
    @property
    def y(self) -> int: ...
    @y.setter
    def y(self, arg: int, /) -> None: ...
    @property
    def z(self) -> int: ...
    @z.setter
    def z(self, arg: int, /) -> None: ...
    def IsValid(self) -> bool: ...

class OSRM:
    @overload
    def __init__(self, arg: EngineConfig, /) -> None:
        """
        Instantiates an instance of OSRM.

        Examples:
                        >>> import osrm
                        >>> osrm_py = osrm.OSRM('.tests/test_data/ch/monaco.osrm')
                        >>> osrm_py = osrm.OSRM(
                                algorithm = 'CH',
                                storage_config = '.tests/test_data/ch/monaco.osrm',
                                max_locations_trip = 3,
                                max_locations_viaroute = 3,
                                max_locations_distance_table = 3,
                                max_locations_map_matching = 3,
                                max_results_nearest = 1,
                                max_alternatives = 1,
                                default_radius = 'unlimited'
                            )

        Args:
                        storage_config (string): File path string to storage config.
                        EngineConfig (osrm.osrm_ext.EngineConfig): Keyword arguments from the EngineConfig class.

        Returns:
                        __init__ (osrm.OSRM): A OSRM object.

        Raises:
                        RuntimeError: On invalid OSRM EngineConfig parameters.
        """

    @overload
    def __init__(self, arg: str, /) -> None: ...
    @overload
    def __init__(self, **kwargs) -> None: ...
    def Match(self, arg: MatchParameters, /) -> Object:
        """
        Matches/snaps given GPS points to the road network in the most plausible way.

        Examples:
                        >>> res = osrm_py.Match(match_params)

        Args:
                        match_params (osrm.MatchParameters): MatchParameters Object.

        Returns:
                        (json): [A Match JSON Response](https://project-osrm.org/docs/v5.24.0/api/#match-service).

        Raises:
                        RuntimeError: On invalid MatchParameters.
        """

    def Nearest(self, arg: NearestParameters, /) -> Object:
        """
        Snaps a coordinate to the street network and returns the nearest matches.

        Examples:
                        >>> res = osrm_py.Nearest(nearest_params)

        Args:
                        nearest_params (osrm.NearestParameters): NearestParameters Object.

        Returns:
                        (json): [A Nearest JSON Response](https://project-osrm.org/docs/v5.24.0/api/#nearest-service).

        Raises:
                        RuntimeError: On invalid NearestParameters.
        """

    def Route(self, arg: RouteParameters, /) -> Object:
        """
        Finds the fastest route between coordinates in the supplied order.

        Examples:
                        >>> res = osrm_py.Route(route_params)

        Args:
                        route_params (osrm.RouteParameters): RouteParameters Object.

        Returns:
                        (json): [A Route JSON Response](https://project-osrm.org/docs/v5.24.0/api/#route-service).

        Raises:
                        RuntimeError: On invalid RouteParameters.
        """

    def Table(self, arg: TableParameters, /) -> Object:
        """
        Computes the duration of the fastest route between all pairs of supplied coordinates.

        Examples:
                        >>> res = osrm_py.Table(table_params)

        Args:
                        table_params (osrm.TableParameters): TableParameters Object.

        Returns:
                        (json): [A Table JSON Response](https://project-osrm.org/docs/v5.24.0/api/#table-service).

        Raises:
                        RuntimeError: On invalid TableParameters.
        """

    def Tile(self, arg: TileParameters, /) -> object:
        """
        Computes the duration of the fastest route between all pairs of supplied coordinates.

        Examples:
                        >>> res = osrm_py.Tile(tile_params)

        Args:
                        tile_params (osrm.TileParameters): TileParameters Object.

        Returns:
                        (json): [A Tile JSON Response](https://project-osrm.org/docs/v5.24.0/api/#tile-service).

        Raises:
                        RuntimeError: On invalid TileParameters.
        """

    def Trip(self, arg: TripParameters, /) -> Object:
        """
        Solves the Traveling Salesman Problem using a greedy heuristic (farthest-insertion algorithm).

        Examples:
                        >>> res = osrm_py.Trip(trip_params)

        Args:
                        trip_params (osrm.TripParameters): TripParameters Object.

        Returns:
                        (json): [A Trip JSON Response](https://project-osrm.org/docs/v5.24.0/api/#trip-service).

        Raises:
                        RuntimeError: On invalid TripParameters.
        """
