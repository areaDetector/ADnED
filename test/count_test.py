#!/usr/bin/python

"""
Author: Matt Pearson
Date: Apr 2015

Description: Check that count record increment ok.
"""

import sys
import time
from random import randint

from adned_lib import adned_lib
from adned_globals import adned_globals
from epics import caget, caput
    
def main():
    """
    Start ADnED and look at the parameters that should be counting up.
    Periodically check them and report any errors. 
    This relies on incoming events.
    """

    pv = str(sys.argv[1])

    print "Testing count parameters on " + pv
    
    lib = adned_lib()
    g = adned_globals()
    
    lib.init_check(pv)

    cycles = range(10)
    tests = range(10)
    wait_time = 5 #Time to wait between checking counters
    
    total_counts = []
    pulse_counter = 0
    proton_charge = 0
    seq_counter = 0

    total_counts_after = []
    pulse_counter_after = 0
    proton_charge_after = 0
    seq_counter_after = 0

    for i in range(g.AD_MAX_DET):
        total_counts.append(0)
        total_counts_after.append(0)
    
    for i in cycles:
        print "Counter cycle " + str(i)

        stat = lib.start(pv)
        if (stat == g.FAIL):
            sys.exit(lib.testComplete(g.FAIL))

        #Read counters immediately after a start 
        for det in range(g.AD_MAX_DET):
            counts_pv = pv + ":Det" + str(det+1) + ":EventTotal_RBV"
            total_counts[det] = caget(counts_pv)
        pulse_counter = caget(pv + ":PulseCounter_RBV")
        proton_charge = caget(pv + ":PChargeIntegrated_RBV")
        seq_counter   = caget(pv + ":SeqCounter0_RBV")

        for test in tests:

            time.sleep(wait_time)

            #Check all counters have increased
            for det in range(g.AD_MAX_DET):
                counts_pv = pv + ":Det" + str(det+1) + ":EventTotal_RBV"
                total_counts_after[det] = caget(counts_pv)
            pulse_counter_after = caget(pv + ":PulseCounter_RBV")
            proton_charge_after = caget(pv + ":PChargeIntegrated_RBV")
            seq_counter_after   = caget(pv + ":SeqCounter0_RBV")

            for det in range(g.AD_MAX_DET):
                if (total_counts[det] >= total_counts_after[det]):
                    print "ERROR: Count problem. total_counts[" + str(det) + "]: " + str(total_counts[det]) \
                        + "  total_counts_after[" + str(det) + "]: " + str(total_counts_after[det])
                    sys.exit(lib.testComplete(g.FAIL))

                if (pulse_counter >= pulse_counter_after):
                    print "ERROR: Count problem. pulse_counter: " + str(pulse_counter) + "  pulse_counter_after: " + str(pulse_counter_after)
                    sys.exit(lib.testComplete(g.FAIL))
                if (proton_charge >= proton_charge_after):
                    print "ERROR: Count problem. proton_charge: " + str(proton_charge) + "  proton_charge_after: " + str(proton_charge_after)
                    sys.exit(lib.testComplete(g.FAIL))
                if (seq_counter >= seq_counter_after):
                    print "ERROR: Count problem. seq_counter: " + str(seq_counter) + "  seq_counter_after: " + str(seq_counter_after)
                    sys.exit(lib.testComplete(g.FAIL))

            print "Test " + str(test) + " on cycle " + str(i) + " is OK." 
        
        stat = lib.stop(pv)
        if (stat == g.FAIL):
            sys.exit(lib.testComplete(g.FAIL))

        #Check event rate (should be zero after a stop)
        for det in range(g.AD_MAX_DET):
            rate_pv = pv + ":Det" + str(det+1) + ":EventRate_RBV"
            rate = caget(rate_pv)
            if (rate != 0):
                print "ERROR: Stop problem. Rate is non zero after a stop. Rate: " + str(rate)
                sys.exit(lib.testComplete(g.FAIL))
            
    sys.exit(lib.testComplete(g.SUCCESS))
   

if __name__ == "__main__":
        main()
