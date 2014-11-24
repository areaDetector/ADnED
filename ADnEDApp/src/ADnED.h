
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
#define ADnEDNumChannelsParamString        "ADNED_NUM_CHANNELS"
#define ADnEDPVNameParamString            "ADNED_PV_NAME"
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
#define ADnEDDetTOFROISizeParamString      "ADNED_DET_TOF_ROI_SIZE"
#define ADnEDDetTOFROIEnableParamString    "ADNED_DET_TOF_ROI_ENABLE"
#define ADnEDDetTOFTransFileParamString    "ADNED_DET_TOF_TRANS_FILE"
#define ADnEDDetPixelMapFileParamString    "ADNED_DET_PIXEL_MAP_FILE"
#define ADnEDDetTOFTransPrintParamString   "ADNED_DET_TOF_TRANS_PRINT"
#define ADnEDDetPixelMapPrintParamString   "ADNED_DET_PIXEL_MAP_PRINT"
#define ADnEDDetPixelMapEnableParamString  "ADNED_DET_PIXEL_MAP_ENABLE"
#define ADnEDDetTOFTransEnableParamString  "ADNED_DET_TOF_TRANS_ENABLE"
#define ADnEDDetTOFTransOffsetParamString  "ADNED_DET_TOF_TRANS_OFFSET"
#define ADnEDDetTOFTransScaleParamString   "ADNED_DET_TOF_TRANS_SCALE"
#define ADnEDDetPixelROIStartXParamString  "ADNED_DET_PIXEL_ROI_START_X"
#define ADnEDDetPixelROISizeXParamString   "ADNED_DET_PIXEL_ROI_SIZE_X"
#define ADnEDDetPixelROIStartYParamString  "ADNED_DET_PIXEL_ROI_START_Y"
#define ADnEDDetPixelROISizeYParamString   "ADNED_DET_PIXEL_ROI_SIZE_Y"
#define ADnEDDetPixelSizeXParamString      "ADNED_DET_PIXEL_SIZE_X"
#define ADnEDDetPixelROIEnableParamString   "ADNED_DET_PIXEL_ROI_ENABLE"
#define ADnEDTOFMaxParamString             "ADNED_TOF_MAX"
#define ADnEDAllocSpaceParamString         "ADNED_ALLOC_SPACE"
#define ADnEDAllocSpaceStatusParamString   "ADNED_ALLOC_SPACE_STATUS"

#define ADNED_MAX_STRING_SIZE 256
#define ADNED_MAX_DETS 4
#define ADNED_MAX_CHANNELS 4

extern "C" {
  asynStatus ADnEDConfig(const char *portName, int maxBuffers, size_t maxMemory, int debug);
  asynStatus ADnEDCreateFactory();
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

  static asynStatus createFactory();

  void eventTask(void);
  void frameTask(void);
  void eventHandler(std::tr1::shared_ptr<epics::pvData::PVStructure> const &pv_struct, epicsUInt32 channelID);
  asynStatus allocArray(void); 
  asynStatus clearParams(void);

 private:

  //Put private functions here
  void printPixelMap(epicsUInt32 det);
  void printTofTrans(epicsUInt32 det);
  asynStatus checkPixelMap(epicsUInt32 det);
  asynStatus setupChannelMonitor(const char *pvName, int channel);
 
  //Put private static data members here
  static const epicsInt32 s_ADNED_MAX_STRING_SIZE;
  static const epicsInt32 s_ADNED_MAX_DETS;
  static const epicsInt32 s_ADNED_MAX_CHANNELS;
  static const epicsUInt32 s_ADNED_ALLOC_STATUS_OK;
  static const epicsUInt32 s_ADNED_ALLOC_STATUS_REQ;
  static const epicsUInt32 s_ADNED_ALLOC_STATUS_FAIL;

  //Put private dynamic here
  epicsUInt32 m_acquiring; 
  epicsUInt32 m_seqCounter[ADNED_MAX_CHANNELS];
  epicsUInt32 m_lastSeqID[ADNED_MAX_CHANNELS];
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
  epics::pvData::TimeStamp m_TimeStamp[ADNED_MAX_CHANNELS];
  epics::pvData::TimeStamp m_TimeStampLast[ADNED_MAX_CHANNELS];
  int m_detStartValues[ADNED_MAX_DETS+1];
  int m_detEndValues[ADNED_MAX_DETS+1];
  int m_NDArrayStartValues[ADNED_MAX_DETS+1];
  int m_NDArrayTOFStartValues[ADNED_MAX_DETS+1];
  int m_detTOFROIStartValues[ADNED_MAX_DETS+1];
  int m_detTOFROISizeValues[ADNED_MAX_DETS+1];
  int m_detTOFROIEnabled[ADNED_MAX_DETS+1];
  int m_detPixelMappingEnabled[ADNED_MAX_DETS+1];
  int m_detTOFTransEnabled[ADNED_MAX_DETS+1];
  double m_detTOFTransScale[ADNED_MAX_DETS+1];
  double m_detTOFTransOffset[ADNED_MAX_DETS+1];
  int m_detPixelROIStartX[ADNED_MAX_DETS+1];
  int m_detPixelROIStartY[ADNED_MAX_DETS+1];
  int m_detPixelROISizeX[ADNED_MAX_DETS+1];
  int m_detPixelROISizeY[ADNED_MAX_DETS+1];
  int m_detPixelSizeX[ADNED_MAX_DETS+1];
  int m_detPixelROIEnable[ADNED_MAX_DETS+1];
  epicsUInt32 m_eventsSinceLastUpdate;
  epicsUInt32 m_detEventsSinceLastUpdate[ADNED_MAX_DETS+1];

  epics::pvAccess::ChannelProvider::shared_pointer p_ChannelProvider;
  std::tr1::shared_ptr<nEDChannel::nEDChannelRequester> p_ChannelRequester;
  std::tr1::shared_ptr<nEDChannel::nEDMonitorRequester> p_MonitorRequester[ADNED_MAX_CHANNELS];
  epics::pvData::Monitor::shared_pointer p_Monitor[ADNED_MAX_CHANNELS];
  epics::pvAccess::Channel::shared_pointer p_Channel[ADNED_MAX_CHANNELS];

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
  int ADnEDNumChannelsParam;
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
  int ADnEDDetTOFROISizeParam;
  int ADnEDDetTOFROIEnableParam;
  int ADnEDDetTOFTransFileParam;
  int ADnEDDetPixelMapFileParam;
  int ADnEDDetTOFTransPrintParam;
  int ADnEDDetPixelMapPrintParam;
  int ADnEDDetPixelMapEnableParam;
  int ADnEDDetTOFTransEnableParam;
  int ADnEDDetTOFTransOffsetParam;
  int ADnEDDetTOFTransScaleParam;
  int ADnEDDetPixelROIStartXParam;
  int ADnEDDetPixelROISizeXParam;
  int ADnEDDetPixelROIStartYParam;
  int ADnEDDetPixelROISizeYParam;
  int ADnEDDetPixelSizeXParam;
  int ADnEDDetPixelROIEnableParam;
  int ADnEDTOFMaxParam;
  int ADnEDAllocSpaceParam;
  int ADnEDAllocSpaceStatusParam;
  int ADnEDLastParam;
  #define ADNED_LAST_DRIVER_COMMAND ADnEDLastParam

};

#define NUM_DRIVER_PARAMS (&ADNED_LAST_DRIVER_COMMAND - &ADNED_FIRST_DRIVER_COMMAND + 1)

#endif //ADNED_H
