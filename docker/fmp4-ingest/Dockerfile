ARG ALPINEVERSION=3.16

FROM alpine:$ALPINEVERSION

# ARGs declared before FROM are in a different scope, so need to be stated again
# https://docs.docker.com/engine/reference/builder/#understand-how-arg-and-from-interact
ARG ALPINEVERSION

# install dependencies
RUN     buildDeps="bash-completion \
                  cmake \
                  coreutils \
                  gcc \
                  g++ \
                  gdb \
                  git \
                  make \
                  curl-dev" && \
        apk add --update ${buildDeps}

# Pull fmp4ingest-tools repo
RUN     cd /root && \
        git clone --recurse-submodules https://github.com/unifiedstreaming/fmp4-ingest.git  && \
        cd fmp4-ingest/ingest-tools && \
        cmake CMakeLists.txt && \
        make

ENV PATH="${PATH}:/root/fmp4-ingest/ingest-tools:/root/fmp4-ingest/ingest-tools/event"

COPY entrypoint.sh /home/entrypoint.sh
RUN chmod +x /home/entrypoint.sh

ENTRYPOINT [ "/home/entrypoint.sh" ]


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
