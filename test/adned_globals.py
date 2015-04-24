#!/usr/bin/python

"""
Author: Matt Pearson
Date: Apr 2015

Description: Class to hold globals for ADnED test scripts
"""

class adned_globals(object):
    """
    Class to hold globals for ADnED example test scripts.

    Constants are local variables to methods, and they are called
    by either the method name or the read-only property associated with that
    method.
    """

    def __init__(self):
        pass

    def getTimeout(self):
        return 10

    def getFail(self):
        return 1

    def getSuccess(self):
        return 0

    def getADIdle(self):
        return 0

    def getADAcquire(self):
        return 1

    def getADMaxDets(self):
        return 2

    def getADMaxROI(self):
        return 8

    TIMEOUT = property(getTimeout, doc="Put callback timeout")
    FAIL = property(getFail, doc="Fail return value")
    SUCCESS = property(getSuccess, doc="Success return value")
    AD_IDLE = property(getADIdle, doc="Area Detector Idle State")
    AD_ACQUIRE = property(getADAcquire, doc="Area Detector Acquire State")
    AD_MAX_DET = property(getADMaxDets, doc="Max number of detectors")
    AD_MAX_ROI = property(getADMaxROI, doc="Max number of ROI")

