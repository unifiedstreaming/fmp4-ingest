# fmp4-ingest
Repository containing documents of fragmented MPEG-4 ingest specification (in progress)

for more information please contact Rufael Mekuria rufael@unified-streaming.com. 

Last update: March 19th 2018

# fmp4-ingest goals 

Todays highly available, highly scalable and high quality video streaming platforms combine high quality live encoders
that perform high quality encoding with video streaming publishing points. These points can handle the massive user scalability, personalization, file format conversions, personalization and encryption to target all of the different types of end user devices in use. Such setups have shown the best results in practical deployments in the most demanding large scale video platforms. 

In such a setup various aspects need to be considered. Best of breed components should be used for live encoder and publishing point 
entities targetting all devices at the best quality and bitrate. Further, to support failover and scalability redundant multiple live encoders and publishing point instances are used. These instances should be synchronized, and support graceful start and tear down of instances and streams. In addition, many use cases require timed meta data and low latency support.

The live media ingest specification develops a protocol and data format for live media ingest from a live media source such as a live encoder towards a publishing entity will handle each of these requirements. This specification will enable lower deployment times and less interop work, and less vendor lock in issues in video streaming platforms.   This specification will follow common industry best practice and supported formats and will not require dramatic new or complex implementations in live encoders and publishing entities.  However, it will enable homogeneous failover, timed meta-data and clock synchronization needed in practical video streaming deployments. 

The vision is that Live encoders should compete based on their encoding quality and performance, not on timed meta data or support for failovers, which should be homogenized as much as possible. Publishing points should compete based on their scalability and specific features targetting different end user devices.

An industry specification will enable video streaming platforms with easier component interop, better redundancy to failover, better support for timed-meta data, best of breed components and less vendor lock-in issues. Ideally, it will reduce the number of output formats supported by encoders and reduce the number of input formats supported by publishing points. Hence this specification will lead to less work for encoder vendors.

As content owners continue to adopt HTTP Adaptive Streaming (HAS) technologies for delivery of their linear content, they demand an ever increasing set of capabilities such as blackout control, encryption, captions, Ad Insertion, etc. While many of these areas have received significant attention and enjoy well defined interfaces, one area, Live Media Ingest, has largely gone un-touched and is a common source of interoperability issues. It is the goal of this specification effort to define how Live Media Content is to be prepared for HAS Delivery and advanced media processing contributed to the system responsible for doing so.

This effort is expected to product the following documents:

  *   Requirements for Contribution of Live Media Content
  *   A Live Media Contribution Interface specification

The live media ingest specification will define a protocol and data format for the contribution, by an encoder, of Live Media Content towards a publishing or media processing entity. Existing industry best practices and supported formats will be used while new or complex protocols and implementations are to be avoided. At a minimum the live media ingest specification is expected to address the following topics:

  *   Ingest Media format (Audio/Video) and transport protocol
  *   Timed meta-data such as Captions and Ad Markers
  *   Authentication
  *   Source failover and synchronization

The intent of this specification is to enable lower deployment times through reduced interop work and eliminate vendor lock in to proprietary or pre-integrated contribution interfaces.

