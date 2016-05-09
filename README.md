# ADnED
EPICS areaDetector driver for V4 neutron event data

The primary purpose of ADnED is to provide live data display for experimental control and visualization.

ADnED can monitor multiple V4 channels for neutron event data and integrate the events for X/Y plots and time-of-flight spectra. It can handle events originating from multiple logical detectors, regardless of how the events are distributed over multiple V4 channels. A detector is defined as a contiguous range of pixel ID numbers. 

ADnED provides the following features:

* start/stop/reset acqusition
* neutron pulse event number, proton charge data, timestamp data, cummulative proton charge
* event rate and total events for each defined detector
* 2-D integrating plots for each detector, which can be X/Y, X/TOF, Y/TOF or PixelID/TOF.
* Time of flight (TOF) integrating plots for each detector
* ROI statistics on all plots (max, min, mean, total events, event rate)
* Filter events going into a TOF spectra based on a X/Y ROI
* Filter events going into a X/Y plot based on TOF ROI
* Re-binning on the TOF spectrum. The waveform sizes can be adjusted at compile time if smaller arrays are sufficient.
* Ability to specify TOF spectrum ROIs in user units (eg. milliseconds). Automatic handling of TOF re-binning.
* Calculate new integrating spectrums based on the TOF and pixel ID, eg. d-space or energy transfer. 
* Ability to clear any of the 1-D plots while an acqusition is in process. This is useful when analyzing a 2-D plot by moving a ROI around on different diffraction peaks, and looking at the effect of the resulting filtered 1-D spectra.
* Using a custom plugin called ADnEDMask (or NDPluginMask) the user has the ability to mask out part of the 2-D pixel plot of the 1-D spectrums. The masks can be set up to filter events out or exclude all other events not inside the mask. This is particulary useful for 1-D plots that have large unwanted peaks due to prompt pulse data.

The CS-Studio OPI files provide additional features:

* Automatic X/Y plot sizing
* TOF plot re-binning & custom x-axis scale 

The V4 structure is defined as:

```
  structure
  // Time stamp for everything in this structure
  // secsPastEpoch, nanoseconds: POSIX Time
  // userTag: Sequence number, incremented by server to allow for missing updates 
  time_t  timeStamp

  // Proton charge, in coulombs (C), associated with this group of events
  NTScalar proton_charge
    double  value

  // Time-of-Flight values for N neutron events
  NTScalarArray time_of_flight
    uint[]  value

  // Pixel IDs for N neutron events
  NTScalarArray pixel
    uint[]  value
```

In order to build ADnED the following needs to be defined in the configure/RELEASE file:

* EPICS base 3.14.12.x
* asyn
* ADCore
* pvAccessCPP
* pvDataCPP
* pvCommonCPP
* pvDatabaseCPP

To build the example IOC, the following will also be required:

* devIocStats
* autosave
* sscan
* calc
* busy


