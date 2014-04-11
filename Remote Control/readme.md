This folder contains Information about the remote control. 

The remote control is based on a N79E814AS20 microcontrollerand BK2423 RD Module with LNA. The circuit has been reverse engineered, but so far no attempt was made to modify the firmware.

There only seems to be a chinese datasheet available.

http://www.nuvoton.com/NuvotonMOSS/Community/ProductInfo.aspx?tp_GUID=c60884e1-c6a9-45da-8268-6be8b32e9cf2


RC-Protocol
===========

Eine Protokollbeschreibung in Deutsch ist in Protokoll.pdf zu finden. Einzelheiten zur Implementierung findet man im Verzeichnis "Firmware für AVR".

The stock JXD-385uses an RC protocol known as "V202". It has been reverse engineered and reimplemented in the Deviation firmware for Devo controls:

http://www.deviationtx.com/forum/protocol-development/1647-v202-protocol

Arduino testcode:
https://bitbucket.org/rivig/v202/src

Sourcecode of firmware:
https://bitbucket.org/PhracturedBlue/deviation/src/c960b8ea4e77/src/protocol/?at=default

The JXD385 remote control sends a stream of 16 byte packets to the quadcopter. There is no reverse channel and no acknowledgement is used. 

```
Format:

Byte 	Function  Range
0  Throttle   0-255, note: Trim will be added by remote control
1  Yaw         bit 7 indicates direction, value in [6:0] depends on speed button settings
2  Pitch       bit 7 indicates direction, value in [6:0] depends on speed button settings
3  roll        bit 7 indicates direction, value in [6:0] depends on speed button settings
4  trim yaw    2-126, 64 is neutral
5  trim pitch  2-126, 64 is neutral
6  trim roll   2-126, 64 is neutral
7  txid1       Unique ID, 0x1fe475 in my RC
8  txid2       Unique ID, 0x1fe475 in my RC
9  txid3       Unique ID, 0x1fe475 in my RC
10  unused  
11  unused  
12  unused  
13  unused  
14  Flags      0x04 is "flip button"
15  Checksum   

Example of packets:

> 74 99 00 00 40 40 3E 1F E4 75 00 00 00 00 00 43
> 80 99 00 00 40 40 3E 1F E4 75 00 00 00 00 00 4F
> FF 00 00 00 40 40 3E 1F E4 75 00 00 00 00 00 35
> FF 00 00 00 40 40 3E 1F E4 75 00 00 00 00 00 35
> FF 14 00 00 40 40 3E 1F E4 75 00 00 00 00 00 49
> FF 15 00 00 40 40 3E 1F E4 75 00 00 00 00 00 4A
```
