#!/usr/bin/python

"""
Author: Matt Pearson
Date: Apr 2015

Description: Check that records are ROI records are reset properly on a reset.
"""

import sys
import time
from random import randint

from adned_lib import adned_lib
from adned_globals import adned_globals
from epics import caget, caput
    
def main():
    """
    Do a start, the periodically do a reset and check the data.
    """

    pv = str(sys.argv[1])

    print "Testing multiple reset cycles on " + pv
    
    lib = adned_lib()
    g = adned_globals()
    
    if (lib.init_check(pv) != g.SUCCESS):
        sys.exit(lib.testComplete(g.FAIL))

    cycles = range(100)
    
    roi_counts = []
    roi_counts_after = []

    initial = True

    for i in range(g.AD_MAX_ROI):
        roi_counts.append(0)
        roi_counts_after.append(0)
    
    stat = lib.start(pv)
    if (stat == g.FAIL):
        sys.exit(lib.testComplete(g.FAIL))

    for i in cycles:
        print "ROI Reset test " + str(i)

        caput(pv + str(":Reset"), 1, wait=True, timeout=g.TIMEOUT)

        #Read counters immediately after a reset 
        for roi in range(g.AD_MAX_ROI):
            roi_pv = pv + ":Det1:TOF:ROI:" + str(roi) + ":Total_RBV"
            roi_counts[roi] = caget(roi_pv)

        #Check counters have been reset on a start.
        #I cannot check if they are zero, because they might have already become non-zero by 
        #the time I read them. So we check they are at least less than what they were before the last stop.
        #This only works for ROIs that are active.
        if not initial:
            for roi in range(g.AD_MAX_ROI):
                if (roi_counts[roi] >= roi_counts_after[roi]):
                    print "ERROR: Reset problem. roi_counts[" + str(roi) + "]: " + str(roi_counts[roi]) \
                        + "  roi_counts_after[" + str(roi) + "]: " + str(roi_counts_after[roi])
                    sys.exit(lib.testComplete(g.FAIL))

        initial = False
        time.sleep(5)
        
        #Read counters before next reset
        for roi in range(g.AD_MAX_ROI):
            roi_pv = pv + ":Det1:TOF:ROI:" + str(roi) + ":Total_RBV"
            roi_counts_after[roi] = caget(roi_pv)
        
    stat = lib.stop(pv)
    if (stat == g.FAIL):
        sys.exit(lib.testComplete(g.FAIL))
    
    sys.exit(lib.testComplete(g.SUCCESS))
   

if __name__ == "__main__":
        main()
