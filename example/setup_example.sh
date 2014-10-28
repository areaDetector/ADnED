#!/bin/bash

caput -c -S BL99:Det:ADnED:PVName "neutrons"
caput -c BL99:Det:ADnED:EventDebug 0
caput -c BL99:Det:ADnED:TOFMax 160000
caput -c BL99:Det:ADnED:NumDetectors 2
caput -c BL99:Det:ADnED:ArrayCallbacks 1

echo "Det1..."

caput -c BL99:Det:ADnED:Det1:Description "Det 1"
caput -c BL99:Det:ADnED:Det1:PixelNumStart 0
caput -c BL99:Det:ADnED:Det1:PixelNumEnd 1023

caput -c BL99:Det:ADnED:Det1:TOF:EnableCallbacks 1
caput -c BL99:Det:ADnED:Det1:TOF:Array:EnableCallbacks 1

caput -c BL99:Det:ADnED:Det1:TOF:ROI1:EnableCallbacks 1
caput -c BL99:Det:ADnED:Det1:TOF:Stat1:EnableCallbacks 1
caput -c BL99:Det:ADnED:Det1:TOF:Stat1:BgdWidth 0

caput -c BL99:Det:ADnED:Det1:XY:EnableCallbacks 1
caput -c BL99:Det:ADnED:Det1:XY:Array:EnableCallbacks 1

caput -c BL99:Det:ADnED:Det1:XY:ROI1:EnableCallbacks 1
caput -c BL99:Det:ADnED:Det1:XY:Stat1:EnableCallbacks 1
caput -c BL99:Det:ADnED:Det1:XY:Stat1:BgdWidth 0
caput -c BL99:Det:ADnED:Det1:XY:ROIScale:EnableCallbacks 1
caput -c BL99:Det:ADnED:Det1:XY:StatScale:EnableCallbacks 1
caput -c BL99:Det:ADnED:Det1:XY:StatScale:BgdWidth 0

echo "Det2..."

caput -c BL99:Det:ADnED:Det2:Description "Det 2"
caput -c BL99:Det:ADnED:Det2:PixelNumStart 2048
caput -c BL99:Det:ADnED:Det2:PixelNumEnd 3072

caput -c BL99:Det:ADnED:Det2:TOF:EnableCallbacks 1
caput -c BL99:Det:ADnED:Det2:TOF:Array:EnableCallbacks 1

caput -c BL99:Det:ADnED:Det2:TOF:ROI1:EnableCallbacks 1
caput -c BL99:Det:ADnED:Det2:TOF:Stat1:EnableCallbacks 1
caput -c BL99:Det:ADnED:Det2:TOF:Stat1:BgdWidth 0
 
caput -c BL99:Det:ADnED:Det2:XY:EnableCallbacks 1
caput -c BL99:Det:ADnED:Det2:XY:Array:EnableCallbacks 1

caput -c BL99:Det:ADnED:Det2:XY:ROI1:EnableCallbacks 1
caput -c BL99:Det:ADnED:Det2:XY:Stat1:EnableCallbacks 1
caput -c BL99:Det:ADnED:Det2:XY:Stat1:BgdWidth 0
caput -c BL99:Det:ADnED:Det2:XY:ROIScale:EnableCallbacks 1
caput -c BL99:Det:ADnED:Det2:XY:StatScale:EnableCallbacks 1
caput -c BL99:Det:ADnED:Det2:XY:StatScale:BgdWidth 0

echo "Allocate Space..."
caput -c BL99:Det:ADnED:AllocSpace.PROC 1

echo "Auto config ROI sizes..."
caput -c BL99:Det:ADnED:Det1:ConfigTOFStart.PROC 1
caput -c BL99:Det:ADnED:Det2:ConfigTOFStart.PROC 1
caput -c BL99:Det:ADnED:Det3:ConfigTOFStart.PROC 1
caput -c BL99:Det:ADnED:Det4:ConfigTOFStart.PROC 1

caput -c BL99:Det:ADnED:Det1:ConfigXYStart.PROC 1
caput -c BL99:Det:ADnED:Det2:ConfigXYStart.PROC 1
caput -c BL99:Det:ADnED:Det3:ConfigXYStart.PROC 1
caput -c BL99:Det:ADnED:Det4:ConfigXYStart.PROC 1


echo "Done"

