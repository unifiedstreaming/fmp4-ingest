# media-ingest
Code for fmp4 ingest https://www.ietf.org/id/draft-mekuria-mmediaingest-01.txt

# overview 
Very simple program to post fmp4 fragments from a single cmaf stream to an origin server. 
fmp4-ingest.exe infile.cmfv http:://publishingpoint/live/stream.isml/streams(1)
dependency on libCURL, tested on windows 10 using unified origin


# features implemented
- parsing of fmp4 stream
- HTTP POST of init and media fragments 
- Retransmission of init fragment in case of failure
- Example track file included for audio, metadata, ttml, webvtt 
- tested with unified origin
- multiple tracks can be uploaded only by multiple processes running the ingest tool (improvement todo)

# features to be added
- HTTP over TLS 
- HTTP AUTH 
- Multiple track file upload
- Chunked transfer mode

# contact 
for more information contact rufael@unified-streaming.com
for more information on the specification: https://github.com/unifiedstreaming/fmp4-ingest

# note 
A more sophisticated closed source tool is available from Unified Streaming, this tool is mainly to support the specification 
work.
