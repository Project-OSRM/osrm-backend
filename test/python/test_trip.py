import osrm
import constants

data_path = constants.data_path
mld_data_path = constants.mld_data_path
three_test_coordinates = constants.three_test_coordinates
two_test_coordinates = constants.two_test_coordinates


class TestTrip:
    osrm_py = osrm.OSRM(storage_config=data_path, use_shared_memory=False)

    def test_trip_manylocations(self):
        trip_parameters = osrm.TripParameters(coordinates=three_test_coordinates[0:5])
        res = self.osrm_py.Trip(trip_parameters)
        for trip in res["trips"]:
            assert trip["geometry"]

    def test_trip_invalidargs(self):
        # Previously used osrm.OSRM() (shared memory); use file mode here
        trip_parameters = osrm.TripParameters(coordinates=two_test_coordinates)
        res = self.osrm_py.Trip(trip_parameters)
        for trip in res["trips"]:
            assert trip["geometry"]

    def test_trip_geometrycompression(self):
        # Previously used osrm.OSRM() (shared memory); use file mode here
        trip_parameters = osrm.TripParameters(
            coordinates=[three_test_coordinates[0], three_test_coordinates[1]]
        )
        res = self.osrm_py.Trip(trip_parameters)
        for trip in res["trips"]:
            assert isinstance(trip["geometry"], str)

    def test_trip_nogeometrycompression(self):
        # Previously used osrm.OSRM() (shared memory); use file mode here
        trip_parameters = osrm.TripParameters(coordinates=two_test_coordinates, geometries="geojson")
        res = self.osrm_py.Trip(trip_parameters)
        for trip in res["trips"]:
            assert isinstance(trip["geometry"]["coordinates"], osrm.Array)

    def test_trip_speedannotations(self):
        # Previously used osrm.OSRM() (shared memory); use file mode here
        trip_parameters = osrm.TripParameters(
            coordinates=two_test_coordinates,
            steps=True,
            annotations=["speed"],
            overview="false",
        )
        res = self.osrm_py.Trip(trip_parameters)
        for trip in res["trips"]:
            assert trip
            for l in trip["legs"]:
                assert len(l["steps"]) > 0 and l["annotation"] and l["annotation"]["speed"]
                assert (
                    "weight" not in l["annotation"]
                    and "datasources" not in l["annotation"]
                    and "duration" not in l["annotation"]
                    and "distance" not in l["annotation"]
                    and "nodes" not in l["annotation"]
                )
                assert "geometry" not in l

    def test_trip_severalannotations(self):
        trip_params = osrm.TripParameters(
            coordinates=two_test_coordinates,
            steps=True,
            annotations=["duration", "distance", "nodes"],
            overview="false",
        )
        res = self.osrm_py.Trip(trip_params)
        assert len(res["trips"]) == 1
        for trip in res["trips"]:
            assert trip
            for l in trip["legs"]:
                assert len(l["steps"]) > 0
                assert (
                    l["annotation"]
                    and l["annotation"]["distance"]
                    and l["annotation"]["duration"]
                    and l["annotation"]["nodes"]
                )
                assert (
                    "weight" not in l["annotation"]
                    and "datasources" not in l["annotation"]
                    and "speed" not in l["annotation"]
                )
                assert "geometry" not in l

    def test_trip_options(self):
        trip_params = osrm.TripParameters(
            coordinates=two_test_coordinates,
            steps=True,
            annotations=["all"],
            overview="false",
        )
        res = self.osrm_py.Trip(trip_params)
        assert len(res["trips"]) == 1
        for trip in res["trips"]:
            assert trip
            for l in trip["legs"]:
                assert len(l["steps"]) > 0 and l["annotation"]
            assert "geometry" not in trip

    def test_trip_nomotorways(self):
        engine = osrm.OSRM(algorithm="MLD", storage_config=mld_data_path, use_shared_memory=False)
        trip_params = osrm.TripParameters(coordinates=two_test_coordinates, exclude=["motorway"])
        res = engine.Trip(trip_params)
        assert len(res["waypoints"]) == 2
        assert len(res["trips"]) == 1
