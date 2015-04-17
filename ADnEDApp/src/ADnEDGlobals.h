
#define ADNED_MAX_STRING_SIZE 256
#define ADNED_MAX_DETS 4
#define ADNED_MAX_CHANNELS 4

//ADnEDTransform params.
#define ADNED_MAX_TRANSFORM_PARAMS 6
#define ADNED_TRANSFORM_NONE 0
#define ADNED_TRANSFORM_DSPACE_STATIC 1
#define ADNED_TRANSFORM_DSPACE_DYNAMIC 2
#define ADNED_TRANSFORM_DELTAE 3
#define ADNED_TRANSFORM_ERROR -1
#define ADNED_TRANSFORM_OK 0

//ADnEDTransform constants
#define ADNED_TRANSFORM_MN 1.674954e-27 //Mass of the neutron in Kg
#define ADNED_TRANSFORM_EV_TO_J 1.60217635e-19 // 1eV = 1.60217635e-19 Joules
#define ADNED_TRANSFORM_TOF_TO_S 1e-7 // The TOF is in units of 100ns
#define ADNED_TRANSFORM_EV_TO_mEV 1e3 // 1eV = 1e3 meV

//PVAccess related params. Used in ADnED.cpp.
#define ADNED_PV_TIMEOUT 2.0
#define ADNED_PV_PRIORITY epics::pvAccess::ChannelProvider::PRIORITY_DEFAULT
#define ADNED_PV_PROVIDER "pva"
#define ADNED_PV_REQUEST "record[queueSize=100]field()"
#define ADNED_PV_PIXELS "pixel.value" 
#define ADNED_PV_TOF "time_of_flight.value" 
#define ADNED_PV_TIMESTAMP "timeStamp"
#define ADNED_PV_SEQ "timeStamp.userTag" 
#define ADNED_PV_PCHARGE "proton_charge.value"
