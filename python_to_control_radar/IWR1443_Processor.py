import numpy as np
import matplotlib.pyplot as plt
from matplotlib import style
from matplotlib.pyplot import plot, show

# Processing class -> takes in a packet and configuration data, returns detected objects, x y z coordinates

class IWR1443_Processor:
    
    def processData(inputPacket, configParameters):
        # Constants
        MMWDEMO_UART_MSG_DETECTED_POINTS = 1;
    
        # word array to convert 4 bytes to a 32 bit number
        word = [1, 2**8, 2**16, 2**24]
        
        # Initialize the pointer index
        idX = 0
        
        # Read the header
        magicNumber = inputPacket[idX:idX+8]
        idX += 8
        version = format(np.matmul(inputPacket[idX:idX+4],word),'x')
        idX += 4
        totalPacketLen = np.matmul(inputPacket[idX:idX+4],word)
        idX += 4
        platform = format(np.matmul(inputPacket[idX:idX+4],word),'x')
        idX += 4
        frameNumber = np.matmul(inputPacket[idX:idX+4],word)
        idX += 4
        timeCpuCycles = np.matmul(inputPacket[idX:idX+4],word)
        idX += 4
        numDetectedObj = np.matmul(inputPacket[idX:idX+4],word)
        idX += 4
        numTLVs = np.matmul(inputPacket[idX:idX+4],word)
        idX += 4
        
        # Read the TLV messages
        for tlvIdx in range(numTLVs):
            
            # word array to convert 4 bytes to a 32 bit number
            word = [1, 2**8, 2**16, 2**24]

            # Check the header of the TLV message
            tlv_type = np.matmul(inputPacket[idX:idX+4],word)
            idX += 4
            tlv_length = np.matmul(inputPacket[idX:idX+4],word)
            idX += 4
            print(tlv_length)
            
            # Read the data depending on the TLV message
            word = [1, 2**8]
            if tlv_type == MMWDEMO_UART_MSG_DETECTED_POINTS:

                # word array to convert 4 bytes to a 16 bit number
                tlv_numObj = np.matmul(inputPacket[idX:idX+2],word)
                idX += 2
                tlv_xyzQFormat = 2**np.matmul(inputPacket[idX:idX+2],word)
                idX += 2
                
                # Initialize the arrays
                rangeIdx = np.zeros(tlv_numObj,dtype = 'int16')
                dopplerIdx = np.zeros(tlv_numObj,dtype = 'int16')
                peakVal = np.zeros(tlv_numObj,dtype = 'int16')
                x = np.zeros(tlv_numObj,dtype = 'int16')
                y = np.zeros(tlv_numObj,dtype = 'int16')
                z = np.zeros(tlv_numObj,dtype = 'int16')
                
                for objectNum in range(tlv_numObj):
                    
                    # Read the data for each object
                    rangeIdx[objectNum] =  np.matmul(inputPacket[idX:idX+2],word)
                    idX += 2
                    dopplerIdx[objectNum] = np.matmul(inputPacket[idX:idX+2],word)
                    idX += 2
                    peakVal[objectNum] = np.matmul(inputPacket[idX:idX+2],word)
                    idX += 2
                    x[objectNum] = np.matmul(inputPacket[idX:idX+2],word)
                    idX += 2
                    y[objectNum] = np.matmul(inputPacket[idX:idX+2],word)
                    idX += 2
                    z[objectNum] = np.matmul(inputPacket[idX:idX+2],word)
                    idX += 2
                    
                # Make the necessary corrections and calculate the rest of the data
                rangeVal = rangeIdx * configParameters["rangeIdxToMeters"]
                dopplerIdx[dopplerIdx > (configParameters["numDopplerBins"]/2 - 1)] = dopplerIdx[dopplerIdx > (configParameters["numDopplerBins"]/2 - 1)] - 65535
                dopplerVal = dopplerIdx * configParameters["dopplerResolutionMps"]
                x = x / tlv_xyzQFormat
                y = y / tlv_xyzQFormat
                z = z / tlv_xyzQFormat
                
                # Store the data in the detObj dictionary
                detObj = {"numObj": tlv_numObj, "rangeIdx": rangeIdx, "range": rangeVal, "dopplerIdx": dopplerIdx, \
                        "doppler": dopplerVal, "peakVal": peakVal, "x": x, "y": y, "z": z}

        return detObj
    
    