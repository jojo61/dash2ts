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

In der Test directory you will find a sample channels.config with the online ORF Streams.
You need to copy the dashstream.sh file to the iptv plugin directory. Also you have
to edit the path to dash2ts in the file.

Currently there are some hardcoded directory path in the source file. Look at dash2ts.cpp 
around Line 200 and 205 for Path to setting.xml and inputstream-adaptive lib. You need to
change them to your needs.

DRM:
----
	DRM with widevine is not straightforward. The programm can decode the stream with
	widevine but it needs the DRM User Token to get the Key. It seems that each Operator 
	with DRM encoded streams has his own method to get the Token. So you need to implement 
	your own way to get the token and provide it to the programm. See the example for ORF 
	in dashstreams.sh in the Test directory.



Install:
--------
	1a) git

	git clone git://github.com/jojo61/dash2ts.git
	cd dash2ts
	make
	cp build/dash2ts /usr/local/bin




Known Bugs:
-----------
	DRM Streams are working but very slow 
