import pytest
import osrm
import constants

data_path = constants.data_path
mld_data_path = constants.mld_data_path
three_test_coordinates = constants.three_test_coordinates
two_test_coordinates = constants.two_test_coordinates


class TestNearest:
    osrm_py = osrm.OSRM(storage_config=mld_data_path, algorithm="MLD", use_shared_memory=False)

    def test_nearest(self):
        nearest_params = osrm.NearestParameters(
            coordinates=[two_test_coordinates[0]], exclude=["motorway"]
        )
        res = self.osrm_py.Nearest(nearest_params)
        assert len(res["waypoints"]) == 1

    def test_nearest_numberofresults(self):
        osrm_py = osrm.OSRM(storage_config=data_path, use_shared_memory=False)
        nearest_params = osrm.NearestParameters(
            coordinates=[three_test_coordinates[0]], number_of_results=3
        )
        res = osrm_py.Nearest(nearest_params)
        assert len(res["waypoints"]) == 3

    def test_nearest_validbearings(self):
        nearest_params = osrm.NearestParameters(
            coordinates=[two_test_coordinates[0]], bearings=[(200, 180)]
        )
        res = self.osrm_py.Nearest(nearest_params)
        assert res["waypoints"]

        nearest_params.bearings = [None]
        res = self.osrm_py.Nearest(nearest_params)
        assert res["waypoints"]

    def test_nearest_validradius(self):
        nearest_params = osrm.NearestParameters(coordinates=[two_test_coordinates[0]], radiuses=[100])
        res = self.osrm_py.Nearest(nearest_params)
        assert res["waypoints"]

        nearest_params.radiuses = [None]
        res = self.osrm_py.Nearest(nearest_params)
        assert res["waypoints"]

    def test_nearest_validapproaches(self):
        # Valid approach strings are "curb", "unrestricted", "opposite", or None.
        for approach in ["curb", "unrestricted", "opposite", None]:
            nearest_params = osrm.NearestParameters(
                coordinates=[two_test_coordinates[0]], approaches=[approach]
            )
            res = self.osrm_py.Nearest(nearest_params)
            assert res["waypoints"]

    def test_nearest_invalidapproach(self):
        with pytest.raises(Exception):
            osrm.NearestParameters(coordinates=[two_test_coordinates[0]], approaches=["sideways"])

    def test_nearest_badparams(self):
        with pytest.raises(Exception):
            nearest_params = osrm.NearestParameters(coordinates=[])
            self.osrm_py.Nearest(nearest_params)

        with pytest.raises(Exception):
            nearest_params = osrm.NearestParameters(
                coordinates=[two_test_coordinates[0]], number_of_results=0
            )
            self.osrm_py.Nearest(nearest_params)
