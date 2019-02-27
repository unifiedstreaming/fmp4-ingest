/*******************************************************************************
Supplementary software media ingest specification:
https://github.com/unifiedstreaming/fmp4-ingest

Copyright (C) 2009-2018 CodeShop B.V.
http://www.code-shop.com

******************************************************************************/

#include "fmp4stream.h"
#include <iostream>
#include <fstream>
#include <exception>
#include "curl/curl.h"
#include <memory>
#include <chrono>
#include <thread>
#include <base64.h>

using namespace fMP4Stream;
using namespace std;

int main(int argc, char *argv[])
{

	ingest_stream ingest_stream;

	if (argc > 1)
	{
		string in_file(argv[1]);
		ifstream input(in_file, ifstream::binary);

		if (!input.good())
		{
			cout << "failed loading input file: " << string(argv[1]) << endl;
			return 0;
		}

		cout << " reading fmp4 input file " << endl;
		ingest_stream.load_from_file(input, true);
		input.close();

		string out_file = "init_" + in_file;
		if (argc > 2)
			out_file = string(argv[2]);
		
		ingest_stream.write_init_to_file(out_file);
	}
	else
	{
		cout << "fmp4init: returns init fragment of fmp4 or cmaf file" << endl;
		cout << "usage fmp4init in_file.cmfv [out_file]" << endl;
	}

	return 0;
}