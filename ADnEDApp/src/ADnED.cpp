
/**
 * @brief areaDetector driver that is a V4 neutron data client for nED.
 *
 * @author Matt Pearson
 * @date Sept 2014
 */

#include <iostream>
#include <string>
#include <stdexcept>
#include "dirent.h"
#include <sys/types.h>
#include <syscall.h> 
#include <stdexcept>

//Epics headers
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsExport.h>
#include <epicsString.h>
#include <iocsh.h>
#include <drvSup.h>
#include <registryFunction.h>

//ADnED
#include "ADnED.h"
#include "nEDChannel.h"
#include "ADnEDFile.h"
#include <pv/pvData.h>

using std::cout;
using std::cerr;
using std::endl;
using std::hex;

using std::tr1::shared_ptr;
using nEDChannel::nEDChannelRequester;
using nEDChannel::nEDMonitorRequester;

//Not sure how we want to handle these yet, so will leave them as #defines for now.
#define ADNED_PV_TIMEOUT 2.0
#define ADNED_PV_PRIORITY epics::pvAccess::ChannelProvider::PRIORITY_DEFAULT
#define ADNED_PV_PROVIDER "pva"
#define ADNED_PV_REQUEST "record[queueSize=100]field()"
#define ADNED_PV_PIXELS "pixel.value" 
#define ADNED_PV_TOF "time_of_flight.value" 
#define ADNED_PV_TIMESTAMP "timeStamp"
#define ADNED_PV_SEQ "timeStamp.userTag" 
#define ADNED_PV_PCHARGE "proton_charge.value"

//Definitions of static class data members
const epicsInt32 ADnED::s_ADNED_MAX_STRING_SIZE = ADNED_MAX_STRING_SIZE;
const epicsInt32 ADnED::s_ADNED_MAX_DETS = ADNED_MAX_DETS;
const epicsInt32 ADnED::s_ADNED_MAX_CHANNELS = ADNED_MAX_CHANNELS;
const epicsUInt32 ADnED::s_ADNED_ALLOC_STATUS_OK = 0;
const epicsUInt32 ADnED::s_ADNED_ALLOC_STATUS_REQ = 1;
const epicsUInt32 ADnED::s_ADNED_ALLOC_STATUS_FAIL = 2;

//C Function prototypes to tie in with EPICS
static void ADnEDEventTaskC(void *drvPvt);
static void ADnEDFrameTaskC(void *drvPvt);

/**
 * Constructor for Xspress3::Xspress3. 
 * This must be called in the Epics IOC startup file.
 * @param portName The Asyn port name to use
 * @param maxBuffers Used by asynPortDriver (set to -1 for unlimited)
 * @param maxMemory Used by asynPortDriver (set to -1 for unlimited)
 * @param debug This debug flag for the driver. 
 */
ADnED::ADnED(const char *portName, int maxBuffers, size_t maxMemory, int debug)
  : ADDriver(portName,
             s_ADNED_MAX_DETS+1, /* maxAddr (different detectors use different asyn address*/ 
             NUM_DRIVER_PARAMS,
             maxBuffers,
             maxMemory,
             asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat32ArrayMask | asynFloat64ArrayMask | asynDrvUserMask | asynOctetMask | asynGenericPointerMask, /* Interface mask */
             asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat32ArrayMask | asynFloat64ArrayMask | asynOctetMask | asynGenericPointerMask,  /* Interrupt mask */
             ASYN_CANBLOCK | ASYN_MULTIDEVICE, /* asynFlags.*/
             1, /* Autoconnect */
             0, /* default priority */
             0), /* Default stack size*/
    m_debug(debug)
{
  int status = asynSuccess;
  const char *functionName = "ADnED::ADnED";

  //Create the epicsEvent for signaling the threads.
  //This will cause it to do a poll immediately, rather than wait for the poll time period.
  m_startEvent = epicsEventMustCreate(epicsEventEmpty);
  if (!m_startEvent) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s epicsEventCreate failure for start event.\n", functionName);
    return;
  }
  m_stopEvent = epicsEventMustCreate(epicsEventEmpty);
  if (!m_stopEvent) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s epicsEventCreate failure for stop event.\n", functionName);
    return;
  }
  m_startFrame = epicsEventMustCreate(epicsEventEmpty);
  if (!m_startFrame) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s epicsEventCreate failure for start frame.\n", functionName);
    return;
  }
  m_stopFrame = epicsEventMustCreate(epicsEventEmpty);
  if (!m_stopFrame) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s epicsEventCreate failure for stop frame.\n", functionName);
    return;
  }

  //Add the params to the paramLib 
  //createParam adds the parameters to all param lists automatically (using maxAddr).
  createParam(ADnEDFirstParamString,              asynParamInt32,       &ADnEDFirstParam);
  createParam(ADnEDResetParamString,              asynParamInt32,       &ADnEDResetParam);
  createParam(ADnEDStartParamString,              asynParamInt32,       &ADnEDStartParam);
  createParam(ADnEDStopParamString,              asynParamInt32,       &ADnEDStopParam);
  createParam(ADnEDEventDebugParamString,         asynParamInt32,       &ADnEDEventDebugParam);
  createParam(ADnEDSeqCounterParamString,       asynParamInt32,       &ADnEDSeqCounterParam);
  createParam(ADnEDPulseCounterParamString,       asynParamInt32,       &ADnEDPulseCounterParam);
  createParam(ADnEDEventRateParamString,       asynParamInt32,       &ADnEDEventRateParam);
  createParam(ADnEDSeqIDParamString,            asynParamInt32,       &ADnEDSeqIDParam);
  createParam(ADnEDSeqIDMissingParamString,            asynParamInt32,       &ADnEDSeqIDMissingParam);
  createParam(ADnEDSeqIDNumMissingParamString,            asynParamInt32,       &ADnEDSeqIDNumMissingParam);
  createParam(ADnEDBadTimeStampParamString,            asynParamInt32,       &ADnEDBadTimeStampParam);
  createParam(ADnEDPChargeParamString,            asynParamFloat64,     &ADnEDPChargeParam);
  createParam(ADnEDPChargeIntParamString,            asynParamFloat64,     &ADnEDPChargeIntParam);
  createParam(ADnEDEventUpdatePeriodParamString,  asynParamFloat64,     &ADnEDEventUpdatePeriodParam);
  createParam(ADnEDFrameUpdatePeriodParamString,  asynParamFloat64,     &ADnEDFrameUpdatePeriodParam);
  createParam(ADnEDNumChannelsParamString,             asynParamInt32,       &ADnEDNumChannelsParam);
  createParam(ADnEDPVNameParamString,          asynParamOctet,       &ADnEDPVNameParam);
  createParam(ADnEDNumDetParamString,             asynParamInt32,       &ADnEDNumDetParam);
  createParam(ADnEDDetPixelNumStartParamString,  asynParamInt32,       &ADnEDDetPixelNumStartParam);
  createParam(ADnEDDetPixelNumEndParamString,    asynParamInt32,       &ADnEDDetPixelNumEndParam);
  createParam(ADnEDDetNDArrayStartParamString,    asynParamInt32,       &ADnEDDetNDArrayStartParam);
  createParam(ADnEDDetNDArrayEndParamString,    asynParamInt32,       &ADnEDDetNDArrayEndParam);
  createParam(ADnEDDetNDArraySizeParamString,    asynParamInt32,       &ADnEDDetNDArraySizeParam);
  createParam(ADnEDDetNDArrayTOFStartParamString,    asynParamInt32,       &ADnEDDetNDArrayTOFStartParam);
  createParam(ADnEDDetNDArrayTOFEndParamString,    asynParamInt32,       &ADnEDDetNDArrayTOFEndParam);
  createParam(ADnEDDetEventRateParamString,    asynParamInt32,       &ADnEDDetEventRateParam);
  createParam(ADnEDDetTOFROIStartParamString,    asynParamInt32,       &ADnEDDetTOFROIStartParam);
  createParam(ADnEDDetTOFROISizeParamString,    asynParamInt32,       &ADnEDDetTOFROISizeParam);
  createParam(ADnEDDetTOFROIEnableParamString,    asynParamInt32,       &ADnEDDetTOFROIEnableParam);
  createParam(ADnEDDetTOFTransFileParamString,          asynParamOctet,       &ADnEDDetTOFTransFileParam);
  createParam(ADnEDDetPixelMapFileParamString,          asynParamOctet,       &ADnEDDetPixelMapFileParam);
  createParam(ADnEDDetTOFTransPrintParamString,    asynParamInt32,       &ADnEDDetTOFTransPrintParam);
  createParam(ADnEDDetPixelMapPrintParamString,    asynParamInt32,       &ADnEDDetPixelMapPrintParam);
  createParam(ADnEDDetPixelMapEnableParamString, asynParamInt32,       &ADnEDDetPixelMapEnableParam);
  createParam(ADnEDDetTOFTransEnableParamString, asynParamInt32,       &ADnEDDetTOFTransEnableParam);
  createParam(ADnEDDetTOFTransOffsetParamString, asynParamFloat64,       &ADnEDDetTOFTransOffsetParam);
  createParam(ADnEDDetTOFTransScaleParamString, asynParamFloat64,       &ADnEDDetTOFTransScaleParam);
  createParam(ADnEDDetPixelROIStartXParamString, asynParamInt32,       &ADnEDDetPixelROIStartXParam);
  createParam(ADnEDDetPixelROISizeXParamString, asynParamInt32,       &ADnEDDetPixelROISizeXParam);
  createParam(ADnEDDetPixelROIStartYParamString, asynParamInt32,       &ADnEDDetPixelROIStartYParam);
  createParam(ADnEDDetPixelROISizeYParamString, asynParamInt32,       &ADnEDDetPixelROISizeYParam);
  createParam(ADnEDDetPixelSizeXParamString, asynParamInt32,       &ADnEDDetPixelSizeXParam);
  createParam(ADnEDDetPixelROIEnableParamString, asynParamInt32,       &ADnEDDetPixelROIEnableParam);
  createParam(ADnEDTOFMaxParamString,             asynParamInt32,       &ADnEDTOFMaxParam);
  createParam(ADnEDAllocSpaceParamString,         asynParamInt32,       &ADnEDAllocSpaceParam);
  createParam(ADnEDAllocSpaceStatusParamString,         asynParamInt32,       &ADnEDAllocSpaceStatusParam);
  createParam(ADnEDLastParamString,               asynParamInt32,       &ADnEDLastParam);

  //Initialize non static, non const, data members
  m_acquiring = 0;
  for (int chan=0; chan<s_ADNED_MAX_CHANNELS; ++chan) {
    m_seqCounter[chan] = 0;
    m_lastSeqID[chan] = -1; //Init to -1 to catch packet trains stuck at zero
  }
  m_pulseCounter = 0;
  m_pChargeInt = 0.0;
  m_nowTimeSecs = 0.0;
  m_lastTimeSecs = 0.0;
  p_Data = NULL;
  m_dataAlloc = true;
  m_dataMaxSize = 0;
  m_bufferMaxSize = 0;
  m_tofMax = 0;
  for (int chan=0; chan<s_ADNED_MAX_CHANNELS; ++chan) {
    m_TimeStamp[chan].put(0,0);
    m_TimeStampLast[chan].put(0,0);
  }

  for (int i=0; i<=s_ADNED_MAX_DETS; ++i) {
    p_PixelMap[i] = NULL;
    p_TofTrans[i] = NULL;
   
    m_TofTransSize[i] = 0;
    m_PixelMapSize[i] = 0;
    
    m_detStartValues[i] = 0;
    m_detEndValues[i] = 0;
    m_NDArrayStartValues[i] = 0;
    m_NDArrayTOFStartValues[i] = 0;
    m_detTOFROIStartValues[i] = 0;
    m_detTOFROISizeValues[i] = 0;
    m_detTOFROIEnabled[i] = 0;
    m_detPixelMappingEnabled[i] = 0;
    m_detTOFTransEnabled[i] = 0;
    m_detTOFTransOffset[i] = 0;
    m_detTOFTransScale[i] = 0;

    m_detPixelROIStartX[i] = 0;
    m_detPixelROISizeX[i] = 0;
    m_detPixelROIStartY[i] = 0;
    m_detPixelROISizeY[i] = 0;
    m_detPixelSizeX[i] = 0;
    m_detPixelROIEnable[i] = 0;
  }
  
  //Create the thread that reads the data 
  status = (epicsThreadCreate("ADnEDEventTask",
                            epicsThreadPriorityHigh,
                            epicsThreadGetStackSize(epicsThreadStackMedium),
                            (EPICSTHREADFUNC)ADnEDEventTaskC,
                            this) == NULL);
  if (status) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s epicsThreadCreate failure for ADnEDEventTask.\n", functionName);
    return;
  }

   //Create the thread that copies the frames for areaDetector plugins 
  status = (epicsThreadCreate("ADnEDFrameTask",
                            epicsThreadPriorityMedium,
                            epicsThreadGetStackSize(epicsThreadStackMedium),
                            (EPICSTHREADFUNC)ADnEDFrameTaskC,
                            this) == NULL);
  if (status) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s epicsThreadCreate failure for ADnEDFrameTask.\n", functionName);
    return;
  }

  std::string channelStr("ADnED Channel");
  std::string monitorStr("ADnED Monitor");
  p_ChannelRequester = (shared_ptr<nEDChannelRequester>)(new nEDChannelRequester(channelStr)); 
  if (!p_ChannelRequester) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s ERROR: Failed to create nEDChannelRequester.\n", functionName);
    return;
  }
  //Create a different monitor requestor for each PVAccess channel that we want to connect to. This is
  //so we can distinguish which channel caused a monitor, which is necessary in the eventHandler function.
  for (int channel=0; channel<s_ADNED_MAX_CHANNELS; ++channel) {
    p_MonitorRequester[channel] = (shared_ptr<nEDMonitorRequester>)(new nEDMonitorRequester(monitorStr, this, channel));  
    if (!p_MonitorRequester[channel]) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s ERROR: Failed to create nEDMonitorRequester. channel: %d\n", functionName, channel);
      return;
    }
  }

  bool paramStatus = true;
  //Initialise any paramLib parameters that need passing up to device support
  paramStatus = ((setIntegerParam(ADnEDResetParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDStartParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDStopParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDEventDebugParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDPulseCounterParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDEventRateParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDSeqIDMissingParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDSeqIDNumMissingParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDBadTimeStampParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setDoubleParam(ADnEDPChargeParam, 0.0) == asynSuccess) && paramStatus);
  paramStatus = ((setDoubleParam(ADnEDPChargeIntParam, 0.0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDNumDetParam, 0) == asynSuccess) && paramStatus);
  //Loop over asyn addresses for detector and channel specific params. We create both here because
  //we are using the Asyn address to handle both the detector and channel params. 
  for (int det=0; det<=s_ADNED_MAX_DETS; det++) {

    //Channel params (0-based)
    paramStatus = ((setIntegerParam(det, ADnEDSeqCounterParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setIntegerParam(det, ADnEDSeqIDParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setStringParam(det, ADnEDPVNameParam, " ") == asynSuccess) && paramStatus);

    //Detector params (1-based) - we just don't use the addr=0 params.
    paramStatus = ((setIntegerParam(det, ADnEDDetPixelNumStartParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setIntegerParam(det, ADnEDDetPixelNumEndParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setIntegerParam(det, ADnEDDetNDArrayStartParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setIntegerParam(det, ADnEDDetNDArrayEndParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setIntegerParam(det, ADnEDDetNDArraySizeParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setIntegerParam(det, ADnEDDetNDArrayTOFStartParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setIntegerParam(det, ADnEDDetNDArrayTOFEndParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setIntegerParam(det, ADnEDDetEventRateParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setIntegerParam(det, ADnEDDetTOFROIStartParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setIntegerParam(det, ADnEDDetTOFROISizeParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setIntegerParam(det, ADnEDDetTOFROIEnableParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setStringParam(det, ADnEDDetTOFTransFileParam, " ") == asynSuccess) && paramStatus);
    paramStatus = ((setStringParam(det, ADnEDDetPixelMapFileParam, " ") == asynSuccess) && paramStatus);
    paramStatus = ((setIntegerParam(det, ADnEDDetPixelMapEnableParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setIntegerParam(det, ADnEDDetTOFTransEnableParam, 0) == asynSuccess) && paramStatus);
    callParamCallbacks(det);
  }
  paramStatus = ((setIntegerParam(ADnEDTOFMaxParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDAllocSpaceParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDAllocSpaceStatusParam, s_ADNED_ALLOC_STATUS_OK) == asynSuccess) && paramStatus);
  paramStatus = ((setStringParam (ADManufacturer, "SNS") == asynSuccess) && paramStatus);
  paramStatus = ((setStringParam (ADModel, "nED areaDetector") == asynSuccess) && paramStatus);

  callParamCallbacks();

  if (!paramStatus) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s Unable To Set Driver Parameters In Constructor.\n", functionName);
  }

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s End Of Constructor.\n", functionName);

  epicsThreadSleep(1);

}

/**
 * Destructor. Should never get here.
 */
ADnED::~ADnED() 
{
  asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "ADnED::~ADnED Called.\n");
}

/**
 * Class function to create a PVAccess client factory
 */
asynStatus ADnED::createFactory()
{
  try {
    printf("Starting ClientFactory::start()\n");
    epics::pvAccess::ClientFactory::start();
  } catch (std::exception &e)  {
    fprintf(stderr, "ERROR: Exception for ClientFactory::start(). Exception: %s\n", e.what());
    return asynError;
  }

  epicsThreadSleep(1);

  return asynSuccess;

}


/** Report status of the driver.
  * Prints details about the detector in us if details>0.
  * It then calls the ADDriver::report() method.
  * \param[in] fp File pointed passed by caller where the output is written to.
  * \param[in] details Controls the level of detail in the report. */
void ADnED::report(FILE *fp, int details)
{
 
  fprintf(fp, "ADnED::report.\n");

  fprintf(fp, "ADnED port=%s\n", this->portName);
  if (details > 0) { 
    fprintf(fp, "ADnED driver details...\n");
  }

  fprintf(fp, "ADnED finished.\n");
  
  //Call the base class method
  asynNDArrayDriver::report(fp, details);

}


/**
 * Reimplementing this function from ADDriver to deal with integer values.
 */ 
asynStatus ADnED::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  asynStatus status = asynSuccess;
  int function = pasynUser->reason;
  int addr = 0;
  int adStatus = 0;
  const char *functionName = "ADnED::writeInt32";
  
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Entry.\n", functionName);

  //Read address (ie. det number).
  status = getAddress(pasynUser, &addr); 
  if (status!=asynSuccess) {
    return(status);
  }

  getIntegerParam(ADStatus, &adStatus);

  if (function == ADnEDResetParam) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Reset.\n", functionName);
    
  } else if (function == ADnEDStartParam) {
    if (value) {
      if ((adStatus == ADStatusIdle) || (adStatus == ADStatusError) || (adStatus == ADStatusAborted)) {
	cout << "Start acqusition." << endl;
	if (clearParams() != asynSuccess) {
	  asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s: ERROR: Failed to run clearParams on start.\n", functionName);
	}
	asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Start Reading Events.\n", functionName);
	cout << "Sending start event" << endl;
	epicsEventSignal(this->m_startEvent);
      }
    } 
  } else if (function == ADnEDStopParam) {
    if (value) {
      if (adStatus == ADStatusAcquire) {
	cout << "Stop acqusition." << endl;
	asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Stop Reading Events.\n", functionName);
	cout << "Sending stop event" << endl;
	epicsEventSignal(this->m_stopEvent);
      }
    }
  } else if (function == ADnEDDetPixelNumStartParam) {
    if (adStatus != ADStatusAcquire) {
      m_dataAlloc = true;
    } else {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s. Cannot configure during acqusition.\n", functionName);
      return asynError;
    }
  } else if (function == ADnEDDetPixelNumEndParam) {
    if (adStatus != ADStatusAcquire) {
      m_dataAlloc = true;
    } else {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s. Cannot configure during acqusition.\n", functionName);
      return asynError;
    }
  } else if (function == ADnEDTOFMaxParam) {
    if (adStatus != ADStatusAcquire) {
      m_dataAlloc = true;
    } else {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s. Cannot configure during acqusition.\n", functionName);
      return asynError;
    }
  } else if (function == ADnEDAllocSpaceParam) {
    if (adStatus != ADStatusAcquire) {
      if (allocArray() != asynSuccess) {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s: ERROR: Failed to allocate array.\n", functionName);
	setIntegerParam(ADnEDAllocSpaceStatusParam, s_ADNED_ALLOC_STATUS_FAIL);
	setStringParam(ADStatusMessage, "allocArray Error");
	setIntegerParam(ADStatus, ADStatusError);
	callParamCallbacks();
	return asynError;
      } 
    } else {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s. Cannot configure during acqusition.\n", functionName);
      return asynError;
    }
  } else if (function == ADnEDNumDetParam) {
    if (adStatus != ADStatusAcquire) {
      if (value > s_ADNED_MAX_DETS) {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
		  "%s Error Setting Number Of Detectors. Max: %d\n", 
		  functionName, s_ADNED_MAX_DETS);
	return asynError;
      }
    } else {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s. Cannot configure during acqusition.\n", functionName);
      return asynError;
    } 
  } else if (function == ADnEDNumChannelsParam) {
      if (adStatus != ADStatusAcquire) {
      if (value > s_ADNED_MAX_CHANNELS) {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
		  "%s Error Setting Number Of PVAccess channels. Max: %d\n", 
		  functionName, s_ADNED_MAX_CHANNELS);
	return asynError;
      }
    } else {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s. Cannot configure during acqusition.\n", functionName);
      return asynError;
    }
  } else if (function == ADnEDDetTOFTransPrintParam) {
    printTofTrans(addr);
  } else if (function == ADnEDDetPixelMapPrintParam) {
    printPixelMap(addr);
  } else if (function == ADnEDDetPixelROISizeXParam) {
    if (value <= 0) {
      value = 1;
    }
  } else if (function == ADnEDDetPixelROIEnableParam) {
    //If we enable Pixel ROI Filter, disable the TOF ROI Filter
    if (value == 1) {
      setIntegerParam(addr, ADnEDDetTOFROIEnableParam, 0);
    }
  } else if (function == ADnEDDetTOFROIEnableParam) {
    //If we enable TOF ROI Filter, disable the Pixel ROI Filter
    if (value == 1) {
      setIntegerParam(addr, ADnEDDetPixelROIEnableParam, 0);
    }
  }

  if (m_dataAlloc) {
    setIntegerParam(ADnEDAllocSpaceStatusParam, s_ADNED_ALLOC_STATUS_REQ);
    callParamCallbacks();
  }

  if (status != asynSuccess) {
    callParamCallbacks(addr);
    return asynError;
  }

  status = (asynStatus) setIntegerParam(addr, function, value);
  if (status!=asynSuccess) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s Error Setting Parameter. Asyn addr: %d, asynUser->reason: %d, value: %d\n", 
              functionName, addr, function, value);
    return(status);
  }

  //Do callbacks so higher layers see any changes 
  callParamCallbacks(addr);

  return status;
}


/**
 * Reimplementing this function from ADDriver to deal with floating point values.
 */ 
asynStatus ADnED::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
  int function = pasynUser->reason;
  int addr = 0;
  asynStatus status = asynSuccess;
  const char *functionName = "ADnED::writeFloat64";
  
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Entry.\n", functionName);
 
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s asynUser->reason: %d, value: %f, addr: %d\n", functionName, function, value, addr);
  //Read address (ie. det number).
  status = getAddress(pasynUser, &addr); 
  if (status!=asynSuccess) {
    return(status);
  }

  if (function == ADnEDFrameUpdatePeriodParam) {
    printf("Setting ADnEDFrameUpdatePeriodParam to %f\n", value);
  }

  if (status != asynSuccess) {
    callParamCallbacks(addr);
    return asynError;
  }

  //Set in param lib so the user sees a readback straight away. We might overwrite this in the 
  //status task, depending on the parameter.
  status = (asynStatus) setDoubleParam(addr, function, value);
  if (status!=asynSuccess) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s Error Setting Parameter. Asyn addr: %d, asynUser->reason: %d, value: %f\n", 
              functionName, addr, function, value);
    return(status);
  }
  
  //Do callbacks so higher layers see any changes 
  callParamCallbacks();
  
  return status;
}



/**
 * Reimplementing this function from asynNDArrayDriver to deal with strings.
 */
asynStatus ADnED::writeOctet(asynUser *pasynUser, const char *value, 
                                    size_t nChars, size_t *nActual)
{
  int function = pasynUser->reason;
  int addr = 0;
  asynStatus status = asynSuccess;
  const char *functionName = "ADnED::writeOctet";

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Entry.\n", functionName);
   
  //Read address (ie. det number).
  status = getAddress(pasynUser, &addr); 
  if (status!=asynSuccess) {
    return(status);
  }
 
  if (function == ADnEDDetTOFTransFileParam) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
	      "%s Set Det %d TOF Transformation File: %s.\n", functionName, addr, value);
    
    if (p_TofTrans[addr]) {
      free(p_TofTrans[addr]);
      p_TofTrans[addr] = NULL;
      m_TofTransSize[addr] = 0;
    }
      
    try {
      ADnEDFile file = ADnEDFile(value);
      if (file.getSize() != 0) { 
	m_TofTransSize[addr] = file.getSize();
	if (p_TofTrans[addr] == NULL) {
	  p_TofTrans[addr] = static_cast<epicsFloat64 *>(calloc(m_TofTransSize[addr], sizeof(epicsFloat64)));
	}
	file.readDataIntoDoubleArray(&p_TofTrans[addr]);
      }
    } catch (std::exception &e) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
		"%s Error Parsing TOF Transformation File. Det: %d. %s\n", functionName, addr, e.what());
    }

  } else if (function == ADnEDDetPixelMapFileParam) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, 
	      "%s Set Det %d Pixel Map File: %s.\n", functionName, addr, value);
    
    if (p_PixelMap[addr]) {
	free(p_PixelMap[addr]);
	p_PixelMap[addr] = NULL;
	m_PixelMapSize[addr] = 0;
      }

    try {
      ADnEDFile file = ADnEDFile(value);
      if (file.getSize() != 0) {
	m_PixelMapSize[addr] = file.getSize();
	if (p_PixelMap[addr] == NULL) {
	  p_PixelMap[addr] = static_cast<epicsUInt32 *>(calloc(m_PixelMapSize[addr], sizeof(epicsUInt32)));
	}
	file.readDataIntoIntArray(&p_PixelMap[addr]);
	if ((status = checkPixelMap(addr)) == asynError) {
	  free(p_PixelMap[addr]);
	  p_PixelMap[addr] = NULL;
	}
      }
    } catch (std::exception &e) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
		"%s Error parsing pixel mapping file. Det: %d. %s\n", functionName, addr, e.what());
      free(p_PixelMap[addr]);
      p_PixelMap[addr] = NULL;
    }
  } else {
    // If this parameter belongs to a base class call its method 
    if (function < ADNED_FIRST_DRIVER_COMMAND) {
      status = asynNDArrayDriver::writeOctet(pasynUser, value, nChars, nActual);
    }
  }
  
  if (status != asynSuccess) {
    callParamCallbacks(addr);
    return asynError;
  }
  
  // Set the parameter in the parameter library. 
  status = (asynStatus)setStringParam(addr, function, (char *)value);
  // Do callbacks so higher layers see any changes 
  status = (asynStatus)callParamCallbacks(addr);
  
  if (status) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s Error Setting Parameter. asynUser->reason: %d\n", 
              functionName, function);
  }
  
  *nActual = nChars;
  return status;
}

/**
 * For debug purposes, print pixel map array to stdout.
 * @param det The detector number (1 based)
 */
void ADnED::printPixelMap(epicsUInt32 det)
{ 
  printf("ADnED::printPixelMap. Det: %d\n", det);
  if ((m_PixelMapSize[det] > 0) && (p_PixelMap[det])) {
    printf("m_PixelMapSize[%d]: %d\n", det, m_PixelMapSize[det]);
    for (epicsUInt32 index=0; index<m_PixelMapSize[det]; ++index) {
      printf("p_PixelMap[%d][%d]: %d\n", det, index, (p_PixelMap[det])[index]);
    }
  } else {
    printf("No pixel mapping loaded.\n");
  }
}

/**
 * For debug purposes, print TOF transformation array to stdout.
 * @param det The detector number (1 based)
 */
void ADnED::printTofTrans(epicsUInt32 det)
{ 
  printf("ADnED::printTofTrans. Det: %d\n", det);
  if ((m_TofTransSize[det] > 0) && (p_TofTrans[det])) {
    printf("m_TofTransSize[%d]: %d\n", det, m_TofTransSize[det]);
    for (epicsUInt32 index=0; index<m_TofTransSize[det]; ++index) {
      printf("p_TofTrans[%d][%d]: %f\n", det, index, (p_TofTrans[det])[index]);
    }
  } else {
    printf("No TOF transformation loaded.\n");
  }
}

/**
 * Check the newly loaded pixel map array. If any of the values are
 * outside the pre-defined range for that detector, then clear the array,
 * and return asynError.
 * @param det The detector number (1 based)
 */
asynStatus ADnED::checkPixelMap(epicsUInt32 det)
{ 
  asynStatus status = asynSuccess;
  const char* functionName = "ADnED::checkPixelMap";

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s\n", functionName);

  int detStartValue = 0;
  int detEndValue = 0;
  getIntegerParam(det, ADnEDDetPixelNumStartParam, &detStartValue);
  getIntegerParam(det, ADnEDDetPixelNumEndParam, &detEndValue);

  if ((m_PixelMapSize[det] > 0) && (p_PixelMap[det])) {
    for (epicsUInt32 index=0; index<m_PixelMapSize[det]; ++index) {
      if (((p_PixelMap[det])[index] < static_cast<epicsUInt32>(detStartValue)) 
	  || ((p_PixelMap[det])[index] > static_cast<epicsUInt32>(detEndValue))) {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
		  "%s Det: %d. Pixel ID %d in mapping array was out of allowed range. Must be between %d and %d.\n", 
		  functionName, det, index, detStartValue, detEndValue);
	memset(p_PixelMap[det], 0, m_PixelMapSize[det]);
	m_PixelMapSize[det] = 0;
	status = asynError;
      }
    }
  } else {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s No pixel mapping loaded.\n", functionName);
    status = asynError;
  }

  return status;
}

/**
 * Event handler callback for monitor
 */
void ADnED::eventHandler(shared_ptr<epics::pvData::PVStructure> const &pv_struct, epicsUInt32 channelID)
{
  int eventDebug = 0;
  bool eventUpdate = false;
  epicsUInt32 seqID = 0;
  bool newPulse = false;
  epicsFloat64 updatePeriod = 0.0;
  int numMissingPackets = 0;
  double timeDiffSecs = 0.0;
  epicsUInt32 eventRate = 0;
  const char* functionName = "ADnED::eventHandler";

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Event Handler. Channel ID %d\n", functionName, channelID);

  /* Get the time and decide if we update the PVs.*/
  lock();
  getDoubleParam(ADnEDEventUpdatePeriodParam, &updatePeriod);
  epicsTimeGetCurrent(&m_nowTime);
  m_nowTimeSecs = m_nowTime.secPastEpoch + (m_nowTime.nsec / 1.e9);
  if ((m_nowTimeSecs - m_lastTimeSecs) < (updatePeriod / 1000.0)) {
    eventUpdate = false;
  } else {
    eventUpdate = true;
    timeDiffSecs = m_nowTimeSecs - m_lastTimeSecs;
    m_lastTimeSecs = m_nowTimeSecs;
  }
  unlock();

  //Sanity check on channelID
  if (channelID >= static_cast<epicsUInt32>(s_ADNED_MAX_CHANNELS)) { //0 based
    if (eventUpdate) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s Invalid channel ID %d.\n", functionName, channelID);
    }
    return;
  }

  int numChan = 0;
  getIntegerParam(ADnEDNumChannelsParam, &numChan);
  if (numChan > s_ADNED_MAX_CHANNELS) {
    numChan = s_ADNED_MAX_CHANNELS;
  }
  int numDet = 0;
  getIntegerParam(ADnEDNumDetParam, &numDet);
  if (numDet > s_ADNED_MAX_DETS) {
    numDet = s_ADNED_MAX_DETS;
  }
  getIntegerParam(ADnEDEventDebugParam, &eventDebug);
  for (int det=1; det<=numDet; det++) {
    getIntegerParam(det, ADnEDDetPixelNumStartParam, &m_detStartValues[det]);
    getIntegerParam(det, ADnEDDetPixelNumEndParam, &m_detEndValues[det]);
    getIntegerParam(det, ADnEDDetNDArrayStartParam, &m_NDArrayStartValues[det]);
    getIntegerParam(det, ADnEDDetNDArrayTOFStartParam, &m_NDArrayTOFStartValues[det]);
    //These two params are used to filter events based on a TOF ROI
    getIntegerParam(det, ADnEDDetTOFROIStartParam, &m_detTOFROIStartValues[det]);
    getIntegerParam(det, ADnEDDetTOFROISizeParam, &m_detTOFROISizeValues[det]);
    getIntegerParam(det, ADnEDDetTOFROIEnableParam, &m_detTOFROIEnabled[det]);
    //Pixel ID mapping
    getIntegerParam(det, ADnEDDetPixelMapEnableParam, &m_detPixelMappingEnabled[det]);
    //TOF Transformation
    getIntegerParam(det, ADnEDDetTOFTransEnableParam, &m_detTOFTransEnabled[det]);
    getDoubleParam(det, ADnEDDetTOFTransOffsetParam, &m_detTOFTransOffset[det]);
    getDoubleParam(det, ADnEDDetTOFTransScaleParam, &m_detTOFTransScale[det]);
    //Pixel ID XY filter
    getIntegerParam(det, ADnEDDetPixelROIStartXParam, &m_detPixelROIStartX[det]);
    getIntegerParam(det, ADnEDDetPixelROIStartYParam, &m_detPixelROIStartY[det]);
    getIntegerParam(det, ADnEDDetPixelROISizeXParam, &m_detPixelROISizeX[det]);
    getIntegerParam(det, ADnEDDetPixelROISizeYParam, &m_detPixelROISizeY[det]);
    getIntegerParam(det, ADnEDDetPixelSizeXParam, &m_detPixelSizeX[det]);
    getIntegerParam(det, ADnEDDetPixelROIEnableParam, &m_detPixelROIEnable[det]);

    if (m_detPixelROISizeX[det] <= 0) {
      if (eventUpdate) {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s Invalid Pixel ROI Size X.\n", functionName);
      }
      return;
    } 
  }

  //Compare timeStamp to last timeStamp to detect a new pulse.
  lock();
  ++m_seqCounter[channelID];  
  try {
    if (!m_PVTimeStamp.attach(pv_struct->getStructureField(ADNED_PV_TIMESTAMP))) {
      if (eventUpdate) {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s Failed to attach PVTimeStamp.\n", functionName);
      }
      unlock();
      return;
    }
    m_PVTimeStamp.get(m_TimeStamp[channelID]);
    //Only use channel ID 0 to integrate the proton charge
    if (channelID == 0) {
      if (m_TimeStampLast[0] != m_TimeStamp[0]) {
	newPulse = true;
      }
    }
    if (m_TimeStampLast[channelID] > m_TimeStamp[channelID]) {
      if (eventUpdate) {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s Backwards timeStamp detected on channel %d.\n", functionName, channelID);
      }
      setIntegerParam(ADnEDBadTimeStampParam, 1);
      unlock();
      return;
    }
    m_TimeStampLast[channelID].put(m_TimeStamp[channelID].getSecondsPastEpoch(), m_TimeStamp[channelID].getNanoseconds());
    
    seqID = static_cast<epicsUInt32>(m_TimeStamp[channelID].getUserTag());
    //Detect missing packets
    if (static_cast<epicsInt32>(m_lastSeqID[channelID]) != -1) {
      if (seqID != m_lastSeqID[channelID]+1) {
	setIntegerParam(ADnEDSeqIDMissingParam, m_lastSeqID[channelID]+1);
	getIntegerParam(ADnEDSeqIDNumMissingParam, &numMissingPackets);
	setIntegerParam(ADnEDSeqIDNumMissingParam, numMissingPackets+(seqID-m_lastSeqID[channelID]+1));
	if (eventUpdate) {
	  asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s: Missing seq ID numbers on channel %d.\n", functionName, channelID);
	}
      }
    }
    m_lastSeqID[channelID] = seqID;
  } catch (std::exception &e)  {
    if (eventUpdate) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
		"%s: Failed to deal with time stamp objects. Exception: %s\n", 
		functionName, e.what());
    }
    unlock();
    return;
  }
  unlock();
  
  epics::pvData::PVDoublePtr pChargePtr = pv_struct->getDoubleField(ADNED_PV_PCHARGE);
  if (!pChargePtr) {
    if (eventUpdate) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s No valid pCharge found.\n", functionName);
    }
    return;
  }

  if ((p_Data == NULL) || (m_dataMaxSize == 0)) {
    return;
  }

  epics::pvData::PVUIntArrayPtr pixelsPtr = pv_struct->getSubField<epics::pvData::PVUIntArray>(ADNED_PV_PIXELS);
  epics::pvData::PVUIntArrayPtr tofPtr = pv_struct->getSubField<epics::pvData::PVUIntArray>(ADNED_PV_TOF);
  if (pixelsPtr && tofPtr) {
    
    epics::pvData::uint32 pixelsLength = pixelsPtr->getLength();
    epics::pvData::shared_vector<const epics::pvData::uint32> pixelsData = pixelsPtr->view();

    epics::pvData::uint32 tofLength = tofPtr->getLength();
    epics::pvData::shared_vector<const epics::pvData::uint32> tofData = tofPtr->view();

    if (pixelsLength != tofLength) {
      if (eventUpdate) {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s pixelsLength != tofLength.\n", functionName);
      }
      return;
    }

    //Count events to calculate event rate.
    m_eventsSinceLastUpdate += pixelsLength;

    lock();

    int mappedPixelIndex = 0;
    int calcIndex = 0; 
    epicsFloat64 tof = 0.0;
    epicsUInt32 tofInt = 0;
    int offset = 0;
    for (size_t i=0; i<pixelsLength; ++i) {
      for (int det=1; det<=numDet; det++) {
	
	//Dtermine if this raw pixel ID is in this DET range.
	if ((pixelsData[i] >= static_cast<epicsUInt32>(m_detStartValues[det])) 
	    && (pixelsData[i] <= static_cast<epicsUInt32>(m_detEndValues[det]))) {
	  
	  tof = static_cast<epicsFloat64>(tofData[i]);
	  //If enabled, do TOF tranformation (to d-space for example).
	  if (m_detTOFTransEnabled[det]) {
	    if (p_TofTrans[det]) {
	      tof = tofData[i] * (p_TofTrans[det])[pixelsData[i]];
	      //Apply scale and offset
	      tof = (tof * m_detTOFTransScale[det]) + m_detTOFTransOffset[det];
	    }
	  }

	  mappedPixelIndex = pixelsData[i];
	  //Do pixel ID mapping if enabled
	  if (m_detPixelMappingEnabled[det]) {
	    if ((m_PixelMapSize[det] > 0) && (p_PixelMap[det])) {
	      mappedPixelIndex = (p_PixelMap[det])[pixelsData[i]];
	    }
	  }
	  
	  //Integrate Pixel ID Data, optionally filtering on TOF ROI filter.
	  if (m_detTOFROIEnabled[det]) {
	    if ((tof >= static_cast<epicsFloat64>(m_detTOFROIStartValues[det])) 
		&& (tof <= static_cast<epicsFloat64>(m_detTOFROIStartValues[det] + m_detTOFROISizeValues[det]))) {
	      offset = mappedPixelIndex-m_detStartValues[det];
	      p_Data[m_NDArrayStartValues[det]+offset]++;
	    }
	  } else { //No TOF ROI filter enabled
	    offset = mappedPixelIndex-m_detStartValues[det];
	    p_Data[m_NDArrayStartValues[det]+offset]++;
	  }

	  //Integrate TOF/D-Space, optionally filtering on Pixel ID X/Y ROI
	  tofInt = static_cast<epicsUInt32>(floor(tof));
	  if (tof <= m_tofMax) {
	    if (m_detPixelROIEnable[det]) {
	      //If pixel mapping is not enabled, this is meaningless, so just integrate as normal.
	      if (!m_detPixelMappingEnabled[det]) { 
		p_Data[m_NDArrayTOFStartValues[det]+tofInt]++;
	      } else {
		//Only integrate TOF if we are inside pixel ID XY ROI.
		//ROI is assumed to start from 0,0 (not from whatever is the pixel ID range. So we need to offset.
		calcIndex = mappedPixelIndex - m_detStartValues[det];
		if (((calcIndex % m_detPixelSizeX[det]) >= m_detPixelROIStartX[det]) && 
		    ((calcIndex % m_detPixelSizeX[det]) <= (m_detPixelROIStartX[det] + m_detPixelROISizeX[det]))) {
		  if ((calcIndex >= (m_detPixelROIStartY[det] * m_detPixelSizeX[det])) &&
		      ((calcIndex <= ((m_detPixelROIStartY[det] + m_detPixelROISizeY[det]) 
				      * m_detPixelSizeX[det]) + m_detPixelSizeX[det]))) {
		    p_Data[m_NDArrayTOFStartValues[det]+tofInt]++;
		  }
		}
	      }
	    } else {
	      p_Data[m_NDArrayTOFStartValues[det]+tofInt]++;
	    }
	  }

	  //Count events to calculate event rate
	  m_detEventsSinceLastUpdate[det]++;
	}

      }
      }
    
    if (newPulse) {
      m_pChargeInt += pChargePtr->get();
      ++m_pulseCounter;
    }    

    //Update params at slower rate
    if (eventUpdate) {
      //Channel params
      setIntegerParam(channelID, ADnEDSeqCounterParam, m_seqCounter[channelID]);
      setIntegerParam(channelID, ADnEDSeqIDParam, seqID);
      //Other params
      setIntegerParam(ADnEDPulseCounterParam, m_pulseCounter);
      eventRate = static_cast<epicsUInt32>(floor(m_eventsSinceLastUpdate/timeDiffSecs));
      setIntegerParam(ADnEDEventRateParam, eventRate);
      m_eventsSinceLastUpdate = 0;
      for (int det=1; det<=numDet; det++) {
	eventRate = static_cast<epicsUInt32>(floor(m_detEventsSinceLastUpdate[det]/timeDiffSecs));
	setIntegerParam(det, ADnEDDetEventRateParam, eventRate);
	m_detEventsSinceLastUpdate[det] = 0;
      }
      setDoubleParam(ADnEDPChargeParam, pChargePtr->get());
      setDoubleParam(ADnEDPChargeIntParam, m_pChargeInt);
      //Callbacks for channel and det related parameters (so start at 0 rather than 1)
      for (int det=0; det<=numDet; det++) {
	callParamCallbacks(det);
      }
      callParamCallbacks();
    }

    unlock();
  }

  if (eventDebug != 0) {
    cout << "channelID: " << channelID << endl;
    pv_struct->dumpValue(cout);
    cout << endl;
  }
  
}

/**
 * Allocate local storage for event handler. This is only done when any of the detector
 * sizes or TOF range has changed. It then publishes the start and end points of
 * the detector and TOF data in the NDArray object. 
 */
asynStatus ADnED::allocArray(void) 
{
  asynStatus status = asynSuccess;
  const char* functionName = "ADnED::allocArray";
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s", functionName);

  if (m_dataAlloc != true) {
    //Nothing has changed
    return asynSuccess;
  }

  int numDet = 0;
  int detStart = 0;
  int detEnd = 0;
  int tofMax = 0;
  getIntegerParam(ADnEDNumDetParam, &numDet);
  getIntegerParam(ADnEDTOFMaxParam, &tofMax);
  m_tofMax = tofMax;

  if (numDet == 0) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s No detectors.\n", functionName);
    return asynError;
  }

  m_dataMaxSize = 0;
  int detSize = 0;

  for (int det=1; det<=numDet; det++) {
    
    getIntegerParam(det, ADnEDDetPixelNumStartParam, &detStart);
    getIntegerParam(det, ADnEDDetPixelNumEndParam, &detEnd);
    
    printf("ADnED::allocArray: det: %d, detStart: %d, detEnd: %d, tofMax: %d\n", 
	 det, detStart, detEnd, tofMax);

    //Calculate sizes and do sanity checks
    if (detStart <= detEnd) {
      detSize = detEnd-detStart+1;
    } else {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s detStart > detEnd.\n", functionName);
      setIntegerParam(ADnEDAllocSpaceStatusParam, s_ADNED_ALLOC_STATUS_FAIL);
      return asynError;
    }
    
    printf("ADnED::allocArray: m_dataMaxSize: %d\n", m_dataMaxSize);
    printf("ADnED::allocArray: detSize: %d\n", detSize);

    setIntegerParam(det, ADnEDDetNDArrayStartParam, m_dataMaxSize);
    setIntegerParam(det, ADnEDDetNDArrayEndParam, m_dataMaxSize+detSize-1);
    setIntegerParam(det, ADnEDDetNDArraySizeParam, detSize);
    callParamCallbacks(det);
    
    m_dataMaxSize += detSize;
    
  }
  
  printf("ADnED::allocArray: final m_dataMaxSize: %d\n", m_dataMaxSize);
  printf("ADnED::allocArray: TOF Size: %d\n", numDet * (tofMax+1));

  epicsUInt32 tofStart = m_dataMaxSize;
  epicsUInt32 tofEnd = 0;
  for (int det=1; det<=numDet; det++) {
    tofEnd = tofStart+tofMax;
    printf("ADnED::allocArray: det %d, TOF start: %d, TOF end: %d\n", det, tofStart, tofEnd);
    setIntegerParam(det, ADnEDDetNDArrayTOFStartParam, tofStart);
    setIntegerParam(det, ADnEDDetNDArrayTOFEndParam, tofEnd);
    tofStart = tofEnd+1;
    callParamCallbacks(det);
  }

  if (p_Data) {
    free(p_Data);
    p_Data = NULL;
  }
  
  if (!p_Data) {
    if (m_dataMaxSize != 0) {
      m_bufferMaxSize = m_dataMaxSize+(numDet * (tofMax+1));
      p_Data = static_cast<epicsUInt32*>(calloc(m_bufferMaxSize, sizeof(epicsUInt32)));
    } else {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s Not allocating zero sized array.\n", functionName);
      setIntegerParam(ADnEDAllocSpaceStatusParam, s_ADNED_ALLOC_STATUS_FAIL);
      status = asynError;
    }
  } else {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s pData already allocated.\n", functionName);
    setIntegerParam(ADnEDAllocSpaceStatusParam, s_ADNED_ALLOC_STATUS_FAIL);
    status = asynError;
  }
  if (!p_Data) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s pData failed to allocate.\n", functionName);
    setIntegerParam(ADnEDAllocSpaceStatusParam, s_ADNED_ALLOC_STATUS_FAIL);
    status = asynError;
  }

  if (status == asynSuccess) {
    m_dataAlloc = false;
    setIntegerParam(ADnEDAllocSpaceStatusParam, s_ADNED_ALLOC_STATUS_OK);
  }
  
  return status;
}

/**
 * Clear parameters and data members on a acqusition start.
 */
asynStatus ADnED::clearParams(void)
{
  bool status = true;
  const char* functionName = "ADnED::clearParams";

  status = ((setIntegerParam(ADnEDPulseCounterParam, 0) == asynSuccess) && status);
  status = ((setDoubleParam(ADnEDPChargeParam, 0.0) == asynSuccess) && status);
  status = ((setDoubleParam(ADnEDPChargeIntParam, 0.0) == asynSuccess) && status);
  status = ((setIntegerParam(ADnEDSeqIDMissingParam, 0) == asynSuccess) && status);
  status = ((setIntegerParam(ADnEDSeqIDNumMissingParam, 0) == asynSuccess) && status);
  status = ((setIntegerParam(ADnEDBadTimeStampParam, 0) == asynSuccess) && status);

  m_pChargeInt = 0.0;
  m_pulseCounter = 0;

  for (int chan=0; chan<s_ADNED_MAX_CHANNELS; ++chan) {
    status = ((setIntegerParam(chan, ADnEDSeqCounterParam, 0) == asynSuccess) && status);
    status = ((setIntegerParam(chan, ADnEDSeqIDParam, 0) == asynSuccess) && status);
    m_seqCounter[chan] = 0;
    m_lastSeqID[chan] = -1;  
    m_TimeStamp[chan].put(0,0);
    m_TimeStampLast[chan].put(0,0);
    callParamCallbacks(chan);
  }

  if (!status) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s: ERROR: Failed to clear params.\n", functionName);
    return asynError;
  }
  
  return asynSuccess;
}


/**
 * Event readout task.
 */
void ADnED::eventTask(void)
{
  epicsEventWaitStatus eventStatus;
  epicsFloat64 timeout = 0.001;
  bool acquire = 0;
  bool error = true;
  int numChannels = 0;
  char pvName[s_ADNED_MAX_CHANNELS][s_ADNED_MAX_STRING_SIZE] = {{0}};
  const char* functionName = "ADnED::eventTask";
 
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Started Event Thread.\n", functionName);

  setStringParam(ADStatusMessage, "Startup");

  while (1) {

    //Wait for a stop event, with a short timeout, to catch any that were done after last one.
    eventStatus = epicsEventWaitWithTimeout(m_stopEvent, timeout);          
    if (eventStatus == epicsEventWaitOK) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Got Stop Event Before Start Event.\n", functionName);
    }

    lock();
    setIntegerParam(ADAcquire, 0);
    if (!error) {
      setStringParam(ADStatusMessage, "Idle");
    }
    callParamCallbacks();
    unlock();

    eventStatus = epicsEventWait(m_startEvent);          
    if (eventStatus == epicsEventWaitOK) {
      printf("Got start event.");
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Got Start Event.\n", functionName);
      acquire = true;
      error = false;
      lock();

      //Get the number of required active PVAccess channels
      numChannels = 0;
      getIntegerParam(ADnEDNumChannelsParam, &numChannels);
      if (numChannels > s_ADNED_MAX_CHANNELS) {
	numChannels = s_ADNED_MAX_CHANNELS;
      }
	
      if (allocArray() != asynSuccess) {
	setStringParam(ADStatusMessage, "allocArray Error");
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s: ERROR: Failed to allocate array.\n", functionName);
	acquire = false;
	error = true;
      } else {
      
	//Clear arrays at start of acquire every time.
	if (p_Data != NULL) {
	  memset(p_Data, 0, m_bufferMaxSize*sizeof(epicsUInt32));
	}
      
	setIntegerParam(ADStatus, ADStatusAcquire);
	setStringParam(ADStatusMessage, "Acquiring Events");
	// Start frame thread
	cout << "Send start frame" << endl;
	epicsEventSignal(this->m_startFrame);
	callParamCallbacks();

	//Read the PV names
	for (int chan=0; chan<numChannels; ++chan) {
	  getStringParam(chan, ADnEDPVNameParam, sizeof(pvName[chan]), pvName[chan]);
	}

	//Connect channel here      
	if (!error) {
	  try {
	    for (int channel=0; channel<numChannels; ++channel) {
	      if (pvName[channel][0] != '0') {
		if (setupChannelMonitor(pvName[channel], channel) != asynSuccess) {
		  throw std::runtime_error("Unknown error from setupChannelMonitor.");
		}
	      }
	    }
	  } catch (std::exception &e)  {
	    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
		      "%s: ERROR: Problem creating monitor. Exception: %s\n", 
		      functionName, e.what());
	    setStringParam(ADStatusMessage, "Failed To Connect To PV.");
	    setIntegerParam(ADStatus, ADStatusError);
	    acquire = false;
	    error = true;
	  }
	}

	if (!error) {
	  for (int channel=0; channel<numChannels; ++channel) {
	    if (p_Monitor[channel]) {
	      p_Monitor[channel]->start();
	    }
	  }
	} else {
	  cout << "Send Stop Frame" << endl;
	  epicsEventSignal(this->m_stopFrame);    
	}	
      
      }
    } // end of if (eventStatus == epicsEventWaitOK)
    
    //If we failed to connect or setup, notify error.
    if (error) {
      setIntegerParam(ADStatus, ADStatusError);    
      acquire = false;
    }

    //Complete Start callback
    callParamCallbacks();
    setIntegerParam(ADnEDStartParam, 0);
    callParamCallbacks();
    unlock();
    
    while (acquire) {
      //Wait for a stop event, with a short timeout.
      //eventStatus = epicsEventWaitWithTimeout(m_stopEvent, timeout);      
      eventStatus = epicsEventWaitWithTimeout(m_stopEvent, 0.1);      
      if (eventStatus == epicsEventWaitOK) {
	cout << "Got stop event" << endl;
        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Got Stop Event.\n", functionName);
        acquire = false;
      }

      if (!acquire) {
	lock();
	setIntegerParam(ADStatus, ADStatusIdle);
	cout << "Send Stop Frame" << endl;
	epicsEventSignal(this->m_stopFrame);
	unlock();
      }
      
    } // End of while(acquire)

    //Stop monitor here
    for (int channel=0; channel<numChannels; ++channel) {
      if (p_Monitor[channel]) {
	p_Monitor[channel]->stop();
      }
    }

    //Complete Stop callback
    callParamCallbacks();
    setIntegerParam(ADnEDStopParam, 0);
    callParamCallbacks();
    
  } // End of while(1)

  asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s: ERROR: Exiting ADnEDEventTask main loop.\n", functionName);

}


//Global C utility functions to tie in with EPICS
static void ADnEDEventTaskC(void *drvPvt)
{
  ADnED *pPvt = (ADnED *)drvPvt;
  
  pPvt->eventTask();
}

/**
 * Set up a PVAccess channel and a associated monitor.
 * This function may throw an exception.
 * @param pvName The PV name to use.
 * @param channel The PVAccess channel number, because we can support more than one channel monitor.
 * @return asynStatus If we complete normally, asynSuccess will be returned
 */
asynStatus ADnED::setupChannelMonitor(const char *pvName, int channel)
{
  bool connectStatus = false;
  bool newMonitor = false;
  const char* functionName = "ADnED::setupChannelMonitor";
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s. PV Name: %s, Channel: %d", functionName, pvName, channel);
  
  cout << "PV Name: " << pvName << "  Channel: " << channel << endl;

  if (channel >= s_ADNED_MAX_CHANNELS) {
    throw std::runtime_error("channel >= s_ADNED_MAX_CHANNELS.");
  }

  if ((!p_ChannelRequester) || (!p_MonitorRequester)) {
    throw std::runtime_error("No Channel or Monitor Requester.");
  }

  if (!p_ChannelProvider) {
    p_ChannelProvider = epics::pvAccess::getChannelProviderRegistry()->getProvider(ADNED_PV_PROVIDER);
    if (!p_ChannelProvider) {
      throw std::runtime_error("No Channel Provider.");
    }
  }
  
  if (!p_Channel[channel]) {
    p_Channel[channel] = (shared_ptr<epics::pvAccess::Channel>)
      (p_ChannelProvider->createChannel(pvName, p_ChannelRequester, ADNED_PV_PRIORITY));
    connectStatus = p_ChannelRequester->waitUntilConnected(ADNED_PV_TIMEOUT);
  } else {
    std::string channelName(p_Channel[channel]->getChannelName());
    if (channelName != pvName) {
      p_Channel[channel]->destroy();
      p_Channel[channel] = (shared_ptr<epics::pvAccess::Channel>)
	(p_ChannelProvider->createChannel(pvName, p_ChannelRequester, ADNED_PV_PRIORITY));
      connectStatus = p_ChannelRequester->waitUntilConnected(ADNED_PV_TIMEOUT);
      newMonitor = true;
    }
    connectStatus = p_Channel[channel]->isConnected();
  }

  if (!connectStatus) {
    throw std::runtime_error("Timeout connecting to PV");
  }
  
  if ((!p_Monitor[channel]) || (newMonitor)) {
    if (p_Monitor[channel]) {
      p_Monitor[channel]->destroy();
    }
    shared_ptr<epics::pvData::PVStructure> pvRequest = epics::pvData::CreateRequest::create()->createRequest(ADNED_PV_REQUEST);
    p_Monitor[channel] = p_Channel[channel]->createMonitor(p_MonitorRequester[channel], pvRequest);
    p_MonitorRequester[channel]->waitUntilConnected(ADNED_PV_TIMEOUT);
  }
  
  return asynSuccess;
}


/**
 * Frame readout task.
 */
void ADnED::frameTask(void)
{
  epicsEventWaitStatus eventStatus;
  epicsFloat64 timeout = 0.001;
  bool acquire = false;
  int arrayCounter = 0;
  int arrayCallbacks = 0;
  epicsFloat64 updatePeriod = 0.0;
  epicsTimeStamp nowTime;
  NDArray *pNDArray = NULL;
  const char* functionName = "ADnED::frameTask";
 
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Started Frame Thread.\n", functionName);

  while (1) {

    //Wait for a stop event, with a short timeout, to catch any that were done during last read.
    eventStatus = epicsEventWaitWithTimeout(m_stopFrame, 0.01);          
    if (eventStatus == epicsEventWaitOK) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Got Stop Frame Event Before Start Frame Event.\n", functionName);
    }
    
    callParamCallbacks();

    eventStatus = epicsEventWait(m_startFrame);          
    if (eventStatus == epicsEventWaitOK) {
      cout << "Got start frame" << endl;
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Got Start Frame Event.\n", functionName);
      acquire = true;
      arrayCounter = 0;
      lock();
      setIntegerParam(NDArrayCounter, arrayCounter);
      //setIntegerParam(ADnEDFrameStatus, ADStatusAcquire);
      //setStringParam(ADnEDFrameStatusMessage, "Acquiring Frames");
      callParamCallbacks();
      unlock();
    }

    while (acquire) {

      //Get the update period.
      getDoubleParam(ADnEDFrameUpdatePeriodParam, &updatePeriod);
      timeout = updatePeriod / 1000.0;
      
      //Wait for a stop event
      eventStatus = epicsEventWaitWithTimeout(m_stopFrame, timeout);
      if (eventStatus == epicsEventWaitOK) {
	cout << "Got Stop Frame" << endl;
        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Got Stop Frame Event.\n", functionName);
        acquire = false;
      }

      if (acquire) {
	//Copy p_Data here into an NDArray of the same size. Do array callbacks.
	getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
	if (arrayCallbacks) {
	  ++arrayCounter;
	  size_t dims[1] = {m_bufferMaxSize};
	  if ((pNDArray = this->pNDArrayPool->alloc(1, dims, NDUInt32, 0, NULL)) == NULL) {
	    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s: ERROR: pNDArrayPool->alloc failed.\n", functionName);
	  } else {
	    epicsTimeGetCurrent(&nowTime);
	    pNDArray->uniqueId = arrayCounter;
	    pNDArray->timeStamp = nowTime.secPastEpoch + nowTime.nsec / 1.e9;
	    pNDArray->pAttributeList->add("TIMESTAMP", "Host Timestamp", NDAttrFloat64, &(pNDArray->timeStamp));
	    lock();
	    memcpy(pNDArray->pData, p_Data, m_bufferMaxSize * sizeof(epicsUInt32));
	    unlock();
	    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s: Calling NDArray callback\n", functionName);
	    doCallbacksGenericPointer(pNDArray, NDArrayData, 0);
	  }

	  lock();
	  //Free the NDArray 
	  pNDArray->release();
	  setIntegerParam(NDArrayCounter, arrayCounter);	  
	  callParamCallbacks();
	  unlock();
	}

      }
      
    } // End of while(acquire)


  } // End of while(1)

  asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s: ERROR: Exiting ADnEDFrameTask main loop.\n", functionName);

}


//Global C utility functions to tie in with EPICS
static void ADnEDFrameTaskC(void *drvPvt)
{
  ADnED *pPvt = (ADnED *)drvPvt;
  
  pPvt->frameTask();
}





/*************************************************************************************/
/** The following functions have C linkage, and can be called directly or from iocsh */

extern "C" {

/**
 * Config function for IOC shell. It instantiates an instance of the driver.
 * @param portName The Asyn port name to use
 * @param maxBuffers Used by asynPortDriver (set to -1 for unlimited)
 * @param maxMemory Used by asynPortDriver (set to -1 for unlimited)
 * @param debug This debug flag is passed to xsp3_config in the Xspress API (0 or 1)
 */
  asynStatus ADnEDConfig(const char *portName, int maxBuffers, size_t maxMemory, int debug)
  {
    asynStatus status = asynSuccess;
    
    /*Instantiate class.*/
    try {
      new ADnED(portName, maxBuffers, maxMemory, debug);
    } catch (...) {
      cout << "Unknown exception caught when trying to construct ADnED." << endl;
      status = asynError;
    }
    
    return(status);
  }

/**
 * Config function for IOC shell. It instantiates a PVAccess client factory for this IOC.
 */
  asynStatus ADnEDCreateFactory()
  { 
    /*Instantiate factory.*/
    return ADnED::createFactory();
  }


   
  /* Code for iocsh registration */
  
  /* ADnEDConfig */
  static const iocshArg ADnEDConfigArg0 = {"Port name", iocshArgString};
  static const iocshArg ADnEDConfigArg1 = {"Max Buffers", iocshArgInt};
  static const iocshArg ADnEDConfigArg2 = {"Max Memory", iocshArgInt};
  static const iocshArg ADnEDConfigArg3 = {"Debug", iocshArgInt};
  static const iocshArg * const ADnEDConfigArgs[] =  {&ADnEDConfigArg0,
                                                         &ADnEDConfigArg1,
                                                         &ADnEDConfigArg2,
                                                         &ADnEDConfigArg3};
  
  static const iocshFuncDef configADnED = {"ADnEDConfig", 4, ADnEDConfigArgs};
  static void configADnEDCallFunc(const iocshArgBuf *args)
  {
    ADnEDConfig(args[0].sval, args[1].ival, args[2].ival, args[3].ival);
  }

  static const iocshFuncDef configADnEDCreateFactory = {"ADnEDCreateFactory", 0, NULL};
  static void configADnEDCreateFactoryCallFunc(const iocshArgBuf *args)
  {
    ADnEDCreateFactory();
  }

  
  static void ADnEDRegister(void)
  {
    iocshRegister(&configADnED, configADnEDCallFunc);
    iocshRegister(&configADnEDCreateFactory, configADnEDCreateFactoryCallFunc);
  }
  
  epicsExportRegistrar(ADnEDRegister);

} // extern "C"


/****************************************************************************************/
