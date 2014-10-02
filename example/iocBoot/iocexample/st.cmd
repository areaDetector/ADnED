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

ADnEDConfig("NED", -1, -1, 1)

#asynSetTraceMask("NED",0,0x11)

#ROI plugins to extract TOF data
NDROIConfigure("DET1.TOF", 100, 0, "NED", 0, -1, -1, 0, 0)
NDROIConfigure("DET2.TOF", 100, 0, "NED", 0, -1, -1, 0, 0)
NDROIConfigure("DET3.TOF", 100, 0, "NED", 0, -1, -1, 0, 0)
NDROIConfigure("DET4.TOF", 100, 0, "NED", 0, -1, -1, 0, 0)
NDROIConfigure("DET5.TOF", 100, 0, "NED", 0, -1, -1, 0, 0)
NDROIConfigure("DET6.TOF", 100, 0, "NED", 0, -1, -1, 0, 0)
NDROIConfigure("DET7.TOF", 100, 0, "NED", 0, -1, -1, 0, 0)
NDROIConfigure("DET8.TOF", 100, 0, "NED", 0, -1, -1, 0, 0)

#Standard arrays plugin for channel access visualisation of the TOF arrays
NDStdArraysConfigure("DET1.TOF.ARR", 100, 0, "DET1.TOF", 0, -1, 0, 0)
NDStdArraysConfigure("DET2.TOF.ARR", 100, 0, "DET2.TOF", 0, -1, 0, 0)
NDStdArraysConfigure("DET3.TOF.ARR", 100, 0, "DET3.TOF", 0, -1, 0, 0)
NDStdArraysConfigure("DET4.TOF.ARR", 100, 0, "DET4.TOF", 0, -1, 0, 0)
NDStdArraysConfigure("DET5.TOF.ARR", 100, 0, "DET5.TOF", 0, -1, 0, 0)
NDStdArraysConfigure("DET6.TOF.ARR", 100, 0, "DET6.TOF", 0, -1, 0, 0)
NDStdArraysConfigure("DET7.TOF.ARR", 100, 0, "DET7.TOF", 0, -1, 0, 0)
NDStdArraysConfigure("DET8.TOF.ARR", 100, 0, "DET8.TOF", 0, -1, 0, 0)

#####################################################

## Load record instances
dbLoadRecords "db/example.db"

cd ${TOP}/iocBoot/${IOC}
iocInit


