#!/usr/bin/python

import sys
from random import randint

import cothread
from cothread.catools import *

STAT_IDLE = 1
STAT_ACQUIRE = 3

def main():
   
    for i in range(5000):

        print "Acquire " + str(i)
   
        caput("BL1A:Det:GblCon:Stop", 1, wait=True, timeout=2000)
        status = caget("BL1A:Det:GblCon:State")
        if (status != "Ready"):
            print "ERROR on nED Stop: i= " + str(i) + \
            " status= " + str(status) + " and it should be: Ready"
            sys.exit(1)
        
        #if ((i % 500) == 0):
        #    caput("BL1A:Det:GblCon:WriteConf", 1, wait=True, timeout=2000)
        #    status = caget("BL1A:Det:GblCon:State")
        #    if (status != "Ready"):
        #        print "ERROR on nED WriteConf: i= " + str(i) + \
        #            " status= " + str(status) + " and it should be: Ready"
        #        sys.exit(1)

        caput("BL1A:Det:GblCon:Start", 1, wait=True, timeout=2000)
        status = caget("BL1A:Det:GblCon:State")
        if (status != "Ready"):
            print "ERROR on nED Start: i= " + str(i) + \
            " status= " + str(status) + " and it should be: Ready"
            sys.exit(1)

        status = caget("BL1A:Det:GblCon:PctSucceeded")
        print "GblCon:PctSucceeded: " + str(status)

        caput("BL1A:CS:RunControl:Start", 1, wait=True, timeout=2000)
        status = caget("BL1A:CS:RunControl:StateEnum")
        if (status != STAT_ACQUIRE):
            print "ERROR on Start: i= " + str(i) + \
            " status= " + str(status) + " and it should be " + str(STAT_ACQUIRE)
            sys.exit(1)

        status = caget("BL1A:Det:N1:DetectorState_RBV")
        if (status != 1):
            print "ERROR on Start: i= " + str(i) + \
            " BL1A:Det:N1:DetectorState_RBV= " + str(status) + " and it should be 1"
            sys.exit(1)

        sleepTime = randint(1,10)
        print "sleepTime: " + str(sleepTime)
        cothread.Sleep(sleepTime)

        caput("BL1A:CS:RunControl:Stop", 1, wait=True, timeout=2000)
        #cothread.Sleep(1) #At the moment callback not supported on a stop.
        status = caget("BL1A:CS:RunControl:StateEnum")
        if (status != STAT_IDLE):
            print "ERROR on Stop: i= " + str(i) + \
            " status= " + str(status) + " and it should be " + str(STAT_IDLE)
            sys.exit(1)

        status = caget("BL1A:Det:N1:DetectorState_RBV")
        if (status != 0):
            print "ERROR on Stop: i= " + str(i) + \
            " BL1A:Det:N1:DetectorState_RBV= " + str(status) + " and it should be 0"
            sys.exit(1)

   

if __name__ == "__main__":
        main()
