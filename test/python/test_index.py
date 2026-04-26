import pytest
import osrm
import constants

data_path = constants.data_path
mld_data_path = constants.mld_data_path
test_memory_path = constants.test_memory_path


class TestIndex:
    def test_default_noparam(self):
        osrm.OSRM()

    def test_throwifnecessarynotexist(self):
        with pytest.raises(RuntimeError) as ex:
            osrm.OSRM("missing.osrm")
        assert str(ex.value) == "Required files are missing"

        with pytest.raises(RuntimeError) as ex:
            osrm.OSRM(storage_config="missing.osrm", algorithm="MLD")
        assert str(ex.value) == "Config Parameters are Invalid"

    def test_shmemarg(self):
        osrm.OSRM(storage_config=data_path, use_shared_memory=False)

    def test_memfile(self):
        # memory_file is deprecated in OSRM v6 (equivalent to use_mmap=True); no datastore needed
        osrm.OSRM(
            storage_config=data_path,
            use_shared_memory=False,
            memory_file=test_memory_path,
        )

    def test_shmemfalsenopath(self):
        with pytest.raises(RuntimeError) as ex:
            osrm.OSRM(use_shared_memory=False)
        assert str(ex.value) == "Config Parameters are Invalid"

    def test_nonstringarg(self):
        with pytest.raises(TypeError):
            osrm.OSRM(True)

    def test_unknownalgo(self):
        with pytest.raises(ValueError) as ex:
            osrm.OSRM(algorithm="Foo")
        ex_str = str(ex.value)
        assert "Invalid Algorithm: 'Foo'" in ex_str
        assert "'CH'" in ex_str
        assert "'MLD'" in ex_str

    def test_invalidalgo(self):
        with pytest.raises(RuntimeError) as ex:
            osrm.OSRM(algorithm=3)
        assert str(ex.value) == "Invalid type passed for argument: algorithm"

    def test_validalgos(self):
        osrm.OSRM(algorithm="MLD", storage_config=mld_data_path, use_shared_memory=False)

        osrm.OSRM(algorithm="CH", storage_config=data_path, use_shared_memory=False)

    def test_datamatchalgo(self):
        with pytest.raises(RuntimeError) as ex:
            osrm.OSRM(algorithm="CH", storage_config=mld_data_path, use_shared_memory=False)
        assert "Could not find any metrics for CH in the data." in str(ex.value)

        with pytest.raises(RuntimeError) as ex:
            osrm.OSRM(algorithm="MLD", storage_config=data_path, use_shared_memory=False)
        assert "Could not find any metrics for MLD in the data." in str(ex.value)

    def test_datasetnamenotstring(self):
        with pytest.raises(RuntimeError) as ex:
            osrm.OSRM(dataset_name=1337)
        assert str(ex.value) == "Invalid type passed for argument: dataset_name"

        # osrm.OSRM(dataset_name = "") requires osrm-datastore (uses shared memory)

        with pytest.raises(RuntimeError) as ex:
            osrm.OSRM(dataset_name="unsued_name___", use_shared_memory=True)
        assert "shared memory" in str(ex.value).lower() or "osrm-datastore" in str(ex.value)

    def test_defaultradius(self):
        osrm.OSRM(storage_config=data_path, use_shared_memory=False, default_radius=1)

    def test_unlimitedradius(self):
        osrm.OSRM(
            storage_config=data_path,
            use_shared_memory=False,
            default_radius="unlimited",
        )

    def test_customlimits(self):
        osrm.OSRM(
            storage_config=mld_data_path,
            algorithm="MLD",
            use_shared_memory=False,
            max_locations_trip=3,
            max_locations_viaroute=3,
            max_locations_distance_table=3,
            max_locations_map_matching=3,
            max_results_nearest=1,
            max_alternatives=1,
            default_radius=1,
        )

    def test_invalidlimits(self):
        with pytest.raises(RuntimeError) as ex:
            osrm.OSRM(
                storage_config=mld_data_path,
                algorithm="MLD",
                max_locations_trip=1,
                max_locations_viaroute=True,
                max_locations_distance_table=False,
                max_locations_map_matching="a lot",
                max_results_nearest=None,
                max_alternatives="10",
                default_radius="10",
            )
        assert "Invalid type passed for argument" in str(ex.value)
