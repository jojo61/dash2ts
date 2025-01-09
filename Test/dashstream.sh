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

# There is a way to specify multiple URLs in the same script. The selection is
# then controlled by the extra parameter passed by IPTV plugin to the script
case ${PARAMETER} in
    1)
        URL="https://orf1-247.mdn.ors.at/orf/orf1/qxa-247/manifest.mpd"
        ;;
    2)
        URL="https://orf2-247.mdn.ors.at/orf/orf2/qxa-247/manifest.mpd"
        ;;
    3)
        URL="https://orf3-247.mdn.ors.at/orf/orf3/qxa-247/manifest.mpd"
        ;;
    *)
        URL="https://orf2-247.mdn.ors.at/orf/orf2/qxa-247/manifest.mpd"
        ;;
esac

if [ -z "${URL}" ]; then
    logger "$0: error: URL not defined!"
    exit 1
fi

/home/jojo/kodi/build/dash2ts ${URL} ${PORT} 

#PID=${!}

#trap 'kill -INT ${PID} 2> /dev/null' INT EXIT QUIT TERM

# Waiting for the given PID to terminate
#wait ${PID}
