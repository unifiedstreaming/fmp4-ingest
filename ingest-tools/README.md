# media-ingest
Code for fmp4 ingest defined in:
https://dashif-documents.azurewebsites.net/Ingest/master/DASH-IF-Ingest.html

# overview 

fmp4ingest: tool for doing fmp4 ingest according to cmaf ingest defined in: 
https://dashif-documents.azurewebsites.net/Ingest/master/DASH-IF-Ingest.html
emulates the ingest source
fmp4Init: retrieves the init fragment of a CMAF file 
fmp4sparse: retrieve a sparse metadata track from a CMAF file with inband emsg
fmp4dump: print the contents of an fmp4 file to the cout, including scte markers 
fmp4DashEvent: convert a sparse track to an XML event stream


# features implemented in fmp4ingest

- Parsing of fmp4 stream
- HTTP POST of init and media fragments in long running post in real-time or not
- Retransmission of init fragment in case of failures
- posting with timestamp offset (to be improved to time accurate, currently fragment accurate)
- HTTPS, HTTP, AUTH, Client TLS certificates
- sample cmaf and sparse track files added in test_files

# Examples 

- push a stream in real time to publishing point: 
fmp4ingest -r -u http://localhost/pubpoint/channel1.isml 1.cmfv 2.cmfv 3.cmft 

- Copy the init fragment to init_in.cmfv:
fmp4init in.cmfv  

- converts a cmfv file with inband messages to a sparse track as defined in the ingest spec:
fmp4sparse in.cmfv out.cmfm  

- print the content of a cmaf or fmp4 to cout:
fmp4dump in.cmfv  




