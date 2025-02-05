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
	o Supports DRM Streams	
    o Adaptive Download Resolution
    o Use Widevine for decoding 




Good luck
jojo61

Quickstart:
-----------


Download the Git Repository. Then run make in zattoo. This will create a build directory.
The Tool is named zattoostream. 

In der Test directory you will find a sample script for IPTV..
You need to copy the zattoostream.sh file to the iptv plugin directory. Also you have
to edit the path to zattoostream in the file.

Run:
----
	First install the Zattoo plugin in Kodi and finalize the Setup. Make shure you can 
	view Zattoo Channels in Kodi.
	Then run zattood -k <path_to_kodi> -c 
	This will print a channels.conf for VDR with all receivable Zattoo channels. You 
	could also run zattoostream -k <path_to_kodi> -c >channels.conf 
	Then copy the channels you want to see (or the complete channels.conf) into the
	channels.conf from VDR.
	
	For VDR you need to run zattood as daemon. In ./Test you will find a sample
	zattood.service file. This shoud be started before VDR comes up.
	
	It is needed that the vdr is running and dash2ts is in the linux PATH.

	Some words to the <path_to_kodi>. 
	I expect the following files under this path:
	<path_to_kodi>/userdata/addon_data/pvr.zattoo/parameter.sqlite
	<path_to_kodi>/userdata/addon_data/pvr.zattoo/settings.xml
	The default is $HOME/.kodi. This should work for all standard Kodi installations
	except for Debian/Ubuntu. There you need to provide the path manually.
	See also Readme for dash2ts for additional files.


DRM:
----
	DRM with widevine is supported. I have tested it with MTV on Zattoo Free. If you 
	find a Channel that works in Kodi, but does not work with zattoostream, let me know.


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
	cp build/zattood /usr/local/bin


TODO:
----
	Read Detail EPG for the Zattoo channels
