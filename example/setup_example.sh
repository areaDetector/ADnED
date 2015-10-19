#!/bin/bash

caput -c -S BL99:Det:N1:PVName0 "neutrons"
caput -c BL99:Det:N1:EventDebug 0
caput -c BL99:Det:N1:TOFMax 160000
caput -c BL99:Det:N1:NumDetectors 4
caput -c BL99:Det:N1:ArrayCallbacks 1
caput -c BL99:Det:N1:EventUpdatePeriod 100
caput -c BL99:Det:N1:FrameUpdatePeriod 100

echo "Det1..."

caput -c BL99:Det:N1:Det1:Description "Det 1"
caput -c BL99:Det:N1:Det1:PixelNumStart 0
caput -c BL99:Det:N1:Det1:PixelNumEnd 1023
caput -c BL99:Det:N1:Det1:PixelNumSize 1024
caput -c BL99:Det:N1:Det1:PixelSizeX 32

caput -c BL99:Det:N1:Det1:TOF:EnableCallbacks 1
caput -c BL99:Det:N1:Det1:TOF:Array:EnableCallbacks 1

caput -c BL99:Det:N1:Det1:TOF:ROI:EnableCallbacks 1
caput -c BL99:Det:N1:Det1:TOF:ROI:1:Use 1

caput -c BL99:Det:N1:Det1:XY:EnableCallbacks 1
caput -c BL99:Det:N1:Det1:XY:Min1 0
caput -c BL99:Det:N1:Det1:XY:Min2 0
caput -c BL99:Det:N1:Det1:XY:Size1 32
caput -c BL99:Det:N1:Det1:XY:Size2 32
caput -c BL99:Det:N1:Det1:XY:Array:EnableCallbacks 1

caput -c BL99:Det:N1:Det1:XY:ROI:EnableCallbacks 1
caput -c BL99:Det:N1:Det1:XY:ROI:0:Use 1
caput -c BL99:Det:N1:Det1:XY:ROI:1:Use 1

caput -c BL99:Det:N1:Det1:XY:ROI:0:MinX 5
caput -c BL99:Det:N1:Det1:XY:ROI:0:MinY 5
caput -c BL99:Det:N1:Det1:XY:ROI:0:SizeX 5
caput -c BL99:Det:N1:Det1:XY:ROI:0:SizeY 5

caput -S -c BL99:Det:N1:Det1:PixelMapFile "/home/controls/epics/ADnED/master/example/mapping/pixel.map"
#caput -c BL99:Det:N1:Det1:PixelMapEnable 1
caput -S -c BL99:Det:N1:Det1:TOFTransFile "/home/controls/epics/ADnED/master/example/mapping/tof.trans"
#caput -c BL99:Det:N1:Det1:TOFTransEnable 1

caput -c BL99:Det:N1:Det1:XY:Stat:EnableCallbacks 1
caput -c BL99:Det:N1:Det1:XY:Stat:ComputeCentroid 1
caput -c BL99:Det:N1:Det1:XY:Stat:ComputeProfiles 1
caput -c BL99:Det:N1:Det1:XY:Stat:CursorX 10
caput -c BL99:Det:N1:Det1:XY:Stat:CursorY 10

caput -c BL99:Det:N1:Det1:2DType 0
caput -c BL99:Det:N1:Det1:TOFNumBins 0

echo "Det2..."

caput -c BL99:Det:N1:Det2:Description "Det 2"
caput -c BL99:Det:N1:Det2:PixelNumStart 2048
caput -c BL99:Det:N1:Det2:PixelNumEnd 3072
caput -c BL99:Det:N1:Det2:PixelNumSize 1024
caput -c BL99:Det:N1:Det2:PixelSizeX 32

caput -c BL99:Det:N1:Det2:TOF:EnableCallbacks 1
caput -c BL99:Det:N1:Det2:TOF:Array:EnableCallbacks 1

caput -c BL99:Det:N1:Det2:TOF:ROI:EnableCallbacks 1
caput -c BL99:Det:N1:Det2:TOF:ROI:1:Use 1
 
caput -c BL99:Det:N1:Det2:XY:EnableCallbacks 1
caput -c BL99:Det:N1:Det2:XY:Min1 0
caput -c BL99:Det:N1:Det2:XY:Min2 0
caput -c BL99:Det:N1:Det2:XY:Size1 32
caput -c BL99:Det:N1:Det2:XY:Size2 32
caput -c BL99:Det:N1:Det2:XY:Array:EnableCallbacks 1

caput -c BL99:Det:N1:Det2:XY:ROI:EnableCallbacks 1
caput -c BL99:Det:N1:Det2:XY:ROI:0:Use 1
caput -c BL99:Det:N1:Det2:XY:ROI:1:Use 1

caput -c BL99:Det:N1:Det2:XY:ROI:0:MinX 5
caput -c BL99:Det:N1:Det2:XY:ROI:0:MinY 5
caput -c BL99:Det:N1:Det2:XY:ROI:0:SizeX 5
caput -c BL99:Det:N1:Det2:XY:ROI:0:SizeY 5

caput -c BL99:Det:N1:Det2:XY:Stat:EnableCallbacks 1
caput -c BL99:Det:N1:Det2:XY:Stat:ComputeCentroid 1
caput -c BL99:Det:N1:Det2:XY:Stat:ComputeProfiles 1

caput -c BL99:Det:N1:Det2:2DType 0
caput -c BL99:Det:N1:Det2:TOFNumBins 0

echo "Det3 (Y/TOF 2D plot)..."

caput -c BL99:Det:N1:Det3:Description "Det 3 Y/TOF"
caput -c BL99:Det:N1:Det3:PixelNumStart 0
caput -c BL99:Det:N1:Det3:PixelNumEnd 1023
caput -c BL99:Det:N1:Det3:PixelNumSize 5120
caput -c BL99:Det:N1:Det3:PixelSizeX 32

caput -c BL99:Det:N1:Det3:TOF:EnableCallbacks 0
caput -c BL99:Det:N1:Det3:TOF:Array:EnableCallbacks 0

caput -c BL99:Det:N1:Det3:TOF:ROI:EnableCallbacks 0
caput -c BL99:Det:N1:Det3:TOF:ROI:1:Use 0
 
caput -c BL99:Det:N1:Det3:XY:EnableCallbacks 1
caput -c BL99:Det:N1:Det3:XY:Min1 0
caput -c BL99:Det:N1:Det3:XY:Min2 0
caput -c BL99:Det:N1:Det3:XY:Size1 160
caput -c BL99:Det:N1:Det3:XY:Size2 32
caput -c BL99:Det:N1:Det3:XY:Array:EnableCallbacks 1

caput -c BL99:Det:N1:Det3:XY:ROI:EnableCallbacks 1
caput -c BL99:Det:N1:Det3:XY:ROI:0:Use 1
caput -c BL99:Det:N1:Det3:XY:ROI:1:Use 1

caput -c BL99:Det:N1:Det3:XY:ROI:0:MinX 5
caput -c BL99:Det:N1:Det3:XY:ROI:0:MinY 5
caput -c BL99:Det:N1:Det3:XY:ROI:0:SizeX 5
caput -c BL99:Det:N1:Det3:XY:ROI:0:SizeY 5

caput -c BL99:Det:N1:Det3:XY:Stat:EnableCallbacks 1
caput -c BL99:Det:N1:Det3:XY:Stat:ComputeCentroid 1
caput -c BL99:Det:N1:Det3:XY:Stat:ComputeProfiles 1

caput -c BL99:Det:N1:Det3:2DType 2
caput -c BL99:Det:N1:Det3:TOFNumBins 160

echo "Det4 (PixelID/TOF 2D plot)..."

caput -c BL99:Det:N1:Det4:Description "Det 4 ID/TOF"
caput -c BL99:Det:N1:Det4:PixelNumStart 0
caput -c BL99:Det:N1:Det4:PixelNumEnd 1023
caput -c BL99:Det:N1:Det4:PixelNumSize 163840
caput -c BL99:Det:N1:Det4:PixelSizeX 32

caput -c BL99:Det:N1:Det4:TOF:EnableCallbacks 0
caput -c BL99:Det:N1:Det4:TOF:Array:EnableCallbacks 0

caput -c BL99:Det:N1:Det4:TOF:ROI:EnableCallbacks 0
caput -c BL99:Det:N1:Det4:TOF:ROI:1:Use 0
 
caput -c BL99:Det:N1:Det4:XY:EnableCallbacks 1
caput -c BL99:Det:N1:Det4:XY:Min1 0
caput -c BL99:Det:N1:Det4:XY:Min2 0
caput -c BL99:Det:N1:Det4:XY:Size1 160
caput -c BL99:Det:N1:Det4:XY:Size2 1024
caput -c BL99:Det:N1:Det4:XY:Array:EnableCallbacks 1

caput -c BL99:Det:N1:Det4:XY:ROI:EnableCallbacks 1
caput -c BL99:Det:N1:Det4:XY:ROI:0:Use 1
caput -c BL99:Det:N1:Det4:XY:ROI:1:Use 1

caput -c BL99:Det:N1:Det4:XY:ROI:0:MinX 5
caput -c BL99:Det:N1:Det4:XY:ROI:0:MinY 5
caput -c BL99:Det:N1:Det4:XY:ROI:0:SizeX 5
caput -c BL99:Det:N1:Det4:XY:ROI:0:SizeY 5

caput -c BL99:Det:N1:Det4:XY:Stat:EnableCallbacks 1
caput -c BL99:Det:N1:Det4:XY:Stat:ComputeCentroid 1
caput -c BL99:Det:N1:Det4:XY:Stat:ComputeProfiles 1

caput -c BL99:Det:N1:Det4:2DType 3
caput -c BL99:Det:N1:Det4:TOFNumBins 160

echo "Allocate Space..."
caput -c BL99:Det:N1:AllocSpace.PROC 1

echo "Auto config ROI sizes..."
caput -c BL99:Det:N1:Det1:ConfigTOFStart.PROC 1
caput -c BL99:Det:N1:Det2:ConfigTOFStart.PROC 1
caput -c BL99:Det:N1:Det3:ConfigTOFStart.PROC 1
caput -c BL99:Det:N1:Det4:ConfigTOFStart.PROC 1

caput -c BL99:Det:N1:Det1:ConfigXYStart.PROC 1
caput -c BL99:Det:N1:Det2:ConfigXYStart.PROC 1
caput -c BL99:Det:N1:Det3:ConfigXYStart.PROC 1
caput -c BL99:Det:N1:Det4:ConfigXYStart.PROC 1


echo "Done"

