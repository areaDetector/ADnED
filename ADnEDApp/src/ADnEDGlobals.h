
#define ADNED_MAX_STRING_SIZE 256
#define ADNED_MAX_DETS 4
#define ADNED_MAX_CHANNELS 4

//ADnEDTransform params.
#define ADNED_MAX_TRANSFORM_PARAMS 6
#define ADNED_TRANSFORM_NONE 0
#define ADNED_TRANSFORM_DSPACE_STATIC 1
#define ADNED_TRANSFORM_DSPACE_DYNAMIC 2
#define ADNED_TRANSFORM_DELTAE 3

//ADnEDTransform constants
#define ADNED_TRANSFORM_MN 1.674954e-27
#define ADNED_TRANSFORM_HALF_MN 0.837477e-27

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
