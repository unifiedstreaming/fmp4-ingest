# fmp4-ingest

I restructure the requirements based on media format for source media content 

**General Requirements (G)**
- fMP4 ingest shall use clock synchronization between streams, preferably based on UTC timestamps form the original (e.g. SDI) signal
- fMP4 ingest shall be based on MPEG technologies and container formats 
- fMP4 ingest shall support low latency media source ingest
- fMP4 shall support redundant robust workflows with multiple sources and multiple processing entities/publishing points
- fMP4 ingest shall be supported by an openly available reference implementation ????

**Requirements for source media content (S)**
- fMP4 ingest specification shall support ingest of MPEG-H and MPEG-4 media including HEVC, AVC, AAC, MPEG-H
- fMP4 ingest specification shall support ingest of media in a media format following the fragmented MPEG-4 format containing moof and mdat boxes

**Requirements on ingest for timed meta data and auxiliary information (M)**

- fMP4 ingest shall support timed meta data such as based on ID3 and SCTE-35, DASH e messages

**Requirements on ingest media transport protocol (P)**
  - fMP4 ingest shall support push based transmission of live stream events
  - fMP4 ingest shall use HTTP POST for media source ingest
  - fMP4 ingest shall support reconnection procedure in case of a disconnect
  
**Requirements on media ingest deployments and workflows (W)**
  - fMP4 ingest spec shall specify failover and restart procedures to gracefully restart in case of failovers
  - fMP4 ingest spec document shall specify best practices for media source redundancy and service redundancy (i.e. continuation of the live event after failure of media source or service node)
  - fMP4 ingest specification shall support graceful teardown of ingest of live stream events
  
