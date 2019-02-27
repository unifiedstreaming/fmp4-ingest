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
	uint32_t l_track_id = 1;  // the track id of the output metadata track
	uint32_t l_announce = 8;  // the ennounce time of the output metadata track
	string l_out_file = "out_file.cmfm";
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

		if (argc > 3) {
			l_track_id = stoi(argv[3]);
			cout << "track id is: " << l_track_id << endl;
		}

		if (argc > 4) {
			l_announce = stoi(argv[4]);
			urn = string(argv[5]);
		    cout << " urn scheme for meta data: " << urn  << endl;
		}

		// cmfv, cmfa files containing emsg boxes 
		l_ingest_stream.load_from_file(l_input);
		l_input.close();

	    // output a sparse fragmented emsg track file 
		cout << " ******* writing " << l_out_file << "********" <<  endl;
		
		l_ingest_stream.write_to_sparse_emsg_file(l_out_file,l_track_id,l_announce,urn);

		return 1;
	}
	else 
	{ 
		cout << "program to convert a cmaf file containing inband emsg to a sparse track" << endl;
		cout << "usage: fmp4meta infile.cmf[atv] outfile_sparse [track_id=1] [urn=myscheme::emsg]" << endl;
	}
		
	return 0;
}




