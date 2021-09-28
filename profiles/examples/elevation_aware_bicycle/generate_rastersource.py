import numpy as np

# SRTM Elevation Raster Tiles: http://srtm.csi.cgiar.org/srtmdata/
# This example fuses the two srtm tiles covering Switzerland.
# Change this python script as needed by arranging any number of tiles with `np.hstack` and `np.vstack`.
srtm_39_03 = np.loadtxt("srtm_39_03.asc", skiprows=6, dtype=np.int)
srtm_38_03 = np.loadtxt("srtm_38_03.asc", skiprows=6, dtype=np.int)
rastersource = np.hstack((srtm_38_03, srtm_39_03))
np.savetxt("rastersource.asc", rastersource, fmt="%d")