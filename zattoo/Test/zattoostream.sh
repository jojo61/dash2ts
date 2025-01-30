#!/bin/sh
#

if [ $# -ne 2 ]; then
    logger "$0: error: Invalid parameter count '$#' $*"
    exit 1
fi

# Channels.conf parameter
PARAMETER=${1}

# Iptv plugin listens this port
PORT=${2}

#echo $PARAMETER
#echo $PORT

KPATH="/storage/.kodi"
PATH=$PATH":/usr/local/bin"

/usr/local/bin/zattoostream  -p ${PORT} -k ${KPATH} -u $PARAMETER 
