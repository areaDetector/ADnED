/*
 * NDPluginMask.cpp
 *
 * Mask plugin for 1-D and 2-D NDArray objects
 * Author: Matt Pearson
 *
 * April 27, 2016
 *
 * This can be used to mask out part of the NDArray. This is
 * useful for display purposes, particulary 1-D or 2-D plots
 * that have unwanted peaks.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <iocsh.h>
#include <epicsExport.h>
#include "NDPluginDriver.h"
#include "ADnEDPluginMask.h"

#define MAX(A,B) (A)>(B)?(A):(B)
#define MIN(A,B) (A)<(B)?(A):(B)

const epicsUInt32 NDPluginMask::s_MASK_TYPE_REJECT = 0;
const epicsUInt32 NDPluginMask::s_MASK_TYPE_PASS = 1;

template <typename epicsType>
void NDPluginMask::doMaskT(NDArray *pArray, NDMask_t *pMask)
{
  size_t xmin = 0; 
  size_t xmax = 0;
  size_t ymin = 0;
  size_t ymax = 0;
  size_t mask_val = 0; 
  size_t mask_type = 0;
  size_t ix = 0;
  size_t iy = 0;
  epicsType *pRow = NULL;
   
  if (pMask != NULL) {

    asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER,
              "NDPluginMask::doMaskT, Xpos=%ld, Ypos=%ld, Xsize=%ld, Ysize=%ld, MaskVal=%ld, MaskType=%ld\n",
              static_cast<long>(pMask->PosX), static_cast<long>(pMask->PosY), 
              static_cast<long>(pMask->SizeX), static_cast<long>(pMask->SizeY), 
              static_cast<long>(pMask->MaskVal), static_cast<long>(pMask->MaskType));
   
    xmin = pMask->PosX;
    xmin = MAX(xmin, 0);
    xmax = pMask->PosX + pMask->SizeX;
    xmax = MIN(xmax, xArrayMax);
    ymin = pMask->PosY;
    ymin = MAX(ymin, 0);
    ymax = pMask->PosY + pMask->SizeY;
    ymax = MIN(ymax, yArrayMax);
    mask_val = pMask->MaskVal;
    mask_type = pMask->MaskType;
  }

  if (pArray != NULL) {
    
    if (pArray->ndims == 1) {
      
      if (mask_type == s_MASK_TYPE_REJECT) {
        for (ix = xmin; ix < xmax; ++ix) {
          (static_cast<epicsType *>(pArray->pData))[ix*this->arrayInfo.xStride] = static_cast<epicsType>(mask_val);
        }
      } else if (mask_type == s_MASK_TYPE_PASS) {
        for (ix = 0; ix < xArrayMax; ++ix) {
          if ((ix<xmin)||(ix>=xmax)) {
            (static_cast<epicsType *>(pArray->pData))[ix*this->arrayInfo.xStride] = static_cast<epicsType>(mask_val);
          }
        }
      }
      
    } else if (pArray->ndims == 2) {

      if (mask_type == s_MASK_TYPE_REJECT) {
        for (iy = ymin; iy < ymax; ++iy) {
          pRow = static_cast<epicsType *>(pArray->pData) + iy*arrayInfo.yStride;
          for ( ix = xmin; ix < xmax; ++ix) {
            pRow[ix*this->arrayInfo.xStride] = static_cast<epicsType>(mask_val);
          }
        }
      } else if (mask_type == s_MASK_TYPE_PASS) {
        for (iy = 0; iy < yArrayMax; ++iy) {
          pRow = static_cast<epicsType *>(pArray->pData) + iy*arrayInfo.yStride;
          for (ix = 0; ix < xArrayMax; ++ix) {
            if ( ((ix<xmin)||(ix>=xmax)) || ((iy<ymin)||(iy>=ymax)) ) {
              pRow[ix*this->arrayInfo.xStride] = static_cast<epicsType>(mask_val);
            }
          }
        }
      }
      
    }
    
  } //(pArray != NULL)
  
}

/**
 * Set the mask in the NDArray. This looks at the data type and 
 * uses a templated function (NDPluginMask::doMaskT)
 * \param[in] pArray Pointer to the NDArray from the callback.
 * \param[in] pMask Pointer to the mask structure to use
 */ 
asynStatus NDPluginMask::doMask(NDArray *pArray, NDMask_t *pMask)
{
  if (pArray == NULL) {
    return asynError;
  }
  switch(pArray->dataType) {
  case NDInt8:
    doMaskT<epicsInt8>(pArray, pMask);
    break;
  case NDUInt8:
    doMaskT<epicsUInt8>(pArray, pMask);
    break;
  case NDInt16:
    doMaskT<epicsInt16>(pArray, pMask);
    break;
  case NDUInt16:
    doMaskT<epicsUInt16>(pArray, pMask);
    break;
  case NDInt32:
    doMaskT<epicsInt32>(pArray, pMask);
    break;
  case NDUInt32:
    doMaskT<epicsUInt32>(pArray, pMask);
    break;
  case NDFloat32:
    doMaskT<epicsFloat32>(pArray, pMask);
    break;
  case NDFloat64:
    doMaskT<epicsFloat64>(pArray, pMask);
    break;
  default:
    return asynError;
    break;
  }
  return asynSuccess;
}

/** Callback function that is called by the NDArray driver with new NDArray data.
  * Draws masks on top of the array.
  * \param[in] pArray  The NDArray from the callback.
  */
void NDPluginMask::processCallbacks(NDArray *pArray)
{
  /*
   * This function is called with the mutex already locked.  
   * It unlocks it during long calculations when private
   * structures don't need to be protected.
   */
  
  int use = 0;
  int itemp = 0;
  int mask = 0;
  asynStatus status = asynSuccess;
  NDArray *pOutput = NULL;
  static const char* functionName = "NDPluginMask::processCallbacks";
  
  /* Call the base class method */
  NDPluginDriver::processCallbacks(pArray);

  /* This plugin only works with 1-D or 2-D arrays */
  if ((pArray->ndims < 1) || (pArray->ndims > 2)) {
    asynPrint(pasynUserSelf, ASYN_TRACE_ERROR,
              "%s: error, number of array dimensions must be 1 or 2\n",
              functionName);
  }
  
  /* We always keep the last array so read() can use it.
   * Release previous one. */
  if (this->pArrays[0]) {
    this->pArrays[0]->release();
  }

  /* Copy the input array so we can modify it. */
  this->pArrays[0] = this->pNDArrayPool->copy(pArray, NULL, 1);
  if (this->pArrays[0] == NULL) {
    asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
              "%s error. Could not allocate array via pNDArrayPool->copy.\n",
              functionName);
  }
  pOutput = this->pArrays[0];
  
  /* Get information about the array needed later */
  pOutput->getInfo(&this->arrayInfo);
  setIntegerParam(NDPluginMaskMaxSizeX, (int)arrayInfo.xSize);
  setIntegerParam(NDPluginMaskMaxSizeY, (int)arrayInfo.ySize);
  xArrayMax = (int)arrayInfo.xSize;
  yArrayMax = (int)arrayInfo.ySize;
  
  /* Loop over the masks in this driver */
  for (mask = 0; mask < this->maxMasks; ++mask) {
    pMask = &this->pMasks[mask];
    getIntegerParam(mask, NDPluginMaskUse, &use);
    asynPrint(pasynUserSelf, ASYN_TRACE_FLOW,
              "NDPluginMask::processCallbacks, mask=%d, use=%d\n",
              mask, use);
    if (!use) {
      continue;
    }
    if (pMask == NULL) {
      continue;
    }
    /* Need to fetch all of these parameters while we still have the mutex */
    getIntegerParam(mask, NDPluginMaskPosX,  &itemp); pMask->PosX = itemp;
    pMask->PosX = MAX(pMask->PosX, 0);
    pMask->PosX = MIN(pMask->PosX, xArrayMax-1);
    getIntegerParam(mask, NDPluginMaskSizeX,      &itemp); pMask->SizeX = itemp;
    pMask->SizeX = MAX(pMask->SizeX, 0);
    pMask->SizeX = MIN(pMask->SizeX, xArrayMax-(pMask->PosX));
    if (pArray->ndims > 1) {
      getIntegerParam(mask, NDPluginMaskPosY,  &itemp); pMask->PosY = itemp;
      pMask->PosY = MAX(pMask->PosY, 0);
      pMask->PosY = MIN(pMask->PosY, yArrayMax-1);
      getIntegerParam(mask, NDPluginMaskSizeY,      &itemp); pMask->SizeY = itemp;
      pMask->SizeY = MAX(pMask->SizeY, 0);
      pMask->SizeY = MIN(pMask->SizeY, yArrayMax-(pMask->PosY));
    }
    getIntegerParam(mask, NDPluginMaskVal,        &itemp); pMask->MaskVal = itemp;
    getIntegerParam(mask, NDPluginMaskType,       &itemp); pMask->MaskType = itemp;

    /* Update any changed parameters */
    setIntegerParam(mask, NDPluginMaskPosX, static_cast<int>(pMask->PosX));
    setIntegerParam(mask, NDPluginMaskSizeX, static_cast<int>(pMask->SizeX));
    if (pArray->ndims > 1) {
      setIntegerParam(mask, NDPluginMaskPosY, static_cast<int>(pMask->PosY));
      setIntegerParam(mask, NDPluginMaskSizeY, static_cast<int>(pMask->SizeY));
    }
    
    /* This function is called with the lock taken, and it must be set when we exit.
     * The following code can be exected without the mutex because we are not accessing memory
     * that other threads can access. */
    this->unlock();
    status = this->doMask(pOutput, pMask);
    if (status != asynSuccess) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s doMask (Mask=%d) failed. status=%d\n",
                functionName, mask, status);
    }
    this->lock();
    callParamCallbacks(mask);
  }
  /* Get the attributes for this driver */
  this->getAttributes(this->pArrays[0]->pAttributeList);
  /* Call any clients who have registered for NDArray callbacks */
  this->unlock();
  doCallbacksGenericPointer(this->pArrays[0], NDArrayData, 0);
  this->lock();
  callParamCallbacks();
}


/** Constructor for NDPluginMask
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] maxMasks The maximum number of masks this plugin supports. 1 is minimum.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
NDPluginMask::NDPluginMask(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr, int maxMasks,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, maxMasks, NUM_NDPLUGIN_MASK_PARAMS, maxBuffers, maxMemory,
                   asynGenericPointerMask,
                   asynGenericPointerMask,
                   ASYN_MULTIDEVICE, 1, priority, stackSize)
{
    static const char *functionName = "NDPluginMask";

    this->maxMasks = maxMasks;
    this->pMasks = (NDMask_t *)callocMustSucceed(maxMasks, sizeof(*this->pMasks), functionName);

    createParam(NDPluginMaskNameString,          asynParamOctet, &NDPluginMaskName);
    createParam(NDPluginMaskUseString,           asynParamInt32, &NDPluginMaskUse);
    createParam(NDPluginMaskMaxSizeXString,      asynParamInt32, &NDPluginMaskMaxSizeX);
    createParam(NDPluginMaskMaxSizeYString,      asynParamInt32, &NDPluginMaskMaxSizeY);
    createParam(NDPluginMaskPosXString,          asynParamInt32, &NDPluginMaskPosX);
    createParam(NDPluginMaskPosYString,          asynParamInt32, &NDPluginMaskPosY);
    createParam(NDPluginMaskSizeXString,         asynParamInt32, &NDPluginMaskSizeX);
    createParam(NDPluginMaskSizeYString,         asynParamInt32, &NDPluginMaskSizeY);
    createParam(NDPluginMaskValString,           asynParamInt32, &NDPluginMaskVal);
    createParam(NDPluginMaskTypeString,          asynParamInt32, &NDPluginMaskType);

    xArrayMax = 0;
    yArrayMax = 0;

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "NDPluginMask");

    // Enable ArrayCallbacks by default  
    setIntegerParam(NDArrayCallbacks, 1);

    /* Try to connect to the array port */
    connectToArrayPort();
}

/**
 * IOC shell command 
 */
extern "C" int NDMaskConfigure(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr, int maxMasks,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize)
{
    new NDPluginMask(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, maxMasks,
                        maxBuffers, maxMemory, priority, stackSize);
    return(asynSuccess);
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxMasks",iocshArgInt};
static const iocshArg initArg6 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg7 = { "maxMemory",iocshArgInt};
static const iocshArg initArg8 = { "priority",iocshArgInt};
static const iocshArg initArg9 = { "stackSize",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8,
                                            &initArg9};
static const iocshFuncDef initFuncDef = {"NDMaskConfigure",10,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    NDMaskConfigure(args[0].sval, args[1].ival, args[2].ival,
                       args[3].sval, args[4].ival, args[5].ival,
                       args[6].ival, args[7].ival, args[8].ival,
                       args[9].ival);
}

extern "C" void NDMaskRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(NDMaskRegister);
}
