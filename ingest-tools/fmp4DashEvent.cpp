/*******************************************************************************
Supplementary software media ingest specification:
https://github.com/unifiedstreaming/fmp4-ingest

Copyright (C) 2009-2018 CodeShop B.V.
http://www.code-shop.com

convert metadatatrack to DASH Events in XML format SCTE-214

******************************************************************************/

#include "fmp4stream.h"
#include <iostream>
#include <fstream>
#include <memory>

using namespace fMP4Stream;
using namespace std;

extern string moov_64_enc;

void print_info()
{
	std::cout << "***                 fmp4DashEvent                     ***" << std::endl;
	std::cout << "***       convert event message track to XML          ***" << std::endl;
	std::cout << "***       Supports specific SCTE-214 schemes:         ***" << std::endl;
	std::cout << "***            urn:scte:scte35:2014:xml+bin           ***" << std::endl;
	std::cout << "***            urn:scte:scte35:2013:bin               ***" << std::endl;
	std::cout << "**writes all other mpd events usign @base64 attribute ***" << std::endl;
	std::cout << "usage: fmp4DashEvent in_event.cmfm out.mpd    " << std::endl;
}

int main(int argc, char *argv[])
{

	ingest_stream ingest_stream;
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

		if (argc > 2) {
			out_file = string(argv[2]);
		}

		// cmfv, cmfa files containing emsg boxes 
		ingest_stream.load_from_file(input);
		input.close();

		// output a sparse fragmented emsg track file 
		cout << " *** writing " << out_file << "***" << endl;

		ingest_stream.write_to_dash_event_stream(out_file);

		return 1;
	}
	else
	{
		print_info();
	}

	return 0;
}