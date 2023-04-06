import serial
import time
import numpy as np


# Streamer class -> use either loadFile + readFromFile (in a loop w/ delays) OR readRealTime (also must be in a loop with delays). Both read methods will return the next packet available, if there is one

class IWR1443_Streamer:

    def __init__(self):
        self.byteBuffer = np.zeros(2**16,dtype = 'uint8')
        self.byteBufferLength = 0
        
    def loadFile(self): # method to read all data in from the file
        byteVec = np.fromfile('xwr14xx_processed_stream_2023_04_05T13_04_55_356.dat', dtype="byte")
        self.addToBuffer(byteVec)

    def readFromFile(self): # method which extracts the next available packet and updates the buffer
        return self.extractNextPacket()

    def readRealtime(self, dataport): # method which reads in any waiting data, adds it to the buffer, and then extracts the next available 
        readBuffer = dataport.read(dataport.in_waiting)
        byteVec = np.frombuffer(readBuffer, dtype = 'uint8')
        self.addToBuffer(byteVec)
        return self.extractNextPacket()

    def addToBuffer(self, byteVec): # method which adds any new data to the buffer, and updates the buffer
        maxBufferSize = 2**16
        magicWord = [2, 1, 4, 3, 6, 5, 8, 7]
        byteCount = len(byteVec)
        
        # add the data to the end of the buffer, update buffer length
        if (self.byteBufferLength + byteCount) < maxBufferSize:
            self.byteBuffer[self.byteBufferLength:self.byteBufferLength + byteCount] = byteVec[:byteCount]
            self.byteBufferLength = self.byteBufferLength + byteCount

         # Check that the buffer has some data
        if self.byteBufferLength > 16:
            
            # Check for all possible locations of the magic word
            possibleLocs = np.where(self.byteBuffer == magicWord[0])[0]

            # Confirm that is the beginning of the magic word and store the index in startIdx
            startIdx = []
            for loc in possibleLocs:
                check = self.byteBuffer[loc:loc+8]
                if np.all(check == magicWord):
                    startIdx.append(loc)
                
            # Check that startIdx is not empty
            if startIdx:
                
                # Remove the data before the first start index
                if startIdx[0] > 0 and startIdx[0] < self.byteBufferLength:
                    self.byteBuffer[:self.byteBufferLength-startIdx[0]] = self.byteBuffer[startIdx[0]:self.byteBufferLength]
                    self.byteBuffer[self.byteBufferLength-startIdx[0]:] = np.zeros(len(self.byteBuffer[self.byteBufferLength-startIdx[0]:]),dtype = 'uint8')
                    self.byteBufferLength = self.byteBufferLength - startIdx[0]

    def extractNextPacket(self): #method to extract the next packet 

        # word array to convert 4 bytes to a 32 bit number
        word = [1, 2**8, 2**16, 2**24]
        
        # Read the total packet length
    
        totalPacketLen = np.matmul(self.byteBuffer[12:12+4],word)
        
        # Check that all the packet has been read
        if (self.byteBufferLength >= totalPacketLen) and (self.byteBufferLength != 0):
            currentPacket = self.byteBuffer[:totalPacketLen] # extract current packet
            self.byteBuffer[:self.byteBufferLength - totalPacketLen] = self.byteBuffer[totalPacketLen:self.byteBufferLength] # remove this packet from the buffer
            self.byteBuffer[self.byteBufferLength - totalPacketLen:] = np.zeros(len(self.byteBuffer[self.byteBufferLength - totalPacketLen:]),dtype = 'uint8') # add zeros where packet previously was
            self.byteBufferLength = self.byteBufferLength - totalPacketLen # update current packet length
            return currentPacket
        