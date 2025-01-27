@file README.md		@brief View Zattoo on VDR

Copyright (c) 2025 by jojo61.  All Rights Reserved.

Contributor(s):

jojo61

License: AGPLv3

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.


View Zattoo on VDR

    o Make channels.conf
	o Supports DRM Streams (some are not working)	
    o Adaptive Download Resolution
    o Use Widevine for decoding 




Good luck
jojo61

Quickstart:
-----------


Download the Git Repository. Then run make in zattoo. This will create a build directory.
The Tool is named zattoo. 

In der Test directory you will find a sample script for IPTV..
You need to copy the zattoostream.sh file to the iptv plugin directory. Also you have
to edit the path to dash2ts and the path to Kodi in the file.

Run:
----
	First install the Zattoo plugin in Kodi and finalize the Setup. Make shure you can 
	view Zattoo Channels in Kodi.
	Then run zattoostream -c 
	This will print a channels.conf for VDR with all receivable Zattoo channels. You 
	could also run zattoostream -c >channels.conf 
	Then copy the channels you want to see (or the complete channels.conf) into the
	channels.conf from VDR.
	Thats it.


DRM:
----
	DRM with widevine is supported. I have tested it with MTV on Zattoo Free. But it
	seems that not all DRM Channels are working. If you find a Channel that works in Kodi,
	but does not work with zattoo, let me know.



Install:
--------
	1a) git

	git clone git://github.com/jojo61/dash2ts.git
	cd dash2ts
	make
	cp build/dash2ts /usr/local/bin
	cd zattoo
	make
	cp build/zattoostream /usr/local/bin


TODO:
----
	Read EPG for the Zattoo channels
