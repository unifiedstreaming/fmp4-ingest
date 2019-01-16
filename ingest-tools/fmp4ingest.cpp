/*******************************************************************************
Supplementary software media ingest specification:
https://github.com/unifiedstreaming/fmp4-ingest

Copyright (C) 2009-2018 CodeShop B.V.
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
			" [-l, --loop]                   Enable endless loop\n"
			" [-r, --realtime]               Enable realtime mode\n"
			" [--tsoffset offset]            Input timestamp offset\n"
			" [--stop_at offset]             Stop at timestamp\n"
			" [--dont_close]                 Do not send empty mfra box\n"
			" [--daemon]                     If you run as daemon, then use this flag\n"
			" [--chunked]                    Use chunked Transfer-Encoding for POST\n"
			" <input_files>                  CMAF streaming files (.cmf[atv])\n"
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

				input_files_.push_back(argv[i]);
			}
		}
		else
			print_options();
	}

	std::string url_;
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
	std::chrono::time_point<std::chrono::system_clock> *start_time_; // time point the post was started
	push_options_t opt_;
};


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
			st->offset_in_fragment_ += buffer_size;
			return buffer_size;
		}
	}
	else // read the next fragment or send pause
	{
		// stop if all fragments send or stop_at_ timestamp based
		
		bool frags_finished = st->str_ptr_->m_media_fragment.size() <= st->fnumber_;
		bool stop_at = false;
		uint64_t c_tfdt = 0;
		if (!frags_finished) {
			c_tfdt = st->str_ptr_->m_media_fragment[st->fnumber_].m_tfdt.m_basemediadecodetime;
			stop_at = st->opt_.stop_at_ && (c_tfdt >= st->opt_.stop_at_);
		}
		if ((frags_finished || stop_at) ) {
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
				cout << "looping" << endl; // this does not work fully as we need to increase the tfdt
				//st->start_time = &chrono::system_clock::now();
				st->fnumber_ = 0;
			}
		}
		
		if (st->opt_.realtime_) // if it is to early sleep until tfdt - frag_dur
		{
			double frag_dur = ((double)st->str_ptr_->m_media_fragment[st->fnumber_].get_duration()) / ((double)st->timescale_);
			double media_time = ( (double) c_tfdt - st->start_time_stamp_)/ ((double) st->timescale_);
			chrono::duration<double> diff = chrono::system_clock::now() - *(st->start_time_);
			double start_offset = (double) st->opt_.tsoffset_ / ((double)st->timescale_);
			if ((diff.count() + start_offset) < (media_time - frag_dur)) // if it is to early sleep until tfdt - frag_dur
			{
				this_thread::sleep_for(chrono::duration<double>((media_time - frag_dur) - diff.count()));
			}
		}
		
		vector<uint8_t> frag_data;
		st->str_ptr_->get_media_segment_data(st->fnumber_,frag_data);
		
		// we can finish the fragment
		if (frag_data.size() - st->offset_in_fragment_  <= buffer_size) 
		{
			memcpy(dest, frag_data.data() + st->offset_in_fragment_, frag_data.size() - st->offset_in_fragment_);
			nr_bytes_written = frag_data.size() - st->offset_in_fragment_;
			st->offset_in_fragment_ = 0;
			

			cout << " pushed media fragment: " << st->fnumber_ << " file_name: " <<  st->file_name_ << " fragment duration: " << \
				((double)st->str_ptr_->m_media_fragment[st->fnumber_].get_duration()) / ((double)st->timescale_) << " seconds " << endl;
			
			st->fnumber_++;

			return nr_bytes_written;
		}
		else // we cannot finish the fragment, write the buffer
		{
			memcpy(dest, frag_data.data() + st->offset_in_fragment_, buffer_size);
			(st->offset_in_fragment_)+=buffer_size;
			return buffer_size;
		}
	}
}

// main loop
int push_thread(std::string file_name, push_options_t opt)
{
	try
	{
		if (file_name.size())
		{
			ifstream input(file_name, ifstream::binary);
			
			if (!input.good())
			{
				cout << "failed loading input file: [cmf[tav]]" << string(file_name) << endl;
				push_options_t::print_options();
				return 0;
			}

			// read all and then start sending
			cout << "---- reading fmp4 input file " << file_name <<  endl;
			ingest_stream ingest_stream;
			ingest_stream.load_from_file(&input);
			input.close();

			// get the timescale
			uint32_t time_scale = ingest_stream.m_init_fragment.get_time_scale();
            uint32_t fragment_duration = ingest_stream.m_media_fragment[0].get_duration();
			cout << "---- timescale movie header: " << time_scale << endl;
			cout << "---- duration of first fragment: " << fragment_duration << endl;
			cout << "url is: " << opt.url_ << endl;
			cout << "---- finished loading fmp4 file starting the ingest " << endl;

			// setup the post
			ingest_post_state_t post_state = {};
			post_state.error_state_ = false;
			post_state.fnumber_ = 0;
			post_state.frag_duration_ = ingest_stream.m_media_fragment[0].get_duration();
			post_state.init_done_ = false;
			post_state.start_time_stamp_ = ingest_stream.m_media_fragment[0].m_tfdt.m_basemediadecodetime;
			post_state.str_ptr_ = &ingest_stream;
			post_state.timescale_ = ingest_stream.m_init_fragment.get_time_scale();
			post_state.is_done_ = false; 
			post_state.offset_in_fragment_ = 0;
			post_state.opt_ = opt;
			post_state.file_name_ = file_name;

			chrono::time_point<std::chrono::system_clock> tp = std::chrono::system_clock::now();
			post_state.start_time_ = &tp; // start time
			std::string post_url_string = opt.url_ + "/Streams(" + file_name + ")";

			if (opt.tsoffset_ > 0) // this offset is in the timescale of the track better would be to work with seconds double
			{
				// find the timestamp offset and update the fnumber, assume constant duration
				unsigned int offset = ((opt.tsoffset_) / post_state.frag_duration_);
				if (offset < ingest_stream.m_media_fragment.size())
					post_state.fnumber_ = offset;
			}

			// setup curl
			CURL * curl;
			CURLcode res;
			curl = curl_easy_init();
			curl_easy_setopt(curl, CURLOPT_URL, post_url_string.data());
			curl_easy_setopt(curl, CURLOPT_POST, 1);
			curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
			curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
			curl_easy_setopt(curl, CURLOPT_READDATA, (void *) &post_state);
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
			struct curl_slist *chunk = NULL;
			chunk = curl_slist_append(chunk, "Transfer-Encoding: chunked");
			chunk = curl_slist_append(chunk, "Expect:");
			res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

			// do the ingest
			while (!post_state.is_done_ && !stop_all) 
			{
				res = curl_easy_perform(curl);

				/* Check for errors */
				if (res != CURLE_OK) {
					fprintf(stderr, "---- long running post failed: %s\n",
						curl_easy_strerror(res));
					post_state.error_state_ = true;
				}
				else
					fprintf(stderr, "---- long running post ok: %s\n",
						curl_easy_strerror(res));

				// if not done wait one fragment duration before resuming
				if(!post_state.is_done_ && !stop_all)
				   this_thread::sleep_for(chrono::duration<double>(post_state.frag_duration_));
			}

			cout << " done pushing file: " << file_name << " press q to quit " << endl;

			/* always cleanup */
			curl_easy_cleanup(curl);
		}
	}
	catch (...)
	{
		cout << "Unkown Error" << endl;
	}

	return 0;
}

int main(int argc, char * argv[])
{
	push_options_t opts;
	opts.parse_options(argc, argv);

	typedef std::shared_ptr<std::thread> thread_ptr;
	typedef std::vector<thread_ptr> threads_t;
	threads_t threads;

	for (auto it = opts.input_files_.begin(); it != opts.input_files_.end(); ++it)
	{
		thread_ptr thread_n(new std::thread(push_thread, *it, opts));
		threads.push_back(thread_n);
	}
	
	// wait for the push threads to finish
	cout << " loaded, q press key to exit " << endl;
	char c='0';
	while(c=cin.get() != 'q');

	stop_all = true;

	for (auto& th : threads)
		th->join();
}