import constants
import pytest

import osrm

mld_data_path = constants.mld_data_path
one_test_coordinate = constants.two_test_coordinates[0]


class TestIsochrone:
    osrm_py = osrm.OSRM(
        storage_config=mld_data_path,
        algorithm="MLD",
        use_shared_memory=False,
    )

    def test_parameters(self):
        params = osrm.IsochroneParameters(
            coordinates=[one_test_coordinate],
            contours=[300.0, 600.0],
            direction="inbound",
            polygons=False,
        )
        assert params.IsValid()
        assert params.contours == [300.0, 600.0]
        assert repr(params.direction) == "inbound"
        assert not params.polygons

    def test_returns_a_geojson_feature_collection(self):
        params = osrm.IsochroneParameters(
            coordinates=[one_test_coordinate],
            contours=[300.0, 600.0],
            polygons=False,
        )
        result = self.osrm_py.Isochrone(params)

        assert result["code"] == "Ok"
        assert result["type"] == "FeatureCollection"
        assert "weight_name" not in result
        assert len(result["features"]) == 2
        assert all(
            feature["type"] == "Feature"
            and feature["geometry"]["type"] == "MultiLineString"
            and isinstance(feature["geometry"]["coordinates"], osrm.Array)
            and feature["properties"]["effective_contour"] <= feature["properties"]["contour"]
            for feature in result["features"]
        )

    def test_reports_the_decisecond_duration_contour_that_was_evaluated(self):
        params = osrm.IsochroneParameters(
            coordinates=[one_test_coordinate],
            contours=[0.15],
        )
        result = self.osrm_py.Isochrone(params)

        assert result["code"] == "Ok"
        assert result["features"][0]["properties"]["effective_contour"] == 0.1

    def test_rejects_invalid_parameters_before_querying(self):
        params = osrm.IsochroneParameters(coordinates=[one_test_coordinate])
        with pytest.raises(RuntimeError, match="Invalid Isochrone Parameters"):
            self.osrm_py.Isochrone(params)
