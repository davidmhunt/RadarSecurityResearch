import numpy as np

#dtype = np.complex64
dtype = np.complex64

try:
    with open("/home/david/Documents/RadarSecurityResearch/FMCW_radar/sample_chirps/chirp_full.bin") as f:
    #with open("/Users/David/Documents/RadarSecurityResearch/FMCW_radar/sample_chirps/chirp_full.bin") as f:
        numpy_data = np.fromfile(f,dtype)
        size = numpy_data.size
    print(numpy_data[531])
    print(size)
except IOError:
    print("Error While Opening the file")