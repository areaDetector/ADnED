
/**
 * @brief Library of functions to use for transforming the time of flight value into
 *        another parameter (for example dspace). 
 *        
 *        The code is compiled into a seperate library and linked in with 
 *        the ADnED IOC application. This makes it easy to modify or swap out with a 
 *        different library. The library must be thread safe, and only one instance
 *        should be created per detector instance. It does depend on the ADnEDGlobals.h file.
 *
 *        Objects of this class have a range of parameters and arrays for use in the 
 *        calculations. They are set by ADnED. Array storage is managed internally. The size
 *        of the arrays must be equal to the pixel ID range size for a particular detector.
 *
 *        The internal parameters that can be set, which are used in the calculations are:
 *        N * epicsUInt32 parameters
 *        N * epicsFloat64 parameters
 *        N * arrays of type epicsFloat64
 *
 *        The documentation for each calculation type will specify which parameter is used.
 *
 * @author Matt Pearson
 * @date April 2015
 */

#ifndef ADNED_TRANSFORM_H
#define ADNED_TRANSFORM_H

#include "epicsTypes.h"
#include "ADnEDGlobals.h"

class ADnEDTransform {
  
 public:
  ADnEDTransform();
  virtual ~ADnEDTransform();
  
  //Call this to transform the TOF using a particular calculation (specified by type)
  epicsFloat64 calculate(epicsUInt32 type, epicsUInt32 pixelID, epicsUInt32 tof);
  
  //Functions to set the internal parameters and arrays used for each calculation
  int setIntParam(epicsUInt32 paramIndex, epicsUInt32 paramVal);
  int setDoubleParam(epicsUInt32 paramIndex, epicsFloat64 paramVal);
  int setDoubleArray(epicsUInt32 paramIndex, const epicsFloat64 *pSource, epicsUInt32 size);

  //Debug function
  void printParams(void);

 private:
  //These are the functions that do the real work.
  epicsFloat64 calc_dspace_static(epicsUInt32 pixelID, epicsUInt32 tof);
  epicsFloat64 calc_dspace_dynamic(epicsUInt32 pixelID, epicsUInt32 tof);
  epicsFloat64 calc_deltaE(epicsUInt32 pixelID, epicsUInt32 tof);
 
  //Storage for parameters and arrays used in the calculations.
  epicsUInt32 m_intParam[ADNED_MAX_TRANSFORM_PARAMS];
  epicsFloat64 m_doubleParam[ADNED_MAX_TRANSFORM_PARAMS];
  epicsFloat64 *p_Array[ADNED_MAX_TRANSFORM_PARAMS];
  epicsUInt32 m_ArraySize[ADNED_MAX_TRANSFORM_PARAMS];

};

#endif //ADNED_TRANSFORM_H
