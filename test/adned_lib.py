#!/usr/bin/python

"""
Author: Matt Pearson
Date: Apr 2015

Description: Class to hold utility functions for testing ADnED
"""

import sys

from epics import caget, caput

from adned_globals import adned_globals

class adned_lib(object):
    """
    Library of useful test functions for ADnED applications
    """

    __g = adned_globals()

    def testComplete(self, fail):
        """
        Function to be called at end of test
        fail = true or false
        """
        if not fail:
            print "Test Complete"
            return self.__g.SUCCESS
        else:
            print "Test Failed"
            return self.__g.FAIL


    def start(self, adned):
        """
        Start ADnED. Check the status.
        """
    
        _start = adned + ":Start"

        try:
            caput(_start, 1, wait=True, timeout=self.__g.TIMEOUT)
        except:
            e = sys.exc_info()
            print str(e)
            print "ERROR: caput failed. Failed to start."
            return self.__g.FAIL

        return self.check_state(adned, self.__g.AD_ACQUIRE)



    def stop(self, adned):
        """
        Stop ADnED. Check the status.
        """
    
        _stop = adned + ":Stop"

        try:
            caput(_stop, 1, wait=True, timeout=self.__g.TIMEOUT)
        except:
            e = sys.exc_info()
            print str(e)
            print "ERROR: caput failed. Failed to stop."
            return self.__g.FAIL

        return self.check_state(adned, self.__g.AD_IDLE)



    def check_state(self, adned, state):
        """
        Verify that the ADnED state is what it should be.
        """
        _detector_state = adned + ":DetectorState_RBV"
        detector_state = caget(_detector_state)
        if (detector_state != state):
            print "ERROR: ADnED state is: " + str(detector_state) + " and it should be: " + str(state)
            return self.__g.FAIL
        
        return self.__g.SUCCESS
        


    def init_check(self, adned):
        """
        Check ADnED for correct state at start of test.
        """

        _detector_state = adned + ":DetectorState_RBV"
        
        try:
            self.verify(_detector_state, self.__g.AD_IDLE)
        except Exception as e:
            print str(e)
            return self.__g.FAIL

        return self.__g.SUCCESS


    def verify(self, adned, value):
        """
        Verify that adned record == value.
        """
        if (caget(adned) != value):
            msg = " ERROR: " + adned + " not equal to " + str(value)
            raise Exception(__name__ + msg)

        return self.__g.SUCCESS

