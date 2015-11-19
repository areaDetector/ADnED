#!/bin/bash

caput -c -S BL99:Det:M1:PVName "neutrons"
caput -c BL99:Det:M1:EventDebug 0
caput -c BL99:Det:M1:TOFMax 160000
caput -c BL99:Det:M1:NumDetectors 2
caput -c BL99:Det:M1:ArrayCallbacks 1
caput -c BL99:Det:M1:EventUpdatePeriod 100
caput -c BL99:Det:M1:FrameUpdatePeriod 100

echo "Det1..."

caput -c BL99:Det:M1:Det1:Description "Det 1"
caput -c BL99:Det:M1:Det1:PixelNumStart 0
caput -c BL99:Det:M1:Det1:PixelNumEnd 1023
caput -c BL99:Det:M1:Det1:PixelNumSize 1024

caput -c BL99:Det:M1:Det1:TOF:EnableCallbacks 1
caput -c BL99:Det:M1:Det1:TOF:Array:EnableCallbacks 1

caput -c BL99:Det:M1:Det1:TOF:ROI:EnableCallbacks 1
caput -c BL99:Det:M1:Det1:TOF:ROI:1:Use 1

caput -c BL99:Det:M1:Det1:XY:EnableCallbacks 1
caput -c BL99:Det:M1:Det1:XY:Min1 0
caput -c BL99:Det:M1:Det1:XY:Min2 0
caput -c BL99:Det:M1:Det1:XY:Size1 32
caput -c BL99:Det:M1:Det1:XY:Size2 32
caput -c BL99:Det:M1:Det1:XY:Array:EnableCallbacks 1

caput -c BL99:Det:M1:Det1:XY:ROI:EnableCallbacks 1
caput -c BL99:Det:M1:Det1:XY:ROI:0:Use 1
caput -c BL99:Det:M1:Det1:XY:ROI:1:Use 1

caput -c BL99:Det:M1:Det1:XY:ROI:0:MinX 5
caput -c BL99:Det:M1:Det1:XY:ROI:0:MinY 5
caput -c BL99:Det:M1:Det1:XY:ROI:0:SizeX 5
caput -c BL99:Det:M1:Det1:XY:ROI:0:SizeY 5

caput -S -c BL99:Det:M1:Det1:PixelMapFile "/home/controls/epics/ADnED/master/example/mapping/pixel.map"
#caput -c BL99:Det:M1:Det1:PixelMapEnable 1
caput -S -c BL99:Det:M1:Det1:TOFTransFile "/home/controls/epics/ADnED/master/example/mapping/tof.trans"
#caput -c BL99:Det:M1:Det1:TOFTransEnable 1

echo "Det2..."

caput -c BL99:Det:M1:Det2:Description "Det 2"
caput -c BL99:Det:M1:Det2:PixelNumStart 2048
caput -c BL99:Det:M1:Det2:PixelNumEnd 3072
caput -c BL99:Det:M1:Det2:PixelNumSize 1024

caput -c BL99:Det:M1:Det2:TOF:EnableCallbacks 1
caput -c BL99:Det:M1:Det2:TOF:Array:EnableCallbacks 1

caput -c BL99:Det:M1:Det2:TOF:ROI:EnableCallbacks 1
caput -c BL99:Det:M1:Det2:TOF:ROI:1:Use 1
 
caput -c BL99:Det:M1:Det2:XY:EnableCallbacks 1
caput -c BL99:Det:M1:Det2:XY:Min1 0
caput -c BL99:Det:M1:Det2:XY:Min2 0
caput -c BL99:Det:M1:Det2:XY:Size1 32
caput -c BL99:Det:M1:Det2:XY:Size2 32
caput -c BL99:Det:M1:Det2:XY:Array:EnableCallbacks 1

caput -c BL99:Det:M1:Det2:XY:ROI:EnableCallbacks 1
caput -c BL99:Det:M1:Det2:XY:ROI:0:Use 1
caput -c BL99:Det:M1:Det2:XY:ROI:1:Use 1

caput -c BL99:Det:M1:Det2:XY:ROI:0:MinX 5
caput -c BL99:Det:M1:Det2:XY:ROI:0:MinY 5
caput -c BL99:Det:M1:Det2:XY:ROI:0:SizeX 5
caput -c BL99:Det:M1:Det2:XY:ROI:0:SizeY 5

echo "Allocate Space..."
caput -c BL99:Det:M1:AllocSpace.PROC 1

echo "Auto config ROI sizes..."
caput -c BL99:Det:M1:Det1:ConfigTOFStart.PROC 1
caput -c BL99:Det:M1:Det2:ConfigTOFStart.PROC 1
caput -c BL99:Det:M1:Det3:ConfigTOFStart.PROC 1
caput -c BL99:Det:M1:Det4:ConfigTOFStart.PROC 1

caput -c BL99:Det:M1:Det1:ConfigXYStart.PROC 1
caput -c BL99:Det:M1:Det2:ConfigXYStart.PROC 1
caput -c BL99:Det:M1:Det3:ConfigXYStart.PROC 1
caput -c BL99:Det:M1:Det4:ConfigXYStart.PROC 1


echo "Done"

