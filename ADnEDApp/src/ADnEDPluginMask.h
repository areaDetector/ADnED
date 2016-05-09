/**
 * See .cpp file for more documentation.
 */

#ifndef NDPluginMask_H
#define NDPluginMask_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"

typedef struct NDMask {
  size_t PosX;
  size_t PosY;
  size_t SizeX;
  size_t SizeY;
  size_t MaskVal;
  size_t MaskType;
} NDMask_t;

#define NDPluginMaskFirstString       "MASK_FIRST"               
#define NDPluginMaskNameString        "MASK_NAME"        /* Name of this mask */
#define NDPluginMaskUseString         "MASK_USE"         /* Use this mask? */
#define NDPluginMaskMaxSizeXString    "MASK_MAX_SIZE_X"  /* Maximum size of mask in X dimension */
#define NDPluginMaskMaxSizeYString    "MASK_MAX_SIZE_Y"  /* Maximum size of mask in Y dimension */
#define NDPluginMaskPosXString        "MASK_POS_X"       /* X position of mask */
#define NDPluginMaskPosYString        "MASK_POS_Y"       /* Y position of mask */
#define NDPluginMaskSizeXString       "MASK_SIZE_X"      /* X size of mask */
#define NDPluginMaskSizeYString       "MASK_SIZE_Y"      /* Y size of mask */
#define NDPluginMaskValString         "MASK_VAL"         /* The mask value */
#define NDPluginMaskTypeString        "MASK_TYPE"        /* The mask type (Pass, Reject, etc.) */
#define NDPluginMaskLastString        "MASK_LAST"               

class epicsShareClass NDPluginMask : public NDPluginDriver {
public:
    NDPluginMask(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr, int maxMasks, 
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);
    template <typename epicsType> void doMaskT(NDArray *pArray, NDMask_t *pMask);
    asynStatus doMask(NDArray *pArray, NDMask_t *pMask);

protected:
    int NDPluginMaskFirst;
    #define FIRST_NDPLUGIN_MASK_PARAM NDPluginMaskFirst
    int NDPluginMaskName;
    int NDPluginMaskUse;
    int NDPluginMaskMaxSizeX;
    int NDPluginMaskMaxSizeY;
    int NDPluginMaskPosX;
    int NDPluginMaskPosY;
    int NDPluginMaskSizeX;
    int NDPluginMaskSizeY;
    int NDPluginMaskVal;
    int NDPluginMaskType;
    int NDPluginMaskLast;
    #define LAST_NDPLUGIN_MASK_PARAM NDPluginMaskLast
                                
private:

    static const epicsUInt32 s_MASK_TYPE_REJECT;
    static const epicsUInt32 s_MASK_TYPE_PASS;

    int maxMasks;
    NDArrayInfo arrayInfo;
    NDMask_t *pMasks;   /* Array of NDMask structures (indexed by Asyn address) */
    NDMask_t *pMask;
    epicsUInt32 xArrayMax;
    epicsUInt32 yArrayMax;
};
#define NUM_NDPLUGIN_MASK_PARAMS ((int)(&LAST_NDPLUGIN_MASK_PARAM - &FIRST_NDPLUGIN_MASK_PARAM + 1))
    
#endif //NDPluginMask_H
