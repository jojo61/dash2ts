@file README.md		@brief A Streamconverter for VDR

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

$Id: 5267da021a68b4a727b479417334bfbe67bbba14 $

A Stream Converter for MPD/DASH Streams for VDR 

    o Convert DASH Streams to mpegts 
    o Adaptive Download Resolution
    o Use Widevine for decoding (not yet testet)




Good luck
jojo61

Quickstart:
-----------


Download the Git Repository. Then run make. This will create a build directory.
The converter is named dash2ts. 

In the Test directory you will find a sample channels.config for the online ORF Streams.
You need to copy the dashstream.sh file to the iptv plugin directory. Also you have
to edit the path to dash2ts and the path to Kodi in the file.
You will need the IPTV plugin from Zabrimus in order to use dash2ts.
https://github.com/Zabrimus/vdr-plugin-iptv


Run:
----
	If you want to test dash2ts directly, you can start it with:
	dash2ts -u <url_to_manifest> -p <portnr> [-k <path_to_kodi>] [-d drm_token] [-v]
	where
	  portnr is the UDP Port where the TS Data will be send
	  path_to_kodi is the path to the addon directory of Kodi default is /storage/.kodi
	  drm_token is the user_token for widevine to get the decoding Keys
	  -v Enables Debug 

	Some words to the <path_to_kodi>. I expect the following files under this path:
	<path_to_kodi>/addons/inputstream.adaptive/inputstream.adaptive.so.xx.yy.zz 
	<path_to_kodi>/addons/inputstream.adaptive/libssd_wv.so
	<path_to_kodi>/cdm/libwidevinecdm.so


DRM:
----
	DRM with widevine is not straightforward. The programm can decode the stream with
	widevine but it needs the DRM User Token to get the Key. It seems that each Operator 
	with DRM encoded streams has his own method to get the Token. So you need to implement 
	your own way to get the token and provide it to the programm. See the example for ORF 
	in dashstreams.sh in the Test directory.
	There is also a Programm for Zattoo in the zattoo directory.


Install:
--------
	1a) git

	git clone git://github.com/jojo61/dash2ts.git
	cd dash2ts
	make
	cp build/dash2ts /usr/local/bin


	
