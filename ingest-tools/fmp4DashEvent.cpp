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

	ingest_stream l_ingest_stream;
	string l_out_file = "out_mpd_event.mpd";
	string urn;

	if (argc > 1)
	{

		ifstream l_input(argv[1], ifstream::binary);

		if (!l_input.good())
		{
			cout << "failed loading input file: " << string(argv[1]) << endl;
			return 0;
		}

		cout << " reading input file: " << string(argv[1]) << std::endl;
		
		if (argc > 2) {
			l_out_file = string(argv[2]);
		}

		// cmfv, cmfa files containing emsg boxes 
		l_ingest_stream.load_from_file(&l_input);
		l_input.close();

	    // output a sparse fragmented emsg track file 
		cout << " ******* writing " << l_out_file << "********" <<  endl;
		
		l_ingest_stream.write_to_dash_event_stream(l_out_file);

		return 1;
	}
	else 
	{ 
		cout << "usage: fmp4meta infile(sparse_meta_emsg) outfile(dash event xml)" << endl;
	}
		
	return 0;
}




