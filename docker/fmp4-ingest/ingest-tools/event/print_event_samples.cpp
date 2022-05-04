/*******************************************************************************
Supplementary software media ingest specification:
https://github.com/unifiedstreaming/fmp4-ingest

Copyright (C) 2009-2021 CodeShop B.V.
http://www.code-shop.com

******************************************************************************/

#include "fmp4stream.h"
#include "event_track.h"
#include <iostream>
#include <fstream>
#include <memory>

using namespace fmp4_stream;
using namespace std;

extern string moov_64_enc;

void print_info()
{
	std::cout << "***       prints the samples of event message track     ***" << std::endl;
}

int main(int argc, char *argv[])
{

	event_track::ingest_event_stream ingest_stream;
	string out_file = "out_mpd_event.mpd";
	string urn;

	if (argc > 1)
	{

		ifstream input(argv[1], ifstream::binary);

		if (!input.good())
		{
			cout << "failed loading input file: " << string(argv[1]) << endl;
			print_info();
			return 0;
		}

		cout << " reading input file: " << string(argv[1]) << std::endl;

		if (argc > 2) 
			out_file = string(argv[2]);

		// cmfv, cmfa files containing emsg boxes 
		ingest_stream.print_samples_from_file(input, false);
		input.close();

		return 0;
	}
	else
	{
		print_info();
	}

	return 0;
}
