
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

#include <pv/pvTimeStamp.h>
#include "ADDriver.h"
#include "nEDChannel.h"

/* These are the drvInfo strings that are used to identify the parameters.
 * They are used by asyn clients, including standard asyn device support */
//System wide settings
#define ADnEDFirstParamString              "ADNED_FIRST"
#define ADnEDLastParamString               "ADNED_LAST"
#define ADnEDResetParamString              "ADNED_RESET"
#define ADnEDStartParamString              "ADNED_START"
#define ADnEDEventDebugParamString         "ADNED_EVENT_DEBUG"
#define ADnEDSeqCounterParamString         "ADNED_SEQ_COUNTER"
#define ADnEDPulseCounterParamString       "ADNED_PULSE_COUNTER"
#define ADnEDEventRateParamString          "ADNED_EVENT_RATE"
#define ADnEDSeqIDParamString              "ADNED_SEQ_ID"
#define ADnEDSeqIDMissingParamString       "ADNED_SEQ_ID_MISSING"
#define ADnEDSeqIDNumMissingParamString    "ADNED_SEQ_ID_NUM_MISSING"
#define ADnEDBadTimeStampParamString       "ADNED_BAD_TIMESTAMP"
#define ADnEDPChargeParamString            "ADNED_PCHARGE"
#define ADnEDPChargeIntParamString         "ADNED_PCHARGE_INT"
#define ADnEDEventUpdatePeriodParamString  "ADNED_EVENT_UPDATE_PERIOD"
#define ADnEDFrameUpdatePeriodParamString  "ADNED_FRAME_UPDATE_PERIOD"
#define ADnEDPVNameParamString             "ADNED_PV_NAME"
#define ADnEDNumDetParamString             "ADNED_NUM_DET"
#define ADnEDDetPixelNumStartParamString   "ADNED_DET_PIXEL_NUM_START"
#define ADnEDDetPixelNumEndParamString     "ADNED_DET_PIXEL_NUM_END"
#define ADnEDDetNDArrayStartParamString    "ADNED_DET_NDARRAY_START"
#define ADnEDDetNDArrayEndParamString      "ADNED_DET_NDARRAY_END"
#define ADnEDDetNDArraySizeParamString     "ADNED_DET_NDARRAY_SIZE"
#define ADnEDDetNDArrayTOFStartParamString "ADNED_DET_NDARRAY_TOF_START"
#define ADnEDDetNDArrayTOFEndParamString   "ADNED_DET_NDARRAY_TOF_END"
#define ADnEDDetEventRateParamString       "ADNED_DET_EVENT_RATE"
#define ADnEDDetTOFROIStartParamString     "ADNED_DET_TOF_ROI_START"
#define ADnEDDetTOFROIEndParamString       "ADNED_DET_TOF_ROI_END"
#define ADnEDDetTOFROIEnableParamString    "ADNED_DET_TOF_ROI_ENABLE"
#define ADnEDDetTOFTransFileParamString    "ADNED_DET_TOF_TRANS_FILE"
#define ADnEDDetPixelMapFileParamString    "ADNED_DET_PIXEL_MAP_FILE"
#define ADnEDDetTOFTransPrintParamString   "ADNED_DET_TOF_TRANS_PRINT"
#define ADnEDDetPixelMapPrintParamString   "ADNED_DET_PIXEL_MAP_PRINT"
#define ADnEDDetPixelMapEnableParamString   "ADNED_DET_PIXEL_MAP_ENABLE"
#define ADnEDTOFMaxParamString             "ADNED_TOF_MAX"
#define ADnEDAllocSpaceParamString         "ADNED_ALLOC_SPACE"
#define ADnEDAllocSpaceStatusParamString   "ADNED_ALLOC_SPACE_STATUS"

#define ADNED_MAX_STRING_SIZE 256
#define ADNED_MAX_DETS 4

extern "C" {
  int ADnEDConfig(const char *portName, int maxBuffers, size_t maxMemory, int debug);
}

namespace epics {
  namespace pvData {
    class PVStructure;
  }
}

class ADnED : public ADDriver {

 public:
  ADnED(const char *portName, int maxBuffers, size_t maxMemory, int debug);
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
  asynStatus clearParams(void);

 private:

  //Put private functions here
  void printPixelMap(epicsUInt32 det);
  void printTofTrans(epicsUInt32 det);
 
  //Put private static data members here
  static const epicsInt32 s_ADNED_MAX_STRING_SIZE;
  static const epicsInt32 s_ADNED_MAX_DETS;
  static const epicsUInt32 s_ADNED_ALLOC_STATUS_OK;
  static const epicsUInt32 s_ADNED_ALLOC_STATUS_REQ;
  static const epicsUInt32 s_ADNED_ALLOC_STATUS_FAIL;

  //Put private dynamic here
  epicsUInt32 m_acquiring; 
  epicsUInt32 m_seqCounter;
  epicsUInt32 m_lastSeqID;
  epicsUInt32 m_pulseCounter;
  epicsFloat64 m_pChargeInt;
  epicsTimeStamp m_nowTime;
  double m_nowTimeSecs;
  double m_lastTimeSecs;
  epicsUInt32 *p_Data;
  epicsFloat64 *p_TofTrans[ADNED_MAX_DETS+1];
  epicsUInt32 m_TofTransSize[ADNED_MAX_DETS+1];
  epicsUInt32 *p_PixelMap[ADNED_MAX_DETS+1];
  epicsUInt32 m_PixelMapSize[ADNED_MAX_DETS+1];
  bool m_dataAlloc;
  epicsUInt32 m_dataMaxSize;
  epicsUInt32 m_bufferMaxSize;
  epicsUInt32 m_tofMax;
  epics::pvData::PVTimeStamp m_PVTimeStamp;
  epics::pvData::TimeStamp m_TimeStamp;
  epics::pvData::TimeStamp m_TimeStampLast;

  //Constructor parameters.
  const epicsUInt32 m_debug;

  epicsEventId m_startEvent;
  epicsEventId m_stopEvent;
  epicsEventId m_startFrame;
  epicsEventId m_stopFrame;
  
  //Values used for pasynUser->reason, and indexes into the parameter library.
  int ADnEDFirstParam;
  #define ADNED_FIRST_DRIVER_COMMAND ADnEDFirstParam
  int ADnEDResetParam;
  int ADnEDStartParam;
  int ADnEDEventDebugParam;
  int ADnEDSeqCounterParam;
  int ADnEDPulseCounterParam;
  int ADnEDEventRateParam;
  int ADnEDSeqIDParam;
  int ADnEDSeqIDMissingParam;
  int ADnEDSeqIDNumMissingParam;
  int ADnEDBadTimeStampParam;
  int ADnEDPChargeParam;
  int ADnEDPChargeIntParam;
  int ADnEDEventUpdatePeriodParam;
  int ADnEDFrameUpdatePeriodParam;
  int ADnEDPVNameParam;
  int ADnEDNumDetParam;
  int ADnEDDetPixelNumStartParam;
  int ADnEDDetPixelNumEndParam;
  int ADnEDDetNDArrayStartParam;
  int ADnEDDetNDArrayEndParam;
  int ADnEDDetNDArraySizeParam;
  int ADnEDDetNDArrayTOFStartParam;
  int ADnEDDetNDArrayTOFEndParam;
  int ADnEDDetEventRateParam;
  int ADnEDDetTOFROIStartParam;
  int ADnEDDetTOFROIEndParam;
  int ADnEDDetTOFROIEnableParam;
  int ADnEDDetTOFTransFileParam;
  int ADnEDDetPixelMapFileParam;
  int ADnEDDetTOFTransPrintParam;
  int ADnEDDetPixelMapPrintParam;
  int ADnEDDetPixelMapEnableParam;
  int ADnEDTOFMaxParam;
  int ADnEDAllocSpaceParam;
  int ADnEDAllocSpaceStatusParam;
  int ADnEDLastParam;
  #define ADNED_LAST_DRIVER_COMMAND ADnEDLastParam

};

#define NUM_DRIVER_PARAMS (&ADNED_LAST_DRIVER_COMMAND - &ADNED_FIRST_DRIVER_COMMAND + 1)

#endif //ADNED_H
