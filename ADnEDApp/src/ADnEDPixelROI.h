/**
 * See .cpp file for more documentation.
 */

#ifndef ADNED_PIXEL_ROI_H
#define ADNED_PIXEL_ROI_H

#include <epicsTypes.h>
#include <asynStandardInterfaces.h>

#include "NDPluginDriver.h"

/* ROI general parameters */
#define ADnEDPixelROINameString               "PIXELROI_NAME"              /* Name of this ROI */

/* ROI definition */
#define ADnEDPixelROIDim0MinString            "PIXELROI_DIM0_MIN"          /* Starting element of 1-D input aray */
#define ADnEDPixelROIDim1MinString            "PIXELROI_DIM1_MIN"          /* Starting element of 2-D X output array */
#define ADnEDPixelROIDim2MinString            "PIXELROI_DIM2_MIN"          /* Starting element of 2-D Y output array */
#define ADnEDPixelROIDim0SizeString           "PIXELROI_DIM0_SIZE"         /* Size of ROI in 1-D input array */
#define ADnEDPixelROIDim1SizeString           "PIXELROI_DIM1_SIZE"         /* Size of ROI in 2-D X output array */
#define ADnEDPixelROIDim2SizeString           "PIXELROI_DIM2_SIZE"         /* Size of ROI in 2-D Y output array */
#define ADnEDPixelROIDim0MaxSizeString        "PIXELROI_DIM0_MAX_SIZE"     /* Maximum size of 1-D input ROI */
#define ADnEDPixelROIDim1MaxSizeString        "PIXELROI_DIM1_MAX_SIZE"     /* Maximum size of 2-D X output ROI */
#define ADnEDPixelROIDim2MaxSizeString        "PIXELROI_DIM2_MAX_SIZE"     /* Maximum size of 2-D Y output ROI */
#define ADnEDPixelROIDataTypeString           "PIXELROI_ROI_DATA_TYPE"     /* Data type for ROI.  -1 means automatic. */

#define ADNED_PIXELROI_MAX_DIMS 3

/** Extract Regions-Of-Interest (ROI) from NDArray data; the plugin can be a source of NDArray callbacks for
  * other plugins, passing these sub-arrays. 
  * The plugin also optionally computes a statistics on the ROI. */
class epicsShareClass ADnEDPixelROI : public NDPluginDriver {
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
    int ADnEDPixelROIDim2Min;
    int ADnEDPixelROIDim0Size;
    int ADnEDPixelROIDim1Size;
    int ADnEDPixelROIDim2Size;
    int ADnEDPixelROIDim0MaxSize;
    int ADnEDPixelROIDim1MaxSize;
    int ADnEDPixelROIDim2MaxSize;
    int ADnEDPixelROIDataType;
    int ADnEDPixelROILast;
    #define LAST_ADNED_PIXELROI_PARAM ADnEDPixelROILast
                                
private:
};
#define NUM_ADNED_PIXELROI_PARAMS ((int)(&LAST_ADNED_PIXELROI_PARAM - &FIRST_ADNED_PIXELROI_PARAM + 1))
    
#endif //ADNED_PIXEL_ROI_H
