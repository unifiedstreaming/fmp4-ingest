#!/bin/sh
set -e

if [ -n "$MODE" ]
  then
    # set env vars to defaults if not already set
    if [ -z "$ANNOUNCE" ]
      then
      export ANNOUNCE=10
    fi

    # validate required variables are set
    if [ -z "$PUB_POINT_URI" ]
      then
      echo >&2 "Error: PUB_POINT_URI environment variable is required but not set."
      exit 1
    fi
    if [ -z "$GOP_LENGTH_MS" ]
      then
      echo >&2 "Error: GOP_LENGTH_MS environment variable is required but not set."
      exit 1
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
    # Compute offset timeing information
    ms_since_epoch=$(date +%s%3N)
    multiple_gops_since_epoch=$(expr $ms_since_epoch / $GOP_LENGTH_MS)
    ism_offset=$(expr $(expr $multiple_gops_since_epoch + 1) \* $GOP_LENGTH_MS)

    # Invoke fmp4ingest using result and variables
    fmp4ingest -r --ism_offset $ism_offset --ism_use_ms -u $PUB_POINT_URI --announce $ANNOUNCE --avail $AVAIL_INTERVAL_MS $AVAIL_LENGTH_MS

    else
      echo >&2 "Error: No valid MODE set - only value LIVE-SCTE35 currently supported"
      exit 1
    fi
  else 
    exec "$@"
fi