#!/usr/bin/python

import sys

import cothread
from cothread.catools import *

STAT_IDLE = 0
STAT_ACQUIRE = 1

def main():
   
    for i in range(1000):

        print "Acquire " + str(i)
   
        caput("BL99:Det:N1:Start", 1, wait=True, timeout=5)
        status = caget("BL99:Det:N1:DetectorState_RBV")
        if (status != STAT_ACQUIRE):
            print "ERROR on Start: i= " + str(i) + \
            " status= " + str(status) + " and it should be " + str(STAT_ACQUIRE)
            sys.exit(1)

        cothread.Sleep(1)

        caput("BL99:Det:N1:Stop", 1, wait=True, timeout=5)
        status = caget("BL99:Det:N1:DetectorState_RBV")
        if (status != STAT_IDLE):
            print "ERROR on Stop: i= " + str(i) + \
            " status= " + str(status) + " and it should be " + str(STAT_IDLE)
            sys.exit(1)



   

if __name__ == "__main__":
        main()
