# Alpine Dockerfile for building locally / pushing to Dockerhub
FROM    alpine:3.14

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

ENV PATH="${PATH}:/root/fmp4-ingest/ingest-tools"

# Invocation examples
# fmp4ingest: CMAF ingest
# docker run --rm -it -v $(pwd):/data -w /data/ fmp4ingest:latest fmp4ingest $@
#
# fmp4Init: retrieve the init fragment or CMAF Header from a fmp4 file
# docker run --rm -it -v $(pwd):/data -w /data/ fmp4ingest:latest fmp4Init $@
#
# fmp4sparse: retrieve a sparse metadata track from a CMAF file with inband emsg
# docker run --rm -it -v $(pwd):/data -w /data/ fmp4ingest:latest fmp4sparse $@
#
# fmp4dump: print the contents of an fmp4 file to the cout, including scte markers
# docker run --rm -it -v $(pwd):/data -w /data/ fmp4ingest:latest fmp4dump $@
#
# fmp4DashEvent: convert a sparse track to an XML event stream
# docker run --rm -it -v $(pwd):/data -w /data/ fmp4ingest:latest fmp4DashEvent $@
#
# dashEventfmp4: convert a DASH mpd or SMIL with events in it to a sparse metadata track
# docker run --rm -it -v $(pwd):/data -w /data/ fmp4ingest:latest dashEventfmp4 $@
