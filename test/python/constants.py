from pathlib import Path

path = Path(__file__).parent.parent.joinpath("data")

# Constants and fixtures for Python tests on our Monaco dataset.

# Somewhere in Monaco
# http://www.openstreetmap.org/#map=18/43.73185/7.41772
three_test_coordinates = [(7.41337, 43.72956), (7.41546, 43.73077), (7.41862, 43.73216)]

two_test_coordinates = three_test_coordinates[0:2]

test_tile = {"at": [17059, 11948, 15], "size": 159125}

data_path = str(path.joinpath("ch", "monaco.osrm").absolute())
mld_data_path = str(path.joinpath("mld", "monaco.osrm").absolute())
corech_data_path = str(path.joinpath("corech", "monaco.osrm").absolute())
test_memory_path = str(path.joinpath("test_memory").absolute())
