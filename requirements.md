# fmp4-ingest

I restructure the requirements based on media format for source media content 

**General Requirements (G)**
- fMP4 ingest shall use clock synchronization between streams, preferably based on UTC timestamps form the original (e.g. SDI) signal
- fMP4 ingest shall be based on MPEG technologies and container formats 
- fMP4 ingest shall support low latency media source ingest
- fMP4 shall support redundant robust workflows with multiple sources and multiple processing entities/publishing points
- fMP4 ingest shall be supported by an openly available reference implementation ????
// added by me to make it more clear that live ingest needs to support active media processing and not only pass through
- fMP4 ingest shall support further media processing in the cloud or network for value added presentations 
- fMP4 ingest shall support re-encryption, content stitching, live to VoD conversion, graphics overlay processing based on accurate timing information downstream
- fMP4 ingest shall support re-generation of the manifest at the publishing point or source node to support different streaming protocols and personalized presentations.
- fMP4 ingest shall decouple the delivery between the live encoder and the processing node from the processing node and the client

**Requirements for source media content (S)**
- fMP4 ingest specification shall support ingest of MPEG-H and MPEG-4 media including HEVC, AVC, AAC, MPEG-H
- fMP4 ingest specification shall support ingest of media in a media format following the fragmented MPEG-4 format containing moof and mdat boxes
- fMP4 ingest should support encrypted media content based on commonly deployed schemes such as specified in MPEG common encryption
- fMP4 shall support ingest of multiple tracks to accomodate for different languages, audio etc.

**Requirements on ingest for timed meta data and auxiliary information (M)**

- fMP4 ingest shall support timed meta data such as based on ID3 and SCTE-35, DASH e messages
- fMP4 ingest shall support ingest of captioning information 
- fMP4 ingest shall support ingest information suitable to different streaming protocols such as MPEG DASH, Apple HLS and Microsoft Smooth

**Requirements on ingest media transport protocol (P)**
  - fMP4 ingest shall support push based transmission of live stream events
  - fMP4 ingest shall support reconnection procedure in case of a disconnect
  - fMP4 ingest shall support secure connections
  - fMP4 ingest shall support a method for authentication of the contributor 
  
  
**Requirements on media ingest deployments and workflows (W)**
  - fMP4 ingest spec shall specify failover and restart procedures to gracefully restart in case of failovers
  - fMP4 ingest spec document shall specify best practices for media source redundancy and service redundancy (i.e. continuation of the live event after failure of media source or service node)
  - fMP4 ingest specification shall support graceful teardown of ingest of live stream events
  
