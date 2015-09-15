TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================
# Build the IOC application

PROD_IOC = example
# example.dbd will be created and installed
DBD += example.dbd

# example.dbd will be made up from these files:
example_DBD += base.dbd
example_DBD += asyn.dbd
example_DBD += ADSupport.dbd
example_DBD += NDPluginSupport.dbd
example_DBD += ADnEDSupport.dbd
example_DBD += asSupport.dbd
example_DBD += devIocStats.dbd
example_DBD += busySupport.dbd
example_DBD += sseqRecord.dbd

# example_registerRecordDeviceDriver.cpp derives from example.dbd
example_SRCS += example_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
example_SRCS_DEFAULT += exampleMain.cpp
example_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#example_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Link in the code from the support library
include $(ADCORE)/ADApp/commonDriverMakefile
example_LIBS += ADnEDSupport
example_LIBS += ADnEDTransform
example_LIBS += pvDatabase
example_LIBS += pvAccess
example_LIBS += pvData
example_LIBS += Com
example_LIBS += autosave
example_LIBS += devIocStats
example_LIBS += busy
example_LIBS += calc

# Finally link to the EPICS Base libraries
example_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

#EXAMPLE_REQ:
#	-mv /home/controls/var/ADnED/example.req /home/controls/var/ADnED/example.req.old
#	-sed 's/$$(P)/BL99:Det:ADnED:/g' $(ADNED)/ADnED.req.template > /home/controls/var/ADnED/example.req

#buildInstall: EXAMPLE_REQ

