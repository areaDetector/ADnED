#!../../bin/linux-x86_64/example

## You may have to change example to something else
## everywhere it appears in this file

< envPaths

cd ${TOP}

epicsEnvSet("ADNED", "BL99:Det:ADnED")

## Register all support components
dbLoadDatabase "dbd/example.dbd"
example_registerRecordDeviceDriver pdbbase

#####################################################
# ADnED

ADnEDConfig("NED", -1, -1, 1)

#asynSetTraceMask("NED",0,0x11)

#ROI plugins to extract TOF data
NDROIConfigure("DET1.TOF", 100, 0, "NED", 0, -1, -1, 0, 0)
NDROIConfigure("DET2.TOF", 100, 0, "NED", 0, -1, -1, 0, 0)
NDROIConfigure("DET3.TOF", 100, 0, "NED", 0, -1, -1, 0, 0)
NDROIConfigure("DET4.TOF", 100, 0, "NED", 0, -1, -1, 0, 0)

#ROI plugins to do ROI on the TOF spectrums
NDROIConfigure("DET1.TOF.ROI1", 100, 0, "DET1.TOF", 0, -1, -1, 0, 0)
NDROIConfigure("DET2.TOF.ROI1", 100, 0, "DET2.TOF", 0, -1, -1, 0, 0)
NDROIConfigure("DET3.TOF.ROI1", 100, 0, "DET3.TOF", 0, -1, -1, 0, 0)
NDROIConfigure("DET4.TOF.ROI1", 100, 0, "DET4.TOF", 0, -1, -1, 0, 0)

#Standard arrays plugin for channel access visualisation of the TOF arrays
NDStdArraysConfigure("DET1.TOF.ARR", 100, 0, "DET1.TOF", 0, -1, 0, 0)
NDStdArraysConfigure("DET2.TOF.ARR", 100, 0, "DET2.TOF", 0, -1, 0, 0)
NDStdArraysConfigure("DET3.TOF.ARR", 100, 0, "DET3.TOF", 0, -1, 0, 0)
NDStdArraysConfigure("DET4.TOF.ARR", 100, 0, "DET4.TOF", 0, -1, 0, 0)

#Stats plugins for ROI on TOF spectra
NDStatsConfigure("DET1.TOF.STAT1", 100, 0, "DET1.TOF.ROI1", 0, -1, -1, 0, 0)
NDStatsConfigure("DET2.TOF.STAT1", 100, 0, "DET2.TOF.ROI1", 0, -1, -1, 0, 0)
NDStatsConfigure("DET3.TOF.STAT1", 100, 0, "DET3.TOF.ROI1", 0, -1, -1, 0, 0)
NDStatsConfigure("DET4.TOF.STAT1", 100, 0, "DET4.TOF.ROI1", 0, -1, -1, 0, 0)

#####################################################

#################################################
# autosave

epicsEnvSet IOCNAME example
epicsEnvSet SAVE_DIR /home/controls/var/ADnED

save_restoreSet_Debug(0)

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
create_monitor_set("$(IOCNAME).req", 5)




