#!/usr/bin/python

import sys
from random import randint

import cothread
from cothread.catools import *

RC_IDLE = 1
RC_RUN = 3

AD_IDLE = 0
AD_ACQUIRE = 1

def main():
   
    for i in range(10000):

        print "Acquire " + str(i)
   
        error = False

        caput("BL1A:CS:RunControl:Start", 1, wait=True, timeout=20)
        status = caget("BL1A:CS:RunControl:StateEnum")
        if (status != RC_RUN):
            print "ERROR on Start: i= " + str(i) + \
            " status= " + str(status) + " and it should be " + str(RC_RUN)
            error = True

        status = caget("BL1A:Det:N1:DetectorState_RBV")
        if (status != AD_ACQUIRE):
            print "ERROR on Start: i= " + str(i) + \
            " BL1A:Det:N1:DetectorState_RBV= " + str(status) + " and it should be " + str(AD_ACQUIRE)
            status_msg = caget("BL1A:Det:N1:StatusMessage_RBV")
            print "ADnED Status message: " + str(status_msg)
            error = True
            
        if error:
            cothread.Sleep(1)
            status = caget("BL1A:Det:N1:DetectorState_RBV")
            print "ADnED delayed status read: " + str(status)
            sys.exit(1)

        sleepTime = randint(1,10)
        print "sleepTime: " + str(sleepTime)
        cothread.Sleep(sleepTime)

        caput("BL1A:CS:RunControl:Stop", 1, wait=True, timeout=20)
        #cothread.Sleep(1) #At the moment callback not supported on a stop.
        status = caget("BL1A:CS:RunControl:StateEnum")
        if (status != RC_IDLE):
            print "ERROR on Stop: i= " + str(i) + \
            " status= " + str(status) + " and it should be " + str(RC_IDLE)
            error = True

        status = caget("BL1A:Det:N1:DetectorState_RBV")
        if (status != AD_IDLE):
            print "ERROR on Stop: i= " + str(i) + \
            " BL1A:Det:N1:DetectorState_RBV= " + str(status) + " and it should be " + str(AD_IDLE)
            status_msg = caget("BL1A:Det:N1:StatusMessage_RBV")
            print "ADnED Status message: " + str(status_msg)
            error = True

        if error:
            cothread.Sleep(1)
            status = caget("BL1A:Det:N1:DetectorState_RBV")
            print "ADnED delayed status read: " + str(status)
            sys.exit(1)


if __name__ == "__main__":
        main()
