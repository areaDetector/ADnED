/**
 * ADnEDTransform base class. Concrete classes should inherit from this.
 */

#include <ADnEDTransformBase.h>

/**
 * Constructor.  
 */
ADnEDTransformBase::ADnEDTransformBase(void) {

  for (int i=0; i<ADNED_MAX_TRANSFORM_PARAMS; ++i) {
    m_intParam[i] = 0;
    m_doubleParam[i] = 0;
    m_ArraySize[i] = 0;
    p_Array[i] = NULL;
  }

  printf("ADnEDTransformBase::ADnEDTransformBase: Created OK\n");

}

/**
 * Destructor.  
 */
ADnEDTransformBase::~ADnEDTransformBase(void) {
  printf("ADnEDTransformBase::~ADnEDTransformBase\n");
}

/**
 * Set integer param.
 * @param paramIndex
 * @param paramVal
 */
int ADnEDTransformBase::setIntParam(epicsUInt32 paramIndex, epicsUInt32 paramVal) {
  
  if ((paramIndex < 0) || (paramIndex >= ADNED_MAX_TRANSFORM_PARAMS)) {
    return ADNED_TRANSFORM_ERROR;
  }
  
  m_intParam[paramIndex] = paramVal;

  return ADNED_TRANSFORM_OK;
}

/**
 * Set double param.
 * @param paramIndex
 * @param paramVal
 */
int ADnEDTransformBase::setDoubleParam(epicsUInt32 paramIndex, epicsFloat64 paramVal) {
  
  if ((paramIndex < 0) || (paramIndex >= ADNED_MAX_TRANSFORM_PARAMS)) {
    return ADNED_TRANSFORM_ERROR;
  }
  
  m_doubleParam[paramIndex] = paramVal;

  return ADNED_TRANSFORM_OK;
}

/**
 * Set array of doubles.
 * @param paramIndex Parameter index number
 * @param pSource Pointer to array of type epicsFloat64
 * @param size The number of elements to copy
 */
int ADnEDTransformBase::setDoubleArray(epicsUInt32 paramIndex, const epicsFloat64 *pSource, epicsUInt32 size) {
  
  if ((paramIndex < 0) || (paramIndex >= ADNED_MAX_TRANSFORM_PARAMS)) {
    return ADNED_TRANSFORM_ERROR;
  }

  if (size <= 0) {
    return ADNED_TRANSFORM_ERROR;
  }

  if (pSource == NULL) {
    return ADNED_TRANSFORM_ERROR;
  }
  
  if (p_Array[paramIndex]) {
    free(p_Array[paramIndex]);
    p_Array[paramIndex] = NULL;
    m_ArraySize[paramIndex] = 0;
  }
  
  m_ArraySize[paramIndex] = size;

  if (p_Array[paramIndex] == NULL) {
    p_Array[paramIndex] = static_cast<epicsFloat64 *>(calloc(m_ArraySize[paramIndex], sizeof(epicsFloat64)));
  }

  memcpy(p_Array[paramIndex], pSource, m_ArraySize[paramIndex]*sizeof(epicsFloat64));

  return ADNED_TRANSFORM_OK;
}

/**
 * For debug, print all to stdout.
 */
void ADnEDTransformBase::printParams(void) const {

  printf("ADnEDTransformBase::printParams.\n");

  //Integers
  for (int i=0; i<ADNED_MAX_TRANSFORM_PARAMS; ++i) {
    printf("  m_intParam[%d]: %d\n", i, m_intParam[i]);
  }

  //Doubles
  for (int i=0; i<ADNED_MAX_TRANSFORM_PARAMS; ++i) {
    printf("  m_doubleParam[%d]: %f\n", i, m_doubleParam[i]);
  }

  //Arrays
  for (int i=0; i<ADNED_MAX_TRANSFORM_PARAMS; ++i) {
    if ((m_ArraySize[i] > 0) && (p_Array[i])) {
      printf("  m_ArraySize[%d]: %d\n", i, m_ArraySize[i]);
      for (epicsUInt32 j=0; j<m_ArraySize[i]; ++j) {
        printf("  p_Array[%d][%d]: %f\n", i, j, (p_Array[i])[j]);
      }
    } else {
      printf("  No transformation array loaded for index %d.\n", i);
    }
  }
  
}

/**
 *
 */
void ADnEDTransformBase::setDebug(bool debug)
{
  m_debug = debug;
}

