# ESP32 BLE presence detector
For my personal smarthome project to open my housedoor over the doorbell when my bluetooth tag is in range i don't find a sketch, which scans to specify beacon Mac addresses and send a mqtt message to my iobroker MQTT Server.

So i write a sketch for an ESP-32 (in my case for an ESP-32 BLE D1 Mini), which scans to my chosen Mac addresses from my used beacons (im my case Gigaset G-Tag's) and send's messages to the mqtt broker, when found a address in bluetooth scan range.

When you have problems with the upload from the sketch to the ESP32, while the Arduino editor gives the error message, that the upload file is to large, so switch the "Partition Scheme" under "Tools" to "Minimal SPIFFS (Large APPS with OTA)".
