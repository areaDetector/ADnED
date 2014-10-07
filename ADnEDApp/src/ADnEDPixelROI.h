/**
 * Originally based on the ROI plugin.
 * Modified to deal with pixel mapping to X/Y co-ordinates.
 *
 * Matt Pearson
 * Oct 2014
 */

#ifndef ADNED_PIXEL_ROI_H
#define ADNED_PIXEL_ROI_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"

/* ROI general parameters */
#define ADnEDPixelROINameString               "PIXELROI_NAME"                /* Name of this ROI */

/* ROI definition */
#define ADnEDPixelROIDim0MinString            "PIXELROI_DIM0_MIN"          /* Starting element of ROI in each dimension */
#define ADnEDPixelROIDim1MinString            "PIXELROI_DIM1_MIN"          /* Starting element of ROI in each dimension */
#define ADnEDPixelROIDim0SizeString           "PIXELROI_DIM0_SIZE"         /* Size of ROI in each dimension */
#define ADnEDPixelROIDim1SizeString           "PIXELROI_DIM1_SIZE"         /* Size of ROI in each dimension */
#define ADnEDPixelROIDim0MaxSizeString        "PIXELROI_DIM0_MAX_SIZE"     /* Maximum size of ROI in each dimension */
#define ADnEDPixelROIDim1MaxSizeString        "PIXELROI_DIM1_MAX_SIZE"     /* Maximum size of ROI in each dimension */
#define ADnEDPixelROIDim0BinString            "PIXELROI_DIM0_BIN"          /* Binning of ROI in each dimension */
#define ADnEDPixelROIDim1BinString            "PIXELROI_DIM1_BIN"          /* Binning of ROI in each dimension */
#define ADnEDPixelROIDim0ReverseString        "PIXELROI_DIM0_REVERSE"      /* Reversal of ROI in each dimension */
#define ADnEDPixelROIDim1ReverseString        "PIXELROI_DIM1_REVERSE"      /* Reversal of ROI in each dimension */
#define ADnEDPixelROIDim0EnableString         "PIXELROI_DIM0_ENABLE"       /* If set then do ROI in this dimension */
#define ADnEDPixelROIDim1EnableString         "PIXELROI_DIM1_ENABLE"       /* If set then do ROI in this dimension */
#define ADnEDPixelROIDim0AutoSizeString       "PIXELROI_DIM0_AUTO_SIZE"    /* Automatically set size to max */
#define ADnEDPixelROIDim1AutoSizeString       "PIXELROI_DIM1_AUTO_SIZE"    /* Automatically  set size to max */
#define ADnEDPixelROIDataTypeString           "PIXELROI_ROI_DATA_TYPE"     /* Data type for ROI.  -1 means automatic. */
#define ADnEDPixelROIEnableScaleString        "PIXELROI_ENABLE_SCALE"      /* Disable/Enable scaling */
#define ADnEDPixelROIScaleString              "PIXELROI_SCALE_VALUE"       /* Scaling value, used as divisor */

/** Extract Regions-Of-Interest (ROI) from NDArray data; the plugin can be a source of NDArray callbacks for
  * other plugins, passing these sub-arrays. 
  * The plugin also optionally computes a statistics on the ROI. */
class ADnEDPixelROI : public NDPluginDriver {
public:
    ADnEDPixelROI(const char *portName, int queueSize, int blockingCallbacks, 
                 const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory,
                 int priority, int stackSize);
    /* These methods override the virtual methods in the base class */
    void processCallbacks(NDArray *pArray);

protected:
    /* ROI general parameters */
    int ADnEDPixelROIFirst;
    #define FIRST_ADNED_PIXELROI_PARAM ADnEDPixelROIFirst
    int ADnEDPixelROIName;

    /* ROI definition */
    int ADnEDPixelROIDim0Min;
    int ADnEDPixelROIDim1Min;
    int ADnEDPixelROIDim0Size;
    int ADnEDPixelROIDim1Size;
    int ADnEDPixelROIDim0MaxSize;
    int ADnEDPixelROIDim1MaxSize;
    int ADnEDPixelROIDim0Bin;
    int ADnEDPixelROIDim1Bin;
    int ADnEDPixelROIDim0Reverse;
    int ADnEDPixelROIDim1Reverse;
    int ADnEDPixelROIDim0Enable;
    int ADnEDPixelROIDim1Enable;    
    int ADnEDPixelROIDim0AutoSize;    
    int ADnEDPixelROIDim1AutoSize;    
    int ADnEDPixelROIDataType;
    int ADnEDPixelROIEnableScale;
    int ADnEDPixelROIScale;
    int ADnEDPixelROILast;
    #define LAST_ADNED_PIXELROI_PARAM ADnEDPixelROILast
                                
private:
};
#define NUM_ADNED_PIXELROI_PARAMS ((int)(&LAST_ADNED_PIXELROI_PARAM - &FIRST_ADNED_PIXELROI_PARAM + 1))
    
#endif //ADNED_PIXEL_ROI_H
