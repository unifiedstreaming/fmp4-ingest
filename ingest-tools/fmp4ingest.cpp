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
#include <chrono>


//std::this_thread::sleep_for(std::chrono::milliseconds(x));

using namespace fMP4Stream;
using namespace std;

int main(int argc, char *argv[])
{

  ingest_stream ingest_stream;

  try
  {
	  if (argc > 1)
	  {

		  ifstream input(argv[1], ifstream::binary);

		  if (!input.good())
		  {
			  cout << "failed loading input file: " << string(argv[1]) << endl;
			  return 0;
		  }

		  cout << "---- reading fmp4 input file " << endl;
		  ingest_stream.load_from_file(&input);
		  input.close();
		 
		  uint32_t time_scale = ingest_stream.m_init_fragment.get_time_scale();
		  cout << "---- timescale movie header: " << ingest_stream.m_init_fragment.get_time_scale() << endl; 
		  cout << "---- duration of first fragment: "<< ingest_stream.m_media_fragment[0].get_duration() << endl;

		  cout << "---- finished loading fmp4 file press key to start ingest " << endl;
		  cin.get();

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
				  fprintf(stderr, "---- post of empty post failed: %s\n",
					  curl_easy_strerror(res));
			  else 
				  fprintf(stderr, "---- post of empty post ok: %s\n",
					  curl_easy_strerror(res));

			  // create databuffer with init segment
			  vector<uint8_t> init_seg_dat;
			  ingest_stream.get_init_segment_data(init_seg_dat);

			  curl_easy_setopt(curl, CURLOPT_URL, argv[2]);
			  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char *)&init_seg_dat[0]);
			  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)init_seg_dat.size());
			  res = curl_easy_perform(curl);

			  /* Check for errors */
			  if (res != CURLE_OK)
				  fprintf(stderr, "---- post of init segment failed: %s\n",
					  curl_easy_strerror(res));
			  else 
				  fprintf(stderr, "---- post of init segment ok: %s\n",
					  curl_easy_strerror(res));
			  
			  for (long i = 0; i < ingest_stream.m_media_fragment.size(); i++)
			  {
				  auto t0 = std::chrono::high_resolution_clock::now();
	
				  // create databuffer with init segment
				  
				  vector<uint8_t> media_seg_dat;
				  uint64_t media_seg_size = ingest_stream.get_media_segment_data(i,media_seg_dat);

				  uint64_t frag_dur = ingest_stream.m_media_fragment[i].get_duration();

				  cout << "---- pushing fragment number: " << i << " with duration of: " << frag_dur/time_scale << endl;

				  curl_easy_setopt(curl, CURLOPT_URL, argv[2]);
				  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char *)&media_seg_dat[0]);
				  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)media_seg_dat.size());

				  res = curl_easy_perform(curl);

				  if (res != CURLE_OK) {
					  fprintf(stderr, "------ post of media segment failed: %s\n",
						  curl_easy_strerror(res));
				      
					  int retry_count = 0;
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
						  this_thread::sleep_for(chrono::milliseconds(2000));
						  retry_count++;
						  if (retry_count > 8) {
							  cout << "error pushing to streaming server " << endl;
							  return 0;
						  }
					  }
				  }
				  else
					  fprintf(stderr, "---- post of media segment ok: %s\n",
						  curl_easy_strerror(res));
				 
				  auto t1 = std::chrono::high_resolution_clock::now();
				  auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
				  
				  cout << " --- segment push completed in ---- " << dt << "  milliseconds" << endl;

				  if(time_scale)
				  if (dt < (long long) (1000 * frag_dur / time_scale)) {
					  if (time_scale)
						  this_thread::sleep_for(chrono::milliseconds(1000 * frag_dur / time_scale - dt));
					  else
						  this_thread::sleep_for(chrono::milliseconds(1000 * frag_dur / time_scale - dt));
				  }
				  cout << " --- posting next segment ---- " << i << endl;
				 
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
		  cout << "usage: fmp4-ingest stream.cmfv  http://pub_point/streams(stream.cmfv) " << endl;
	  }
    }
	catch (...)
	{
		cout << "exception occured when processing the input file, please insert correct input file" << endl;
	}
	
   return 0;
}