![Image](unifiedstreaming-logo-black.jpg?raw=true)

# Unified Streaming fmp4ingest Tools <br/> DASH-IF Live Media Ingest Protocol - Interface 1 (CMAF)

## Overview

![Image](fmp4ingest_flow.png?raw=true)

fmp4ingest: CMAF ingest source defined in: 

https://dashif-documents.azurewebsites.net/Ingest/master/DASH-IF-Ingest.html

fmp4Init: retrieve the init fragment or CMAF Header from a fmp4 file

fmp4dump: print the contents of an fmp4 file to the cout, including scte markers 

ingest_receiver_node.js: simple receiver based on node.js works with short running posts and using the Streams(), 
                         stores the ingested content as cmaf track files


## Features implemented in fmp4ingest

- Parsing of fmp4 stream
- HTTP POST of init and media fragments in long running post in real-time or not
- Retransmission of init fragment in case of failures
- posting with timestamp offset (to be improved to time accurate, currently fragment accurate)
- HTTPS, HTTP, AUTH, Client TLS certificates
- sample CMAF and sparse track files added in test_files
- ingest of timed text, audio and video

## Usage 

- Push a stream in real time to publishing point: 

fmp4ingest -r -u http://localhost/pubpoint/channel1.isml 1.cmfv 2.cmfv 3.cmft 

- Receive ingest streams using node.js (https://nodejs.org/en/) 

node ingest_receiver_node.js

- Copy the init fragment to init_in.cmfv:

fmp4init in.cmfv  

## Mode: LIVE-SCTE35

If MODE=LIVE-SCTE35 is passed to the container the following environment
variables can be set to provie basic functionality to generate scte35 opportunities
(emsg) to be ingested to a live origin alongside a live stream. 

Configuration is done using environment variables:

| Variable                     | Mandatory | Usage                                    |
|------------------------------|-----------|------------------------------------------|
| PUB_POINT_URI                | yes       | URI used to ingest to                    |
| GOP_LENGTH_MS                | yes       | GOP Length in milliseconds, eg 960 (24 GOP @ 25fps - 040 * 24 = 960)                        |
| AVAIL_INTERVAL_MS            | yes       | Time in which each event should occur in milliseconds. This should ideally be a multiple of the GOP, eg 59520 (59,52s) |
| AVAIL_LENGTH_MS              | yes       | Event duration in milliseconds. Again a multiple of the gop, eg 19200  (19,2s)            |
| ANNOUNCE                     | no        | Number of seconds ahead of the event presentation should be announced to the receiving device.(Default 10s) |
| MODE                         | no        | eg, LIVE-SCTE35 otherwise commard/args need setting manually |



## New for DASH-IF ingest v1.1 distinct segment uri path based on segmentTemplate

In DASH-IF ingest v1.1. the (relative) paths of each segment may be determined 
by the SegmentTemplate, in this update the SegmentTemplate@initialization 
and SegmentTemplate@media are required to be identical for each SegmentTemplateElement.
Further, each @initialization and @media shall contain the (sub-)string $Representation$. 
The @media shall contain substring $Time$ or $Number$ (not both). 
This enables easy mapping of segment url to representations and back. 

The @media and @initialization can be given as commandline arguments to fmp4ingest

fmp4ingest --initialization $RepresentationID$-init.m4s --media $RepresentationID$-0-I-$Number$.m4s  -r -u http://localhost/pubpoint/channel1.isml 1.cmfv 2.cmfv 3.cmft 

will use the naming scheme for the segments via the string from the SegmentTemplate. 

Unified Origin does not support this naming natively, thus a script is included
based on python to generate rewrite rules:

Python get_rewrite.py in.mpd  

Will print the apache rewrite rules using mod rewrite to map this to /Streams() mapping. 
It is expected that such features will be supported natively in later releases.



## Reference 

See ingest-tools folder in this repository for source code of reference implementation of file based cmaf-ingest

a demo of ingesting cmaf content ot Unified origin using docker containers and docker compose can be found here: 

https://github.com/unifiedstreaming/cmaf-ingest-demo

## Notes

Update readme to check internal mirror for changes
