# Media and Timed Meta-Data Ingest for Distributed Media Processing Entities Draft
* Status: Draft
* Status: For Discussion

## Overview 
This specification describes a protocol and format for media ingest from a live encoder or other media source towards media processing entities such as publishing points, origins and/or content delivery networks. These entities provide further processing of the media as needed for advanced streaming workflows.

**Diagram 1**

```text
Live Media Source (e.g. live encoder) -> Publishing Point (media processing entity) -> Content Delivery Network -> End User/client
```

The workflow architecture diagram is shown in Diagram 1. A live encoder or media source pushes media towards a media processing entity that can be either passive (e.g. pass through) or active (e.g. altering or processing the media).  The media processing entity provides functionalities for further delivery such as content stitching, encryption, packaging, manifest generation, transcoding, scalable delivery etc. Ingest of live media is still often based on proprietary protocols. This often leads to interop issues as implementations and specifications are often incomplete or not based on the latest technologies and standards used in the industry (e.g. timed meta-data, emerging encoding standards like HEVC). 

In practice, interop problems relate to the file format or encoder settings or the transmission protocol. Protocols on top of TCP, UDP or HTTP are often used to connect the live encoder/media source to the processing entity. When multiple live encoders/media sources serve as ingest it is important that different encoders adhere to the same protocol. Other interop issues  occur when passing live meta-data from broadcast workflows into cloud media processing such as based on ID3 tags, SCTE-35 markers. This type of meta-data ingest will also be addressed in this specification. Further meta-data like timed text, captions, subtitles and images are important.In addition meta-data for supporting media processing operations such as transcoding, content stitching etc. can be useful e.g. camera information, motion vector data for guided transcoding etc and shall not be exclused in extensions of this specification. 

This text aims at specification for the interop of live media ingest that includes both advanced media like HEVC and advanced meta-data such as ad-markers, timed/text, closed captions and so on.

## Conformance Notation

The key words "MUST," "MUST NOT," "REQUIRED," "SHALL," "SHALL NOT," "SHOULD," "SHOULD NOT," "RECOMMENDED," "MAY," and "OPTIONAL" in this document are to be interpreted as they are described in RFC 2119 [17].

## Terminology 

This specification uses the following terminology.

ISO BMFF: ISO BMFF refers to the file/data formatting described in [3].

**Ftyp**: the filetype and compatibility “ftyp” box as described in the ISO BMFF [3] that describes the brand of the media file format brand. 

**Moov**: the container for all metadata formated “moov” box as described in the ISO BMFF base media file format [3]

**Moof**: the movie fragment “moof” box as described in the ISO BMFF base media file format [3] that describes the meta data of a fragment of media.

**Mdat**: the media data container “mdat” box contained in an ISO BMFF  [3], this box contains the physical media samples for example the coded using the HEVC [4] codec or other codecs.

**Kind**: the track kind “kind” box defined in the ISO BMFF [3]  to label a track with its usage (i.e. user defined data) 

**Mfra**: the movie fragment random access “mfra” box defined in the ISO BMFF [3]  to signal random access samples (these are samples that require not prior or other samples for decoding) [3].

**Tfdt**: the TrackFragmentDecodeTimeBox box “tfdt” in ISO BMFF base media file format [3] used to signal decode time of the media 
fragment moof box.

**Mdhd**: The media header Box as defined in [3] , this box contains information on the media such as timescale, duration, language using ISO 639‐2/T code

**Pssh** The protection specific system header box defined in [14] that can be used to signal the content protection information according to the MPEG Common Encryption (CENC)

**Sinf** Protection Scheme Information Box defined in [3] that provides information on the encryption scheme used in the file

**Elng** extended language tag defined in [3] that can overide the language information

**Nmhd**: The null media header Box as defined in [3] to signal a track for which no specific media header is defined 

**HTTP**: Hyper Text Transfer Protocol, version 1.1 as specified by RFC 2626 [5]

**HTTP POST**: Command used in the Hyper Text Transfer Protocol for sending data from a source to a destination [5]

**fragmentedMP4stream**: stream of ISO BMFF [3] fragments (moof and mdat), the payload of the live media ingest.

**POST_URL**:  Target URL of a POST command in the HTTP protocol for pushing data from a source to a destination. 

**TCP**: Transmission Control Protocol (TCP) as defined in RFC 793 [6]

**URI_SAFE_IDENTIFIER**: identifier/string formatted according to [7]

**Connection**: connection setup between a host and a source.

**Live stream event**: the total media broadcast stream of the encoder ingest or source to be pushed to the destination. 

**(Live) encoder**: entity performing live encoding producing a high quality live encoded broadcast stream

**(Media) Ingest source**: a media source ingesting media content to a processing function, typically a live encoder but not restricted to this, the media ingest source could by any type of media ingest source such as a stored file that is send in partial chunks towards the media processing entity

**Publishing point**: entity used to publish the media content, consumes/receives the incoming media ingest stream

**Media processing function/entity**: entity used to process media content, receives/consumes a media ingest stream, which is activily processed before further delivery.

## Overall Media Ingest Protocol Behavior Specification
The media and timed meta-data ingest specification uses multiple HTTP POST and/or PUT requests to transmit the manifest followed by encoded media data packaged in fragmented ISO BMFF [3]. The subsequent posted segments correspond to those decribed in the manifest.  Each HTTP POST sends a complete manifest or media segment towards the processing entity. The sequence of POST commands starts with the the manifest and init segments that includes header boxes (ftyp and moov boxes), and continues with a sequence of segments (combinations of moof and mdat boxes) as defined in the initial manifest. 

An example of the Fragmented Media Ingest POST URL targeting the publishing point is: http://HostName/presentationPath/manifestpath/rsegmentpath/Identifier

The PostURL the syntax is defined as follows using the IETF RFC 5234 ANB [14] to specify the structure. 
* PostURL = Protocol “://” BroadcastURL Identifier
* Protocol = "http" / "https" 
* BroadcastURL = HostName "/" PresentationPath 
* HostName = URI_SAFE_IDENTIFIER 
* PresentationPath = URI_SAFE_IDENTIFIER 
* ManifestPath = URI_SAFE_IDENTIFIER 
* Rsegmentpath = URI_SAFE_IDENTIFIER 
* Identifier = segment_file_name

In this PostURL the server address is typically the hostname or IP address of the media processing entity or publishing point. The presentation path is the path to the specific presentation at the publishing point. The manifest path can be used to signal the specific manifest of the presentation.  The rsegmentpath can be a different extended path based on the relative paths in the manifest file. The identifier desribes the filename of the segment as described in the manifest. The live source sender first sends the manifest to the path http://mypublishingpoint/presentation allowing the receiving entity to setup reception paths for the following segmentsand manifests. The payload and content of the media ingest stream are manifests described by the manifest and segments based on fragmented MPEG-4 [3]. The fragmented MPEG-4 segment streams can be defined using the IETF RFC 5234 ANB [14] as follows. 

* fragmentedMP4stream = HeaderBoxes Fragments
* HeaderBoxes = FileType Moov
* Fragments = X Fragment
* Fragment = moof mdat 

The communication between the live encoder/media ingest source and the receiving procesing entitiy/publishing point follows the following requirements.

1.	The live encoder or ingest source communicates to the publishing point/processing entity using the HTTP POST method as defined in the HTTP protocol [5], or in the case for manifest updates the HTTP PUT Method
2.	The live encoder or ingest source SHOULD start the broadcast by sending an HTTP POST request with an empty “body” (zero content length) by using the same POSTURL. This can help the live encoder or media ingest source to quickly detect whether the live ingest publishing point is valid, and if there are any authentication or other conditions required. Per HTTP protocol, the server can't send back an HTTP response until the entire request, including the POST body, is received.
3. The live encoder/media source SHOULD use secured transmission using HTTPS protocol as specified in [18] for connecting to the receiving processing entity or publishing point
4. In case HTTPS protocol is used basic authentication HTTP AUTH or better methods like TLS client certificates SHOULD be used to secure the connection
5. Before sending the segments based on fragmented MPEG-4 the live encoder/source SHOULD send a manifest with the following the limitations/constraints 
   - 1. Only relative URL paths to be used for each segment 
   - 2. Only unique paths are used for each new presentation 

4. In case the manifest contains these relative paths, these paths MAY be used in combination with the  POST_URL + relative URLs to POST each of the different segments from the live encoder to the processing entity. In case the manifest contains no relative paths the segments SHOULD be posted to the original POST_URL specified by the service.

For example:

Encoder POSTs a manifest at some processing entity: http://someprocessingservice.com/live/customerA/manifest.mpd
The encoder then starts POSTING segments in short running POST operations: http://someoriginservice.com/live/customerA/adaptionset1/representation1/segment1.cmfv
in case of a disconnect during the segment POST operation, the segment MUST be retransmitted.  
     
5. The live encoder MAY send an updated version of the manifest, this manifest cannot override current settings and relative paths    or break currently running and incoming POST requests. The updated manifest can only be slightly different from the one that was send    previously, e.g. introduce new segments available or event messages. The updated manifest SHOULD be send using a PUT request instead of a POST request. 
6.	The encoder or ingest source MUST handle any error or failed authentication responses received from the media processing entity such as 403 (forbidden), 400 bad request, 415 unsupported media type, 412 not fulfilling conditions
7. In case of a 412 not fullfilling conditions or 415 unsupported media type, the live source/encoder MUST resend the manifest and init segment consisting of a moov and ftyp box.
8.	The encoder or ingest source MUST start a new HTTP POST segment request with the media segment these SHOULD be corresponding to the segments listed in the manifest. The payload MAY start with the header boxes ftyp and moov, followed by segments which consist of combination of moof and mdat boxes. Note that the ftyp, and moov boxes (in this order) MAY be transmitted with each request, especially if the encoder must reconnect because the previous POST request was terminated prior to the end of the stream with a 412 or 415 message. Resending the moov and ftyp boxes allows the receiving entitity to recover the init segment.
9.	The encoder or ingest source MAY use chunked transfer encoding option of the HTTP POST command [5] for uploading as it might be difficult to predict the entire content length of the segment.
10.	The encoder or ingest source SHOULD use individual HTTP POST commands [5] for uploading media fragments when ready if it is possible to predict the entire content length after the fragment became available. The encoder or ingest source MAY send the ftyp and moov boxes (in this order) with each individual request, followed by the media segments consisting of moof and mdat boxes.
11.	If the HTTP POST request terminates or times out with a TCP error prior to the end of the stream, the encoder MUST issue a new POST request by using a new connection, and follow the preceding requirements. Additionally, the encoder MAY resend the previous two segments that were already sent again.  
12.	In case fixed length POST Commands are used, the live source entity MUST resend the segment to be posted decribed in the manifest entirely in case of responses HTTP 400, 412 or 415 together with the init segment consisting of moov and ftyp boxes. 
13. In case the live stream event is over the live media source/encoder should signal the stop by transmitting an empty "mfra" box towards the publishing point/processing entity 
14.	The trackFragmentDecodeTime box Tfdt box MUST be present for each segment posted.
15.	Version 2 of the tfdt box SHOULD be used to generate media segments that have identical URLs in multiple datacenters. The fragment index field is REQUIRED for cross-datacenter failover of index-based streaming formats such as Apple HLS and index-based MPEG-DASH. To enable cross-data center failover, the time stamps MUST be synced across multiple live encoders/media ingest sources and be increased by 1 or a multiple of 1 for each successive media fragment, even across encoder restarts or failures. Encoders should use the timing information (Universal Time Stamps) from the original SDI input signal (if available) in order to allow exact synchronization of the Universal Time Stamps in the streams. Reconnecting encoders or media sources SHOULD transmit in sync with other encoders or media sources.
16.	The ISOBMFF media fragment duration SHOULD be constant, to reduce the size of the client manifests. A constant MPEG-4 fragment duration also improves client download heuristics through the use of repeat tags. The duration MAY fluctuate to compensate for non-integer frame rates.  By choosing an appropriate timescale (a multiple of the frame rate is recommended) this issue should be avoided.
17.	The MPEG-4 fragment duration SHOULD be between approximately 2 and 6 seconds.
18.	The fragment decode timestamps tdft of fragments in the fragmentedMP4stream and the indexes base_media_decode_ time SHOULD arrive in increasing order for each specific bit-rate stream in the adaptationsets. 
19.	The segments formated as fragmented MP4 stream SHOULD use a 90-KHz timescale for video streams and 44.1 KHz or 48.1 KHz for audio streams or any another timescale that enables integer increments of the decode times of fragments signalled in the tfdt box based on this scale. 
20. The manifest SHOULD be used to signal the language, which SHOULD also be signalled in the mdhd box or elng boxes in the init segment and/or moof headers 
21. The manifest SHOULD be used to signal encryption specific information, which SHOULD also be signalled in the pssh, schm and sinf boxes in the segments of the init and media segments
22. The manifest SHOULD be used to signal information about the different tracks such as the durations, media encoding types, content types, which SHOULD also be signalled in the moov box in the init segment or the moof box in the media segments
23. The manifest SHOULD be used to signal information about the timed text, images and sub-titles in adaptation sets and this information SHOULD also be signalled in the moov boxes, see next section.

## Formatting Requirements for Timed Text, Images, Captions and Sub-titles

The specification supports ingest of timed text, images, captions and subtitles in sparse tracks. 

1. The tracks containing timed text, images, captions or subtitles MAY be signalled in the manifest by an adaptationset with the different segments containing the data of the track.  
2. The segment data SHOULD be posted to the URL corresponding to the path in the manifest for the segment or towards the original POST_URL  
3. The track will be a sparse track signalled by a null media header (nmhd) containing the timed text, images, captions corresponding to the recommendation of such tracks in [8]
4. Based on this recommendation the trackhandler hdlr shall be set to "text" for WebVTT and "subt" for TTML 
5. In case TTML is used the track must use the XMLSampleEntry to signal sample description of the sub-title stream 
6. In case WebVVT is used the track must use the WVVTSampleEntry to signal sample description of the text stream
7. These boxes SHOULD signal the mime type and specifics as described in [8] sections 11.3 ,11.4 and 11.5
8. The boxes described in 3-7 must be present in the init segment (ftyp + moov) for the given track and in subsequent 
9. subtitles in CTA-608 and CTA-708 can be transmitted following the recommendation section 11.5 in [8] via SEI messages in the video track
10. The ftyp init segment for the track containing timed text, images, captions and sub-titles can use signalling using CMAF profiles based on [8]
   1.WebVTT 	Specified in ‎11.2 ISO/IEC 14496-30 [16]	'cwvt'
   1.TTML IMSC1 Text	Specified in ‎11.3.3 [16] IMSC1 Text Profile	'im1t'
   1.TTML IMSC1 Image	Specified in ‎11.3.4 [16] IMSC1 Image Profile	'im1i'
   1.CEA	CTA-608 and CTA-708 Specified in ‎11.4 [8] Caption data is embedded in SEI messages in video track; 'ccea'
11. The segments of the tracks containing Timed Text, Images, Captions and Sub-titles SHOULD use the bit-rate box to signal bit-rate of the track. 
## Formatting Requirements for Timed Meta-Data Ingest relating to ad markers, program events and program information

This section discusses the specific formatting requirements for ingest of timed meta-data related to events and markers for ad- insertion. When delivering a live streaming presentation with a rich client experience, often it is necessary to transmit time-synced events, meta-data or other signals in-band with the main media data. An example of these are opportunities for dynamic live ad insertion signalled by SCTE-35 markers. This type of event signalling is different from regular audio/video streaming because of its sparse nature. In other words, the signalling data usually does not happen continuously, and the interval can be hard to predict. 
Examples of timed meta-data are ID3 tags (http://www.id3.org/), SCTE-35 markers [2] and DASH emsg messages defined in section 5.10.3.3 of [1]. For example, DASH Event messages contain a schemeIdUri that defines the payload of the message. Table 1 provide some example schemes in DASH event messages and Table 2 illustrates an example of a SCTE-35 marker stored in a dash emsg. The presented approach allows ingest of timed meta-data form different sources possibly on different locations.

Table 1 Example of DASH emsg schemes  URI


Scheme URI	                | Value	| Description	                                 | Reference
----------------------------|-------|-----------------------------------------------|--------------------------------------
urn:mpeg:dash:event:2012    | 1	   |	Signals DASH specific events for DASH clients|	ISO / IEC 23009-1 (2014), §5.10.4
urn:dvb:iptv:cpm:2014       |	1     |	Basic metadata relating to current program	| ETSI TS 103 285, §9.1.2.1 (pdf)
urn:scte:scte35:2013:bin    | 1     |  Contains a binary SCTE-35 message	         | ANSI / SCTE 14-3 (2015), §7.3.2
www.nielsen.com:id3:v1      | 1     |  Contains a Nielsen ID3 tag	                  | Nielsen ID3 in MPEG-DASH


Table 2 example of a SCTE-35 marker embedded in a DASH emsg

Tag	                    |          Value
------------------------|-----------------------------
scheme_uri_id           |	"urn:scte:scte35:2013:bin"
Value                   |	the value of the SCTE 35 PID
Timescale               |	positive number
presentation_time_delta	| non-negative number expressing splice time relative to track fragment base media decode time (tfdt) 
event_duration          |	     duration of event in media presentation time, 0xFFFFFFFF indicates unknown duration
Id                      |	unique identifier for message
message_data	          |           splice info section including CRC

The following steps are a recommended for timed metadata ingest related to events, tags, ad markers and program information:
1. Create a fragmentedMP4stream that contains only sparse tracks meta-data track, i.e. timed meta-data streams which are streams without audio/video tracks.
1. Meta-data tracks MAY be stored in a manifest using an adaptationset with a sparse track
1.	For this meta-data track the media handler type is ("meta") and the tracks handler box is a null media header box ("nmhd").
1.	The URIMetaSampleEntry entry contains, in a URIbox, the URI following the URI syntax in [7] defining the form of the metadata (see the ISO Base media file format specification [3]). 	For example, the URIBox could contain for ID3 tags the URL  http://www.id3.org
1.	For the case of ID3, a sample contains a single ID3 tag. The ID3 tag may contain one or more ID3 frames.
1.	For the case of DASH emsg, a sample may contain one or more event message ("emsg") boxes.  Version 0 Event Message should be used. The presentation_time_delta field is relative to the absolute timestamp specified in the TrackFragmentBaseMediaDecode-TimeBox ("tfdt"). The timescale field should match the value specified in the media header box mdhd.
1.	For the case of a DASH emsg, the kind box (contained in the udta) MUST be used to signal the scheme URI of the type of metadata
1.	A BitRateBox ("btrt") SHOULD be present at the end of MetaDataSampleEntry to signal the bit rate information of the stream.
1.	If the specific format uses internal timing values, then the timescale must match the timescale field set in the media header box mdhd.
1.	All Timed Metadata samples are sync samples [3], defining the entire set of meta-data for the time interval they cover. Hence, the sync sample table box is not present.
1.	When Timed Metadata is stored in a TrackRunBox ("trun"), a single sample is present with the duration set to the duration for that run.
Given the sparse nature of the signalling event, the following is recommended:
1.	At the beginning of the live event, the encoder or media ingest source sends the initial header boxes to the processing entity/publishing point, which allows the service to register the sparse track in the client manifest.
1.	During the time when signalling data is not available, the encoder SHOULD close the HTTP POST request. While the POST request is active, the encoder SHOULD send data.
1.	When sending segments with a new connection, the encoder SHOULD start sending from the header boxes, followed by the new fragments. 
1.	The sparse track segment becomes available to the publishing point/processing entity when the corresponding parent track fragment that has an equal or larger timestamp value is made available. For example, if the sparse fragment has a timestamp of t=1000, it is expected that after the publishing point/processing entity sees "video" (assuming the parent track name is "video") fragment timestamp 1000 or beyond, it can download the sparse fragment t=1000. Note that the actual signal could be used for a different position in the presentation timeline for its designated purpose. In this example, it’s possible that the sparse fragment of t=1000 has an XML payload, which is for inserting an ad in a position that’s a few seconds later.
1.	The payload of sparse track fragments can be in different formats (such as XML, text, or binary), depending on the scenario
1. Alternatively, Meta-data MAY be stored in the DASH Manifest using DASH as as event messages

## Live Stream Ingest Option constraints
The segments and manifest are the basic unit of operation for composing live presentations. Handling streaming failover and redundancy scenarios is increasingly important for media streaming workflows. A full live presentation might contain one or more streams, depending on the configuration of the live encoders. For this specification it is recommended that each bit-rate stream is send over a seperate connection TCP, but this is not strictly necessary. 

##  Service (publishing point,media processing entity) failover
Given the nature of live streaming, good failover support is critical for ensuring the availability of the service. Typically, media services are designed to handle various types of failures, including network errors, server errors, and storage issues. When used in conjunction with proper failover logic from the live encoder side, customers can achieve a highly reliable live streaming service from the cloud. In this section, we discuss service failover scenarios. In this case, the failure happens somewhere within the service, and it manifests itself as a network error. Here are some recommendations for the encoder implementation for handling service failover:
1.	Use a 10-second timeout for establishing the TCP connection. If an attempt to establish the connection takes longer than 10 seconds, abort the operation and try again.
2.	Use a short timeout for sending the HTTP request message chunks. If the target fragment duration is N seconds, use a send timeout between N and 2 N seconds; for example, if the fragment duration is 6 seconds, use a timeout of 6 to 12 seconds. If a timeout occurs, reset the connection, open a new connection, and resume stream ingest on the new connection. This is needed to avoid latency introduced by failing connectivity in the workflow.
3. completely resend segments from the ingest source for which a connection was terminated early
4.	We recommend that the encoder or ingest source does NOT limit the number of retries to establish a connection or resume streaming after a TCP error occurs.
5.	After a TCP error:
a. The current connection MUST be closed, and a new connection MUST be created for a new HTTP POST request.
b. The new HTTP POST URL MUST be the same as the initial POST URL for the segment to be ingested.
c. The new HTTP POST MUST include stream headers (ftyp, and moov boxes) that are identical to the stream headers in the initial POST request for fragmented media ingest.
d. The last two fragments sent for each segment MAY be resent. Other ISO BMFF fragment timestamps MUST increase continuously, even across HTTP POST requests.
6.	The encoder or ingest source SHOULD terminate the HTTP POST request if data is not being sent at a rate commensurate with the MP4 fragment duration. An HTTP POST request that does not send data can prevent publishing points or media processing entities from quickly disconnecting from the live encoder or media ingest source in the event of a service update. For this reason, the HTTP POST for sparse (ad signal) tracks SHOULD be short-lived, terminating as soon as the sparse fragment is sent. An example of the error handling is shown in the following figure:

![](/ingest-interactions/output/main.png)

## Ingest source (Encoder) failover
Encoder or media ingest source failover is the second type of failover scenario that needs to be addressed for end-to-end live streaming delivery. In this scenario, the error condition occurs on the encoder side. The following expectations apply from the live ingestion endpoint when encoder failover happens:
1.	A new encoder or media ingest source instance SHOULD be created to continue streaming
2.	The new encoder or media ingest source MUST use the same URL for HTTP POST requests as the failed instance.
3.	The new encoder’s or media ingest source POST request MUST include the same header boxes moov and ftyp as the failed instance.
4.	The new encoder or media ingest source MUST be properly synced with all other running encoders for the same live presentation to generate synced audio/video samples with aligned fragment boundaries. This implies that UTC timestamps for fragments in the tdft match between decoders, and encoders start running at an appropriate segment boundary.
5.	The new stream MUST be semantically equivalent with the previous stream, and interchangeable at the header and fragment levels.
6.	The new encoder or media ingest source SHOULD try to minimize data loss. The basemediadecodetime tdft of media fragments SHOULD increase from the point where the encoder last stopped. The basemediadecodetime in the tdft box SHOULD increase in a continuous manner, but it is permissible to introduce a discontinuity, if necessary. Media processing entities or publishing points can ignore fragments that it has already received and processed, so it's better to error on the side of resending fragments than to introduce discontinuities in the media timeline.

## Encoder/Ingest Source redundancy
For certain critical live event streams that demand even higher availability and quality of experience, it is recommended to use multiple active redundant encoders to achieve seamless failover with no data loss. When two groups of encoders or media ingest sources push two copies of each stream simultaneously into the live service. This setup is supported because publishing points can filter out duplicate fragments based on stream ID and fragment timestamp. The resulting live stream and archive is a single copy of all the streams that is the best possible aggregation from the two sources. For example, in a hypothetical extreme case, as long as there is one encoder (it doesn’t have to be the same one) running at any given point in time for each stream, the resulting live stream from the service is continuous without data loss. The requirements for this scenario are as the requirements in the "Encoder failover" case in most cases, with the exception that the second set of encoders are running at the same time as the primary encoders.

## Processing entity or publishing point redundancy
For highly redundant global distribution, sometimes you must have cross-region backup to handle regional disasters. Expanding on the “Encoder redundancy” topology, customers can choose to have a redundant service deployment in a different region that's connected with the second set of encoders. Customers also can work with a Content Delivery Network provider to deploy a Global Traffic Manager in front of the two service deployments to seamlessly route client traffic. The requirements for the live encoders or ingest source are the same as the “Encoder redundancy” case. The only exception is that the second set of encoders needs to be pointed to a different live ingest endpoint. 

## References
* [1]	ISO/IEC JCT1/SC29 WG11 (MPEG), "ISO/IEC 23009-1:2014: Dynamic adaptive streaming over HTTP (DASH) -- Part 1: Media presentation description and segment formats," 2014.
* [2]	Society of Cable Television Engineers, "SCTE-35 (ANSI/SCTE 35 2013) Digital Program Insertion Cueing Message for Cable," SCTE-35 (ANSI/SCTE 35 2013),.
* [3]	ISO/IEC JTC1/SC29 WG11 (MPEG), "Information technology -- Coding of audio-visual objects -- Part 12: ISO base media file format," ISO/IEC 14496-12:2012 , 2016.
* [4]	ISO/IEC JTC1/SC29 WG11 (MPEG), "Information technology -- High efficiency coding and media delivery in heterogeneous environments -- Part 2: High efficiency video coding", ISO/IEC 23008-2:2015, 2015.
* [5]	IETF. Hypertext Transfer Protocol HTTP/1.1 , RFC 2626 [Online]. https://tools.ietf.org/html/rfc2616
* [6]	IETF DARPA, "TRANSMISSION CONTROL PROTOCOL," IETF, request for comments (international standards track) RFC 793, 1981.
* [7]	IETF R. Fielding, L. Masinter, T. Berners Lee, "Uniform Resource Identifiers (URI): Generic Syntax," IETF, IETF Request for comments (international standards track) RFC 2396, 1998.
* [8]	ISO/IEC JTC1/SC29 WG11 (MPEG), "(MPEG-A) -- Part 19: Common media application format (CMAF) for segmented media," MPEG, ISO/IEC Draft International standard ISO/IEC FDIS 23000-19, 2017.
* [9]	ISO/IEC JTC1 SC29 WG11 (MPEG), "Information technology -- Generic coding of moving pictures and associated audio information: Systems," ISO/IEC 13818-1:2007, 2007.
* [10] Microsoft Azure. Media ingest workflow with live encoder in Microsoft Azure. [Online]. https://docs.microsoft.com/en-us/azure/media-services/media-services-manage-live-encoder-enabled-channels
* [11] ISO/IEC JTC1 SC29 WG11 (MPEG), Use cases and draft requirement for NBMP (v2), MPEG Network Based Media Processing , w17262 ISO/IEC MPEG Macau China
* [12] Streaming video Alliance (last accessed Jan 2018) https://www.streamingvideoalliance.org/
* [13] DASH Industry Forum (last accessed Jan 2018) http://dashif.org/
* [14] D. Crocker Augmented BNF for Syntax Specifications: ABNF https://tools.ietf.org/html/rfc5234
* [15] ISO/IEC 23001-7:2016 Information technology -- MPEG systems technologies -- Part 7: Common encryption in ISO base media file format files https://www.iso.org/standard/68042.html
* [16] ISO/IEC 14496-30:2014 Information technology -- Coding of audio-visual objects -- Part 30: Timed text and other visual overlays in ISO base media file format
* [17] S. Bradner IETF RFC 2119 Key words for use in RFCs to Indicate Requirement Levels
* [18] IETF E. Rescorla RFC 2818 HTTP over TLS  https://tools.ietf.org/html/rfc281 
