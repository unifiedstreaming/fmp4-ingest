/*******************************************************************************
Supplementary software media ingest specification:
https://github.com/unifiedstreaming/fmp4-ingest

Copyright (C) 2009-2018 CodeShop B.V.
http://www.code-shop.com

simple program for creating sparse metadatatracks

******************************************************************************/

#include "fmp4stream.h"
#include <iostream>
#include <fstream>
#include <memory>

using namespace fMP4Stream;
using namespace std;

extern string moov_64_enc;

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
			cout << "usage: fmp4meta infile(sparse_meta_emsg, cmfc) outfile(dash event xml)" << endl;
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
		cout << " ******* writing " << out_file << "********" <<  endl;
		
		ingest_stream.write_to_dash_event_stream(out_file);

		return 1;
	}
	else 
	{ 
		cout << "usage: fmp4meta infile(sparse_meta_emsg, cmfc) outfile(dash event xml)" << endl;
	}
		
	return 0;
}




