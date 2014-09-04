#!../../bin/linux-x86_64/example

## You may have to change example to something else
## everywhere it appears in this file

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/example.dbd"
example_registerRecordDeviceDriver pdbbase

## Load record instances
dbLoadRecords "db/example.db"

#####################################################
# ADnED

ADnEDConfig("NED", "neutrons", -1, -1, 1)

#####################################################


cd ${TOP}/iocBoot/${IOC}
iocInit


