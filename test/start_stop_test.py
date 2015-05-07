#!/usr/bin/python

"""
Author: Matt Pearson
Date: Apr 2015

Description: Check the start & stop process multiple times. 
Check the detector state is the correct value each time.
"""

import sys
import time
from random import randint

from adned_lib import adned_lib
from adned_globals import adned_globals
from epics import caget, caput
    
def main():
    """
    Make sure the DetectorState_RBV state is correct after a 
    start and stop. This can be critical for higher level software.
    """

    pv = str(sys.argv[1])

    print "Testing multiple start/stop cycles on " + pv
    
    lib = adned_lib()
    g = adned_globals()
    
    lib.init_check(pv)

    cycles = range(100)
    
    for i in cycles:
        print "Start/Stop test " + str(i)

        stat = lib.start(pv)
        if (stat == g.FAIL):
            sys.exit(lib.testComplete(g.FAIL))

        sleepTime = randint(1,5)
        print "count time: " + str(sleepTime)
        time.sleep(sleepTime)
        
        stat = lib.stop(pv)
        if (stat == g.FAIL):
            sys.exit(lib.testComplete(g.FAIL))

    sys.exit(lib.testComplete(g.SUCCESS))
   

if __name__ == "__main__":
        main()
