/*******************************************************************************
Supplementary software media ingest specification:
https://github.com/unifiedstreaming/fmp4-ingest

Copyright (C) 2009-2019 CodeShop B.V.
http://www.code-shop.com

CMAF ingest from stored CMAF files, emulates a live encoder posting CMAF content

******************************************************************************/

#include "fmp4stream.h"
#include "curl/curl.h"

#include <iostream>
#include <fstream>
#include <exception>
#include <memory>
#include <chrono>
#include <thread>
#include <cstring>

using namespace fMP4Stream;
using namespace std;
bool stop_all = false;

///////////////////// experimental code for fetching remote time code ///////////////////////////
// helper to read the time remote timecode
size_t write_function(void *ptr, size_t size, size_t nmemb, std::string* data)
{
	data->append((char*)ptr, size * nmemb);
	return size * nmemb;
}

// do a request to get a synced network epoch, default is time.akamai.com
int get_remote_sync_epoch(uint64_t *res_time, string &wc_uri_)
{
	CURL *curl;
	CURLcode res;
	std::string response_string;

	curl = curl_easy_init();
	if (curl)
	{

		curl_easy_setopt(curl, CURLOPT_URL, wc_uri_.c_str());

		/* example.com is redirected, so we tell libcurl to follow redirection */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);

		/* Check for errors */
		if (res != CURLE_OK)
			fprintf(stderr, "network time lookup failed: %s\n",
				curl_easy_strerror(res));

		unsigned long ul = std::stoul(response_string, nullptr, 0);
		cout << ul << endl;
		*res_time = ul;
		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// options for the ingest
struct push_options_t
{
	push_options_t()
		: url_("http://localhost/live/video.isml/video.ism")
		, realtime_(false)
		, daemon_(false)
		, loop_(false)
		, tsoffset_(0)
		, stop_at_(0)
		, wc_off_(true)
		, wc_uri_("http://time.akamai.com")
		, dont_close_(true)
		, chunked_(false)
		, drop_every_(0)
		, prime_(false)
		, dry_run_(false)
		, verbose_(2)
	{
	}

	static void print_options()
	{
		printf("Usage: fmp4ingest [options] <input_files>\n");
		printf(
			" [-u url]                       Publishing Point URL\n"
			" [-r, --realtime]               Enable realtime mode\n"
			" [--start_at offset]            Input timestamp offset in seconds (fragment accuracy only, relative to start of file) \n"
			" [--stop_at offset]             Stop at timestamp in seconds (fragment accuracy only, relative to start of file) \n"
			" [--wc_offset]                  (boolean )Add a wallclock time offset for converting VoD (0) asset to Live \n"
			" [--wc_uri]                     uri for fetching wall clock time default time.akamai.com \n"
			" [--close_pp]                   Close the publishing point at the end of stream or termination \n"
			" [--chunked]                    Use chunked Transfer-Encoding for POST (long running post) otherwise short running posts \n"
			" [--auth]                       Basic Auth Password \n"
			" [--aname]                      Basic Auth User Name \n"
			" [--sslcert]                    TLS 1.2 client certificate \n"
			" [--sslkey]                     TLS private Key \n"
			" [--sslkeypass]                 passphrase \n"
			" <input_files>                  CMAF files to ingest (.cmf[atvm])\n"

			"\n");
	}
	
	void parse_options(int argc, char * argv[]) 
	{
		if (argc > 2)
		{
			for (int i = 1; i < argc; i++)
			{
				string t(argv[i]);
				if (t.compare("-u") == 0) { url_ = string(argv[++i]); continue; }
				if (t.compare("-l") == 0 || t.compare("--loop") == 0) { loop_ = true; continue; }
				if (t.compare("-r") == 0 || t.compare("--realtime") == 0) { realtime_ = true; continue; }
				if (t.compare("--start_at") == 0) { tsoffset_ = atoi(argv[++i]); continue; } // only up to 32 bit inputs
				if (t.compare("--stop_at") == 0) { stop_at_ = atoi(argv[++i]); continue; } // only up to 32 bit inputs
				if (t.compare("--close_pp") == 0) { dont_close_ = false; continue; }
				if (t.compare("--daemon") == 0) { daemon_ = true; continue; }
				if (t.compare("--chunked") == 0) { chunked_ = true; continue; }
				if (t.compare("--wc_offset") == 0) { wc_off_ = true; continue; }
				if (t.compare("--wc_uri") == 0) { wc_uri_ = string(argv[++i]); continue; }
				if (t.compare("--auth") == 0) { basic_auth_ = string(argv[++i]); continue; }
				if (t.compare("--aname") == 0) { basic_auth_name_ = string(argv[++i]); continue; }
				if (t.compare("--sslcert") == 0) { ssl_cert_ = string(argv[++i]); continue; }
				if (t.compare("--sslkey") == 0) { ssl_key_ = string(argv[++i]); continue; }
				if (t.compare("--keypass") == 0) { ssl_key_pass_ = string(argv[++i]); continue; }
				if (t.compare("--keypass") == 0) { basic_auth_ = string(argv[++i]); continue; }
				input_files_.push_back(argv[i]);
			}

			// get the wallclock offset 
			if (wc_off_)
				get_remote_sync_epoch(&wc_time_start_, wc_uri_);
		}
		else
			print_options();
	}
	

	string url_;
	bool realtime_;
	bool daemon_;
	bool loop_;
	bool wc_off_;

	uint64_t tsoffset_;
	uint64_t stop_at_;
	uint64_t wc_time_start_;
	bool dont_close_;
	bool chunked_;

	unsigned int drop_every_;
	bool prime_;
	bool dry_run_;
	int verbose_;

	string ssl_cert_;
	string ssl_key_;
	string ssl_key_pass_;
	string wc_uri_; // uri for fetching wallclock time in seconds default is time.akamai.com

	string basic_auth_name_; // user name with basic auth
	string basic_auth_; // password with basic auth

	vector<string> input_files_;
};

// struct to keep track of the ingest post state
struct ingest_post_state_t
{
	bool init_done_; // flag if init fragment was sent
	bool error_state_; // flag signalling error state
	uint64_t fnumber_; // fragment nnmber in queue being set
	uint64_t frag_duration_; //  fragment dration
	uint32_t timescale_; // timescale of the media track
	uint32_t offset_in_fragment_; // for partial chunked sending keep track of fragment offset
	uint64_t start_time_stamp_; // ts offset
	ingest_stream *str_ptr_; // pointer to the ingest stream
	bool is_done_; // flag set when the stream is done
	string file_name_;
	chrono::time_point<chrono::system_clock> *start_time_; // time point the post was started
	push_options_t opt_;
	uint64_t tfdt_loop_offset_;
};

// callback for long running post
static size_t read_callback(void *dest, size_t size, size_t nmemb, void *userp)
{
	ingest_post_state_t *st = (ingest_post_state_t *)userp;
	size_t buffer_size = size * nmemb;
	size_t nr_bytes_written=0;
	if (st->is_done_ || stop_all) // return if we are done
		return 0;

	// error state or non inititalized, resend the init fragment
	if (!(st->init_done_) || st->error_state_)
	{
		// 
		vector<uint8_t> init_data;
		st->str_ptr_->get_init_segment_data(init_data);

		if (st->error_state_) {
			st->offset_in_fragment_ = 0;
			st->init_done_ = false;
			st->error_state_ = false;
		}

		if ((init_data.size() - st->offset_in_fragment_) <= buffer_size) 
		{
			memcpy(dest, init_data.data() + st->offset_in_fragment_, init_data.size());
			nr_bytes_written = init_data.size();
			st->init_done_ = true;
			st->offset_in_fragment_ = 0;
			cout << " pushed init fragment " << endl;
			return nr_bytes_written;
		}
		else 
		{
			memcpy(dest, init_data.data(), buffer_size);
			st->offset_in_fragment_ += (uint32_t) buffer_size;
			return buffer_size;
		}
	}
	else // read the next fragment or send pause
	{
		// stop if all fragments send or stop_at_ timestamp based
		
		bool frags_finished = st->str_ptr_->media_fragment_.size() <= st->fnumber_;
		bool stop_at = false;
		uint64_t c_tfdt = 0;

		if (!frags_finished) {
			uint64_t t_diff = st->str_ptr_->media_fragment_[st->fnumber_].tfdt_.base_media_decode_time_ - st->str_ptr_->media_fragment_[0].tfdt_.base_media_decode_time_;
			stop_at = st->opt_.stop_at_ && (t_diff >= (st->opt_.stop_at_ * st->timescale_));
		}

		if ((frags_finished || stop_at) ) 
		{
			if (!st->opt_.loop_) {
				if (!st->opt_.dont_close_) {
					memcpy(dest, (char *)empty_mfra, 8);
					st->is_done_ = true;
					// todo add sending an empty chunk
					cout << " end of stream encountered sending mfra" << endl;
					return 8;
				}
				else {
					st->is_done_ = true;
					return 0;
				}
			}
			else // fix the looping, right now there is no good support for updating the tfdt
			{
				c_tfdt = st->str_ptr_->media_fragment_[st->fnumber_ - 1].tfdt_.base_media_decode_time_ \
					+ st->str_ptr_->media_fragment_[st->fnumber_ - 1].get_duration();
				cout << "looping" << endl; // this does not work fully as we need to increase the tfdt
				//st->start_time = &chrono::system_clock::now();
				st->tfdt_loop_offset_ = c_tfdt;
				st->fnumber_ = 0;
			}
		}
		
		if (st->opt_.realtime_) // if it is to early sleep until tfdt - frag_dur
		{
			// calculate elapsed media time
			c_tfdt = st->str_ptr_->media_fragment_[st->fnumber_ ].tfdt_.base_media_decode_time_;
			const uint64_t start_timestamp = st->str_ptr_->media_fragment_[0].tfdt_.base_media_decode_time_;
			const double media_time = ((double)c_tfdt - start_timestamp) / st->timescale_;
			const double frag_delay = 2.0; //((double)st->str_ptr_->media_fragment_[st->fnumber_].get_duration()) / ((double)st->timescale_);

			//calculate elapsed system time
			chrono::duration<double> diff = chrono::system_clock::now() - *(st->start_time_);

			// calculate start offset and convert to double
			double start_offset = (double)st->opt_.tsoffset_; // / ((double)st->timescale_);

			if ((diff.count() + start_offset) < (media_time - frag_delay)) // if it is to early sleep until tfdt - frag_dur
			{
				//cout << " sleeping for: " << (media_time - frag_delay) - diff.count() - start_offset << " seconds " << endl;
				this_thread::sleep_for(chrono::duration<double>((media_time - frag_delay) - diff.count() - start_offset));
			}
		}
		
		vector<uint8_t> frag_data;
		st->str_ptr_->get_media_segment_data((long)st->fnumber_,frag_data);
		
		// we can finish the fragment
		if (frag_data.size() - st->offset_in_fragment_  <= buffer_size) 
		{
			memcpy(dest, frag_data.data() + st->offset_in_fragment_, frag_data.size() - st->offset_in_fragment_);
			nr_bytes_written = frag_data.size() - st->offset_in_fragment_;
			st->offset_in_fragment_ = 0;
			

			cout << " pushed media fragment: " << st->fnumber_ << " file_name: " <<  st->file_name_ << " fragment duration: " << \
				((double)st->str_ptr_->media_fragment_[st->fnumber_].get_duration()) / ((double)st->timescale_) << " seconds ";

			// calculate elapsed media time
			c_tfdt = st->str_ptr_->media_fragment_[st->fnumber_].tfdt_.base_media_decode_time_;
			const uint64_t start_timestamp = st->str_ptr_->media_fragment_[0].tfdt_.base_media_decode_time_;
			const double media_time = ((double)c_tfdt + st->frag_duration_ - start_timestamp) / st->timescale_;

			cout << " media time elapsed: " << media_time  << endl;

			st->fnumber_++;

			return nr_bytes_written;
		}
		else // we cannot finish the fragment, write the buffer
		{
			memcpy(dest, frag_data.data() + st->offset_in_fragment_, buffer_size);
			(st->offset_in_fragment_)+= (uint32_t) buffer_size;
			return buffer_size;
		}
	}
}

// main loop
int push_thread(string file_name, push_options_t opt)
{
	try
	{
		if (file_name.size())
		{
			ifstream input(file_name, ifstream::binary);

			if (!input.good())
			{
				cout << "failed loading input file: [cmf[tavm]]" << string(file_name) << endl;
				push_options_t::print_options();
				return 0;
			}

			// read all and then start sending
			cout << "---- reading fmp4 input file " << file_name << endl;
			ingest_stream l_ingest_stream;
			l_ingest_stream.load_from_file(input);

			// patch the tfdt values with an offset time
			if (opt.wc_off_)
			{
				l_ingest_stream.patch_tfdt(opt.wc_time_start_);
			}

			input.close();

			// get the timescale
			// uint32_t time_scale = ingest_stream.init_fragment_.get_time_scale();
			// uint32_t fragment_duration = (uint32_t) ingest_stream.media_fragment_[0].get_duration();
			//cout << "---- timescale movie header: " << time_scale << endl;
			//cout << "---- duration of first fragment: " << fragment_duration << endl;
			//cout << "url is: " << opt.url_ << endl;
			string post_url_string = opt.url_ + "/Streams(" + file_name + ")";
			cout << "---- finished loading fmp4 file starting the ingest to: " << post_url_string << endl;

			// check the connection with the post server
			// create databuffer with init segment
			vector<uint8_t> init_seg_dat;
			l_ingest_stream.get_init_segment_data(init_seg_dat);

			// setup curl
			CURL * curl;
			CURLcode res;
			curl = curl_easy_init();

			curl_easy_setopt(curl, CURLOPT_URL, post_url_string.c_str());
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char *)&init_seg_dat[0]);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)init_seg_dat.size());

			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

			if (opt.basic_auth_.size())
			{
				curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
				curl_easy_setopt(curl, CURLOPT_USERNAME, opt.basic_auth_name_.c_str());
				curl_easy_setopt(curl, CURLOPT_USERPWD, opt.basic_auth_.c_str());
			}

			if (opt.ssl_cert_.size())
				curl_easy_setopt(curl, CURLOPT_SSLCERT, opt.ssl_cert_.c_str());
			if (opt.ssl_key_.size())
				curl_easy_setopt(curl, CURLOPT_SSLKEY, opt.ssl_key_.c_str());
			if (opt.ssl_key_pass_.size())
				curl_easy_setopt(curl, CURLOPT_KEYPASSWD, opt.ssl_key_pass_.c_str());

			res = curl_easy_perform(curl);

			/* Check for errors */
			if (res != CURLE_OK)
			{
				fprintf(stderr, "---- connection with server failed  %s\n",
					curl_easy_strerror(res));
				curl_easy_cleanup(curl);
				return 0; // nothing todo when connection fails
			}
			else
			{
				fprintf(stderr, "---- connection with server sucessfull %s\n",
					curl_easy_strerror(res));
			}

			// reinitialize curl, we will use long running post
			curl_easy_reset(curl);

			// setup the post
			ingest_post_state_t post_state = {};
			post_state.error_state_ = false;
			post_state.fnumber_ = 0;
			post_state.frag_duration_ = l_ingest_stream.media_fragment_[0].get_duration();
			post_state.init_done_ = true; // the init fragment was already send
			post_state.start_time_stamp_ = l_ingest_stream.media_fragment_[0].tfdt_.base_media_decode_time_;
			post_state.str_ptr_ = &l_ingest_stream;
			post_state.timescale_ = l_ingest_stream.init_fragment_.get_time_scale();
			post_state.is_done_ = false;
			post_state.offset_in_fragment_ = 0;
			post_state.opt_ = opt;
			post_state.file_name_ = file_name;
			post_state.tfdt_loop_offset_ = 0;

			chrono::time_point<chrono::system_clock> tp = chrono::system_clock::now();
			post_state.start_time_ = &tp; // start time


			if (opt.tsoffset_ > 0) // frame accurate offset from beginning of file
			{
				// find the timestamp offset and update the fnumber, assume constant duration
				unsigned int offset = (unsigned int)(((opt.tsoffset_) * post_state.timescale_ )/ post_state.frag_duration_);
				if (offset < l_ingest_stream.media_fragment_.size())
					post_state.fnumber_ = offset;
			}

			curl_easy_setopt(curl, CURLOPT_URL, post_url_string.data());
			curl_easy_setopt(curl, CURLOPT_POST, 1);
			curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
			if (opt.chunked_)
			{
				curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
				curl_easy_setopt(curl, CURLOPT_READDATA, (void *)&post_state);
			}
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

			if (opt.basic_auth_.size()) {
				curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
				curl_easy_setopt(curl, CURLOPT_USERPWD, opt.basic_auth_.c_str());
			}

			// client side TLS certificates
			if (opt.ssl_cert_.size()) {
				curl_easy_setopt(curl, CURLOPT_SSLCERT, opt.ssl_cert_.c_str());
				// use TLS 1.2
				curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
			}
			if (opt.ssl_key_.size())
				curl_easy_setopt(curl, CURLOPT_SSLKEY, opt.ssl_key_.c_str());
			if (opt.ssl_key_pass_.size())
				curl_easy_setopt(curl, CURLOPT_KEYPASSWD, opt.ssl_key_pass_.c_str());

			struct curl_slist *chunk = NULL;
			if (opt.chunked_)
			{
				chunk = curl_slist_append(chunk, "Transfer-Encoding: chunked");
				chunk = curl_slist_append(chunk, "Expect:");
				res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
			}
			// long running post with chunked transfer
			if (opt.chunked_)
			{
				cout << "************* long running chunked post mode ************" << endl;
				// do the ingest
				while (!post_state.is_done_ && !stop_all)
				{
					res = curl_easy_perform(curl);

					/* Check for errors */
					// 412
					// 415
					// 403
					if (res != CURLE_OK)
					{
						fprintf(stderr, "---- long running post failed: %s\n",
							curl_easy_strerror(res));
						// wait two seconds before trying again
						post_state.offset_in_fragment_ = 0; // start again from beginning of fragment
						this_thread::sleep_for(chrono::duration<double>(2));
						post_state.error_state_ = true;
					}
					else
						fprintf(stderr, "---- long running post ok: %s\n",
							curl_easy_strerror(res));

					// if not done wait one fragment duration before resuming
					if (!post_state.is_done_ && !stop_all)
						this_thread::sleep_for(chrono::duration<double>(2.0));
				}
				cout << " done pushing file: " << file_name << " press q to quit " << endl;
			}
			else  // short running post 
			{
				cout << "************* short running post mode ************" << endl;

				// create databuffer with init segment
				vector<uint8_t> init_seg_dat;
				l_ingest_stream.get_init_segment_data(init_seg_dat);

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

				chrono::time_point<chrono::system_clock> start_time = chrono::system_clock::now();

				for (uint64_t i = post_state.fnumber_; i < l_ingest_stream.media_fragment_.size(); i++)
				{
					// create databuffer with init segment

					vector<uint8_t> media_seg_dat;
					uint64_t media_seg_size = l_ingest_stream.get_media_segment_data(i, media_seg_dat);

					curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char *)&media_seg_dat[0]);
					curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)media_seg_dat.size());

					
					res = curl_easy_perform(curl);
					chrono::time_point<chrono::system_clock> end_time = chrono::system_clock::now();
					
					if (res != CURLE_OK) 
					{
						fprintf(stderr, "post of media segment failed: %s\n",
							curl_easy_strerror(res));

						// media segment post failed resend the init segment and the segment
						while (res != CURLE_OK) 
						{
							vector<uint8_t> retry_segment;
							retry_segment.resize(init_seg_dat.size() + media_seg_dat.size());
							copy(init_seg_dat.begin(), init_seg_dat.end(), back_inserter(retry_segment));
							copy(media_seg_dat.begin(), media_seg_dat.end(), back_inserter(retry_segment));
							curl_easy_setopt(curl, CURLOPT_URL, post_url_string.data());
							curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char *)&retry_segment[0]);
							curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)retry_segment.size());
							res = curl_easy_perform(curl);

							// wait for 1000 milliseconds before retrying
							std::this_thread::sleep_for(std::chrono::milliseconds(1000));
						}
					}
					else 
					{
						fprintf(stderr, "post of media segment ok: %s\n",
							curl_easy_strerror(res));
					}

					// check if we need to stop at a timestamp offset
					uint64_t t_diff = l_ingest_stream.media_fragment_[i].tfdt_.base_media_decode_time_ - l_ingest_stream.media_fragment_[0].tfdt_.base_media_decode_time_;
					
					bool stop_at = opt.stop_at_ && (t_diff >= (opt.stop_at_ * post_state.timescale_));
					
					cout << " pushed media fragment: " << i << " file_name: " << post_state.file_name_ << " fragment duration: " << \
						(l_ingest_stream.media_fragment_[i].get_duration()) / ((double)post_state.timescale_) << " seconds ";
					cout << " media time elapsed: " << (t_diff + l_ingest_stream.media_fragment_[i].get_duration()) / post_state.timescale_ << endl;

					if (opt.realtime_) 
					{
						// calculate elapsed media time
						const uint64_t c_tfdt = l_ingest_stream.media_fragment_[i].tfdt_.base_media_decode_time_;
						const uint64_t start_timestamp = l_ingest_stream.media_fragment_[0].tfdt_.base_media_decode_time_;
						const double media_time = ((double)c_tfdt - start_timestamp) / post_state.timescale_;
						const double frag_delay = 2.0; 
						
						// calculate elapsed system time
						chrono::duration<double> diff = chrono::system_clock::now() - start_time;

						// calculate the start offset
						double start_offset = (double) opt.tsoffset_; // / ((double)st->timescale_);

						// wait untin media time - frag_delay > elapsed time + initial offset
						if ((diff.count() + start_offset) < (media_time - frag_delay)) // if it is to early sleep until tfdt - frag_dur
						{
							//cout << " sleeping for: " << (media_time - frag_delay) - diff.count() - start_offset << " seconds " << endl;
							this_thread::sleep_for(chrono::duration<double>((media_time - frag_delay) - diff.count() - start_offset));
						}

					}
					else 
					{ // non real time just sleep for 500 seconds
						std::this_thread::sleep_for(std::chrono::milliseconds(500));
					}
					
					//std::cout << " --- posting next segment ---- " << i << std::endl;
					if (post_state.is_done_ || stop_all || stop_at) {
						i = (uint64_t)l_ingest_stream.media_fragment_.size();
						break;
					}
				}
			}

			// only close with mfra if dont close is not set
			if (!opt.dont_close_) 
			{
					// post the empty mfra segment
					curl_easy_setopt(curl, CURLOPT_URL, post_url_string.data());
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
			}
			/* always cleanup */
			curl_easy_cleanup(curl);
		}
	}
	catch (...)
	{
		cout << "Unknown Error" << endl;
	}

	return 0;
}

//
// entry point 
// 
int main(int argc, char * argv[])
{
	push_options_t opts;
	opts.parse_options(argc, argv);

	typedef shared_ptr<thread> thread_ptr;
	typedef vector<thread_ptr> threads_t;
	threads_t threads;

	for (auto it = opts.input_files_.begin(); it != opts.input_files_.end(); ++it)
	{
		thread_ptr thread_n(new thread(push_thread, *it, opts));
		threads.push_back(thread_n);
	}
	
	// wait for the push threads to finish
	cout << " fmp4 and CMAF ingest, press q and enter to exit " << endl;
	char c='0';
	while((c=cin.get()) != 'q');

	stop_all = true;

	for (auto& th : threads)
		th->join();
}