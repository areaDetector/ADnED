#!/usr/bin/python

"""
Author: Matt Pearson
Date: Apr 2015

Description: Check that records are reset properly on a start.
"""

import sys
import time
from random import randint

from adned_lib import adned_lib
from adned_globals import adned_globals
from epics import caget, caput
    
def main():
    """
    Do a series of starts & stops. After the start, immediately
    check that the counters have been reset to 0, by comparing them
    to the values just before the previous stop.

    After a stop, make sure that the event rate parameters are zero.
    """

    pv = str(sys.argv[1])

    print "Testing multiple start/reset cycles on " + pv
    
    lib = adned_lib()
    g = adned_globals()
    
    lib.init_check(pv)

    cycles = range(100)
    
    total_counts = []
    pulse_counter = 0
    proton_charge = 0
    seq_counter = 0

    total_counts_after = []
    pulse_counter_after = 0
    proton_charge_after = 0
    seq_counter_after = 0

    initial = True

    for i in range(g.AD_MAX_DET):
        total_counts.append(0)
        total_counts_after.append(0)
    
    for i in cycles:
        print "Start/Reset test " + str(i)

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

        #Check counters have been reset on a start.
        #I cannot check if they are zero, because they might have already become non-zero by 
        #the time I read them. So we check they are at least less than what they were before the last stop.
        if not initial:
            for det in range(g.AD_MAX_DET):
                if (total_counts[det] >= total_counts_after[det]):
                    print "ERROR: Reset problem. total_counts[" + str(det) + "]: " + str(total_counts[det]) \
                        + "  total_counts_after[" + str(det) + "]: " + str(total_counts_after[det])
                    sys.exit(lib.testComplete(g.FAIL))

                if (pulse_counter >= pulse_counter_after):
                    print "ERROR: Reset problem. pulse_counter: " + str(pulse_counter) + "  pulse_counter_after: " + str(pulse_counter_after)
                    sys.exit(lib.testComplete(g.FAIL))
                if (proton_charge >= proton_charge_after):
                    print "ERROR: Reset problem. proton_charge: " + str(proton_charge) + "  proton_charge_after: " + str(proton_charge_after)
                    sys.exit(lib.testComplete(g.FAIL))
                if (seq_counter >= seq_counter_after):
                    print "ERROR: Reset problem. seq_counter: " + str(seq_counter) + "  seq_counter_after: " + str(seq_counter_after)
                    sys.exit(lib.testComplete(g.FAIL))

        initial = False
        time.sleep(5)
        
        #Read counters before stopping
        for det in range(g.AD_MAX_DET):
            counts_pv = pv + ":Det" + str(det+1) + ":EventTotal_RBV"
            total_counts_after[det] = caget(counts_pv)
        pulse_counter_after = caget(pv + ":PulseCounter_RBV")
        proton_charge_after = caget(pv + ":PChargeIntegrated_RBV")
        seq_counter_after   = caget(pv + ":SeqCounter0_RBV")
        
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
