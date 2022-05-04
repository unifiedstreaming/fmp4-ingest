# EventMessageTrackSamples

ISO/IEC 23001-18 defines the Event Message Track format.

This format can be used the carry DASH Event Message in ISO-BMFF tracks.

This repository contains sample code for converting a list of DASH Event Messages based on ISO/IEC 23009-1 to 
Event Message Track Samples and track segments. 

The algorithm for conversion from clause 9.2 of ISO/IEC 23001-18
is implemented. This triggers a sample boundary at each time 
the set of active events changes.

The code is available under MIT license.

## find_event_samples.hpp 
Includes a function for converting a list of DASH Event Messages to 
Event Message Track samples output. Only detects sample boundaries and contents,
the function does not write any compliant output. The function takes 
segment start and end, allowing it to be used for generation of Event 
Message Track segments, i.e. detecting all samples and content between 
segment start and segment end.

Only the conceptual conversion from DASHEvents to samples is covered, 
this code does not write any MP4, DASH, ISO-BMFF 
compliant output. 

Libraries or code writing such compliant content may include the provided 
function to detect sample boundaries and contents and implement
the conversion required to implement the Event Message Track Format.

## generate_example.cpp
Small program to create examples of Event message track sample formatting. 

It prints random events to the std::out. 
Subsequently it prints the Event message track samples formatting.

Examples are for illustrative purpose and written to std::out.

## dash_event_fmp4.cpp

Program for converting MPD events in an EventStream Element with optionally added attributes @startTime and @endTime to 
fragmented (CMAF based) event message track

usage dash_event_fmp4 in.mpd out_event_track.cmfm track_id target_segment_duration (0=entire track)

Note that if the duration is not known from the mpd you need to set it in the Manifest EventStream@endTime,
as the cmaf event track needs to have a duration. 

## fmp4_dash_event.cpp

Program for converting an event track (CMAF based) back to XML format based on EventStream

usage fmp4_dash_event in_event_track.cmfm out.mpd warning does only work for CMAF based event tracks

## gen_avails.cpp

Program for creating an event track (CMAF based) and XML representation of periodic splice inserts to signal ad breaks.

usage gen_avail_track track_duration[ms] segment_duration[ms] slot_duration[ms] avail_interval[ms]

e.g. 

gen_avail_track 600000 2000 30000 180000

generates an avail event message track of 600 seconds, with 2 second segments and slots of 30 seconds every 180 seconds. 
The avails use the splice insert command from SCTE-35

## print_event_samples.cpp

prints the contents of event samples (emib, emeb) etc... of an event track

## unittest.cpp
Unit tests validation using catch framework https://github.com/catchorg/Catch2
