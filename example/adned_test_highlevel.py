#!/usr/bin/python

import sys

import cothread
from cothread.catools import *

STAT_OK = 0

def main():
   
    for i in range(10000):

        print "Acquire " + str(i)

        error = False
   
        caput("BL1A:Det:ADnED:Start", 1, wait=True, timeout=10)
        status = caget("BL1A:Det:ADnED:State")
        if (status != STAT_OK):
            print "ERROR on ADnED start. High level status: " + str(status) + ". It should be " + str(STAT_OK)
            adned_status = caget("BL1A:Det:N1:DetectorState_RBV")
            adned_status_msg = caget("BL1A:Det:N1:StatusMessage_RBV")
            print "ADnED status: " + str(adned_status) + ". Message: " + str(adned_status_msg)
            error = True

        if error:
            cothread.Sleep(1)
            status = caget("BL1A:Det:N1:DetectorState_RBV")
            print "ADnED delayed status read: " + str(status)
            status = caget("BL1A:Det:N1:DetectorState_RBV")
            sys.exit(1)

        cothread.Sleep(1)

        caput("BL1A:Det:ADnED:Stop", 1, wait=True, timeout=10)
        #cothread.Sleep(1) #At the moment callback not supported on a stop.
        status = caget("BL1A:Det:ADnED:State")
        if (status != STAT_OK):
            print "ERROR on Stop. High level status: " + str(status) + ". It should be " + str(STAT_OK)
            adned_status = caget("BL1A:Det:N1:DetectorState_RBV")
            adned_status_msg = caget("BL1A:Det:N1:StatusMessage_RBV")
            print "ADnED Status: " + str(adned_status) + ". Message: " + str(adned_status_msg)
            error = True

        if error:
            cothread.Sleep(1)
            status = caget("BL1A:Det:N1:DetectorState_RBV")
            print "ADnED delayed status read: " + str(status)
            sys.exit(1)
        
if __name__ == "__main__":
        main()
