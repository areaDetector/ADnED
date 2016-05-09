#!/usr/bin/python

import sys
import os
from epics import caput

BL = os.environ["BL"]
ADNED = "N1"
BASE_PV = str(BL)+":Det:"+ADNED+":"
DETS = 4

def main():
    
    print "Setting up NDArrayPorts..."
    
    for det in range(1,DETS+1):
        print "Detector: " + str(det)
        NEW_PV = BASE_PV + "Det" + str(det) + ":"
        print "Base PV: " + NEW_PV
        #TOF
        caput(NEW_PV+"TOF:NDArrayPort",      ADNED,wait=True)
        caput(NEW_PV+"TOF:Mask:NDArrayPort", ADNED+".DET"+str(det)+".TOF",wait=True)
        caput(NEW_PV+"TOF:Array:NDArrayPort",ADNED+".DET"+str(det)+".TOF.MASK",wait=True)
        caput(NEW_PV+"TOF:ROI:NDArrayPort",  ADNED+".DET"+str(det)+".TOF.MASK",wait=True)
        #XY
        caput(NEW_PV+"XY:NDArrayPort",       ADNED,wait=True)
        caput(NEW_PV+"XY:Mask:NDArrayPort",  ADNED+".DET"+str(det)+".XY",wait=True)
        caput(NEW_PV+"XY:Array:NDArrayPort", ADNED+".DET"+str(det)+".XY.MASK",wait=True)
        caput(NEW_PV+"XY:ROI:NDArrayPort",   ADNED+".DET"+str(det)+".XY.MASK",wait=True)
        caput(NEW_PV+"XY:Stat:NDArrayPort",  ADNED+".DET"+str(det)+".XY.MASK",wait=True)

    print "Done."


if __name__ == "__main__":
        main()

