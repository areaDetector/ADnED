/*
 * NDPluginPixelROI.cpp
 *
 * ROI plugin for the pixel event data.
 * It extracts the 1-D array for a specific detector and converts 
 * to a 2-D NDArray. Some of the original ROI features have been removed.
 * 
 * Dim0 is used to extract the 1-D data from the large 1-D NDArray input.
 * Dim1 and Dim2 are used to specify X/Y sizes to create a 2-D NDArray.
 *
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
    int enableScale, enableDim[3], autoSize[3];
    size_t i;
    double scale;
    
    //const char* functionName = "processCallbacks";
    
    memset(dims, 0, sizeof(NDDimension_t) * ND_ARRAY_MAX_DIMS);

    /* Get all parameters while we have the mutex */
    getIntegerParam(ADnEDPixelROIDim0Min,      &itemp); dims[0].offset = itemp;
    getIntegerParam(ADnEDPixelROIDim1Min,      &itemp); dims[1].offset = itemp;
    getIntegerParam(ADnEDPixelROIDim2Min,      &itemp); dims[2].offset = itemp;
    getIntegerParam(ADnEDPixelROIDim0Size,     &itemp); dims[0].size = itemp;
    getIntegerParam(ADnEDPixelROIDim1Size,     &itemp); dims[1].size = itemp;
    getIntegerParam(ADnEDPixelROIDim2Size,     &itemp); dims[2].size = itemp;
    getIntegerParam(ADnEDPixelROIDataType,     &dataType);

    /* printf(" ND_ARRAY_MAX_DIMS: %d\n", ND_ARRAY_MAX_DIMS);
    printf(" dims[0].offset: %d\n",  dims[0].offset);
    printf(" dims[1].offset: %d\n",  dims[1].offset);
    printf(" dims[2].offset: %d\n",  dims[2].offset);
    printf(" dims[0].size: %d\n",  dims[0].size);
    printf(" dims[1].size: %d\n",  dims[1].size);
    printf(" dims[2].size: %d\n",  dims[2].size);*/

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

    /* Make sure dimensions are valid, fix them if they are not */
    for (dim=0; dim<3; dim++) {
      pDim = &dims[dim];
      pDim->offset  = MAX(pDim->offset,  0);
      pDim->offset  = MIN(pDim->offset,  pArray->dims[0].size-1);
      pDim->size    = MAX(pDim->size,    1);
      pDim->size    = MIN(pDim->size,    pArray->dims[0].size - pDim->offset);
      pDim->binning = MAX(pDim->binning, 1);
    }

    /* Update the parameters that may have changed */
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
    setIntegerParam(ADnEDPixelROIDim1Min,  (int)pDim->offset);
    setIntegerParam(ADnEDPixelROIDim1Size, (int)pDim->size);

    //printf(" Dim 0\n");
    //printf(" pDim->dims[0].size: %d\n", pArray->dims[0].size);
    //printf(" pDim->dims[0].binning: %d\n", pArray->dims[0].binning);
    //printf(" pDim->offset: %d\n", pDim->offset);
    //printf(" pDim->size: %d\n", pDim->size);

    

    //printf(" Dim 1\n");
    //printf(" pDim->dims[1].size: %d\n", pArray->dims[1].size);
    //printf(" pDim->dims[1].binning: %d\n", pArray->dims[1].binning);
    //printf(" pDim->offset: %d\n", pDim->offset);
    //printf(" pDim->size: %d\n", pDim->size);

    //pDim = &dims[2];
    //setIntegerParam(ADnEDPixelROIDim2MaxSize, (int)pArray->dims[userDims[0]].size);
    //setIntegerParam(ADnEDPixelROIDim2Min,  (int)pDim->offset);
    //setIntegerParam(ADnEDPixelROIDim2Size, (int)pDim->size);

    //printf(" Dim 2\n");
    //printf(" pDim->dims[2].size: %d\n", pArray->dims[2].size);
    //printf(" pDim->offset: %d\n", pDim->offset);
    //printf(" pDim->size: %d\n", pDim->size);
    
    /* This function is called with the lock taken, and it must be set when we exit.
     * The following code can be exected without the mutex because we are not accessing memory
     * that other threads can access. */
    this->unlock();

    /* Extract this ROI from the input array.  The convert() function allocates
     * a new array and it is reserved (reference count = 1) */
    if (dataType == -1) {
      dataType = (int)pArray->dataType;
    }
   
    //First extract 1-D block
    //NDDimension_t new_dims[1] = {0};
    //new_dims[0].size = dims[0].size;
    //new_dims[0].offset = dims[0].offset;
    //new_dims[0].binning = 1;

    //Extract 1-D, but using a 2-D NDDimension_t, with the 2nd dimension set to 0 for now.
    NDDimension_t new_dims[2] = {0};
    new_dims[0].size = dims[0].size;
    new_dims[0].offset = dims[0].offset;
    new_dims[0].binning = 1;
    new_dims[1].binning = 1;
 
    //printf(" Before this->pNDArrayPool->convert...\n");
    //printf("  pArray->ndims: %d\n", pArray->ndims);
    //printf("  pArray->dims[0].size: %d\n", pArray->dims[0].size);
    //printf("  pArray->dims[1].size: %d\n", pArray->dims[1].size);
    //printf("  pArray->dims[2].size: %d\n", pArray->dims[2].size);
    //printf("  new_dims[0].size: %d\n", new_dims[0].size);
    //printf("  new_dims[1].size: %d\n", new_dims[1].size);
    this->pNDArrayPool->convert(pArray, &this->pArrays[0], (NDDataType_t)dataType, new_dims);
    //printf(" After this->pNDArrayPool->convert.\n");
    pOutput = this->pArrays[0];
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
