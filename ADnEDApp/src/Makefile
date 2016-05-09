TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================
# Build the IOC support library

USR_CXXFLAGS += -Wunused-but-set-variable

LIBRARY_IOC += ADnEDSupport
LIBRARY_IOC += ADnEDTransform

# Install DBD files into <top>/dbd
DBD += ADnEDSupport.dbd

# Compile and add the code to the support library
ADnEDSupport_SRCS += ADnED.cpp
ADnEDSupport_SRCS += nEDChannel.cpp
ADnEDSupport_SRCS += ADnEDPixelROI.cpp
ADnEDSupport_SRCS += ADnEDFile.cpp
ADnEDSupport_SRCS += ADnEDAxis.c
ADnEDSupport_SRCS += ADnEDPluginMask.cpp

ADnEDTransform_SRCS += ADnEDTransformBase.cpp
ADnEDTransform_SRCS += ADnEDTransform.cpp

ADnEDSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

