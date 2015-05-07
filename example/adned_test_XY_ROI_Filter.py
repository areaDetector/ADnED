#!/usr/bin/python

import sys

import cothread
from cothread.catools import *

STAT_IDLE = 0
STAT_ACQUIRE = 1

def main():
   
    for i in range(2):

        print "Acquire " + str(i)
   
        #Set up ROI1 on XY and enable ROI filter
        caput("BL99:Det:N1:Det1:PixelROIFilterEnable", 1, wait=True, timeout=20)

        caput("BL99:Det:N1:Start", 1, wait=True, timeout=2000)
        status = caget("BL99:Det:N1:DetectorState_RBV")
        if (status != STAT_ACQUIRE):
            print "ERROR on Start: i= " + str(i) + \
                " status= " + str(status) + " and it should be " + str(STAT_ACQUIRE)
            sys.exit(1)

        for x in range(32):
            for y in range(32):

                print "x: " + str(x)
                print "y: " + str(y)
                caput("BL99:Det:N1:Det1:XY:ROI:1:MinX", x, wait=True, timeout=20)
                caput("BL99:Det:N1:Det1:XY:ROI:1:MinY", y, wait=True, timeout=20)

                for xsize in range(32):
                    for ysize in range(32):
                    
                        
                        print "xsize: " + str(xsize)
                        print "ysize: " + str(ysize)
                        caput("BL99:Det:N1:Det1:XY:ROI:1:SizeX", xsize, wait=True, timeout=20)
                        caput("BL99:Det:N1:Det1:XY:ROI:1:SizeY", ysize, wait=True, timeout=20)

                        cothread.Sleep(0.1)

        caput("BL16B:Det:ADnED:Stop", 1, wait=True, timeout=2000)
        status = caget("BL16B:Det:ADnED:DetectorState_RBV")
        if (status != STAT_IDLE):
            print "ERROR on Stop: i= " + str(i) + \
                " status= " + str(status) + " and it should be " + str(STAT_IDLE)
            sys.exit(1)


if __name__ == "__main__":
        main()
