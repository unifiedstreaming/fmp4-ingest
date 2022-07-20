# syntax=docker/dockerfile:1
FROM alpine:3.16 AS builder

# Install dependencies
ARG build_deps="bash-completion cmake coreutils gcc g++ gdb git make curl-dev"
RUN apk --no-cache add --update ${build_deps}

# Pull fmp4-ingest tools repo
WORKDIR /root/
RUN git clone --recurse-submodules https://github.com/unifiedstreaming/fmp4-ingest.git
WORKDIR /root/fmp4-ingest/ingest-tools/event/
RUN git pull https://github.com/unifiedstreaming/EventMessageTrack.git master

# Build fmp4-ingest tools
WORKDIR /root/fmp4-ingest/
RUN cmake -S . -B build
RUN cmake --build build --clean-first

# Create lean fmp4-ingest container image
FROM alpine:3.16

ARG runtime_deps="gcc curl-dev"
RUN apk --no-cache add --update ${runtime_deps}
WORKDIR /root/fmp4-ingest/
COPY --from=builder /root/fmp4-ingest/build ./
ENV PATH="${PATH}:/root/fmp4-ingest/ingest-tools:/root/fmp4-ingest/ingest-tools/event"

# Invocation examples
# fmp4ingest: CMAF ingest
# docker run --rm -it -v $(pwd):/data -w /data/ fmp4ingest:latest fmp4ingest $@
#
# fmp4_init: retrieve the init fragment or CMAF Header from a fmp4 file
# docker run --rm -it -v $(pwd):/data -w /data/ fmp4ingest:latest fmp4_init $@
#
# fmp4dump: print the contents of an fmp4 file to the cout, including scte markers
# docker run --rm -it -v $(pwd):/data -w /data/ fmp4ingest:latest fmp4dump $@
#
# fmp4_dash_event: convert a sparse track to an XML event stream
# docker run --rm -it -v ${PWD}:/data -w /data/ fmp4ingest:latest fmp4_dash_event $@
#
# dash_event_fmp4: convert a DASH mpd or SMIL with events in it to a sparse metadata track
# docker run --rm -it -v ${PWD}:/data -w /data/ fmp4ingest:latest dash_event_fmp4 $@
#
# gen_avail_track: tool for generating a splice insert avail track
# docker run --rm -it -v ${PWD}:/data -w /data/ fmp4ingest:latest gen_avail_track
