
#include <ADnED.h>
#include <ADnEDTransform.h>

/**
 * Constructor.  
 */
ADnEDTransform::ADnEDTransform(void) {
  printf("ADnEDTransform::ADnEDTransform: Created OK\n");
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
 * Note: this could be a static factory method, returning ADnEDTransformBase objects
 *
 * Transform types are:
 *
 * ADNED_TRANSFORM_TYPE1 - multiply the TOF by a pixel ID lookup in doubleArray[0]. 
 *                         Can be used for fixed geometry instruments to calculate 
 *                         dspace. Can be used on Vulcan for example.
 *
 * ADNED_TRANSFORM_TYPE2 - calculate dspace where the theta angle changes. 
 *                         Used for direct geometry instruments like Hyspec. 
 *                         NOTE: not sure how this works yet.  
 *
 * ADNED_TRANSFORM_TYPE3 - calculate deltaE for indirect geometry instruments for 
 *                         their inelastic detectors. This uses an equation which 
 *                         depends on the mass of the neutron, L1, an array of L2 
 *                         and an array of Ef. The final energy per-pixelID, Ef, 
 *                         is known because of the use of mirrors to select energy. 
 *                         We can calculate incident energy using the known final 
 *                         energy and the TOF. So we are calculating energy 
 *                         transfer for each event.
 *                              
 *                         deltaE = (1/2)Mn * (L1 / (TOF - (L2*sqrt(Mn/(2*Ef))) ) )**2 - Ef
 *                         where:
 *                         Mn = mass of neutron in 1.674954 × 10-27
 *                         L1 = constant (in meters)
 *                         Ef and L2 are double arrays based on pixelID. 
 *                         The Ef input array must be in units of MeV. The L2 array is in meters.
 *                         TOF = time of flight (in seconds)
 *
 *                         Energy must be in Joules (1 eV = 1.602176565(35) × 10−19 J)
 * 
 *                         Once the deltaE has been obtained in Joules, it is converted back to MeV. 
 */
epicsFloat64 ADnEDTransform::calculate(epicsUInt32 type, epicsUInt32 pixelID, epicsUInt32 tof) const {
  
  if (type < 0) {
    return ADNED_TRANSFORM_ERROR;
  }
  
  if (pixelID < 0) {
    return ADNED_TRANSFORM_ERROR;
  }

  if (type == ADNED_TRANSFORM_TYPE1) {
    return calc_dspace_static(pixelID, tof);
  } else if (type == ADNED_TRANSFORM_TYPE2) {
    return calc_dspace_dynamic(pixelID, tof);
  } else if (type == ADNED_TRANSFORM_TYPE3) {
    return calc_deltaE(pixelID, tof);
  }

  return ADNED_TRANSFORM_ERROR;
  
}

/**
 * Type = ADNED_TRANSFORM_TYPE1
 * Parameters used:
 *   p_Array[0]
 */
epicsFloat64 ADnEDTransform::calc_dspace_static(epicsUInt32 pixelID, epicsUInt32 tof) const {
  
  epicsFloat64 result = 0;

  if (m_debug) {
    printf("ADnEDTransform::calc_dspace_static. pixelID: %d, tof: %d\n", pixelID, tof);
  }

  if ((p_Array[0] != NULL) && (pixelID < m_ArraySize[0])) {
    result = tof*((p_Array[0])[pixelID]);
    if (m_debug) {
      printf("  result: %f\n", result);
    }
    return result;
  }

  return ADNED_TRANSFORM_ERROR;
}

/**
 * Type = ADNED_TRANSFORM_TYPE2
 */
epicsFloat64 ADnEDTransform::calc_dspace_dynamic(epicsUInt32 pixelID, epicsUInt32 tof) const {
  return ADNED_TRANSFORM_ERROR;
}

/**
 * Type = ADNED_TRANSFORM_TYPE3
 * Parameters used:
 *   ADNED_TRANSFORM_MN - The mass of the neutron in Kg
 *   m_doubleParam[0] - L1 in meters
 *   p_Array[0] - Ef in MeV (indexed by pixel ID)
 *   p_Array[1] - L2 in meters (indexed by pixel ID)
 *
 * The equation uses SI units. So the input parameters are converted internally. 
 *   
 */
epicsFloat64 ADnEDTransform::calc_deltaE(epicsUInt32 pixelID, epicsUInt32 tof) const {
  
  epicsFloat64 deltaE = 0;
  epicsFloat64 Ef = 0;
  epicsFloat64 Ei = 0;
  epicsFloat64 tof_s = 0;

  if (m_debug) {
    printf("ADnEDTransform::calc_deltaE. pixelID: %d, tof: %d\n", pixelID, tof);
  }

  //Checks
  if ((p_Array[0] == NULL) || (p_Array[1] == NULL)) {
    if (m_debug) {
      printf("  Arrays are NULL.\n");
    }
    return ADNED_TRANSFORM_ERROR;
  } 
  if ((p_Array[0][pixelID] <= 0) || (p_Array[1][pixelID] <= 0)) {
    if (m_debug) {
      printf("  Array elements are zero.\n");
    }
    return ADNED_TRANSFORM_ERROR;
  } 
  if (tof == 0) {
    if (m_debug) {
      printf("  TOF is zero.\n");
    }
    return ADNED_TRANSFORM_ERROR;
  }

  //Convert Ef (in meV) to Joules
  Ef = (p_Array[0][pixelID] / ADNED_TRANSFORM_EV_TO_mEV) * ADNED_TRANSFORM_EV_TO_J;  
  //Convert TOF to seconds
  tof_s = static_cast<epicsFloat64>(tof) * ADNED_TRANSFORM_TOF_TO_S;

  if (m_debug) {
    printf("  Ef in Joules: %g\n", Ef);
    printf("  TOF in seconds: %g\n", tof_s);
  }
  
  Ei = 0.5 * ADNED_TRANSFORM_MN * pow(m_doubleParam[0] / (tof_s - (p_Array[1][pixelID] * sqrt(ADNED_TRANSFORM_MN/(2*Ef))) ),2);
  if (m_debug) {
    printf("  Ei in Joules: %g\n", Ei);
  }

  deltaE = Ei - Ef;
  
  //Convert back to meV
  deltaE = (deltaE / ADNED_TRANSFORM_EV_TO_J) * ADNED_TRANSFORM_EV_TO_mEV;

  if (m_debug) {
    printf("  deltaE: %f.\n", deltaE);
  }

  return deltaE;

}

