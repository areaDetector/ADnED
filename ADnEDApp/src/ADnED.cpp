
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
#include <pv/pvData.h>

using std::cout;
using std::cerr;
using std::endl;

using std::tr1::shared_ptr;
using nEDChannel::nEDChannelRequester;
using nEDChannel::nEDMonitorRequester;

//Not sure how we want to handle these yet, so will leave them as #defines for now.
#define ADNED_PV_TIMEOUT 2.0
#define ADNED_PV_PRIORITY epics::pvAccess::ChannelProvider::PRIORITY_DEFAULT
#define ADNED_PV_REQUEST "record[queueSize=100]field()"
#define ADNED_PV_PIXELS "pixel.value" 
#define ADNED_PV_TIMESTAMP "timeStamp"
#define ADNED_PV_SEQ "timeStamp.userTag" 
#define ADNED_PV_PCHARGE "protonCharge.value"

//Definitions of static class data members
const epicsInt32 ADnED::s_ADNED_MAX_STRING_SIZE = ADNED_MAX_STRING_SIZE;
const epicsInt32 ADnED::s_ADNED_MAX_DETS = 8;

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
  createParam(ADnEDEventDebugParamString,         asynParamInt32,       &ADnEDEventDebugParam);
  createParam(ADnEDSeqCounterParamString,       asynParamInt32,       &ADnEDSeqCounterParam);
  createParam(ADnEDPulseCounterParamString,       asynParamInt32,       &ADnEDPulseCounterParam);
  createParam(ADnEDSeqIDParamString,            asynParamInt32,       &ADnEDSeqIDParam);
  createParam(ADnEDSeqIDMissingParamString,            asynParamInt32,       &ADnEDSeqIDMissingParam);
  createParam(ADnEDSeqIDNumMissingParamString,            asynParamInt32,       &ADnEDSeqIDNumMissingParam);
  createParam(ADnEDBadTimeStampParamString,            asynParamInt32,       &ADnEDBadTimeStampParam);
  createParam(ADnEDPChargeParamString,            asynParamFloat64,     &ADnEDPChargeParam);
  createParam(ADnEDPChargeIntParamString,            asynParamFloat64,     &ADnEDPChargeIntParam);
  createParam(ADnEDEventUpdatePeriodParamString,  asynParamFloat64,     &ADnEDEventUpdatePeriodParam);
  createParam(ADnEDPVNameParamString,          asynParamOctet,       &ADnEDPVNameParam);
  createParam(ADnEDNumDetParamString,             asynParamInt32,       &ADnEDNumDetParam);
  createParam(ADnEDDetPixelNumStartParamString,  asynParamInt32,       &ADnEDDetPixelNumStartParam);
  createParam(ADnEDDetPixelNumEndParamString,    asynParamInt32,       &ADnEDDetPixelNumEndParam);
  createParam(ADnEDDetNDArrayStartParamString,    asynParamInt32,       &ADnEDDetNDArrayStartParam);
  createParam(ADnEDDetNDArrayEndParamString,    asynParamInt32,       &ADnEDDetNDArrayEndParam);
  createParam(ADnEDDetNDArraySizeParamString,    asynParamInt32,       &ADnEDDetNDArraySizeParam);
  createParam(ADnEDDetNDArrayTOFStartParamString,    asynParamInt32,       &ADnEDDetNDArrayTOFStartParam);
  createParam(ADnEDDetNDArrayTOFEndParamString,    asynParamInt32,       &ADnEDDetNDArrayTOFEndParam);
  createParam(ADnEDTOFMaxParamString,             asynParamInt32,       &ADnEDTOFMaxParam);
  createParam(ADnEDLastParamString,               asynParamInt32,       &ADnEDLastParam);

  //Initialize non static, non const, data members
  m_acquiring = 0;
  m_seqCounter = 0;
  m_pulseCounter = 0;
  m_pChargeInt = 0.0;
  m_nowTimeSecs = 0.0;
  m_lastTimeSecs = 0.0;
  p_Data = NULL;
  m_dataAlloc = true;
  m_dataMaxSize = 0;
  m_TimeStamp.put(0,0);
  m_TimeStampLast.put(0,0);

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

  bool paramStatus = true;
  //Initialise any paramLib parameters that need passing up to device support
  paramStatus = ((setIntegerParam(ADnEDResetParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDEventDebugParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDSeqCounterParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDPulseCounterParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDSeqIDParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDSeqIDMissingParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDSeqIDNumMissingParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDBadTimeStampParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setDoubleParam(ADnEDPChargeParam, 0.0) == asynSuccess) && paramStatus);
  paramStatus = ((setDoubleParam(ADnEDPChargeIntParam, 0.0) == asynSuccess) && paramStatus);
  paramStatus = ((setStringParam(ADnEDPVNameParam, " ") == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDNumDetParam, 0) == asynSuccess) && paramStatus);
  //Loop over asyn addresses for detector specific params
  for (int det=0; det<=s_ADNED_MAX_DETS; det++) {
    paramStatus = ((setIntegerParam(det, ADnEDDetPixelNumStartParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setIntegerParam(det, ADnEDDetPixelNumEndParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setIntegerParam(det, ADnEDDetNDArrayStartParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setIntegerParam(det, ADnEDDetNDArrayEndParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setIntegerParam(det, ADnEDDetNDArraySizeParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setIntegerParam(det, ADnEDDetNDArrayTOFStartParam, 0) == asynSuccess) && paramStatus);
    paramStatus = ((setIntegerParam(det, ADnEDDetNDArrayTOFEndParam, 0) == asynSuccess) && paramStatus);
  }
  paramStatus = ((setIntegerParam(ADnEDTOFMaxParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setStringParam (ADManufacturer, "SNS") == asynSuccess) && paramStatus);
  paramStatus = ((setStringParam (ADModel, "nED areaDetector") == asynSuccess) && paramStatus);

  callParamCallbacks();

  if (!paramStatus) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s Unable To Set Driver Parameters In Constructor.\n", functionName);
  }

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s End Of Constructor.\n", functionName);

}

ADnED::~ADnED() 
{
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "ADnED::~ADnED Called.\n");
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
    
  } else if (function == ADAcquire) {
    if (value) {
      if (adStatus != ADStatusAcquire) {
	cout << "Start acqusition." << endl;
	m_seqCounter = 0;
	asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Start Reading Events.\n", functionName);
	epicsEventSignal(this->m_startEvent);
      }
    } else {
      	cout << "Stop acqusition." << endl;
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Stop Reading Events.\n", functionName);
      epicsEventSignal(this->m_stopEvent);
    }
  } else if (function == ADnEDDetPixelNumStartParam) {
    m_dataAlloc = true;
  } else if (function == ADnEDDetPixelNumEndParam) {
    m_dataAlloc = true;
  } else if (function == ADnEDTOFMaxParam) {
    m_dataAlloc = true;
  } else if (function == ADnEDNumDetParam) {
    if (value > s_ADNED_MAX_DETS) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
		"%s Error Setting Number Of Detectors. Max: %d\n", 
		functionName, s_ADNED_MAX_DETS);
      return asynError;
    }
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
    asynStatus status = asynSuccess;
    const char *functionName = "ADnED::writeOctet";

    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Entry.\n", functionName);
    
    //if (function == xsp3ConfigPathParam) {
    //  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Set Config Path Param.\n", functionName);
    //} else if (function == xsp3ConfigSavePathParam) {
    //  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Set Config Save Path Param.\n", functionName);
    //  status = checkSaveDir(value);
    //} else {
    //    // If this parameter belongs to a base class call its method 
    //  if (function < XSP3_FIRST_DRIVER_COMMAND) {
    //    status = asynNDArrayDriver::writeOctet(pasynUser, value, nChars, nActual);
    //  }
    //}

    if (status != asynSuccess) {
      callParamCallbacks();
      return asynError;
    }
    
    // Set the parameter in the parameter library. 
    status = (asynStatus)setStringParam(function, (char *)value);
    // Do callbacks so higher layers see any changes 
    status = (asynStatus)callParamCallbacks();
    
    if (status) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s Error Setting Parameter. asynUser->reason: %d\n", 
              functionName, function);
    }

    *nActual = nChars;
    return status;
}

/**
 * Event handler callback for monitor
 */
void ADnED::eventHandler(shared_ptr<epics::pvData::PVStructure> const &pv_struct)
{
  
  int eventDebug = 0;
  bool eventUpdate = false;
  epicsUInt32 seqID = 0;
  bool newPulse = false;
  static epicsUInt32 lastSeqID;
  epicsFloat64 updatePeriod = 0.0;
  int numMissingPackets = 0;
  const char* functionName = "ADnED::eventHandler";
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Event Handler.\n", functionName);

  /* Get the time and decide if we update the PVs.*/
  getDoubleParam(ADnEDEventUpdatePeriodParam, &updatePeriod);
  epicsTimeGetCurrent(&m_nowTime);
  m_nowTimeSecs = m_nowTime.secPastEpoch + (m_nowTime.nsec / 1.e9);
  if ((m_nowTimeSecs - m_lastTimeSecs) < (updatePeriod / 1000.0)) {
    eventUpdate = false;
  } else {
    eventUpdate = true;
    m_lastTimeSecs = m_nowTimeSecs;
  }
  
  int detStartValues[s_ADNED_MAX_DETS+1] = {0};
  int detEndValues[s_ADNED_MAX_DETS+1] = {0};
  int NDArrayStartValues[s_ADNED_MAX_DETS+1] = {0};
  int numDet = 0;
  getIntegerParam(ADnEDNumDetParam, &numDet);
  getIntegerParam(ADnEDEventDebugParam, &eventDebug);
  for (int det=1; det<=numDet; det++) {
    getIntegerParam(det, ADnEDDetPixelNumStartParam, &detStartValues[det]);
    getIntegerParam(det, ADnEDDetPixelNumEndParam, &detEndValues[det]);
    getIntegerParam(det, ADnEDDetNDArrayStartParam, &NDArrayStartValues[det]);
  }

  //for (int det=1; det<=numDet; det++) {
  //  cout << "det: " << det << endl;
  //  cout << " detStartValues: " << detStartValues[det] << endl;
  //  cout << " detEndValues: " << detEndValues[det] << endl;
  //  cout << " NDArrayStartValues: " << NDArrayStartValues[det] << endl;
  //}

  //epics::pvData::PVIntPtr seqIDPtr = pv_struct->getIntField(ADNED_PV_SEQ);
  //if (!seqIDPtr) {
  //  asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s No valid timeStamp.userTag found.\n", functionName);
  //  return;
  //}

  //Compare timeStamp to last timeStamp to detect a new pulse.
  lock();
  ++m_seqCounter;  
  try {
    if (!m_PVTimeStamp.attach(pv_struct->getStructureField(ADNED_PV_TIMESTAMP))) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s Failed to attach PVTimeStamp.\n", functionName);
      return;
    }
    m_PVTimeStamp.get(m_TimeStamp);
    if (m_TimeStampLast != m_TimeStamp) {
      newPulse = true;
    }
    setIntegerParam(ADnEDBadTimeStampParam, 0);
    if (m_TimeStampLast > m_TimeStamp) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s Backwards timeStamp detected.\n", functionName);
      setIntegerParam(ADnEDBadTimeStampParam, 1);
      return;
    }
    m_TimeStampLast.put(m_TimeStamp.getSecondsPastEpoch(), m_TimeStamp.getNanoseconds());
    seqID = m_TimeStamp.getUserTag();
    //Detect missing packets
    if (seqID != lastSeqID+1) {
      setIntegerParam(ADnEDSeqIDMissingParam, lastSeqID+1);
      getIntegerParam(ADnEDSeqIDNumMissingParam, &numMissingPackets);
      setIntegerParam(ADnEDSeqIDNumMissingParam, numMissingPackets+(seqID-lastSeqID+1));
    }
    lastSeqID = seqID;
  } catch (std::exception &e)  {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
	      "%s: Failed to deal with time stamp objects. Exception: %s\n", 
	      functionName, e.what());
    unlock();
    return;
  }
  unlock();
  
  epics::pvData::PVDoublePtr pChargePtr = pv_struct->getDoubleField(ADNED_PV_PCHARGE);
  if (!pChargePtr) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s No valid pCharge found.\n", functionName);
    return;
  }

  if ((p_Data == NULL) || (m_dataMaxSize == 0)) {
    return;
  }

  epics::pvData::PVUIntArrayPtr eventsPtr = pv_struct->getSubField<epics::pvData::PVUIntArray>(ADNED_PV_PIXELS);
  if (eventsPtr) {
    epics::pvData::uint32 length = eventsPtr->getLength();
    epics::pvData::shared_vector<const epics::pvData::uint32> getData = eventsPtr->view();

    lock();

    int offset = 0;
    for (size_t i=0; i<length; ++i) {
      //cout << " " << getData[i];
      int pixel = getData[i];
      for (int det=1; det<=numDet; det++) {
	if ((pixel >= detStartValues[det]) && (pixel <= detEndValues[det])) {
	  if (det==1) {
	    p_Data[pixel]++;
	  } else {
	    offset = pixel-detStartValues[det];
	    p_Data[NDArrayStartValues[det]+offset]++;
	  }
	}
      }
    }
    
    if (newPulse) {
      m_pChargeInt += pChargePtr->get();
      ++m_pulseCounter;
    }    

    //Update params at slower rate
    //Some logic here to check time expired since last update
    if (eventUpdate) {
      setIntegerParam(ADnEDSeqCounterParam, m_seqCounter);
      setIntegerParam(ADnEDPulseCounterParam, m_pulseCounter);
      setIntegerParam(ADnEDSeqIDParam, seqID);
      setDoubleParam(ADnEDPChargeParam, pChargePtr->get());
      setDoubleParam(ADnEDPChargeIntParam, m_pChargeInt);
      callParamCallbacks();
    }

    unlock();
  }

  if (eventDebug != 0) {
    cout << "m_seqCounter: " << m_seqCounter << endl;
    pv_struct->dumpValue(cout);
    cout << "p_Data: " << endl;
    cout << " m_dataMaxSize: " << m_dataMaxSize << endl;
    for (epicsUInt32 i=0; i<m_dataMaxSize; ++i) {
      cout << " " << p_Data[i];
    }
    cout << endl;

    //cout << "PulseID: " << std::hex << value->get() << ", " << std::dec << value->get() << endl;
  }
  
}

/**
 * Allocate local storage for event handler
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

  if (numDet == 0) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s No detectors.\n", functionName);
    return asynError;
  }

  m_dataMaxSize =0;
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
      return asynError;
    }
    
    printf("ADnED::allocArray: m_dataMaxSize: %d\n", m_dataMaxSize);
    printf("ADnED::allocArray: detSize: %d\n", detSize);

    setIntegerParam(det, ADnEDDetNDArrayStartParam, m_dataMaxSize);
    setIntegerParam(det, ADnEDDetNDArrayEndParam, m_dataMaxSize+detSize);
    setIntegerParam(det, ADnEDDetNDArraySizeParam, detSize);
    callParamCallbacks(det);
    
    m_dataMaxSize += detSize;
    
  }
  
  printf("ADnED::allocArray: final m_dataMaxSize: %d\n", m_dataMaxSize);
  printf("ADnED::allocArray: TOF Size: %d\n", numDet * (tofMax+1));
  
  if (p_Data) {
    free(p_Data);
    p_Data = NULL;
  }
  
  if (!p_Data) {
    if (m_dataMaxSize != 0) {
      p_Data = static_cast<epicsUInt32*>(calloc(m_dataMaxSize+(numDet * (tofMax+1)), sizeof(epicsUInt32)));
    } else {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s Not allocating zero sized array.\n", functionName);
    }
  } else {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s pData already allocated at start of acqusition.\n", functionName);
    status = asynError;
  }
  if (!p_Data) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s pData failed to allocate.\n", functionName);
    status = asynError;
  }

  if (status == asynSuccess) {
    m_dataAlloc = false;
  }
  
  return status;
}


/**
 * Event readout task.
 */
void ADnED::eventTask(void)
{
  epicsEventWaitStatus eventStatus;
  epicsFloat64 timeout = 0.001;
  int acquire = 0;
  int status = 0;
  char pvName[s_ADNED_MAX_STRING_SIZE] = {0};
  const char* functionName = "ADnED::dataTask";
 
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Started Event Thread.\n", functionName);

  cout << "Event readout thread PID: " << getpid() << endl;
  cout << "Event readout thread TID syscall(SYS_gettid): " << syscall(SYS_gettid) << endl;
  cout << "Event readout thread this pointed addr: " << std::hex << this << std::dec << endl;
  
  

  while (1) {

    //Wait for a stop event, with a short timeout, to catch any that were done during last read.
    eventStatus = epicsEventWaitWithTimeout(m_stopEvent, timeout);          
    if (eventStatus == epicsEventWaitOK) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Got Stop Event Before Start Event.\n", functionName);
    }

    setIntegerParam(ADAcquire, 0);
    callParamCallbacks();

    eventStatus = epicsEventWait(m_startEvent);          
    if (eventStatus == epicsEventWaitOK) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Got Start Event.\n", functionName);
      acquire = 1;
      lock();
     
      //Read the PV name
      getStringParam(ADnEDPVNameParam, sizeof(pvName), pvName);
	
      if (allocArray() != asynSuccess) {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s: ERROR: Failed to allocate array.\n", functionName);
      }
      
      //Clear arrays at start of acquire every time.
      if (p_Data != NULL) {
	memset(p_Data, 0, m_dataMaxSize*sizeof(epicsUInt32));
      }
      m_pChargeInt = 0.0;
      m_seqCounter = 0;
      m_pulseCounter = 0;
      
      //
      // Reset event counter params here (driver specific records.)
      //
      setIntegerParam(ADStatus, ADStatusAcquire);
      setStringParam(ADStatusMessage, "Acquiring Events");
      setIntegerParam(ADnEDSeqCounterParam, 0);
      setIntegerParam(ADnEDPulseCounterParam, 0);
      setIntegerParam(ADnEDSeqIDParam, 0);
      setDoubleParam(ADnEDPChargeParam, 0.0);
      setDoubleParam(ADnEDPChargeIntParam, 0.0);
      m_TimeStamp.put(0,0);
      m_TimeStampLast.put(0,0);
      // Start frame thread
      epicsEventSignal(this->m_startFrame);
      callParamCallbacks();
      
      //Connect channel here
      try {
	cout << "Starting ClientFactory::start() " << endl;
	epics::pvAccess::ClientFactory::start();
      } catch (std::exception &e)  {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
		  "%s: ERROR: Exception for ClientFactory::start(). Exception: %s\n", 
		  functionName, e.what());
      }
      
      epics::pvAccess::ChannelProvider::shared_pointer channelProvider = epics::pvAccess::getChannelProviderRegistry()->getProvider("pva");
      if (!channelProvider) {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s: ERROR: No Channel Provider.\n", functionName);
      }
      
      if (pvName[0] != '0') {
	try {
	  std::string channelStr("ADnED Channel");
	  shared_ptr<nEDChannelRequester> channelRequester(new nEDChannelRequester(channelStr));
	  shared_ptr<epics::pvAccess::Channel> channel(channelProvider->createChannel(pvName, channelRequester, ADNED_PV_PRIORITY));
	  channelRequester->waitUntilConnected(ADNED_PV_TIMEOUT);
	  
	  std::string monitorStr("ADnED Monitor");
	  shared_ptr<epics::pvData::PVStructure> pvRequest = epics::pvData::CreateRequest::create()->createRequest(ADNED_PV_REQUEST);
	  shared_ptr<nEDMonitorRequester> monitorRequester(new nEDMonitorRequester(monitorStr, this));
	  
	  shared_ptr<epics::pvData::Monitor> monitor = channel->createMonitor(monitorRequester, pvRequest);
	} catch (std::exception &e)  {
	  asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
		    "%s: ERROR: Problem creating monitor. Exception: %s\n", 
		    functionName, e.what());
	}
      }
      //Call this if we want to block here forever.
      //monitorRequester->waitUntilDone();
      
      //epicsThreadSleep(10);
      
      unlock();
    }
    
    while (acquire) {

      //Wait for a stop event, with a short timeout.
      //eventStatus = epicsEventWaitWithTimeout(m_stopEvent, timeout);      
      eventStatus = epicsEventWaitWithTimeout(m_stopEvent, 0.1);      
      if (eventStatus == epicsEventWaitOK) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Got Stop Event.\n", functionName);
        acquire = 0;
      }

      //if (acquire) {
      //	cout << "Reading Events!" << endl;
      //}
      
      if (!acquire) {
	lock();
	setIntegerParam(ADStatus, ADStatusIdle);
	epicsEventSignal(this->m_stopFrame);
	unlock();
      }
      
    } // End of while(acquire)

    epics::pvAccess::ClientFactory::stop();

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
 * Frame readout task.
 */
void ADnED::frameTask(void)
{
  epicsEventWaitStatus eventStatus;
  epicsFloat64 timeout = 0.001;
  int acquire = 0;
  int status = 0;
  epicsTimeStamp nowTime;
  const char* functionName = "ADnED::frameTask";
 
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Started Frame Thread.\n", functionName);

  cout << "Frame readout thread PID: " << getpid() << endl;
  cout << "Frame readout thread TID syscall(SYS_gettid): " << syscall(SYS_gettid) << endl;

  while (1) {

    //Wait for a stop event, with a short timeout, to catch any that were done during last read.
    eventStatus = epicsEventWaitWithTimeout(m_stopFrame, timeout);          
    if (eventStatus == epicsEventWaitOK) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Got Stop Frame Event Before Start Frame Event.\n", functionName);
    }
    
    //setIntegerParam(ADnEDFrameAcquire, 0);
    callParamCallbacks();

    eventStatus = epicsEventWait(m_startFrame);          
    if (eventStatus == epicsEventWaitOK) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Got Start Frame Event.\n", functionName);
      acquire = 1;
      lock();
      setIntegerParam(NDArrayCounter, 0);
      //setIntegerParam(ADnEDFrameStatus, ADStatusAcquire);
      //setStringParam(ADnEDFrameStatusMessage, "Acquiring Frames");
      callParamCallbacks();
      unlock();
    }

    while (acquire) {

      //Wait for a stop event, with a short timeout.
      //eventStatus = epicsEventWaitWithTimeout(m_stopEvent, timeout);      
      eventStatus = epicsEventWaitWithTimeout(m_stopFrame, 1);      
      if (eventStatus == epicsEventWaitOK) {
        asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Got Stop Frame Event.\n", functionName);
        acquire = 0;
      }

      //if (acquire) {
      //	cout << "Reading Frames!" << endl;
      //}
      
      if (!acquire) {
	lock();
	//setIntegerParam(ADnEDFrameStatus, ADStatusIdle);
	unlock();
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
  int ADnEDConfig(const char *portName, int maxBuffers, size_t maxMemory, int debug)
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
  
  static void ADnEDRegister(void)
  {
    iocshRegister(&configADnED, configADnEDCallFunc);
  }
  
  epicsExportRegistrar(ADnEDRegister);

} // extern "C"


/****************************************************************************************/
