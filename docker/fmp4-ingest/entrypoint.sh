#!/bin/sh
set -e

if [ -n "$MODE" ]
  then
    # set env vars to defaults if not already set
    if [ -z "$ANNOUNCE" ]
      then
      export ANNOUNCE=4000
    fi

    # validate required variables are set
    if [ -z "$PUB_POINT_URI" ]
      then
      echo >&2 "Error: PUB_POINT_URI environment variable is required but not set."
      exit 1
    fi
    if [ -z "$SEG_LENGTH_MS" ]
      then
      echo >&2 "Info: Setting SEG_LENGTH_MS variable to 1920."
      export SEG_LENGTH_MS=1920
    fi
    if [ -z "$AVAIL_LENGTH_MS" ]
      then
      echo >&2 "Error: AVAIL_LENGTH environment variable is required but not set."
      exit 1
    fi

    if [ -z "$AVAIL_INTERVAL_MS" ]
      then
      echo >&2 "Error: AVAIL_INTERVAL environment variable is required but not set."
      exit 1
    fi

    if [ "$MODE" == "LIVE-SCTE35" ]
    then
    sleep 5

    push_markers --track_id 100 --announce $ANNOUNCE --seg_dur $SEG_LENGTH_MS  --vtt -r -u $PUB_POINT_URI --avail $AVAIL_INTERVAL_MS $AVAIL_LENGTH_MS 

    else
      echo >&2 "Error: No valid MODE set - only value LIVE-SCTE35 currently supported"
      exit 1
    fi
  else 
    exec "$@"
fi