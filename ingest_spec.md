# Media and Timed Meta-Data Ingest Draft
* Status: Draft
* Status: For Discussion

## Overview 
This specification describes a protocol and format for live media ingest from a live encoder or media source towards media processing entities such as publishing points or origins and/or content delivery networks. It serves as a starting point for an industry wide specification for live media ingest following current best practices and formats.

**Diagram 1**

Live Media Source (e.g. live encoder) -> publishing point (media processing entity) -> Content Delivery Network -> End User/client

The workflow architecture diagram is shown in above in Diagram 1. A live encoder or ingest entity (e.g. a live encoder) pushes media to a media processing entity or a Content Delivery Network (CDN).  The media processing entity provides functionality for further delivery such as content stitching, encryption, packaging, manifest generation, transcoding, scalable delivery and/or other media processing functions. The connection between a live media source and such a media processing entity or publishing point is still often based on proprietary protocols. For live encoder ingest still legacy or proprietary protocols are often used leading to interop issues in implementations as many specification are not complete and/or based on the latest technologies and standards used in the industry (e.g. timed meta-data based, HEVC etc.). 

Overall, this is hampering deployments with live encoding and advanced media processing functions.  In practice, interop problems often arise related to the file format (MPEG-2-TS, fMP4), or related to encoder settings and the transmission protocol layer on top of protocols like TCP/USP/HTTP that is used to connect the live encoder/ live media source to the media processing entity /publishing point (e.g. synchronization, handling connection failures). This is also problematic when using multiple live encoders/live media sources as ingest to a live media source or media processing entity. other interop issues  occur when passing live meta-data from broadcast workflows into cloud media processing such as based on ID3 tags, SCTE-35 markers. This is also adressed in this specification.

This specification aims at creating a specification for interop of live encoders with streaming origins and serves as a starting point for an indstry wide specification.

## Conformance Notation

The key words "MUST," "MUST NOT," "REQUIRED," "SHALL," "SHALL NOT," "SHOULD," "SHOULD NOT," "RECOMMENDED," "MAY," and "OPTIONAL" in this document are to be interpreted as they are described in RFC 2119.

## Terminology 

This specification uses the following terminology

ISO BMFF: ISO BMFF in this document refers to part 12 of the ISO/IEC specification which describes the ISO Base media file format and the box structures related in this file format [3].

**Ftyp**: the filetype and compatibility “ftyp” box as described in the ISO BMFF [3] that describes the brand of the media stream file brand. 

**Moov**: the container for all metadata “moov” box as described in the ISO BMFF base media file format [3] that describes the metadata of the media and tracks in the presentation.

**Moof**: the movie fragment “moof” box as described in the ISO BMFF base media file format [3] that describes the meta data of a fragment of media.

**Mdat**: the media data container “mdat” box contained in an ISO BMFF  [3], this box contains the physical media samples based on a codec such as for example the HEVC [4] codec.

**Kind**: the track kind “kind” box defined in the ISO BMFF [3]  to label a track with its usage (i.e. user defined data) 

**Mfra**: the movie fragment random access “mfra” box defined in the ISO BMFF [3]  to signal random access samples [3].

**Tfdt**: the TrackFragmentDecodeTimeBox box “tfdt” in MPEG-4 ISO base media file format [3] used to signal decode time of the media 
fragment moof box.

**nmhd**: The null media header Box as defined in [3] to signal a track for which no specific media header is defined 

**HTTP**: Hyper Text Transfer Protocol, version 1.1 as specified by RFC 2626 [5]

**HTTP POST**: Command used in the Hyper Text Transfer Protocol for sending data from a source to a destination [5]

**fragmentedMP4stream**: stream of ISO BMFF [1] fragments (moof and mdat), the payload of the live media ingest.

**POST_URL**:  Target URL of a post command in the HTTP protocol for pushing data from a source to a destination. 

**TCP**: Transmission Control Protocol (TCP) as defined in RFC 793 [6]

**URI_SAFE_IDENTIFIER**: identifier/string formatted according to [7]

**Connection**: connection setup between a host and a source using the TCP protocol and the HTTP POST method.

**Live stream event**: the total media broadcast stream of the encoder ingest or source to be pushed to the destination. 

**(Live) encoder**: entity performing live encoding producing a high quality live encoded broadcast stream

**(Media) Ingest source**: a media source ingesting media content to a processing function, typically a live encoder but not restricted to this, the media ingest source could by any type of media ingest source

**Publishing point**: entity used to publish the media content, consumes the incoming media ingest stream

**Media processing function/entity**: entity used to process media content, can be a producer or consumer of a media ingest stream (or both), typically it is a consumer of stream of a live source, while it outputs content ready for client consumption.

## Ingest Protocol Behavior Specification
The media and timed meta-data ingest specification uses a standard long-running HTTP POST request to transmit encoded media data packaged in fragmented ISO BMFF [3] format to the media processing entity or publishing point.  Each HTTP POST sends a complete fragmented MP4 bitstream ("stream"), starting from the beginning with header boxes (ftyp and moov boxes), and continuing with a sequence of fragments (moof and mdat boxes) to the publishing point/ processing entity. 

An example of the Fragmented Media Ingest POST URL targeting the publishing point is:
http://mypublishingpoint/ingest.isml/streams(720p)

The POST URL and syntax is defined as follows using the IETF RFC 5234 ANB [14] to specify the structure. 
* PostURL = Protocol “://” BroadcastURL Identifier
* Protocol = "http" / "https" 
* BroadcastURL = ServerAddress "/" PresentationPath 
* ServerAddress = URI_SAFE_IDENTIFIER 
* PresentationPath = URI_SAFE_IDENTIFIER ".isml" 
* Identifier = [EventID] StreamID 
* EventID = "/Events(URI_SAFE_IDENTIFIER)”
* StreamID = "/" Streams(URI_SAFE_IDENTIFIER)" 

In this PostURL the server address is typically the hostname or IP address of the media processing entity or publishing point and the presentation path is the path to the specific media function instance.  The identifier, Event ID and stream ID can be used to signal the stream and can be generated by various means such as by the system administrator, by the live encoder or by the control plane of the cloud setting, or manually by assigning a number to a stream or service. The Events of Streams function could be used to generate such identifier if available in an API or other API functions could be used to generate based on the specific platform implementation. The actual generation of the identifier is out of scope of this specification and any implementer is free to add its own identifier to the stream as long as it is known/understood by both the publishing point and live encoder.

The payload and content of the media ingest stream in the long running post operation is a fragmentedMP4stream defined using the IETF RFC 5234 ANB [14] as follows. 

* fragmentedMP4stream = HeaderBoxes Fragments
* HeaderBoxes = FileType Moov
* Fragments = X Fragment
* Fragment = moof mdat 

During operation the communication between the media ingest encoder and the streaming end point follows the following requirements.

1.	The live encoder or ingest source communicates to the publishing point/processing entity using the HTTP POST method as defined in the HTTP protocol [5]
2.	The live encoder or ingest source SHOULD start the broadcast by sending an HTTP POST request with an empty “body” (zero content length) by using the same POSTURL. This can help the live encoder or media ingest source to quickly detect whether the live ingest publishing point is valid, and if there are any authentication or other conditions required. Per HTTP protocol, the server can't send back an HTTP response until the entire request, including the POST body, is received. Given the long-running nature of a live event, without this step, the encoder might not be able to detect any error until it finishes sending all the data.
3.	The encoder or ingest source MUST handle any errors or authentication challenges because of (1). If (1) succeeds with a 200 response, continue.
4.	The encoder or ingest source MUST start a new HTTP POST request with the fragmented MP4 stream. The payload MUST start with the header boxes ftyp and moov, followed by fragments signalled by the moof and mdat boxes. Note that the ftyp, and moov boxes (in this order) MUST be sent with each request, even if the encoder must reconnect because the previous request was terminated prior to the end of the stream.
5.	The encoder or ingest source SHOULD use chunked transfer encoding option of the HTTP POST command [5] for uploading as it is impossible to predict the entire content length of the live stream.
6.	The encoder or ingest source MAY use individual HTTP POST commands [5] for uploading media fragments when ready if it is possible to predict the entire content length after the fragment became available. The encoder MUST send the ftyp and moov boxes (in this order) with each individual request, followed by the media fragments consisting of moof and mdat boxes.
7.	When the live stream event is over, after sending the last fragment, the encoder or ingest source MUST gracefully end the chunked transfer encoding message sequence (most HTTP client stacks handle it automatically) by signalling the stop. The encoder or ingest source MUST wait for the service to return the final response code, and then terminate the connection.
8.	The stop message MUST be sent by the encoder or live ingest source to signal an end of stream (end of the live stream event) by sending a movie fragment random access “mfra” box in the stream.
9.	If the HTTP POST request terminates or times out with a TCP error prior to the end of the stream, the encoder MUST issue a new POST request by using a new connection, and follow the preceding requirements. Additionally, the encoder MAY resend the previous two MP4 fragments for each track in the stream, and resume without introducing a discontinuity in the media timeline. Resending the last two ISO BMFF fragments for each track ensures that there is no data loss. In other words, if a stream contains both an audio and a video track, and the current POST request fails, the encoder or media ingest source must reconnect and resend the last two fragments for the audio track, which were previously successfully sent, and the last two fragments for the video track, which were previously successfully sent, to ensure that there is no data loss. The encoder MAY maintain a “forward” buffer of media fragments, which it resends when it reconnects.
10.	In case of an encoder or media ingest source restart and none of the last two ISO BMFF fragments can be send, the timestamps of tdft of fragments that are sent MUST be higher than previously sent media fragments.   
11.	The ftyp, and moov boxes MUST be sent with each request (HTTP POST). These boxes MUST be sent at the beginning of the stream and any time the encoder must reconnect to resume stream ingest. 
12.	The trackFragmentDecodeTime box Tfdt box MUST be present for each fragment.
13.	Version 2 of the tfdt box SHOULD be used to generate media segments that have identical URLs in multiple datacenters. The fragment index field is REQUIRED for cross-datacenter failover of index-based streaming formats such as Apple HLS and index-based MPEG-DASH. To enable cross-data center failover, the time stamps MUST be synced across multiple encoders and be increased by 1 or a multiple of 1 for each successive media fragment, even across encoder restarts or failures. Encoders should use the timing information (Universal Time Stamps) from the original SDI input signal (if available) in order to allow exact synchronization of the Universal Time Stamps in the streams. Reconnecting encoders or media sources SHOULD transmit in sync with other encoders or media sources.
14.	The ISOBMFF media fragment duration SHOULD be constant, to reduce the size of the client manifests. A constant MP4 fragment duration also improves client download heuristics through the use of repeat tags. The duration MAY fluctuate to compensate for non-integer frame rates.  By choosing an appropriate timescale (a multiple of the frame rate is recommended) this issue should be avoided.
15.	The ISO BMFF fragment duration SHOULD be between approximately 2 and 6 seconds.
16.	The fragment decode timestamps tdft of fragments in the fragmentedMP4stream and the indexes base_media_decode_ time SHOULD arrive in increasing order. 
17.	The fragmented MP4 stream SHOULD use a 90-KHz timescale for video streams and 44.1 KHz or 48.1 KHz for audio streams or any another timescale that enables integer increments of the decode times of fragments signalled in the tfdt box based on this scale. 

## Formatting Requirements for Timed Meta-Data Ingest

This section discusses the specific formatting requirements for ingest of timed meta-data.
When delivering a live streaming presentation with a rich client experience, often it is necessary to transmit time-synced events, meta-data or other signals in-band with the main media data. An example of this are opportunities for dynamic live ad insertion signalled by SCTE-35 markers. This type of event signalling is different from regular audio/video streaming because of its sparse nature. In other words, the signalling data usually does not happen continuously, and the interval can be hard to predict. 
Examples of timed meta-data are ID3 tags (http://www.id3.org/), SCTE-35 markers [2] and DASH emsg messages defined in section 5.10.3.3 of [1]. For example, DASH Event messages contain a schemeIdUri that defines the payload of the message. Table 1 provide some example schemes in DASH event messages and Table 2 illustrates an example of a SCTE-35 marker stored in a dash emsg. 

Table 1 Example of DASH emsg schemes  URI


Scheme URI	                | Value	| Description	                                  | Reference
----------------------------|-------|-----------------------------------------------|--------------------------------------
urn:mpeg:dash:event:2012    | 1	    |	Signals DASH specific events for DASH clients |	ISO / IEC 23009-1 (2014), §5.10.4
urn:dvb:iptv:cpm:2014       |	1     |	Basic metadata relating to current program	  | ETSI TS 103 285, §9.1.2.1 (pdf)
urn:scte:scte35:2013:bin    | 1     | Contains a binary SCTE-35 message	            | ANSI / SCTE 14-3 (2015), §7.3.2
www.nielsen.com:id3:v1      | 1     | Contains a Nielsen ID3 tag	                  | Nielsen ID3 in MPEG-DASH


Table 2 example of a scte-35 marker embedded in a DASH emsg


Tag	                    |          Value
------------------------|-----------------------------
scheme_uri_id           |	"urn:scte:scte35:2013:bin"
Value                   |	the value of the SCTE 35 PID
Timescale               |	positive number
presentation_time_delta	| non-negative number expressing splice time relative to track fragment base media decode time (tfdt) 
event_duration          |	     duration of event in media presentation time, 0xFFFFFFFF indicates unknown duration
Id                      |	unique identifier for message
message_data	          |           splice info section including CRC

The following steps are a recommended for timed metadata ingest:
1.	Create a fragmentedMP4stream that contains only sparse tracks meta-data track, i.e. timed metadata streams which are streams without audio/video tracks.
1.	For this meta-data track the media handler type is ("meta") and the tracks handler box is a null media header box ("nmhd").
1.	The URIMetaSampleEntry entry contains, in a URIbox, the URI following the URI syntax in [7] defining the form of the metadata (see the ISO Base media file format specification [3]). 	For example, the URIBox could contain for ID3 tags the URL  http://www.id3.org
1.	For the case of ID3, a sample contains a single ID3 tag. The ID3 tag may contain one or more ID3 frames.
1.	For the case of DASH emsg, a sample may contain one or more event message ("emsg") boxes.  Version 0 Event Message should be used. The presentation_time_delta field is relative to the absolute timestamp specified in the TrackFragmentBaseMediaDecode-TimeBox ("tfdt"). The timescale field should match the value specified in the media header box mdhd.
1.	For the case of a DASH emsg, the kind box (contained in the udta) shall be used to signal the scheme URI of the types of metadata
1.	A BitRateBox ("btrt") should be present at the end of MetaDataSampleEntry to signal the bit rate information of the stream.
1.	If the specific format uses internal timing values, then the timescale must match the timescale field set in the media header box mdhd.
1.	All Timed Metadata samples are sync samples [3], defining the entire set of metadata for the time interval they cover. Hence, the sync sample table box is not present.
1.	When Timed Metadata is stored in a TrackRunBox ("trun"), a single sample is present with the duration set to the duration for that run.
Given the sparse nature of the signalling event, the following is recommended:
1.	At the beginning of the live event, the encoder or media ingest source sends the initial header boxes to the service, which allows the service to register the sparse track in the client manifest.
1.	The encoder SHOULD terminate the HTTP POST request when data is not being sent. A long-running HTTP POST that does not send data can prevent Media Services from quickly disconnecting from the encoder in the event of a service update or server reboot. In these cases, the media server is temporarily blocked in a receive operation on the socket.
1.	During the time when signalling data is not available, the encoder SHOULD close the HTTP POST request. While the POST request is active, the encoder SHOULD send data.
1.	When sending sparse fragments with a new connection, the encoder SHOULD start sending from the header boxes, followed by the new fragments. This is for cases in which failover happens in-between, and the new sparse connection is being established to a new server that has not seen the sparse track before.
1.	The sparse track fragment becomes available to the publishing point/processing entity when the corresponding parent track fragment that has an equal or larger timestamp value is made available. For example, if the sparse fragment has a timestamp of t=1000, it is expected that after the publishing point/processing entity sees "video" (assuming the parent track name is "video") fragment timestamp 1000 or beyond, it can download the sparse fragment t=1000. Note that the actual signal could be used for a different position in the presentation timeline for its designated purpose. In this example, it’s possible that the sparse fragment of t=1000 has an XML payload, which is for inserting an ad in a position that’s a few seconds later.
1.	The payload of sparse track fragments can be in different formats (such as XML, text, or binary), depending on the scenario

## Live Stream Ingest Option constraints
The fragmented MP4 Stream is the basic unit of operation in live ingestion for composing live presentations, handling streaming failover, and redundancy scenarios. It is defined as one unique, fragmented MP4 bitstream that might contain a single track or multiple tracks. A full live presentation might contain one or more streams, depending on the configuration of the live encoders. The following examples illustrate various options of using streams to compose a full live presentation.

Example:
A company wants to create a live streaming presentation that includes the following audio/video bitrates:
Video – 3000 kbps, 1500 kbps, 750 kbps
Audio – 128 kbps

For this specification, only one option is recommended: Each track in a separate stream.  This to some extend is corresponding to recent CMAF specification [8], but full conformance is not necessary for conforming to this specification. The main idea is to have each track in a sperate stream. In this option, the encoder or live ingest source puts one track into each fragment MP4 bitstream, and then posts all of the streams over separate HTTP connections. This can be done with one encoder or with multiple encoders. The live ingestion publishing point or processing entity sees this live presentation as composed of four streams.

##  Service (publishing point,media processing entity) failover
Given the nature of live streaming, good failover support is critical for ensuring the availability of the service. Typically, media services are designed to handle various types of failures, including network errors, server errors, and storage issues. When used in conjunction with proper failover logic from the live encoder side, customers can achieve a highly reliable live streaming service from the cloud.
In this section, we discuss service failover scenarios. In this case, the failure happens somewhere within the service, and it manifests itself as a network error. Here are some recommendations for the encoder implementation for handling service failover:
1.	Use a 10-second timeout for establishing the TCP connection. If an attempt to establish the connection takes longer than 10 seconds, abort the operation and try again.
2.	Use a short timeout for sending the HTTP request message chunks. If the target fragment duration is N seconds, use a send timeout between N and 2 N seconds; for example, if the fragment duration is 6 seconds, use a timeout of 6 to 12 seconds. If a timeout occurs, reset the connection, open a new connection, and resume stream ingest on the new connection.
3.	Maintain a rolling buffer that has the last two fragments for each track that were successfully and completely sent to the service. If the HTTP POST request for a stream is terminated or times out prior to the end of the stream, open a new connection and begin another HTTP POST request, resend the stream headers, resend the last two fragments for each track, and resume the stream without introducing a discontinuity in the media timeline. This reduces the chance of data loss.
4.	We recommend that the encoder or ingest source does NOT limit the number of retries to establish a connection or resume streaming after a TCP error occurs.
5.	After a TCP error:
a. The current connection MUST be closed, and a new connection MUST be created for a new HTTP POST request.
b. The new HTTP POST URL MUST be the same as the initial POST URL for fragmented media ingest.
c. The new HTTP POST MUST include stream headers (ftyp, and moov boxes) that are identical to the stream headers in the initial POST request for fragmented media ingest.
d. The last two fragments sent for each track may be resent. Other ISO BMFF fragment timestamps must increase continuously, even across HTTP POST requests.
6.	The encoder or ingest source SHOULD terminate the HTTP POST request if data is not being sent at a rate commensurate with the MP4 fragment duration. An HTTP POST request that does not send data can prevent publishing points or media processing entities from quickly disconnecting from the live encoder or media ingest source in the event of a service update. For this reason, the HTTP POST for sparse (ad signal) tracks SHOULD be short-lived, terminating as soon as the sparse fragment is sent.

## Ingest source (Encoder) failover
Encoder or ingest source failover is the second type of failover scenario that needs to be addressed for end-to-end live streaming delivery. In this scenario, the error condition occurs on the encoder side.
The following expectations apply from the live ingestion endpoint when encoder failover happens:
1.	A new encoder or media ingest source instance SHOULD be created to continue streaming
2.	The new encoder or media ingest source MUST use the same URL for HTTP POST requests as the failed instance.
3.	The new encoder’s or media ingest source POST request MUST include the same fragmented MP4 header boxes as the failed instance.
4.	The new encoder or media ingest source MUST be properly synced with all other running encoders for the same live presentation to generate synced audio/video samples with aligned fragment boundaries. This implies that UTC timestamps for fragments in the tdft match between decoders, and encoders start running at an appropriate segment boundary.
5.	The new stream MUST be semantically equivalent with the previous stream, and interchangeable at the header and fragment levels.
6.	The new encoder or media ingest source SHOULD try to minimize data loss. The basemediadecodetime tdft of media fragments SHOULD increase from the point where the encoder last stopped. The basemediadecodetime in the tdft box SHOULD increase in a continuous manner, but it is permissible to introduce a discontinuity, if necessary. Media processing entities or publishing points can ignore fragments that it has already received and processed, so it's better to err on the side of resending fragments than to introduce discontinuities in the media timeline.

## Encoder/Ingest Source redundancy
For certain critical live event streams that demand even higher availability and quality of experience, it is recommended to use active redundant encoders to achieve seamless failover with no data loss. When two groups of encoders or media ingest sources push two copies of each stream simultaneously into the live service. This setup is supported because publishing points can filter out duplicate fragments based on stream ID and fragment timestamp. The resulting live stream and archive is a single copy of all the streams that is the best possible aggregation from the two sources. For example, in a hypothetical extreme case, as long as there is one encoder (it doesn’t have to be the same one) running at any given point in time for each stream, the resulting live stream from the service is continuous without data loss. The requirements for this scenario are as the requirements in the "Encoder failover" case in most cases, with the exception that the second set of encoders are running at the same time as the primary encoders.

## processing entity or publishing point redundancy
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

