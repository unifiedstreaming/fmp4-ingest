/*******************************************************************************

Supplementary software media ingest specification:
https://github.com/unifiedstreaming/fmp4-ingest

Copyright (C) 2009-2021 CodeShop B.V.
http://www.code-shop.com

generate an avail track with scte-35 splice_insert in event messages

******************************************************************************/

#include "fmp4stream.h"
#include "event_track.h"
#include "base64.h"
#include "tinyxml2.h"
#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <algorithm>

using namespace fmp4_stream;
using namespace event_track;
extern std::string moov_64_enc;



/*
 generate an avail track based on 
        track duration (ms) 
		segment duration (ms) 
		avail duration (ms)
        avail frequency (ms)		
*/
int main(int argc, char *argv[])
{

	std::string out_file_cmfm = "out_avail_track.cmfm";
	std::string out_file_mpd = "out_avail_track.mpd";
	
	uint32_t timescale = 1000;
	uint32_t track_duration = 0;
	uint32_t seg_duration_ticks_ms = 0; // segmentation duration 0 = entire track
	uint32_t avail_duration = 0; 
	uint32_t avail_interval = 0;
	uint32_t track_id = 99; // default track_id

	if (argc > 4) {

		track_duration = atoi(argv[1]);
		seg_duration_ticks_ms = atoi(argv[2]);
		avail_duration = atoi(argv[3]);
		avail_interval = atoi(argv[4]);
		uint64_t st = 0;
		if (argc >5)
			st = atol(argv[5]);

		gen_avail_files(track_duration, seg_duration_ticks_ms, avail_duration, avail_interval,st);
	
	}
	else
	{
		std::cout << " Tool for generating a splice insert avail track  " << std::endl;
		std::cout << "  splice insert commands in events in event message track / MPD " << std::endl;
		std::cout << " Format is under consideration for standardisation in MPEG as event message track " << std::endl;
		std::cout << std::endl;
		std::cout << std::endl;
		std::cout << " Usage: gen_avails track_duration[ms] segment_duration avail_duration[ms] avail_interval[ms]" << std::endl;
	}
	
	return 0;
}
