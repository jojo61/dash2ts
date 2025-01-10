#!/bin/sh
#
# iptvstream.sh can be used by the VDR iptv plugin to transcode external
# sources
#
# (C) 2007 Rolf Ahrenberg, Antti Seppälä
#
# iptvstream.sh is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this package; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
# MA 02110-1301, USA.

if [ $# -ne 2 ]; then
    logger "$0: error: Invalid parameter count '$#' $*"
    exit 1
fi

# Channels.conf parameter
PARAMETER=${1}

# Iptv plugin listens this port
PORT=${2}

DRM=""

# There is a way to specify multiple URLs in the same script. The selection is
# then controlled by the extra parameter passed by IPTV plugin to the script
case ${PARAMETER} in
    1)
        #URL="https://orf1-247.mdn.ors.at/orf/orf1/qxa-247/manifest.mpd"
	URL="https://api-tvthek.orf.at/api/v4.3/livestream/14379066"
        ;;
    2)
        #URL="https://orf2-247.mdn.ors.at/orf/orf2/qxa-247/manifest.mpd"
	URL="https://api-tvthek.orf.at/api/v4.3/livestream/14378920"
        ;;
    3)
        #URL="https://orf3-247.mdn.ors.at/orf/orf3/qxa-247/manifest.mpd"
	URL="https://api-tvthek.orf.at/api/v4.3/livestream/14378956"
        ;;
    4)
        #URL="https://orfs.mdn.ors.at/orf/orfs/drmqxa/manifest.mpd"
	URL="https://api-tvthek.orf.at/api/v4.3/livestream/14379001"
        ;;
    *)
	URL="https://api-tvthek.orf.at/api/v4.3/timeshift/channel/3026625/sources"
        ;;
esac

if [ -z "${URL}" ]; then
    logger "$0: error: URL not defined!"
    exit 1
fi

JSON=$(wget --header='Authorization: Basic b3JmX29uX3Y0MzpqRlJzYk5QRmlQU3h1d25MYllEZkNMVU41WU5aMjhtdA=='\
              --header='User_Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/116.0.0.0 Safari/537.36'\
              -q -O /tmp/channel.json $URL)
#JSON=${CMD}
drm_token=$(jq .drm_token /tmp/channel.json)
EURL=$(jq .sources.dash /tmp/channel.json | grep qxa | cut -f1 -d ":" --complement | tr "\"," " " | cut -f1 -d "?")

echo URL: $EURL
echo Token: $drm_token



/home/jojo/dash2ts/build/dash2ts ${EURL} ${PORT} ${drm_token} 

#PID=${!}

#trap 'kill -INT ${PID} 2> /dev/null' INT EXIT QUIT TERM

# Waiting for the given PID to terminate
#wait ${PID}
