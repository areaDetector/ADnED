
#include <ADnED.h>
#include <ADnEDTransform.h>

/**
 * Constructor.  
 */
ADnEDTransform::ADnEDTransform(void) {
  
  printf(" TOF Transform Types:\n");
  printf("  ADNED_TRANSFORM_DSPACE_STATIC: %d\n", ADNED_TRANSFORM_DSPACE_STATIC);
  printf("  ADNED_TRANSFORM_DSPACE_DYNAMIC: %d\n", ADNED_TRANSFORM_DSPACE_DYNAMIC);
  printf("  ADNED_TRANSFORM_DELTAE: %d\n", ADNED_TRANSFORM_DELTAE);

  for (int i=0; i<ADNED_MAX_TRANSFORM_PARAMS; ++i) {
    m_intParam[i] = 0;
    m_doubleParam[i] = 0;
    m_ArraySize[i] = 0;
    p_Array[i] = NULL;
  }

  printf("Transform::Transform: Created OK\n");
}

/**
 * Destructor.  
 */
ADnEDTransform::~ADnEDTransform(void) {
  printf("Transform::~Transform\n");
}

/**
 * Transform the time of flight value into a another parameter. The calculation used is
 * specified by the type input param. The associated pixel ID is required for any 
 * calculation involving a pixelID based lookup in an array.
 *
 * Transform types are:
 *
 * ADNED_TRANSFORM_DSPACE_STATIC - multiply the TOF by a pixel ID lookup in doubleArray[0]. 
 *                                 Can be used for fixed geometry instruments to calculate 
 *                                 dspace. Can be used on Vulcan for example.
 *
 * ADNED_TRANSFORM_DSPACE_DYNAMIC - calculate dspace where the theta angle changes. 
 *                                  Used for direct geometry instruments like Hyspec. 
 *                                  NOTE: not sure how this works yet.  
 *
 * ADNED_TRANSFORM_DELTAE - calculate deltaE for indirect geometry instruments for 
 *                          their inelastic detectors. This uses an equation which 
 *                          depends on the mass of the neutron, L1, an array of L2 
 *                          and an array of Ef. The final energy per-pixelID, Ef, 
 *                          is known because of the use of mirrors to select energy. 
 *                          We can calculate incident energy using the known final 
 *                          energy and the TOF. So we are calculating energy 
 *                          transfer for each event.
 *                              
 *                          deltaE = (1/2)Mn * (L1 / (TOF - (L2*sqrt(Mn/(2*Ef))) ) )**2 - Ef
 *                          where:
 *                          Mn = mass of neutron in 1.674954 × 10-27
 *                          L1 = constant (in meters)
 *                          Ef and L2 are double arrays based on pixelID
 *                          TOF = time of flight (in seconds)
 *
 *                          Energy must be in Joules (1 eV = 1.602176565(35) × 10−19 J)
 * 
 *                          Once the deltaE has been obtained in Joules, it is converted back to eV. 
 */
epicsFloat64 ADnEDTransform::calculate(epicsUInt32 type, epicsUInt32 pixelID, epicsUInt32 tof) {
  
  if (type < 0) {
    return -1;
  }
  
  if (pixelID < 0) {
    return -1;
  }
  
  if (type == ADNED_TRANSFORM_DSPACE_STATIC) {
    return calc_dspace_static(pixelID, tof);
  } else if (type == ADNED_TRANSFORM_DSPACE_DYNAMIC) {
    return calc_dspace_dynamic(pixelID, tof);
  } else if (type == ADNED_TRANSFORM_DELTAE) {
    return calc_deltaE(pixelID, tof);
  }
  
}

/**
 * Type = ADNED_TRANSFORM_DSPACE_STATIC
 * Use a pixel ID dependant multiplier on the TOF.
 * This uses (doubleArray[0])[pixelID]
 */
epicsFloat64 ADnEDTransform::calc_dspace_static(epicsUInt32 pixelID, epicsUInt32 tof) {
  if ((p_Array[0] != NULL) && (pixelID < m_ArraySize[0])) {
    return tof*(p_Array[0])[pixelID];
  }
  return 0;
}

/**
 * Type = ADNED_TRANSFORM_DSPACE_DYNAMIC
 */
epicsFloat64 ADnEDTransform::calc_dspace_dynamic(epicsUInt32 pixelID, epicsUInt32 tof) {
  return 0;
}

/**
 * Type = ADNED_TRANSFORM_DELTAE
 */
epicsFloat64 ADnEDTransform::calc_deltaE(epicsUInt32 pixelID, epicsUInt32 tof) {
  return 0;
}

/**
 * Set integer param.
 * @param paramIndex
 * @param paramVal
 */
int ADnEDTransform::setIntParam(epicsUInt32 paramIndex, epicsUInt32 paramVal) {
  
  if ((paramIndex < 0) || (paramIndex >= ADNED_MAX_TRANSFORM_PARAMS)) {
    return -1;
  }
  
  m_intParam[paramIndex] = paramVal;

  return 0;
}

/**
 * Set double param.
 * @param paramIndex
 * @param paramVal
 */
int ADnEDTransform::setDoubleParam(epicsUInt32 paramIndex, epicsFloat64 paramVal) {
  
  if ((paramIndex < 0) || (paramIndex >= ADNED_MAX_TRANSFORM_PARAMS)) {
    return -1;
  }
  
  m_doubleParam[paramIndex] = paramVal;

  return 0;
}

/**
 * Set array of doubles.
 * @param pSource Pointer to array of type epicsFloat64
 * @param size The number of elements to copy
 */
int ADnEDTransform::setDoubleArray(epicsUInt32 paramIndex, const epicsFloat64 *pSource, epicsUInt32 size) {
  
  if ((paramIndex < 0) || (paramIndex >= ADNED_MAX_TRANSFORM_PARAMS)) {
    return -1;
  }

  if (size <= 0) {
    return -1;
  }

  if (pSource == NULL) {
    return -1;
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

  return 0;
}

/**
 * For debug, print all to stdout.
 */
void ADnEDTransform::printParams() {

  printf("ADnEDTransform::printParams.\n");

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

