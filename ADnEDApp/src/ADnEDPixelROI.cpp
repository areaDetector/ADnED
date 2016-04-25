/*
 * NDPluginPixelROI.cpp
 *
 * ROI plugin for the pixel event data.
 * It extracts the 1-D array for a specific detector and converts 
 * to a 2-D NDArray. 
 * 
 * Dim0 offset and size is used to extract the 1-D data from the large 1-D NDArray input.
 * Dim1 and Dim2 are used to specify X/Y sizes/offsets to create a 2-D NDArray.
 *
 * Originally based on standard ROI plugin written by Mark Rivers. However, many 
 * of the original features have been removed (binning and scaling, color conversion).
 * 
 * Matt Pearson
 * Oct 2014
 */

#include <stdlib.h>
#include <stdio.h>

#include <epicsMutex.h>
#include <iocsh.h>

#include "NDArray.h"
#include "ADnEDPixelROI.h"
#include <epicsExport.h>

#define MAX(A,B) (A)>(B)?(A):(B)
#define MIN(A,B) (A)<(B)?(A):(B)

/** Callback function that is called by the NDArray driver with new NDArray data.
  * Extracts the NthrDArray data into each of the ROIs that are being used.
  * \param[in] pArray  The NDArray from the callback.
  */
void ADnEDPixelROI::processCallbacks(NDArray *pArray)
{
    /* This function computes the ROIs.
     * It is called with the mutex already locked.  It unlocks it during long calculations when private
     * structures don't need to be protected.
     */

    int dataType = 0;
    int dim = 0;
    int itemp = 0;
    int status = ND_SUCCESS;
    NDDimension_t dims[ADNED_PIXELROI_MAX_DIMS];
    NDDimension_t *pDim = NULL;
    NDArray *pOutput = NULL;
    static const char *functionName = "ADnEDPixelROI::processCallbacks";

    memset(dims, 0, sizeof(NDDimension_t) * ADNED_PIXELROI_MAX_DIMS);

    /* Get all parameters while we have the mutex */
    getIntegerParam(ADnEDPixelROIDim0Min,      &itemp); dims[0].offset = itemp;
    getIntegerParam(ADnEDPixelROIDim1Min,      &itemp); dims[1].offset = itemp;
    getIntegerParam(ADnEDPixelROIDim2Min,      &itemp); dims[2].offset = itemp;
    getIntegerParam(ADnEDPixelROIDim0Size,     &itemp); dims[0].size = itemp;
    getIntegerParam(ADnEDPixelROIDim1Size,     &itemp); dims[1].size = itemp;
    getIntegerParam(ADnEDPixelROIDim2Size,     &itemp); dims[2].size = itemp;
    getIntegerParam(ADnEDPixelROIDataType,     &dataType);

    /* Call the base class method */
    NDPluginDriver::processCallbacks(pArray);

    /* We always keep the last array so read() can use it.
     * Release previous one. Reserve new one below. */
    if (this->pArrays[0]) {
        this->pArrays[0]->release();
        this->pArrays[0] = NULL;
    }

    /* Make sure dimensions are valid, fix them if they are not */
    /* Make sure each new X/Y size is not bigger than the Dim0 size.*/
    for (dim=0; dim<3; dim++) {
      pDim = &dims[dim];
      pDim->offset  = MAX(pDim->offset,  0);
      pDim->offset  = MIN(pDim->offset,  pArray->dims[0].size-1);
      pDim->size    = MAX(pDim->size,    1);
      pDim->size    = MIN(pDim->size,    dims[0].size);
      pDim->binning = MAX(pDim->binning, 1);
    }

    /* Make sure end result of X*Y is not bigger than pArray->dims[0].size 
       (the total size of the original NDArray for all detectors. In this 
       case something is wrong so we clamp to 1.*/
    if ((dims[1].size * dims[2].size) >= pArray->dims[0].size) {
      dims[1].size = 1;
      dims[2].size = 1;
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
		"%s ERROR: 2-D sizes are too large for original NDArray size.\n", functionName);
    }

    /* Update the parameters that may have have been fixed */
    setIntegerParam(ADnEDPixelROIDim0MaxSize, 0);
    setIntegerParam(ADnEDPixelROIDim1MaxSize, 0);
    setIntegerParam(ADnEDPixelROIDim2MaxSize, 0);
    
    setIntegerParam(ADnEDPixelROIDim0MaxSize, (int)pArray->dims[0].size);
    setIntegerParam(ADnEDPixelROIDim1MaxSize, (int)pArray->dims[0].size);
    setIntegerParam(ADnEDPixelROIDim2MaxSize, (int)pArray->dims[0].size);
    pDim = &dims[0];
    setIntegerParam(ADnEDPixelROIDim0Min,  (int)pDim->offset);
    setIntegerParam(ADnEDPixelROIDim0Size, (int)pDim->size);
    pDim = &dims[1];
    setIntegerParam(ADnEDPixelROIDim1Min,  (int)pDim->offset);
    setIntegerParam(ADnEDPixelROIDim1Size, (int)pDim->size);
    pDim = &dims[2];
    setIntegerParam(ADnEDPixelROIDim2Min,  (int)pDim->offset);
    setIntegerParam(ADnEDPixelROIDim2Size, (int)pDim->size);
    
    /* This function is called with the lock taken, and it must be set when we exit.
     * The following code can be exected without the mutex because we are not accessing memory
     * that other threads can access. */
    this->unlock();

    /* Extract this ROI from the input array.  The convert() function allocates
     * a new array and it is reserved (reference count = 1) */
    if (dataType == -1) {
      dataType = (int)pArray->dataType;
    }
 
    //Extract 1-D, but using a 2-D NDDimension_t, with the 2nd dimension set to 0 for now.
    NDDimension_t new_dims[2] = {{0}};
    new_dims[0].size = dims[0].size;
    new_dims[0].offset = dims[0].offset;
    new_dims[0].binning = 1;
    new_dims[1].binning = 1;

    status = this->pNDArrayPool->convert(pArray, &this->pArrays[0], (NDDataType_t)dataType, new_dims); 
    if (status != ND_SUCCESS) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
		"%s ERROR: cannot convert from 1-D to 2-D.\n", functionName);
      this->lock();
      return;
    }
    pOutput = this->pArrays[0];

    if (pOutput == NULL) {
      asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR, 
		"%s ERROR: pOutput == NULL.\n", functionName);
      this->lock();
      return;
    }

   //Now we have extraced the 1-D ROI, set the 2-D dims
    pOutput->ndims = 2;
    pOutput->dims[0].size = dims[1].size;
    pOutput->dims[1].size = dims[2].size;
    pOutput->dims[0].offset = 0;
    pOutput->dims[1].offset = 0;
    pOutput->dims[0].binning = 1;
    pOutput->dims[1].binning = 1;

    this->lock();

    /* Set the image size of the ROI image data */
    setIntegerParam(NDArraySizeX, 0);
    setIntegerParam(NDArraySizeY, 0);
    setIntegerParam(NDArraySizeZ, 0);
    if (pOutput->ndims > 0) setIntegerParam(NDArraySizeX, (int)this->pArrays[0]->dims[0].size);
    if (pOutput->ndims > 1) setIntegerParam(NDArraySizeY, (int)this->pArrays[0]->dims[1].size);

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
    createParam(ADnEDPixelROIDim2MinString,           asynParamInt32, &ADnEDPixelROIDim2Min);
    createParam(ADnEDPixelROIDim0SizeString,          asynParamInt32, &ADnEDPixelROIDim0Size);
    createParam(ADnEDPixelROIDim1SizeString,          asynParamInt32, &ADnEDPixelROIDim1Size);
    createParam(ADnEDPixelROIDim2SizeString,          asynParamInt32, &ADnEDPixelROIDim2Size);
    createParam(ADnEDPixelROIDim0MaxSizeString,       asynParamInt32, &ADnEDPixelROIDim0MaxSize);
    createParam(ADnEDPixelROIDim1MaxSizeString,       asynParamInt32, &ADnEDPixelROIDim1MaxSize);
    createParam(ADnEDPixelROIDim2MaxSizeString,       asynParamInt32, &ADnEDPixelROIDim2MaxSize);
    createParam(ADnEDPixelROIDataTypeString,          asynParamInt32, &ADnEDPixelROIDataType);

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
