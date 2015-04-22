#!/usr/bin/python

"""
Author: Matt Pearson
Date: Apr 2015

Description: Run all the tests listed in run_tests.txt

The tests will almost certainly fail if there are no incoming
events, so it is assumed to be run against a beamline installation
with shutter open, or a simulation.

Run like:
cd test
./run_tests.py {base pv name}

Or, can run individual tests in the same manner.
"""

import sys
import os

filename = "run_tests.txt"

def main():

    pv = sys.argv[1]
    
    print "Running tests in " + str(os.getcwd()) + " on " + pv

    data = None
    try:
        file = open(filename)
        data = file.read().splitlines()
        file.close()
    except IOError:
        print __file__ + " ERROR: Could not read " + filename
        data = None
        sys.exit(1)

    stat = 0
    for T in data:
        print T
        stat = os.system("./" + T + " " + pv)
        if stat > 0:
            print  __file__ + " ERROR: Test " + T + " failed."
            sys.exit(1)

    print "Success"
    sys.exit(0)


if __name__ == "__main__":
    main()
