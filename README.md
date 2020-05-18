![Image](unifiedstreaming-logo-black.jpg?raw=true)

# fmp4ingest Tools

## Overview

![Image](fmp4ingest_flow.png?raw=true)

fmp4ingest: CMAF ingest source defined in: 

https://dashif-documents.azurewebsites.net/Ingest/master/DASH-IF-Ingest.html

fmp4Init: retrieve the init fragment or CMAF Header from a fmp4 file

fmp4sparse: retrieve a sparse metadata track from a CMAF file with inband emsg

fmp4dump: print the contents of an fmp4 file to the cout, including scte markers 

fmp4DashEvent: convert a sparse track to an XML event stream

dashEventfmp4: convert a DASH mpd or SMIL with events in it to a sparse metadata track

ingest_receiver_node.js: simple receiver based on node.js works with short running posts and using the Streams(), 
                         stores the ingested content as cmaf track files


## Features implemented in fmp4ingest

- Parsing of fmp4 stream
- HTTP POST of init and media fragments in long running post in real-time or not
- Retransmission of init fragment in case of failures
- posting with timestamp offset (to be improved to time accurate, currently fragment accurate)
- HTTPS, HTTP, AUTH, Client TLS certificates
- sample cmaf and sparse track files added in test_files
- ingest of timed text, audio and video

## Usage 

- Push a stream in real time to publishing point: 

fmp4ingest -r -u http://localhost/pubpoint/channel1.isml 1.cmfv 2.cmfv 3.cmft 

- Receive ingest streams using node.js (https://nodejs.org/en/) 

node ingest_receiver_node.js

- Copy the init fragment to init_in.cmfv:

fmp4init in.cmfv  

- Convert a cmfv file with inband messages to a sparse track as defined in the ingest spec:

fmp4sparse in.cmfv out.cmfm  

- Print the content of a cmaf or fmp4 to cout:

fmp4dump in.cmfv  

- convert an mpd with events to a sparse metatrack 

dashEventfmp4 in.mpd out.cmfm


## Reference 

see ingest-tools folder in this repository for source code of reference implementation of file based cmaf ingest

a demo of ingesting cmaf content ot Unified origin using docker containers and docker compose can be found here: 

https://github.com/unifiedstreaming/cmaf-ingest-demo
