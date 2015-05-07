/**
 * ADnEDTransform base class. Concrete classes should inherit from this.
 * This base class provides default implementations for the parameter storage, 
 * parameter handling and debug functions.
 */

#ifndef ADNED_TRANSFORM_BASE_H
#define ADNED_TRANSFORM_BASE_H

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "epicsTypes.h"
#include "ADnEDGlobals.h"

class ADnEDTransformBase {

 public: 
  ADnEDTransformBase();
  virtual ~ADnEDTransformBase();

  virtual epicsFloat64 calculate(epicsUInt32 type, epicsUInt32 pixelID, epicsUInt32 tof) const = 0;
  int setIntParam(epicsUInt32 paramIndex, epicsUInt32 paramVal);
  int setDoubleParam(epicsUInt32 paramIndex, epicsFloat64 paramVal);
  int setDoubleArray(epicsUInt32 paramIndex, const epicsFloat64 *pSource, epicsUInt32 size);
  void printParams(void) const;
  void setDebug(bool debug);

 protected:

  //Storage for parameters and arrays used in the calculations.
  epicsUInt32 m_intParam[ADNED_MAX_TRANSFORM_PARAMS];
  epicsFloat64 m_doubleParam[ADNED_MAX_TRANSFORM_PARAMS];
  epicsFloat64 *p_Array[ADNED_MAX_TRANSFORM_PARAMS];
  epicsUInt32 m_ArraySize[ADNED_MAX_TRANSFORM_PARAMS];

  //Flag to print out intermediate calculation steps for debug (true or false)
  bool m_debug;

};


#endif //ADNED_TRANSFORM_BASE_H
