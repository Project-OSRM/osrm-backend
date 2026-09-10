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
            contours_seconds=[300.0, 600.0],
            direction="inbound",
            polygons=False,
            generalize=25.0,
            denoise=0.25,
        )
        assert params.IsValid()
        assert params.contours_seconds == [300.0, 600.0]
        assert not hasattr(params, "contours")
        assert repr(params.direction) == "inbound"
        assert not params.polygons
        assert params.generalize == 25.0
        assert params.denoise == 0.25

        params.generalize = 0.0
        assert params.IsValid()
        assert params.generalize == 0.0

        params.generalize = None
        assert params.IsValid()
        assert params.generalize is None

        params.denoise = 0.0
        assert params.IsValid()
        assert params.denoise == 0.0

        params.denoise = None
        assert params.IsValid()
        assert params.denoise is None

    def test_returns_a_geojson_feature_collection(self):
        params = osrm.IsochroneParameters(
            coordinates=[one_test_coordinate],
            contours_seconds=[300.0, 600.0],
            polygons=False,
            generalize=25.0,
            denoise=0.1,
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
            and feature["properties"]["effective_contour_seconds"]
            <= feature["properties"]["contour_seconds"]
            for feature in result["features"]
        )

    def test_reports_the_decisecond_duration_contour_that_was_evaluated(self):
        params = osrm.IsochroneParameters(
            coordinates=[one_test_coordinate],
            contours_seconds=[0.15],
        )
        result = self.osrm_py.Isochrone(params)

        assert result["code"] == "Ok"
        assert result["features"][0]["properties"]["effective_contour_seconds"] == 0.1

    def test_rejects_invalid_parameters_before_querying(self):
        params = osrm.IsochroneParameters(coordinates=[one_test_coordinate])
        with pytest.raises(RuntimeError, match="Invalid Isochrone Parameters"):
            self.osrm_py.Isochrone(params)

    def test_rejects_legacy_contours_argument(self):
        with pytest.raises(TypeError):
            osrm.IsochroneParameters(
                coordinates=[one_test_coordinate],
                contours=[300.0],
            )

    @pytest.mark.parametrize("generalize", [-1.0, float("inf"), float("nan")])
    def test_rejects_invalid_generalize_before_querying(self, generalize):
        params = osrm.IsochroneParameters(
            coordinates=[one_test_coordinate],
            contours_seconds=[300.0],
            generalize=generalize,
        )
        with pytest.raises(RuntimeError, match="Invalid Isochrone Parameters"):
            self.osrm_py.Isochrone(params)

    def test_rejects_non_numeric_generalize(self):
        with pytest.raises(TypeError):
            osrm.IsochroneParameters(
                coordinates=[one_test_coordinate],
                contours_seconds=[300.0],
                generalize="25",
            )

    @pytest.mark.parametrize("denoise", [-0.1, 1.1, float("inf"), float("nan")])
    def test_rejects_invalid_denoise_before_querying(self, denoise):
        params = osrm.IsochroneParameters(
            coordinates=[one_test_coordinate],
            contours_seconds=[300.0],
            denoise=denoise,
        )
        with pytest.raises(RuntimeError, match="Invalid Isochrone Parameters"):
            self.osrm_py.Isochrone(params)

    def test_rejects_non_numeric_denoise(self):
        with pytest.raises(TypeError):
            osrm.IsochroneParameters(
                coordinates=[one_test_coordinate],
                contours_seconds=[300.0],
                denoise="0.25",
            )
