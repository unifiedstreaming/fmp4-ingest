/*******************************************************************************
Supplementary software media ingest specification:
https://github.com/unifiedstreaming/fmp4-ingest
Copyright (C) 2009-2021 CodeShop B.V.
http://www.code-shop.com
CMAF ingest from stored CMAF files, emulates a live encoder posting CMAF content
******************************************************************************/

#include "event/fmp4stream.h"
#include "event/event_track.h"
#include "curl/curl.h"
#include <iostream>
#include <fstream>
#include <exception>
#include <memory>
#include <chrono>
#include <thread>
#include <cstring>
#include <bitset>
#include <iomanip>
#include "event/base64.h"

using namespace fmp4_stream;
using namespace std;
bool stop_all = false;

// an ugly global cross thread variable for the cmaf presentation duration
double cmaf_presentation_duration;

size_t write_function(void *ptr, size_t size, size_t nmemb, std::string* data)
{
	data->append((char*)ptr, size * nmemb);
	return size * nmemb;
}

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
		*res_time = ul ? ul : 0;

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	return 0;
}

struct push_options_t
{
	push_options_t()
		: url_("http://localhost/live/video.isml/video.ism")
		, realtime_(false)
		, daemon_(false)
		, loop_(0)
		, wc_off_(false)
		, wc_uri_("http://time.akamai.com")
		, ism_offset_(0)
		, ism_use_ms_(0)
		, wc_time_start_(0)
		, dont_close_(true)
		, chunked_(false)
		, drop_every_(0)
		, prime_(false)
		, dry_run_(false)
		, verbose_(2)
		, cmaf_presentation_duration_(0)
		, avail_(0)
		, avail_dur_(0)
		, announce_(60.0)
		,anchor_scale_(1)
	{
	}

	static void print_options()
	{
		printf("Usage: fmp4ingest [options] <input_files>\n");
		printf(
			" [-u url]                       Publishing Point URL\n"
			" [-r, --realtime]               Enable realtime mode\n"
			" [-l, --loop]                   Enable looping arg1 + 1 times \n"
			" [--wc_offset]                  (boolean )Add a wallclock time offset from time server for converting VoD (0) asset to Live \n"
			" [--ism_offset]                 insert a fixed value for the wallclock time offset instead of using a remote time source uri\n"
			" [--ism_use_ms]                 indicates that the ism_offset is given in milliseconds \n"
			" [--wc_uri]                     uri for fetching wall clock time default time.akamai.com \n"
			" [--initialization]             SegmentTemplate@initialization sets the relative path for init segments, shall include $RepresentationID$ \n"
			" [--media]                      SegmentTemplate@media sets the relative path for media segments, shall include $RepresentationID$ and $Time$ or $Number$ \n"
//			" [--chunked]                    Use chunked Transfer-Encoding for POST (long running post) otherwise short running per fragment post \n"
			" [--avail]                      signal an advertisment slot every arg1 ms with duration of arg2 ms \n"
			" [--dry_run]                    Do a dry run and write the output files to disk directly for checking file and box integrity\n"
			" [--announce]                   specify the number of seconds in advance to presenation time to send an avail"
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
				char* pEnd;
				string t(argv[i]);
				if (t.compare("-u") == 0) { url_ = string(argv[++i]); continue; }
				if (t.compare("-l") == 0 || t.compare("--loop") == 0) { loop_ = atoi(argv[++i]); continue; }
				if (t.compare("-r") == 0 || t.compare("--realtime") == 0) { realtime_ = true; continue; }
				if (t.compare("--close_pp") == 0) { dont_close_ = false; continue; }
				if (t.compare("--daemon") == 0) { daemon_ = true; continue; }
//				if (t.compare("--chunked") == 0) { chunked_ = true; continue; }
				if (t.compare("--wc_offset") == 0) { wc_off_ = true; continue; }
				if (t.compare("--ism_offset") == 0) { ism_offset_ = strtoull(argv[++i], NULL,10); continue; }
				if (t.compare("--ism_use_ms") == 0) { ism_use_ms_ = 1; anchor_scale_ = 1000; continue; }
				if (t.compare("--dry_run") == 0) { dry_run_ = true; continue; }
				if (t.compare("--wc_uri") == 0) { wc_uri_ = string(argv[++i]); continue; }
				if (t.compare("--auth") == 0) { basic_auth_ = string(argv[++i]); continue; }
				if (t.compare("--avail") == 0) { avail_ = strtoull(argv[++i],NULL,10); avail_dur_= strtoull(argv[++i],NULL,10); continue; }
				if (t.compare("--announce") == 0) { announce_ = atof(argv[++i]); continue; }
				if (t.compare("--aname") == 0) { basic_auth_name_ = string(argv[++i]); continue; }
				if (t.compare("--sslcert") == 0) { ssl_cert_ = string(argv[++i]); continue; }
				if (t.compare("--sslkey") == 0) { ssl_key_ = string(argv[++i]); continue; }
				if (t.compare("--keypass") == 0) { ssl_key_pass_ = string(argv[++i]); continue; }
				if (t.compare("--keypass") == 0) { basic_auth_ = string(argv[++i]); continue; }
				if (t.compare("--media") == 0) { segmentTemplate_media_ = string(argv[++i]); continue; }
				if (t.compare("--initialization") == 0) { segmentTemplate_init_ = string(argv[++i]); continue; }
				input_files_.push_back(argv[i]);
			}

			// get the wallclock offset 
			if (wc_off_)
				get_remote_sync_epoch(&wc_time_start_, wc_uri_);
			if (ism_offset_ > 0) 
			{
				wc_time_start_ = ism_offset_;
				wc_off_ = true;
			}
		}
		else
			print_options();
	}

	string url_;
	string segmentTemplate_media_;
	string segmentTemplate_init_;

	bool realtime_;
	bool daemon_;
	int loop_;
	bool wc_off_;

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

	double announce_; // number of seconds to send an announce in advance
	double cmaf_presentation_duration_;
	vector<string> input_files_;

	uint64_t avail_; // insert an avail every X milli seconds
	uint64_t avail_dur_; // of duratoin Y milli seconds
	uint64_t ism_offset_;
	uint32_t ism_use_ms_;
	uint32_t anchor_scale_;
};

struct ingest_post_state_t
{
	bool init_done_; // flag if init fragment was sent
	bool error_state_; // flag signalling error state
	uint64_t fnumber_; // fragment nnmber in queue being set
	uint64_t frag_duration_; //  fragment duration
	uint32_t timescale_; // timescale of the media track
	uint32_t offset_in_fragment_; // for partial chunked sending keep track of fragment offset
	uint64_t start_time_stamp_; // ts offset
	ingest_stream *str_ptr_; // pointer to the ingest stream
	bool is_done_; // flag set when the stream is done
	string file_name_;
	chrono::time_point<chrono::system_clock> *start_time_; // time point the post was started
	push_options_t *opt_;
};

// generate segment name (init or media)
// init shall not contain $Number$ or $Time$ 
// media shall contain $Number$ or $Time$

string get_path_from_template(
	string &template_string,
	string &file_name,
	uint64_t time,
	uint64_t number)
{
	string out_string;

	const string number_str = "$Number$";
	const string time_str = "$Time$";
	const string rep_str = "$RepresentationID$";

	string rep_name;

	if (file_name.size() > 0)
	{
		size_t poss = file_name.find_last_of(".");
		if (poss != std::string::npos)
			rep_name = file_name.substr(0, poss);
	}

	size_t rep_pos = template_string.find_first_of(rep_str);

	if (rep_pos != std::string::npos)
	{
		string b, c;

		if (rep_pos != 0)
			b = template_string.substr(0, rep_pos);

		if (rep_pos + rep_str.size() < template_string.size())
			c = template_string.substr(rep_pos + rep_str.size());

		out_string = b + rep_name + c;
	}

	size_t time_pos = out_string.find(time_str);
	size_t num_pos = out_string.find(number_str);

	if (time_pos != std::string::npos)
	{
		string b, c;

		if (time_pos != 0)
			b = out_string.substr(0, time_pos);

		if (time_pos + time_str.size() < out_string.size())
			c = out_string.substr(time_pos + time_str.size());

		out_string = b + std::to_string(time) + c;
	}
	else if (num_pos != std::string::npos)
	{
		string b, c;

		if (num_pos != 0)
			b = out_string.substr(0, num_pos);

		if (num_pos + number_str.size() < out_string.size())
			c = out_string.substr(num_pos + number_str.size());

		out_string = b + std::to_string(number) + c;
	}

	return out_string;
}

int push_thread(ingest_stream l_ingest_stream, 
	push_options_t opt, string post_url_string, std::string file_name)
{
	try
	{
		vector<uint8_t> init_seg_dat;
		l_ingest_stream.get_init_segment_data(init_seg_dat);

	    std:string out_file = "o_" + file_name;
		ofstream outf = std::ofstream(out_file, std::ios::binary);
		if (outf.good() && opt.dry_run_)
			outf.write((char *)&init_seg_dat[0], init_seg_dat.size());

		// setup curl
		CURL * curl;
		CURLcode res;
		curl = curl_easy_init();
		string post_init_url_string = post_url_string;

		if (opt.segmentTemplate_init_.size())
		{
			post_init_url_string = opt.url_  + "/" + get_path_from_template(
				opt.segmentTemplate_init_,
				file_name,
				0,
				0);
		}

		curl_easy_setopt(curl, CURLOPT_URL, post_init_url_string.c_str());
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

		if (!opt.dry_run_) {
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
		}
		// setup the post
		ingest_post_state_t post_state = {};
		post_state.error_state_ = false;
		post_state.fnumber_ = 0;
		post_state.frag_duration_ = l_ingest_stream.media_fragment_[0].get_duration();
		post_state.init_done_ = true; // the init fragment was already sent
		post_state.start_time_stamp_ = l_ingest_stream.media_fragment_[0].tfdt_.base_media_decode_time_;
		post_state.str_ptr_ = &l_ingest_stream;
		post_state.timescale_ = l_ingest_stream.init_fragment_.get_time_scale();
		post_state.is_done_ = false;
		post_state.offset_in_fragment_ = 0;
		post_state.opt_ = &opt;
		post_state.file_name_ = file_name;

		chrono::time_point<chrono::system_clock> tp = chrono::system_clock::now();
		post_state.start_time_ = &tp; // start time

		curl_easy_setopt(curl, CURLOPT_URL, post_url_string.data());
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

		struct curl_slist *chunk = NULL;
		chrono::time_point<chrono::system_clock> start_time = chrono::system_clock::now();
		
		while (!stop_all)
		{

			for (uint64_t i = 0; i < l_ingest_stream.media_fragment_.size(); i++)
			{
				vector<uint8_t> media_seg_dat;
				uint64_t media_seg_size = l_ingest_stream.get_media_segment_data(i, media_seg_dat);
			
				if (!opt.dry_run_) {

					if (opt.segmentTemplate_media_.size())
					{
						uint64_t l_time = l_ingest_stream.media_fragment_[i].tfdt_.base_media_decode_time_;

						post_url_string = opt.url_ + "/" + get_path_from_template(
							opt.segmentTemplate_media_,
							file_name,
							l_time,
							i);
					    
						curl_easy_setopt(curl, CURLOPT_URL, post_url_string.data());
						curl_easy_setopt(curl, CURLOPT_POST, 1);
						curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
					}
					
					curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char *)&media_seg_dat[0]);
					curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)media_seg_dat.size());
					res = curl_easy_perform(curl);

					if (res != CURLE_OK)
					{
						// media segment post failed resend the init segment and the segment
						int retry_count = 0;
						while (res != CURLE_OK)
						{
							curl_easy_setopt(curl, CURLOPT_URL, post_init_url_string.data());
							curl_easy_setopt(curl, CURLOPT_POST, 1);
							curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

							curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char *)&init_seg_dat[0]);
							curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)init_seg_dat.size());
							res = curl_easy_perform(curl);
							std::this_thread::sleep_for(std::chrono::milliseconds(300));
							retry_count++;
							if (retry_count == 2)
								break;
						}
					}
					if (res == CURLE_OK)
					{
						fprintf(stderr, "post of media segment ok: %s\n",
							curl_easy_strerror(res));
					}
					else
					{
						fprintf(stderr, "post of media segment failed: %s\n",
							curl_easy_strerror(res));
					}
				}
				else
				{
					if (outf.good() && opt.dry_run_)
						outf.write((char *)&media_seg_dat[0], media_seg_dat.size());
				}

				uint64_t c_tfdt = l_ingest_stream.media_fragment_[i].tfdt_.base_media_decode_time_;
				uint64_t t_diff = c_tfdt - l_ingest_stream.media_fragment_[0].tfdt_.base_media_decode_time_;

				cout << " pushed media fragment: " << i << " file_name: " << post_state.file_name_ << " fragment duration: " << \
					(l_ingest_stream.media_fragment_[i].get_duration()) / ((double)post_state.timescale_) << " seconds ";

				if (post_state.timescale_ > 0)
					cout << " media time elapsed: " << (double) (t_diff + l_ingest_stream.media_fragment_[i].get_duration()) / (double) post_state.timescale_ << endl;

				if (opt.realtime_)
				{
					chrono::duration<double> diff = chrono::system_clock::now() - start_time;
					const double media_time = ((double)(c_tfdt - l_ingest_stream.get_start_time())) / l_ingest_stream.init_fragment_.get_time_scale();


					// wait untill media time - frag_delay > elapsed time + initial offset
					if ((diff.count()) < (media_time)) // if it is to early sleep until tfdt - frag_dur
					{
						double fdel = (double)(l_ingest_stream.media_fragment_[i].get_duration()) / ((double)post_state.timescale_);
						// sleep but the maximum sleep time is one fragment duration
						if (fdel < (media_time - diff.count()))
							this_thread::sleep_for(chrono::duration<double>(fdel));
						else
							this_thread::sleep_for(chrono::duration<double>((media_time)-diff.count()));
					}

				}
				else
				{ // non real time just sleep for 10 milli seconds
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}

				//std::cout << " --- posting next segment ---- " << i << std::endl;
				if (post_state.is_done_ || stop_all) {
					i = (uint64_t)l_ingest_stream.media_fragment_.size();
					break;
				}
			}

			if (opt.loop_ > 0) {
				l_ingest_stream.patch_tfdt(
					(uint64_t)opt.cmaf_presentation_duration_ \
					* l_ingest_stream.init_fragment_.get_time_scale(), 
					false
				);
				start_time = chrono::system_clock::now();
				opt.loop_--;
			}
			else 
			{
				stop_all = true;
			}
		}


		// only close with mfra if dont close is not set
		if (!opt.dont_close_ && !opt.dry_run_)
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
		if (opt.dry_run_)
			if (outf.good())
				outf.close();
	}
	catch (...)
	{
		cout << "Unknown Error" << endl;
	}

	return 0;
}

int main(int argc, char * argv[])
{
	push_options_t opts;
	opts.parse_options(argc, argv);
	vector<ingest_stream> l_istreams(opts.input_files_.size());
	typedef shared_ptr<thread> thread_ptr;
	typedef vector<thread_ptr> threads_t;
	ingest_stream meta_ingest_stream; 
	threads_t threads;
	int l_index = 0;

	for (auto it = opts.input_files_.begin(); it != opts.input_files_.end(); ++it)
	{
		ifstream input(*it, ifstream::binary);

		if (!input.good())
		{
			std::cout << "failed loading input file: [cmf[tavm]]" << string(*it) << endl;
			push_options_t::print_options();
			return 0;
		}

		ingest_stream &l_ingest_stream = l_istreams[l_index];
		l_ingest_stream.load_from_file(input);

		// patch the tfdt values with an offset time
		if (opts.wc_off_)
			l_ingest_stream.patch_tfdt(opts.wc_time_start_, true, opts.anchor_scale_);

		
		double l_duration = (double) l_ingest_stream.get_duration() / (double) l_ingest_stream.init_fragment_.get_time_scale();

		if (l_duration > opts.cmaf_presentation_duration_) {
			opts.cmaf_presentation_duration_ = l_duration;
			std::cout << "CMAF presentation duration updated to: " << l_duration << " seconds " << std::endl;
		}

		l_index++;
		input.close();
	}
	l_index = 0;

	if (opts.avail_)
	{
		string avail_track = "out_avail_track.cmfm";
		string post_url_string = opts.url_ + "/Streams(" + "out_avail_track.cmfm" + ")";
		
		if (opts.cmaf_presentation_duration_ == 0.0)  // no media tracks exist
		{
			opts.cmaf_presentation_duration_ = 4000 * opts.avail_ / 1000;
		}

		event_track::gen_avail_files((uint32_t ) (opts.cmaf_presentation_duration_ * 1000), 2000, opts.avail_dur_, opts.avail_, opts.wc_time_start_);

		
		ifstream input_file_meta(avail_track, ifstream::binary);
		meta_ingest_stream.load_from_file(input_file_meta);

		if (opts.wc_off_)
			meta_ingest_stream.patch_tfdt(opts.wc_time_start_, true, opts.anchor_scale_);
		
		// create the file
		thread_ptr thread_n(new thread(push_thread, meta_ingest_stream, opts, post_url_string, avail_track));
		threads.push_back(thread_n);

		// delay the media threads compared to the timed metadata tracks
		if(opts.announce_)
			std::this_thread::sleep_for(std::chrono::milliseconds(1000 *  (int) opts.announce_));
		else
		    std::this_thread::sleep_for(std::chrono::milliseconds(4000));
	}

	for (auto it = opts.input_files_.begin(); it != opts.input_files_.end(); ++it)
	{
		string post_url_string = opts.url_ + "/Streams(" + *it + ")";

		if(it->substr(it->find_last_of(".") + 1) == "cmfm")
        {
			cout << "push thread: " << post_url_string << endl;
		    thread_ptr thread_n(new thread(push_thread, l_istreams[l_index], opts, post_url_string, (string) *it));
		    threads.push_back(thread_n);
        }
		else 
		{
			cout << "push thread: " << post_url_string << endl;
			thread_ptr thread_n(new thread(push_thread, l_istreams[l_index], opts, post_url_string, (string) *it));
			threads.push_back(thread_n);
		}	
		l_index++;
	}


	for (auto& th : threads)
		th->join();
}
