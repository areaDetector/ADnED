/*
 * NDPluginPixelROI.cpp
 *
 * ROI plugin for the pixel event data.
 * It extracts the 1-D array for a specific detector, and does X/Y mapping
 * to create a 2-D NDArray. Some of the original ROI features have been removed.
 * Originally based on standard ROI plugin written by Mark Rivers.
 * 
 * Matt Pearson
 * Oct 2014
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <epicsString.h>
#include <epicsMutex.h>
#include <iocsh.h>

#include "NDArray.h"
#include "ADnEDPixelROI.h"
#include <epicsExport.h>

#define MAX(A,B) (A)>(B)?(A):(B)
#define MIN(A,B) (A)<(B)?(A):(B)


/** Callback function that is called by the NDArray driver with new NDArray data.
  * Extracts the NthrDArray data into each of the ROIs that are being used.
  * Computes statistics on the ROI if ADnEDPixelROIComputeStatistics is 1.
  * Computes the histogram of ROI values if ADnEDPixelROIComputeHistogram is 1.
  * \param[in] pArray  The NDArray from the callback.
  */
void ADnEDPixelROI::processCallbacks(NDArray *pArray)
{
    /* This function computes the ROIs.
     * It is called with the mutex already locked.  It unlocks it during long calculations when private
     * structures don't need to be protected.
     */

    int dataType;
    int dim;
    int itemp;
    NDDimension_t dims[ND_ARRAY_MAX_DIMS], *pDim;
    size_t userDims[ND_ARRAY_MAX_DIMS];
    NDArrayInfo arrayInfo, scratchInfo;
    NDArray *pScratch, *pOutput;
    double *pData;
    int enableScale, enableDim[2], autoSize[2];
    size_t i;
    double scale;
    
    //const char* functionName = "processCallbacks";
    
    memset(dims, 0, sizeof(NDDimension_t) * ND_ARRAY_MAX_DIMS);

    /* Get all parameters while we have the mutex */
    getIntegerParam(ADnEDPixelROIDim0Min,      &itemp); dims[0].offset = itemp;
    getIntegerParam(ADnEDPixelROIDim1Min,      &itemp); dims[1].offset = itemp;
    getIntegerParam(ADnEDPixelROIDim0Size,     &itemp); dims[0].size = itemp;
    getIntegerParam(ADnEDPixelROIDim1Size,     &itemp); dims[1].size = itemp;
    getIntegerParam(ADnEDPixelROIDim0Bin,      &dims[0].binning);
    getIntegerParam(ADnEDPixelROIDim1Bin,      &dims[1].binning);
    getIntegerParam(ADnEDPixelROIDim0Reverse,  &dims[0].reverse);
    getIntegerParam(ADnEDPixelROIDim1Reverse,  &dims[1].reverse);
    getIntegerParam(ADnEDPixelROIDim0Enable,   &enableDim[0]);
    getIntegerParam(ADnEDPixelROIDim1Enable,   &enableDim[1]);
    getIntegerParam(ADnEDPixelROIDim0AutoSize, &autoSize[0]);
    getIntegerParam(ADnEDPixelROIDim1AutoSize, &autoSize[1]);
    getIntegerParam(ADnEDPixelROIDataType,     &dataType);
    getIntegerParam(ADnEDPixelROIEnableScale,  &enableScale);
    getDoubleParam(ADnEDPixelROIScale, &scale);

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);

    /* We always keep the last array so read() can use it.
     * Release previous one. Reserve new one below. */
    if (this->pArrays[0]) {
        this->pArrays[0]->release();
        this->pArrays[0] = NULL;
    }
    
    /* Get information about the array */
    pArray->getInfo(&arrayInfo);
    
    userDims[0] = arrayInfo.xDim;
    userDims[1] = arrayInfo.yDim;

    /* Make sure dimensions are valid, fix them if they are not */
    for (dim=0; dim<pArray->ndims; dim++) {
        pDim = &dims[dim];
        if (enableDim[dim]) {
            pDim->offset  = MAX(pDim->offset,  0);
            pDim->offset  = MIN(pDim->offset,  pArray->dims[userDims[dim]].size-1);
            if (autoSize[dim]) pDim->size = pArray->dims[userDims[dim]].size;
            pDim->size    = MAX(pDim->size,    1);
            pDim->size    = MIN(pDim->size,    pArray->dims[userDims[dim]].size - pDim->offset);
            pDim->binning = MAX(pDim->binning, 1);
            pDim->binning = MIN(pDim->binning, (int)pDim->size);
        } else {
            pDim->offset  = 0;
            pDim->size    = pArray->dims[userDims[dim]].size;
            pDim->binning = 1;
        }
    }

    /* Update the parameters that may have changed */
    setIntegerParam(ADnEDPixelROIDim0MaxSize, 0);
    setIntegerParam(ADnEDPixelROIDim1MaxSize, 0);
    if (pArray->ndims > 0) {
        pDim = &dims[0];
        setIntegerParam(ADnEDPixelROIDim0MaxSize, (int)pArray->dims[userDims[0]].size);
        if (enableDim[0]) {
            setIntegerParam(ADnEDPixelROIDim0Min,  (int)pDim->offset);
            setIntegerParam(ADnEDPixelROIDim0Size, (int)pDim->size);
            setIntegerParam(ADnEDPixelROIDim0Bin,  pDim->binning);
        }
    }
    if (pArray->ndims > 1) {
        pDim = &dims[1];
        setIntegerParam(ADnEDPixelROIDim1MaxSize, (int)pArray->dims[userDims[1]].size);
        if (enableDim[1]) {
            setIntegerParam(ADnEDPixelROIDim1Min,  (int)pDim->offset);
            setIntegerParam(ADnEDPixelROIDim1Size, (int)pDim->size);
            setIntegerParam(ADnEDPixelROIDim1Bin,  pDim->binning);
        }
    }

    /* This function is called with the lock taken, and it must be set when we exit.
     * The following code can be exected without the mutex because we are not accessing memory
     * that other threads can access. */
    this->unlock();

    /* Extract this ROI from the input array.  The convert() function allocates
     * a new array and it is reserved (reference count = 1) */
    if (dataType == -1) {
      dataType = (int)pArray->dataType;
    }
    
    if (enableScale && (scale != 0) && (scale != 1)) {
        /* This is tricky.  We want to do the operation to avoid errors due to integer truncation.
         * For example, if an image with all pixels=1 is binned 3x3 with scale=9 (divide by 9), then
         * the output should also have all pixels=1. 
         * We do this by extracting the ROI and converting to double, do the scaling, then convert
         * to the desired data type. */
        this->pNDArrayPool->convert(pArray, &pScratch, NDFloat64, dims);
        pScratch->getInfo(&scratchInfo);
        pData = (double *)pScratch->pData;
        for (i=0; i<scratchInfo.nElements; i++) pData[i] = pData[i]/scale;
        this->pNDArrayPool->convert(pScratch, &this->pArrays[0], (NDDataType_t)dataType);
        pScratch->release();
    } 
    else {        
        this->pNDArrayPool->convert(pArray, &this->pArrays[0], (NDDataType_t)dataType, dims);
    }
    pOutput = this->pArrays[0];

    this->lock();

    /* Set the image size of the ROI image data */
    setIntegerParam(NDArraySizeX, 0);
    setIntegerParam(NDArraySizeY, 0);
    if (pOutput->ndims > 0) setIntegerParam(NDArraySizeX, (int)this->pArrays[0]->dims[userDims[0]].size);
    if (pOutput->ndims > 1) setIntegerParam(NDArraySizeY, (int)this->pArrays[0]->dims[userDims[1]].size);

    /* Get the attributes for this driver */
    this->getAttributes(this->pArrays[0]->pAttributeList);
    /* Call any clients who have registered for NDArray callbacks */
    this->unlock();
    doCallbacksGenericPointer(this->pArrays[0], NDArrayData, 0);
    /* We must enter the loop and exit with the mutex locked */
    this->lock();
    callParamCallbacks();

}


/** Constructor for ADnEDPixelROI; most parameters are simply passed to NDPluginDriver::NDPluginDriver.
  * After calling the base class constructor this method sets reasonable default values for all of the
  * ROI parameters.
  * \param[in] portName The name of the asyn port driver to be created.
  * \param[in] queueSize The number of NDArrays that the input queue for this plugin can hold when
  *            NDPluginDriverBlockingCallbacks=0.  Larger queues can decrease the number of dropped arrays,
  *            at the expense of more NDArray buffers being allocated from the underlying driver's NDArrayPool.
  * \param[in] blockingCallbacks Initial setting for the NDPluginDriverBlockingCallbacks flag.
  *            0=callbacks are queued and executed by the callback thread; 1 callbacks execute in the thread
  *            of the driver doing the callbacks.
  * \param[in] NDArrayPort Name of asyn port driver for initial source of NDArray callbacks.
  * \param[in] NDArrayAddr asyn port driver address for initial source of NDArray callbacks.
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
ADnEDPixelROI::ADnEDPixelROI(const char *portName, int queueSize, int blockingCallbacks,
                         const char *NDArrayPort, int NDArrayAddr,
                         int maxBuffers, size_t maxMemory,
                         int priority, int stackSize)
    /* Invoke the base class constructor */
    : NDPluginDriver(portName, queueSize, blockingCallbacks,
                   NDArrayPort, NDArrayAddr, 1, NUM_ADNED_PIXELROI_PARAMS, maxBuffers, maxMemory,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   asynInt32ArrayMask | asynFloat64ArrayMask | asynGenericPointerMask,
                   ASYN_MULTIDEVICE, 1, priority, stackSize)
{
    //const char *functionName = "ADnEDPixelROI";

    /* ROI general parameters */
    createParam(ADnEDPixelROINameString,              asynParamOctet, &ADnEDPixelROIName);

     /* ROI definition */
    createParam(ADnEDPixelROIDim0MinString,           asynParamInt32, &ADnEDPixelROIDim0Min);
    createParam(ADnEDPixelROIDim1MinString,           asynParamInt32, &ADnEDPixelROIDim1Min);
    createParam(ADnEDPixelROIDim0SizeString,          asynParamInt32, &ADnEDPixelROIDim0Size);
    createParam(ADnEDPixelROIDim1SizeString,          asynParamInt32, &ADnEDPixelROIDim1Size);
    createParam(ADnEDPixelROIDim0MaxSizeString,       asynParamInt32, &ADnEDPixelROIDim0MaxSize);
    createParam(ADnEDPixelROIDim1MaxSizeString,       asynParamInt32, &ADnEDPixelROIDim1MaxSize);
    createParam(ADnEDPixelROIDim0BinString,           asynParamInt32, &ADnEDPixelROIDim0Bin);
    createParam(ADnEDPixelROIDim1BinString,           asynParamInt32, &ADnEDPixelROIDim1Bin);
    createParam(ADnEDPixelROIDim0ReverseString,       asynParamInt32, &ADnEDPixelROIDim0Reverse);
    createParam(ADnEDPixelROIDim1ReverseString,       asynParamInt32, &ADnEDPixelROIDim1Reverse);
    createParam(ADnEDPixelROIDim0EnableString,        asynParamInt32, &ADnEDPixelROIDim0Enable);
    createParam(ADnEDPixelROIDim1EnableString,        asynParamInt32, &ADnEDPixelROIDim1Enable);
    createParam(ADnEDPixelROIDim0AutoSizeString,      asynParamInt32, &ADnEDPixelROIDim0AutoSize);
    createParam(ADnEDPixelROIDim1AutoSizeString,      asynParamInt32, &ADnEDPixelROIDim1AutoSize);
    createParam(ADnEDPixelROIDataTypeString,          asynParamInt32, &ADnEDPixelROIDataType);
    createParam(ADnEDPixelROIEnableScaleString,       asynParamInt32, &ADnEDPixelROIEnableScale);
    createParam(ADnEDPixelROIScaleString,             asynParamFloat64, &ADnEDPixelROIScale);

    /* Set the plugin type string */
    setStringParam(NDPluginDriverPluginType, "ADnEDPixelROI");

    /* Try to connect to the array port */
    connectToArrayPort();
}

/** Configuration command */
extern "C" int ADnEDPixelROIConfig(const char *portName, int queueSize, int blockingCallbacks,
                                 const char *NDArrayPort, int NDArrayAddr,
                                 int maxBuffers, size_t maxMemory,
                                 int priority, int stackSize)
{
    new ADnEDPixelROI(portName, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr,
                    maxBuffers, maxMemory, priority, stackSize);
    return(asynSuccess);
}

/* EPICS iocsh shell commands */
static const iocshArg initArg0 = { "portName",iocshArgString};
static const iocshArg initArg1 = { "frame queue size",iocshArgInt};
static const iocshArg initArg2 = { "blocking callbacks",iocshArgInt};
static const iocshArg initArg3 = { "NDArrayPort",iocshArgString};
static const iocshArg initArg4 = { "NDArrayAddr",iocshArgInt};
static const iocshArg initArg5 = { "maxBuffers",iocshArgInt};
static const iocshArg initArg6 = { "maxMemory",iocshArgInt};
static const iocshArg initArg7 = { "priority",iocshArgInt};
static const iocshArg initArg8 = { "stackSize",iocshArgInt};
static const iocshArg * const initArgs[] = {&initArg0,
                                            &initArg1,
                                            &initArg2,
                                            &initArg3,
                                            &initArg4,
                                            &initArg5,
                                            &initArg6,
                                            &initArg7,
                                            &initArg8};
static const iocshFuncDef initFuncDef = {"ADnEDPixelROIConfig",9,initArgs};
static void initCallFunc(const iocshArgBuf *args)
{
    ADnEDPixelROIConfig(args[0].sval, args[1].ival, args[2].ival,
                   args[3].sval, args[4].ival, args[5].ival,
                   args[6].ival, args[7].ival, args[8].ival);
}

extern "C" void ADnEDPixelROIRegister(void)
{
    iocshRegister(&initFuncDef,initCallFunc);
}

extern "C" {
epicsExportRegistrar(ADnEDPixelROIRegister);
}
