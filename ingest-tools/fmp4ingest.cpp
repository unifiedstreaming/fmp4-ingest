/*******************************************************************************
Supplementary software media ingest specification:
https://github.com/unifiedstreaming/fmp4-ingest

Copyright (C) 2009-2019 CodeShop B.V.
http://www.code-shop.com

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
		, dont_close_(false)
		, chunked_(true)
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
			" [--tsoffset offset]            Input timestamp offset (fragment accurate) \n"
			" [--stop_at offset]             Stop at timestamp (fragment accurate) \n"
			" [--dont_close]                 Do not send empty mfra box\n"
			" [--chunked]                    Use chunked Transfer-Encoding for POST\n"
			" [--auth]                       Basic Auth Password \n"
			" [--sslcert]                    TLS 1.2 client certificate \n"
			" [--sslkey]                     SSL Key \n"
			" [--sslkeypass]                 SSL Password \n"
			" <input_files>                  CMAF streaming files (.cmf[atvm])\n"

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
				if (t.compare("--tsoffset") == 0) { tsoffset_ = atoi(argv[++i]); continue; } // only up to 32 bit inputs
				if (t.compare("--stop_at") == 0) { stop_at_ = atoi(argv[++i]); continue; } // only up to 32 bit inputs
				if (t.compare("--dont_close") == 0) { dont_close_ = true; continue; }
				if (t.compare("--daemon") == 0) { daemon_ = true; continue; }
				if (t.compare("--chunked") == 0) { chunked_ = true; continue; }
				if (t.compare("--sslcert") == 0) { ssl_cert_ = string(argv[++i]); continue; }
				if (t.compare("--sslkey") == 0) { ssl_key_ = string(argv[++i]); continue; }
				if (t.compare("--keypass") == 0) {ssl_key_pass_= string(argv[++i]); continue;}
				if (t.compare("--keypass") == 0) { basic_auth_ = string(argv[++i]); continue; }
				input_files_.push_back(argv[i]);
			}
				
		}
		else
			print_options();
	}

	string url_;
	bool realtime_;
	bool daemon_;
	bool loop_;
	uint64_t tsoffset_;
	uint64_t stop_at_;
	bool dont_close_;
	bool chunked_;

	unsigned int drop_every_;
	bool prime_;
	bool dry_run_;
	int verbose_;

	string ssl_cert_;
	string ssl_key_;
	string ssl_key_pass_;
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
			c_tfdt = st->str_ptr_->media_fragment_[st->fnumber_].tfdt_.base_media_decode_time_;
			stop_at = st->opt_.stop_at_ && (c_tfdt >= st->opt_.stop_at_);
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
			// fixed delay of 3 seconds when posting fragments
			double frag_delay = 2.0; //((double)st->str_ptr_->media_fragment_[st->fnumber_].get_duration()) / ((double)st->timescale_);
			double media_time = ( (double) c_tfdt - st->start_time_stamp_)/ ((double) st->timescale_);
			chrono::duration<double> diff = chrono::system_clock::now() - *(st->start_time_);
			double start_offset = (double) st->opt_.tsoffset_ / ((double)st->timescale_);
			if ((diff.count() + start_offset) < (media_time - frag_delay)) // if it is to early sleep until tfdt - frag_dur
			{
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
				((double)st->str_ptr_->media_fragment_[st->fnumber_].get_duration()) / ((double)st->timescale_) << " seconds " << endl;
			
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
			cout << "---- reading fmp4 input file " << file_name <<  endl;
			ingest_stream ingest_stream;
			ingest_stream.load_from_file(input);
			input.close();

			// get the timescale
			// uint32_t time_scale = ingest_stream.init_fragment_.get_time_scale();
			// uint32_t fragment_duration = (uint32_t) ingest_stream.media_fragment_[0].get_duration();
			//cout << "---- timescale movie header: " << time_scale << endl;
			//cout << "---- duration of first fragment: " << fragment_duration << endl;
			//cout << "url is: " << opt.url_ << endl;
			string post_url_string = opt.url_ + "/Streams(" + file_name + ")";
			cout << "---- finished loading fmp4 file starting the ingest to: " << post_url_string  << endl;

			// check the connection with the post server
			// create databuffer with init segment
			vector<uint8_t> init_seg_dat;
			ingest_stream.get_init_segment_data(init_seg_dat);

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
				curl_easy_setopt(curl, CURLOPT_USERPWD, opt.basic_auth_.c_str());
			}

			if(opt.ssl_cert_.size())
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
			post_state.frag_duration_ = ingest_stream.media_fragment_[0].get_duration();
			post_state.init_done_ = true; // the init fragment was already send
			post_state.start_time_stamp_ = ingest_stream.media_fragment_[0].tfdt_.base_media_decode_time_;
			post_state.str_ptr_ = &ingest_stream;
			post_state.timescale_ = ingest_stream.init_fragment_.get_time_scale();
			post_state.is_done_ = false; 
			post_state.offset_in_fragment_ = 0;
			post_state.opt_ = opt;
			post_state.file_name_ = file_name;
			post_state.tfdt_loop_offset_ = 0;

			chrono::time_point<chrono::system_clock> tp = chrono::system_clock::now();
			post_state.start_time_ = &tp; // start time
			

			if (opt.tsoffset_ > 0) // this offset is in the timescale of the track better would be to work with seconds double
			{
				// find the timestamp offset and update the fnumber, assume constant duration
				unsigned int offset = (unsigned int) ((opt.tsoffset_) / post_state.frag_duration_);
				if (offset < ingest_stream.media_fragment_.size())
					post_state.fnumber_ = offset;
			}

			curl_easy_setopt(curl, CURLOPT_URL, post_url_string.data());
			curl_easy_setopt(curl, CURLOPT_POST, 1);
			curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
			curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
			curl_easy_setopt(curl, CURLOPT_READDATA, (void *) &post_state);
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

			if (opt.basic_auth_.size()) {
				curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
				curl_easy_setopt(curl, CURLOPT_USERPWD, opt.basic_auth_.c_str());
			}

			// client side TLS certificates
			if (opt.ssl_cert_.size())
				curl_easy_setopt(curl, CURLOPT_SSLCERT, opt.ssl_cert_.c_str());
			if (opt.ssl_key_.size())
				curl_easy_setopt(curl, CURLOPT_SSLKEY, opt.ssl_key_.c_str());
			if (opt.ssl_key_pass_.size())
				curl_easy_setopt(curl, CURLOPT_KEYPASSWD, opt.ssl_key_pass_.c_str());

			struct curl_slist *chunk = NULL;
			chunk = curl_slist_append(chunk, "Transfer-Encoding: chunked");
			chunk = curl_slist_append(chunk, "Expect:");
			res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

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
				if(!post_state.is_done_ && !stop_all)
				   this_thread::sleep_for(chrono::duration<double>(2.0));
			}

			cout << " done pushing file: " << file_name << " press q to quit " << endl;

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

// entry point
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