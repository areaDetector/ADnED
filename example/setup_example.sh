#!/bin/bash

caput -c -S BL99:Det:ADnED:PVName "neutrons"
caput -c BL99:Det:ADnED:EventDebug 0
caput -c BL99:Det:ADnED:Det1:PixelNumStart 0
caput -c BL99:Det:ADnED:Det1:PixelNumEnd 200
caput -c BL99:Det:ADnED:Det2:PixelNumStart 2100
caput -c BL99:Det:ADnED:Det2:PixelNumEnd 2400
caput -c BL99:Det:ADnED:TOFMax 160000
caput -c BL99:Det:ADnED:NumDetectors 3
caput -c BL99:Det:ADnED:Det3:PixelNumStart 2700
caput -c BL99:Det:ADnED:Det3:PixelNumEnd 2800

