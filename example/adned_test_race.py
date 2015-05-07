#!/usr/bin/python

import sys

import cothread
from cothread.catools import *

STAT_IDLE = 0
STAT_ACQUIRE = 1

def main():
   
    for i in range(100):

        print "Acquire " + str(i)
   
        det1_counts = caget("BL99:Det:N1:Det1:EventTotal_RBV")
        det2_counts = caget("BL99:Det:N1:Det2:EventTotal_RBV")
        print "det1_counts before: " + str(det1_counts)
        print "det2_counts before: " + str(det2_counts)

        caput("BL99:Det:N1:Start", 1, wait=True, timeout=5)
        status = caget("BL99:Det:N1:DetectorState_RBV")
        if (status != STAT_ACQUIRE):
            print "ERROR on Start: i= " + str(i) + \
            " status= " + str(status) + " and it should be " + str(STAT_ACQUIRE)
            sys.exit(1)


        det1_counts = caget("BL99:Det:N1:Det1:EventTotal_RBV")
        det2_counts = caget("BL99:Det:N1:Det2:EventTotal_RBV")
        print "det1_counts after: " + str(det1_counts)
        print "det2_counts after: " + str(det2_counts)

        cothread.Sleep(5)

        caput("BL99:Det:N1:Stop", 1, wait=True, timeout=5)
        status = caget("BL99:Det:N1:DetectorState_RBV")
        if (status != STAT_IDLE):
            print "ERROR on Stop: i= " + str(i) + \
            " status= " + str(status) + " and it should be " + str(STAT_IDLE)
            sys.exit(1)



   

if __name__ == "__main__":
        main()
