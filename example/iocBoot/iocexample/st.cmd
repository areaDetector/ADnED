#!../../bin/linux-x86_64/example

## You may have to change example to something else
## everywhere it appears in this file

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/example.dbd"
example_registerRecordDeviceDriver pdbbase

#####################################################
# ADnED

< $(ADNED)/st.cmd.config

#asynSetTraceIOMask("NED",0,0xFF)
#asynSetTraceMask("NED",0,0xFF)

#####################################################

#################################################
# autosave

epicsEnvSet IOCNAME example
epicsEnvSet SAVE_DIR /home/controls/var/ADnED

save_restoreSet_Debug(1)

### status-PV prefix, so save_restore can find its status PV's.
save_restoreSet_status_prefix("BL99:CS:Det:ADnED")

set_requestfile_path("$(SAVE_DIR)")
set_savefile_path("$(SAVE_DIR)")

save_restoreSet_NumSeqFiles(3)
save_restoreSet_SeqPeriodInSeconds(600)
set_pass0_restoreFile("$(IOCNAME).sav")
set_pass1_restoreFile("$(IOCNAME).sav")

#################################################

## Load record instances
dbLoadRecords "db/example.db"

cd ${TOP}/iocBoot/${IOC}
iocInit

# Create request file and start periodic 'save'
epicsThreadSleep(5)
makeAutosaveFileFromDbInfo("$(SAVE_DIR)/$(IOCNAME).req", "autosaveFields")
#makeAutosaveFileFromDbInfo("$(SAVE_DIR)/$(IOCNAME)_File.req", "autosaveFields_File")
create_monitor_set("$(IOCNAME).req", 10)
#create_monitor_set("$(IOCNAME)_File.req", 10)

epicsThreadSleep(5)
# To fix autosave sometimes restoring the pixel map files before the pixel 
# ranges have been set up, restore the pixel file records here.
#fdbrestore("$(IOCNAME)_File.sav") #This can seg fault!

dbpf "BL99:Det:ADnED:Det1:PixelMapFile.PROC","1"
dbpf "BL99:Det:ADnED:Det2:PixelMapFile.PROC","1"
dbpf "BL99:Det:ADnED:Det3:PixelMapFile.PROC","1"
dbpf "BL99:Det:ADnED:Det4:PixelMapFile.PROC","1"









