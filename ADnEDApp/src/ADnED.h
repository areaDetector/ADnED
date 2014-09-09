
/**
 * @brief areaDetector driver that is a V4 neutron data client for nED.
 *
 * @author Matt Pearson
 * @date Sept 2014
 */

#ifndef ADNED_H
#define ADNED_H

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <epicsTime.h>
#include <epicsTypes.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsMutex.h>
#include <epicsString.h>
#include <epicsStdio.h>
#include <cantProceed.h>
#include <epicsTypes.h>

#include <asynOctetSyncIO.h>

#include "ADDriver.h"
#include "nEDChannel.h"

/* These are the drvInfo strings that are used to identify the parameters.
 * They are used by asyn clients, including standard asyn device support */
//System wide settings
#define ADnEDFirstParamString              "ADNED_FIRST"
#define ADnEDLastParamString               "ADNED_LAST"
#define ADnEDResetParamString              "ADNED_RESET"
#define ADnEDEventDebugParamString         "ADNED_EVENT_DEBUG"
#define ADnEDPulseCounterParamString       "ADNED_PULSE_COUNTER"
#define ADnEDPulseIDParamString            "ADNED_PULSE_ID"
#define ADnEDEventUpdatePeriodParamString  "ADNED_EVENT_UPDATE_PERIOD"
#define ADnEDDetPVNameParamString          "ADNED_DET_PV_NAME"
#define ADnEDDet1PixelNumStartParamString  "ADNED_DET1_PIXEL_NUM_START"
#define ADnEDDet2PixelNumStartParamString  "ADNED_DET2_PIXEL_NUM_START"
#define ADnEDDet1PixelNumEndParamString    "ADNED_DET1_PIXEL_NUM_END"
#define ADnEDDet2PixelNumEndParamString    "ADNED_DET2_PIXEL_NUM_END"

#define NED_MAX_STRING_SIZE 256

extern "C" {
  int ADnEDConfig(const char *portName, const char *pvname, int maxBuffers, size_t maxMemory, int debug);
}

namespace epics {
  namespace pvData {
    class PVStructure;
  }
}

class ADnED : public ADDriver {

 public:
  ADnED(const char *portName, const char *pvname, int maxBuffers, size_t maxMemory, int debug);
  virtual ~ADnED();

  /* These are the methods that we override from asynPortDriver */
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
  virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, 
                                  size_t nChars, size_t *nActual);
  virtual void report(FILE *fp, int details);

  void eventTask(void);
  void frameTask(void);
  void eventHandler(std::tr1::shared_ptr<epics::pvData::PVStructure> const &pv_struct);
  asynStatus allocArray(void); 

 private:

  //Put private functions here

  //Put private static data members here
  static const epicsInt32 NED_MAX_STRING_SIZE_;

  //Put private dynamic here
  epicsUInt32 acquiring_; 
  epicsUInt32 pulseCounter_;
  char pvname_[NED_MAX_STRING_SIZE];
  epicsTimeStamp nowTime_;
  double nowTimeSecs_;
  double lastTimeSecs_;
  epicsUInt32 *pData_;
  bool dataAlloc_;
  epicsUInt32 dataMaxSize_;
  epicsUInt32 det1Size_;
  epicsUInt32 det2Size_;

  //Constructor parameters.
  const epicsUInt32 debug_;

  epicsEventId startEvent_;
  epicsEventId stopEvent_;
  epicsEventId startFrame_;
  epicsEventId stopFrame_;
  
  //Values used for pasynUser->reason, and indexes into the parameter library.
  int ADnEDFirstParam;
  #define ADNED_FIRST_DRIVER_COMMAND ADnEDFirstParam
  int ADnEDResetParam;
  int ADnEDEventDebugParam;
  int ADnEDPulseCounterParam;
  int ADnEDPulseIDParam;
  int ADnEDEventUpdatePeriodParam;
  int ADnEDDetPVNameParam;
  int ADnEDDet1PixelNumStartParam;
  int ADnEDDet2PixelNumStartParam;
  int ADnEDDet1PixelNumEndParam;
  int ADnEDDet2PixelNumEndParam;
  int ADnEDLastParam;
  #define ADNED_LAST_DRIVER_COMMAND ADnEDLastParam

};

#define NUM_DRIVER_PARAMS (&ADNED_LAST_DRIVER_COMMAND - &ADNED_FIRST_DRIVER_COMMAND + 1)

#endif //ADNED_H
