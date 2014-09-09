
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
using std::endl;

using namespace std::tr1;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace nEDChannel;

//Not sure how we want to these yet, so will leave them as #defines for now.
#define ADNED_PV_TIMEOUT 2.0
#define ADNED_PV_PRIORITY ChannelProvider::PRIORITY_DEFAULT
#define ADNED_PV_REQUEST "record[queueSize=100]field()"
#define ADNED_PV_PIXELS "pixel.value" 
#define ADNED_PV_PULSE "pulse.value" 


//Definitions of static class data members
//const epicsInt32 ADnED::NED_MAX_STRING_SIZE_ = 256;

//C Function prototypes to tie in with EPICS
static void ADnEDEventTaskC(void *drvPvt);
static void ADnEDFrameTaskC(void *drvPvt);

/**
 * Constructor for Xspress3::Xspress3. 
 * This must be called in the Epics IOC startup file.
 * @param portName The Asyn port name to use
 * @param pvname The V4 PV name to use to read the event stream
 * @param maxBuffers Used by asynPortDriver (set to -1 for unlimited)
 * @param maxMemory Used by asynPortDriver (set to -1 for unlimited)
 * @param debug This debug flag for the driver. 
 */
ADnED::ADnED(const char *portName, const char *pvname, int maxBuffers, size_t maxMemory, int debug)
  : ADDriver(portName,
             0, /* maxAddr */ 
             NUM_DRIVER_PARAMS,
             maxBuffers,
             maxMemory,
             asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat32ArrayMask | asynFloat64ArrayMask | asynDrvUserMask | asynOctetMask | asynGenericPointerMask, /* Interface mask */
             asynInt32Mask | asynInt32ArrayMask | asynFloat64Mask | asynFloat32ArrayMask | asynFloat64ArrayMask | asynOctetMask | asynGenericPointerMask,  /* Interrupt mask */
             ASYN_CANBLOCK | ASYN_MULTIDEVICE, /* asynFlags.*/
             1, /* Autoconnect */
             0, /* default priority */
             0), /* Default stack size*/
    debug_(debug)
{
  int status = asynSuccess;
  const char *functionName = "ADnED::ADnED";
 
  strncpy(pvname_, const_cast<char*>(pvname), NED_MAX_STRING_SIZE-1);
  cout << "pvname_: " <<  pvname_ << endl;

  //Create the epicsEvent for signaling the threads.
  //This will cause it to do a poll immediately, rather than wait for the poll time period.
  startEvent_ = epicsEventMustCreate(epicsEventEmpty);
  if (!startEvent_) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s epicsEventCreate failure for start event.\n", functionName);
    return;
  }
  stopEvent_ = epicsEventMustCreate(epicsEventEmpty);
  if (!stopEvent_) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s epicsEventCreate failure for stop event.\n", functionName);
    return;
  }
  startFrame_ = epicsEventMustCreate(epicsEventEmpty);
  if (!startFrame_) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s epicsEventCreate failure for start frame.\n", functionName);
    return;
  }
  stopFrame_ = epicsEventMustCreate(epicsEventEmpty);
  if (!stopFrame_) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s epicsEventCreate failure for stop frame.\n", functionName);
    return;
  }

  //Add the params to the paramLib 
  //createParam adds the parameters to all param lists automatically (using maxAddr).
  createParam(ADnEDFirstParamString,              asynParamInt32,       &ADnEDFirstParam);
  createParam(ADnEDResetParamString,              asynParamInt32,       &ADnEDResetParam);
  createParam(ADnEDEventDebugParamString,         asynParamInt32,       &ADnEDEventDebugParam);
  createParam(ADnEDPulseCounterParamString,       asynParamInt32,       &ADnEDPulseCounterParam);
  createParam(ADnEDPulseIDParamString,            asynParamInt32,       &ADnEDPulseIDParam);
  createParam(ADnEDEventUpdatePeriodParamString,  asynParamFloat64,     &ADnEDEventUpdatePeriodParam);
  createParam(ADnEDDetPVNameParamString,          asynParamOctet,       &ADnEDDetPVNameParam);
  createParam(ADnEDDet1PixelNumStartParamString,  asynParamInt32,       &ADnEDDet1PixelNumStartParam);
  createParam(ADnEDDet2PixelNumStartParamString,  asynParamInt32,       &ADnEDDet2PixelNumStartParam);
  createParam(ADnEDDet1PixelNumEndParamString,    asynParamInt32,       &ADnEDDet1PixelNumEndParam);
  createParam(ADnEDDet2PixelNumEndParamString,    asynParamInt32,       &ADnEDDet2PixelNumEndParam);
  createParam(ADnEDLastParamString,               asynParamInt32,       &ADnEDLastParam);

  //Initialize non static, non const, data members
  acquiring_ = 0;
  pulseCounter_ = 0;
  nowTimeSecs_ = 0.0;
  lastTimeSecs_ = 0.0;
  pData_ = NULL;
  dataAlloc_ = true;
  dataMaxSize_ = 0;
  det1Size_ = 0;
  det2Size_ = 0;

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
  paramStatus = ((setIntegerParam(ADnEDPulseCounterParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDPulseIDParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setStringParam(ADnEDDetPVNameParam, " ") == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDDet1PixelNumStartParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDDet2PixelNumStartParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDDet1PixelNumEndParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setIntegerParam(ADnEDDet2PixelNumEndParam, 0) == asynSuccess) && paramStatus);
  paramStatus = ((setStringParam (ADManufacturer, "SNS") == asynSuccess) && paramStatus);
  paramStatus = ((setStringParam (ADModel, "nED areaDetector") == asynSuccess) && paramStatus);

  callParamCallbacks();

  if (!paramStatus) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s Unable To Set Driver Parameters In Constructor.\n", functionName);
  }

  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s End Of Constructor.\n", functionName);

  cout << "PV name: " << pvname << endl;

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

  getIntegerParam(ADStatus, &adStatus);

  if (function == ADnEDResetParam) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Reset.\n", functionName);
    
  } else if (function == ADAcquire) {
    if (value) {
      if (adStatus != ADStatusAcquire) {
	cout << "Start acqusition." << endl;
	pulseCounter_ = 0;
	asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Start Reading Events.\n", functionName);
	epicsEventSignal(this->startEvent_);
      }
    } else {
      	cout << "Stop acqusition." << endl;
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Stop Reading Events.\n", functionName);
      epicsEventSignal(this->stopEvent_);
    }
  } else if (function == ADnEDDet1PixelNumStartParam) {
    dataAlloc_ = true;
  } else if (function == ADnEDDet2PixelNumStartParam) {
    dataAlloc_ = true;
  } else if (function == ADnEDDet1PixelNumEndParam) {
    dataAlloc_ = true;
  } else if (function == ADnEDDet2PixelNumEndParam) {
    dataAlloc_ = true;
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
  epicsFloat64 updatePeriod = 0.0;
  const char* functionName = "ADnED::eventHandler";
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Event Handler.\n", functionName);

  /* Get the time and decide if we update the array.*/
  getDoubleParam(ADnEDEventUpdatePeriodParam, &updatePeriod);
  epicsTimeGetCurrent(&nowTime_);
  nowTimeSecs_ = nowTime_.secPastEpoch + (nowTime_.nsec / 1.e9);
  if ((nowTimeSecs_ - lastTimeSecs_) < (updatePeriod / 1000.0)) {
    eventUpdate = false;
  } else {
    eventUpdate = true;
    lastTimeSecs_ = nowTimeSecs_;
  }
  
  int det1start = 0;
  int det2start = 0;
  int det1end = 0;
  int det2end = 0;

  lock();
  getIntegerParam(ADnEDEventDebugParam, &eventDebug);
  getIntegerParam(ADnEDDet1PixelNumStartParam, &det1start);
  getIntegerParam(ADnEDDet2PixelNumStartParam, &det2start);
  getIntegerParam(ADnEDDet1PixelNumEndParam, &det1end);
  getIntegerParam(ADnEDDet2PixelNumEndParam, &det2end);
  ++pulseCounter_;
  unlock();

  shared_ptr<PVULong> pulseIDPtr = pv_struct->getULongField(ADNED_PV_PULSE);
  if (!pulseIDPtr) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s No valid pulse ID found.\n", functionName);
    return;
  }

  if (pData_ == NULL) {
    return;
  }

  PVUIntArrayPtr eventsPtr = pv_struct->getSubField<PVUIntArray>(ADNED_PV_PIXELS);
  if (eventsPtr) {
    uint32 length = eventsPtr->getLength();
    shared_vector<const uint32> getData = eventsPtr->view();

    lock();

    int offset = 0;
    for (size_t i=0; i<length; ++i) {
      //cout << " " << getData[i];
      int pixel = getData[i];
      if ((pixel >= det1start) && (pixel <= det1end)) {
	pData_[pixel]++;
      } else if ((pixel >= det2start) && (pixel <= det2end)) {
	offset = pixel-det2start;
      	pData_[det1Size_+offset]++;
      }
    }

    //Update params at slower rate
    //Some logic here to check time expired since last update
    if (eventUpdate) {
      setIntegerParam(ADnEDPulseCounterParam, pulseCounter_);
      setIntegerParam(ADnEDPulseIDParam, pulseIDPtr->get());
      callParamCallbacks();
    }

    unlock();
  }

  if (eventDebug != 0) {
    cout << "pulseCounter_: " << pulseCounter_ << endl;
    pv_struct->dumpValue(cout);
    cout << "pData_: " << endl;
    cout << " dataMaxSize_: " << dataMaxSize_ << endl;
    cout << " det1Size_: " << det1Size_ << endl;
    cout << " det2Size_: " << det2Size_ << endl;
    for (int i=0; i<dataMaxSize_; ++i) {
      cout << " " << pData_[i];
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

  if (dataAlloc_ != true) {
    //Nothing has changed
    return asynSuccess;
  }

  int det1start = 0;
  int det2start = 0;
  int det1end = 0;
  int det2end = 0;
  det1Size_ = 0;
  det2Size_ = 0;

  getIntegerParam(ADnEDDet1PixelNumStartParam, &det1start);
  getIntegerParam(ADnEDDet2PixelNumStartParam, &det2start);
  getIntegerParam(ADnEDDet1PixelNumEndParam, &det1end);
  getIntegerParam(ADnEDDet2PixelNumEndParam, &det2end);
  
  printf("ADnED::allocArray: det1start: %d, det1end: %d, det2start: %d, det2end: %d\n", 
	 det1start, det1end, det2start, det2end);

  //Calculate sizes and do sanity checks
  if ((det1start != 0) && (det1end != 0)) {
    if (det1start <= det1end) {
      det1Size_ = det1end-det1start+1;
    } else {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s det1start > det1end.\n", functionName);
      return asynError;
    }
  }

  if ((det2start != 0) && (det2end != 0)) {
    if (det2start <= det2end) {
      det2Size_ = det2end-det2start+1;
    } else {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s det2start > det2end.\n", functionName);
    }
  }
  
  dataMaxSize_ = det1Size_ + det2Size_;
  printf("ADnED::allocArray: det1Size_: %d\n", det1Size_);
  printf("ADnED::allocArray: det2Size_: %d\n", det2Size_);
  printf("ADnED::allocArray: dataMaxSize_: %d\n", dataMaxSize_);
  
  if (pData_) {
    free(pData_);
    pData_ = NULL;
  }
  
  if (!pData_) {
    pData_ = static_cast<epicsUInt32*>(calloc(dataMaxSize_, sizeof(epicsUInt32)));
  } else {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s pData already allocated at start of acqusition.\n", functionName);
    status = asynError;
  }
  if (!pData_) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s pData failed to allocate.\n", functionName);
    status = asynError;
  }

  if (status == asynSuccess) {
    dataAlloc_ = false;
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
  const char* functionName = "ADnED::dataTask";
 
  asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Started Event Thread.\n", functionName);

  cout << "Event readout thread PID: " << getpid() << endl;
  cout << "Event readout thread TID syscall(SYS_gettid): " << syscall(SYS_gettid) << endl;
  cout << "Event readout thread this pointed addr: " << std::hex << this << std::dec << endl;
  
  

  while (1) {

    //Wait for a stop event, with a short timeout, to catch any that were done during last read.
    eventStatus = epicsEventWaitWithTimeout(stopEvent_, timeout);          
    if (eventStatus == epicsEventWaitOK) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Got Stop Event Before Start Event.\n", functionName);
    }

    setIntegerParam(ADAcquire, 0);
    callParamCallbacks();

    eventStatus = epicsEventWait(startEvent_);          
    if (eventStatus == epicsEventWaitOK) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Got Start Event.\n", functionName);
      acquire = 1;
      lock();
     
      if (allocArray() != asynSuccess) {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s: ERROR: Failed to allocate array.\n", functionName);
      }

      //Clear arrays at start of acquire every time.
      if (pData_ != NULL) {
	memset(pData_, 0, dataMaxSize_*sizeof(epicsUInt32));
      }
      
      //
      // Reset event counter params here (driver specific records.)
      //
      setIntegerParam(ADStatus, ADStatusAcquire);
      setStringParam(ADStatusMessage, "Acquiring Events");
      // Start frame thread
      epicsEventSignal(this->startFrame_);
      callParamCallbacks();
      
      //Connect channel here
      try {
	cout << "Starting ClientFactory::start() " << endl;
	ClientFactory::start();
      } catch (std::exception &e)  {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
		  "%s: ERROR: Exception for ClientFactory::start(). Exception: %s\n", 
		  functionName, e.what());
	PRINT_EXCEPTION2(e, stderr);
        cout << SHOW_EXCEPTION(e);
      }
	
      ChannelProvider::shared_pointer channelProvider = getChannelProviderRegistry()->getProvider("pva");
      if (!channelProvider) {
	asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, "%s: ERROR: No Channel Provider.\n", functionName);
      }
	
      std::string channelStr("ADnED Channel");
      shared_ptr<nEDChannelRequester> channelRequester(new nEDChannelRequester(channelStr));
      shared_ptr<Channel> channel(channelProvider->createChannel(pvname_, channelRequester, ADNED_PV_PRIORITY));
      channelRequester->waitUntilConnected(ADNED_PV_TIMEOUT);
      
      //epicsThreadSleep(1);
      
      std::string monitorStr("ADnED Monitor");
      shared_ptr<PVStructure> pvRequest = CreateRequest::create()->createRequest(ADNED_PV_REQUEST);
      shared_ptr<nEDMonitorRequester> monitorRequester(new nEDMonitorRequester(monitorStr, this));
      
      shared_ptr<Monitor> monitor = channel->createMonitor(monitorRequester, pvRequest);
      
      //Call this if we want to block here forever.
      //monitorRequester->waitUntilDone();

      //epicsThreadSleep(10);
   
      unlock();
    }

    while (acquire) {

      //Wait for a stop event, with a short timeout.
      //eventStatus = epicsEventWaitWithTimeout(stopEvent_, timeout);      
      eventStatus = epicsEventWaitWithTimeout(stopEvent_, 0.1);      
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
	epicsEventSignal(this->stopFrame_);
	unlock();
      }
      
    } // End of while(acquire)

    ClientFactory::stop();

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
    eventStatus = epicsEventWaitWithTimeout(stopFrame_, timeout);          
    if (eventStatus == epicsEventWaitOK) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "%s Got Stop Frame Event Before Start Frame Event.\n", functionName);
    }
    
    //setIntegerParam(ADnEDFrameAcquire, 0);
    callParamCallbacks();

    eventStatus = epicsEventWait(startFrame_);          
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
      //eventStatus = epicsEventWaitWithTimeout(stopEvent_, timeout);      
      eventStatus = epicsEventWaitWithTimeout(stopFrame_, 1);      
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
 * @param pvname The 4V PV name to use
 * @param maxBuffers Used by asynPortDriver (set to -1 for unlimited)
 * @param maxMemory Used by asynPortDriver (set to -1 for unlimited)
 * @param debug This debug flag is passed to xsp3_config in the Xspress API (0 or 1)
 */
  int ADnEDConfig(const char *portName, const char *pvname, int maxBuffers, size_t maxMemory, int debug)
  {
    asynStatus status = asynSuccess;
    
    /*Instantiate class.*/
    try {
      new ADnED(portName, pvname, maxBuffers, maxMemory, debug);
    } catch (...) {
      cout << "Unknown exception caught when trying to construct ADnED." << endl;
      status = asynError;
    }
    
    return(status);
  }


   
  /* Code for iocsh registration */
  
  /* ADnEDConfig */
  static const iocshArg ADnEDConfigArg0 = {"Port name", iocshArgString};
  static const iocshArg ADnEDConfigArg1 = {"V4 PV Name", iocshArgString};
  static const iocshArg ADnEDConfigArg2 = {"Max Buffers", iocshArgInt};
  static const iocshArg ADnEDConfigArg3 = {"Max Memory", iocshArgInt};
  static const iocshArg ADnEDConfigArg4 = {"Debug", iocshArgInt};
  static const iocshArg * const ADnEDConfigArgs[] =  {&ADnEDConfigArg0,
                                                         &ADnEDConfigArg1,
                                                         &ADnEDConfigArg2,
                                                         &ADnEDConfigArg3,
                                                         &ADnEDConfigArg4};
  
  static const iocshFuncDef configADnED = {"ADnEDConfig", 5, ADnEDConfigArgs};
  static void configADnEDCallFunc(const iocshArgBuf *args)
  {
    ADnEDConfig(args[0].sval, args[1].sval, args[2].ival, args[3].ival, args[4].ival);
  }
  
  static void ADnEDRegister(void)
  {
    iocshRegister(&configADnED, configADnEDCallFunc);
  }
  
  epicsExportRegistrar(ADnEDRegister);

} // extern "C"


/****************************************************************************************/
