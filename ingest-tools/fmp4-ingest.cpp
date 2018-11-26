/*******************************************************************************
Supplementary software media ingest specification: 
https://github.com/unifiedstreaming/fmp4-ingest

Copyright (C) 2009-2018 CodeShop B.V.
http://www.code-shop.com

******************************************************************************/

#include "fMP4Stream.h"
#include <iostream>
#include <fstream>
#include <Exception>
#include <curl/curl.h>
#include <memory>
#include <chrono>
#include <thread>

using namespace fMP4Stream;
using namespace std;

int main(int argc, char *argv[])
{

  IngestStream ingest_stream;

  try
  {
	  if (argc > 1) 
	  {

		  ifstream input(argv[1], ifstream::binary);

		  cout << " reading fmp4 input file " << endl;
		  ingest_stream.parseFromFile(&input);
		  input.close();

		  cout << " finished reading fmp4 file press key to start ingest " << std::endl;
		  cin.get();
		  
		  //string out_file = "out.mp4";
		  //ingest_stream.writeToFile(out_file);
		  //return 1

		  CURL *curl;
		  CURLcode res;
		  curl = curl_easy_init();
		  
		  // post the init segment using curl
		  if (curl)
		  {
			  char empty_post[1] = "";
			  curl_easy_setopt(curl, CURLOPT_URL, argv[2]);
			  //curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char *)&empty_post[0]);
			  //curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)0L);

			  /* Perform the request, res will get the return code */
			  res = curl_easy_perform(curl);

			  /* Check for errors */
			  if (res != CURLE_OK)
				  fprintf(stderr, "post of empty post failed: %s\n",
					  curl_easy_strerror(res));
			  else 
				  fprintf(stderr, "post of empty post ok: %s\n",
					  curl_easy_strerror(res));

			  // create databuffer with init segment
			  uint64_t init_seg_size;
			  vector<uint8_t> init_seg_dat;
			  ingest_stream.getInitSegmentData(init_seg_dat);

			  curl_easy_setopt(curl, CURLOPT_URL, argv[2]);
			  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char *)&init_seg_dat[0]);
			  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)init_seg_dat.size());
			  res = curl_easy_perform(curl);

			  /* Check for errors */
			  if (res != CURLE_OK)
				  fprintf(stderr, "post of init segment failed: %s\n",
					  curl_easy_strerror(res));
			  else 
				  fprintf(stderr, "post of init segment ok: %s\n",
					  curl_easy_strerror(res));
			  
			  for (long i = 0; i < ingest_stream.m_media_fragment.size(); i++)
			  {
				  // create databuffer with init segment
				  
				  vector<uint8_t> media_seg_dat;
				  uint64_t media_seg_size = ingest_stream.getMediaSegmentData(i,media_seg_dat);

				  curl_easy_setopt(curl, CURLOPT_URL, argv[2]);
				  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char *)&media_seg_dat[0]);
				  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)media_seg_dat.size());

				  res = curl_easy_perform(curl);

				  if (res != CURLE_OK) {
					  fprintf(stderr, "post of media segment failed: %s\n",
						  curl_easy_strerror(res));
				      
					  // media segment post failed resend the init segment and the segment
					  while (res != CURLE_OK) {
						  vector<uint8_t> retry_segment;
						  retry_segment.resize(init_seg_dat.size() + media_seg_dat.size());
						  copy(init_seg_dat.begin(), init_seg_dat.end(), back_inserter(retry_segment));
						  copy(media_seg_dat.begin(), media_seg_dat.end(), back_inserter(retry_segment));
						  curl_easy_setopt(curl, CURLOPT_URL, argv[2]);
						  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char *)&retry_segment[0]);
						  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)retry_segment.size());
						  res = curl_easy_perform(curl);

						  // wait for 1500 milliseconds before retrying
						  this_thread::sleep_for(std::chrono::milliseconds(1500));
					  }
				  }
				  else
					  fprintf(stderr, "post of media segment ok: %s\n",
						  curl_easy_strerror(res));
				 
				  this_thread::sleep_for(std::chrono::milliseconds(1500));
				  cout << " --- posting next segment ---- " << i << std::endl;
				 
			  }

			  // post the empty mfra segment
			  curl_easy_setopt(curl, CURLOPT_URL, argv[2]);
			  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char *)empty_mfra);
			  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)8u);
			  /* Perform the request, res will get the return code */
			  res = curl_easy_perform(curl);

			  /* Check for errors */
			  if (res != CURLE_OK)
				  fprintf(stderr, "post of mfra signalling segment failed: %s\n",
					  curl_easy_strerror(res));
			  else
				  fprintf(stderr, "post of mfra segment failed: %s\n",
					  curl_easy_strerror(res));

			   /* always cleanup */
			   curl_easy_cleanup(curl);
		  }
	  }
	  else
	  {
		  cout << " error: no input file argument given " << endl;
		  cout << "usage: fmp4-ingest.exe stream.cmfv  http://pub_point/streams(stream.cmfv) " << endl;
	  }
    }
	catch (...)
	{
		cout << "exception occured when processing the input file, please insert correct input file" << endl;
	}
	
   return 0;
}
