#!/bin/bash

caput -c -S BL99:Det:ADnED:PVName "neutrons"
caput -c BL99:Det:ADnED:EventDebug 0
caput -c BL99:Det:ADnED:Det1:PixelNumStart 0
caput -c BL99:Det:ADnED:Det1:PixelNumEnd 1023
caput -c BL99:Det:ADnED:Det2:PixelNumStart 2048
caput -c BL99:Det:ADnED:Det2:PixelNumEnd 3072
caput -c BL99:Det:ADnED:TOFMax 160000
caput -c BL99:Det:ADnED:NumDetectors 2
caput -c BL99:Det:ADnED:Det3:PixelNumStart 0
caput -c BL99:Det:ADnED:Det3:PixelNumEnd 0
caput -c BL99:Det:ADnED:ArrayCallbacks 1

caput -c BL99:Det:ADnED:Det1:TOF:EnableCallbacks 1
caput -c BL99:Det:ADnED:Det1:TOF:MinX 2049
caput -c BL99:Det:ADnED:Det1:TOF:SizeX 160000
caput -c BL99:Det:ADnED:Det1:TOF:Array:EnableCallbacks 1

caput -c BL99:Det:ADnED:Det1:TOF:EnableCallbacks 1
caput -c BL99:Det:ADnED:Det1:TOF:MinX 162050
caput -c BL99:Det:ADnED:Det1:TOF:SizeX 160000
caput -c BL99:Det:ADnED:Det1:TOF:Array:EnableCallbacks 1
