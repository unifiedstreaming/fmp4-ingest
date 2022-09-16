# Media-ingest

Code for fmp4/CMAF ingest defined in:

https://dashif-documents.azurewebsites.net/Ingest/master/DASH-IF-Ingest.html

Implement DASH-IF Event Processing Framework

# Overview 

fmp4ingest: CMAF ingest source defined in: 

https://dashif-documents.azurewebsites.net/Ingest/master/DASH-IF-Ingest.html

fmp4Init: retrieve the init fragment or CMAF Header from a fmp4 file

fmp4dump: print the contents of an fmp4 file to the cout, including scte markers 

ingest_receiver_node.js: simple receiver based on node.js works with short running posts and using the Streams(), 
                         stores the ingested content as cmaf track files


# Features implemented in fmp4ingest

- Parsing of fmp4 stream
- HTTP POST of init and media fragments in long running post in real-time or not
- Retransmission of init fragment in case of failures
- posting with timestamp offset (to be improved to time accurate, currently fragment accurate)
- HTTPS, HTTP, AUTH, Client TLS certificates
- sample cmaf and sparse track files added in test_files
- ingest of timed text, audio and video


# Commandline 
```bash
Usage: fmp4ingest <options> <input_files>
 -u url                       Publishing Point URL
 -r, --realtime               Enable realtime mode
 -l, --loop                   Enable looping arg1 + 1 times (without this the track ingest would loop over only once)
 --wc_offset                  (boolean )Add a wallclock time offset from time server for converting VoD (0) asset to Live
 --ism_offset                 insert a fixed value for the wallclock time offset instead of using a remote time source uri
 --ism_use_ms                 indicates that the ism_offset is given in milliseconds
 --wc_uri                     uri for fetching wall clock time default time.akamai.com
 --initialization             SegmentTemplate@initialization sets the relative path for init segments, shall include $RepresentationID$
 --media                      SegmentTemplate@media sets the relative path for media segments, shall include $RepresentationID$ and $Time$ or $Number$
 --avail                     signal an advertisment slot every arg1 ms with duration of arg2 ms
 --dry_run                    Do a dry run and write the output files to disk directly for checking file and box integrity
 --announce                   specify the number of seconds in advance to presenation time to send an avail (default is 60 seconds set to 0 to have the avails in sync with media)
 --auth                       Basic Auth Password
 --aname                      Basic Auth User Name
 --sslcert                    TLS 1.2 client certificate
 --sslkey                     TLS private Key
 --sslkeypass                 passphrase
 <input_files>                  CMAF files to ingest (.cmf atvm)
```
# Examples 

- Push a stream in real time to publishing point: 

fmp4ingest -r -u http://localhost/pubpoint/channel1.isml 1.cmfv 2.cmfv 3.cmft 

- Receive ingest streams using node.js (https://nodejs.org/en/) 

node ingest_receiver_node.js

- Copy the init fragment to init_in.cmfv:

fmp4_init in.cmfv  

- Print the content of a cmaf or fmp4 to cout:

fmp4dump in.cmfv  

## New for DASH-IF ingest v1.1 distinct segment uri path based on SegmentTemplate

In DASH-IF ingest v1.1. the (relative) paths of each segment may be determined 
by the SegmentTemplate, in this case the SegmentTemplate@initialization 
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

Will print the Apache rewrite rules using mod_rewrite to map this to /Streams() mapping. 
It is expected that such features will be supported natively in later releases.
