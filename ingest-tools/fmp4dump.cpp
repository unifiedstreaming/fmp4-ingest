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
#include <memory>

using namespace fMP4Stream;
using namespace std;

int main(int argc, char *argv[])
{
	ingest_stream ingest_stream;
	if (argc > 1)
	{

		ifstream input(argv[1], ifstream::binary);

		if (!input.good())
		{
			cout << "failed loading input file: " << string(argv[1]) << endl;
			return 0;
		}

		cout << " reading fmp4 input file " << std::endl;

		try {
			ingest_stream.load_from_file(input);
			input.close();
			ingest_stream.print();
		}
		catch (...)
		{
			cout << " unknown error reading the file " << endl;
		}
		return 1;
	}
	else 
	{
		cout << "fmp4dump: dumps fmp4/cmaf information about fragments and emsg to the screen" << endl;
		cout << "usage: fmp4dump input_file" << endl;
	}
}