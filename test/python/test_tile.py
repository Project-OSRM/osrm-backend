import pytest
import osrm
import constants

data_path = constants.data_path
mld_data_path = constants.mld_data_path
test_tile = constants.test_tile


class TestTile:
    osrm_py = osrm.OSRM(storage_config=data_path, use_shared_memory=False)

    def test_tile(self):
        tile_params = osrm.TileParameters(test_tile["at"])
        res = self.osrm_py.Tile(tile_params)
        assert len(res) == test_tile["size"]

    def test_tile_preconditions(self):
        with pytest.raises(Exception):
            # Must be an array
            tile_params = osrm.TileParameters(17059, 11948, -15)
        with pytest.raises(Exception):
            # Must be unsigned
            tile_params = osrm.TileParameters([17059, 11948, -15])
        tile_params = osrm.TileParameters([17059, 11948, 15])
        self.osrm_py.Tile(tile_params)
