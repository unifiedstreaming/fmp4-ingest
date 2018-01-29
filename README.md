# fmp4-ingest
Repository containing documents of fragmented MPEG-4 ingest specification (in progress)

for more information please contact Rufael Mekuria rufael@unified-streaming.com. 

Updates will soon be added.

# fmp4-ingest goals 

Todays highly available, highly scalable and high quality video streaming platforms combine high quality live encoders
that perform high quality encoding with video streaming publishing points. Thse points can handle the massive user scalability, personalization, file format conversions, personalization and encryption to target all of the different types of end user devices in use. Such setups have shown the best results in practical deployments in the most demanding large scale video platforms. 

In such a setup various aspects need to be considered. Best of breed components should be used for live encoder and publishing point 
entities targetting all devices at the best quality and bitrate. Further, to support failover and scalability redundant multiple live encoders and publishing point instances are used. These instances should be synchronized, and support graceful start and tear down of instances and streams. In addition, many use cases require timed meta data and low latency support.

The live media ingest specification develops a protocol and data format for live media ingest from a live media source such as a live encoder towards a publishing entity will handle each of these requirements. This specification will enable lower deployment times and less interop work, and less vendor lock in issues in video streaming platforms.   This specification will follow common industry best practice and supported formats and will not require dramatic new or complex implementations in live encoders and publishing entities.  However, it will enable homogeneous failover, timed meta-data and clock synchronization needed in practical video streaming deployments. 

The vision is that Live encoders should compete based on their encoding quality and performance, not on timed meta data or support for failovers, which should be homogenized as much as possible. Publishing points should compete based on their scalability and specific features targetting different end user devices.

An industry specification will enable video streaming platforms with easier component interop, better redundancy to failover, better support for timed-meta data, best of breed components and less vendor lock-in issues. Ideally, it will reduce the number of output formats supported by encoders and reduce the number of input formats supported by publishing points. Hence this specification will lead to less work for encoder vendors.
