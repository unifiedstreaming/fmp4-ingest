#include<iostream>
#include<ctime>
#include<string>
#include<vector>
#include "curl/curl.h"
#include <iostream>
#include <cmath>

#include "event/fmp4stream.h"
#include "event/event_track.h"
#include "event/base64.h"

#include "event/event_track.h"
#include <fstream>

/*
    separate program to push automatically generated timed metadata track 

    timed metadata track is based on iso/iec 23001-18 

    breaks are intervals given as arguments: --avail avail_interval avail_duration

    all times/duration are in milliseconds 

    option --dry_run does not post the segments but writes local file instead 

    follows the encoder synchronization patter of generating aligned segements 
    and ad breaks, i.e. 
    
    a break every K * avail_interval since epoch 
    a segment boundary every N * segment_duration since epoch

    the marker ad break signalling uses splice_insert command from SCTE-35

*/

struct push_marker_options_t
{
    push_marker_options_t()
        : url_("http://127.0.0.1:8080")
        , dry_run_(false)
        , avail_interval_(60000)
        , avail_dur_(10000)
        , seg_dur_(2000)
        , timescale_(1000)
        , track_id_(1)
        , announce_(0)
    {
    }

    static void print_options()
    {
        printf("Usage: push_markers [options] \n");
        printf(
            " [-u url]                       Publishing Point URL without /Streams() extension\n"
            " [--avail]                      signal an advertisment slot every arg1 ms with duration of arg2 ms (default is 60000 and 10000) \n"
            " [--seg_dur]                    segment duration of avail segments in the timed metadata track in ms (default=2000ms) \n"                 
            " [--track_id]                   Track id to put in the track (default is 1)"
            " [--dry_run]                    Do a dry run and write the output files to disk directly for checking file and box integrity default is false\n"
            " [--announce]                   specify the number of milliseconds seconds in advance to presenation time to send an avail (default is 0)"
            "\n");
    }

    void parse_options(int argc, char* argv[])
    {
        if (argc > 1)
        {
            for (int i = 1; i < argc; i++)
            {
                char* pEnd;
                std::string t(argv[i]);
                if (t.compare("-u") == 0) { url_ = std::string(argv[++i]); continue; }
                if (t.compare("--seg_dur") == 0) { seg_dur_ = strtoull(argv[++i], NULL, 10); continue; }
                if (t.compare("--avail") == 0) { 
                    avail_interval_ = strtoull(argv[++i], NULL, 10);
                    avail_dur_ = strtoull(argv[++i], NULL, 10);
                    continue; }
                if (t.compare("--track_id") == 0) { track_id_ = (uint32_t) strtoull(argv[++i], NULL, 10); continue; }
                if (t.compare("--announce") == 0) { announce_ = (uint32_t) strtoull(argv[++i], NULL, 10); continue; }
                if (t.compare("--dry_run") == 0) { dry_run_ = true; continue; }
                
            }
          
        }
        else
            print_options();
    }

    std::string url_;
    bool dry_run_;
    uint64_t announce_;       // number of milliseconds to send an announce X ms in advance
    uint64_t seg_dur_;        // duration of segments
    uint64_t avail_interval_; // interval of avails
    uint64_t avail_dur_;      // avail duration in ms
    uint32_t track_id_;       //  track id of the metadata track
    uint32_t timescale_;      // timescale of the metadata track
};

// use curl to push the segment/hedaer using HTTP post over HTTP 1.1.
struct PostCurlIngestConnection 
{

    static CURLcode send_curl_post(std::string &post_url, std::vector<uint8_t> &data) {
       
            try {
                CURL* curl = curl_easy_init();
                curl_easy_setopt(curl, CURLOPT_URL, post_url.c_str());
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
                curl_easy_setopt(curl, CURLOPT_POST, 1);
                curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (char*)&data[0]);
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)data.size());
                CURLcode res = curl_easy_perform(curl);
                curl_easy_cleanup(curl);

                if (res != CURLE_OK)
                    fprintf(stderr, " CURL HTTP post of segment failed:  %s\n",
                        curl_easy_strerror(res));
                else
                    return res;
            }
            catch (...)
            {
                std::cout << "unkown error occured" << std::endl;
                return CURLcode::CURLE_INTERFACE_FAILED;
            }
       
    }
};



// wrapper generates splice insert message
event_track::DASHEventMessageBoxv1 generate_event_message_splice_insert(
    uint32_t id, 
    uint64_t presentation_time, 
    uint64_t duration, 
    uint32_t timescale)
{

    event_track::DASHEventMessageBoxv1 ev;
    ev = {};
    ev.id_ = id;
    ev.presentation_time_ = presentation_time;
    ev.event_duration_ = (uint32_t) duration;
    ev.timescale_ = timescale;
    ev.scheme_id_uri_ = "urn:scte:scte35:2013:bin";
    fmp4_stream::gen_splice_insert(
        ev.message_data_, ev.id_, 
        ev.event_duration_ * 90
    );
    return ev;
}

int main(int argc, char* argv[])
{
    push_marker_options_t opts;
    opts.parse_options(argc, argv);

    
    uint64_t last_K = 0;
    std::string uri = opts.url_ + "/Streams(out_meta.cmfm)";

    // ad break and event characteristics
   
    uint64_t last_L = 0;
    uint64_t next_L = 0;

    std::fstream oft("out_meta_track.cmfm" , std::ios::binary | std::ios::out);

    std::vector<uint8_t> header_bytes;
    event_track::get_meta_header_bytes(opts.track_id_, opts.timescale_, header_bytes);
    
    if(opts.dry_run_)
       oft.write((const char *)&header_bytes[0], header_bytes.size());
    else
       PostCurlIngestConnection::send_curl_post(uri, header_bytes);

    while (1) {
        
        std::time_t t = std::time(0);  // t is an integer type
        uint64_t t_d = (uint64_t)t * 1000 + opts.announce_;

        uint64_t next_K =  (uint64_t) std::ceil( t_d / (double) opts.seg_dur_ ); // calculate the next segment boundary
        last_L = (uint64_t)std::floor( t_d / (double) opts.avail_interval_);
        
        // the previous/current ad break (probably already ended) 
        uint64_t ad_start_last = last_L * opts.avail_interval_ ;
        uint64_t ad_end_last = ad_start_last + opts.avail_dur_ ;

        // the next ad break to announce
        uint64_t ad_start_next = (last_L + 1) * opts.avail_interval_;
        uint64_t ad_end_next = ad_start_next + opts.avail_dur_ ;

        if (next_K != last_K) {
            // fixed timescale 1000 and milliseconds based durations
            uint64_t seg_start = (uint64_t) ( next_K * opts.seg_dur_ );
            uint64_t seg_end = (uint64_t)   ( opts.seg_dur_ + seg_start);

            std::cout << " next_K is : " << next_K << std::endl;
            // std::cout << "next segment boundary is after: " << next_K * opts.seg_dur_  - t_d << " milliseconds" << std::endl;
            // std::cout << "previous tfdt was  " << t_d - last_K * opts.seg_dur_  << "  milli seconds ago" << std::endl;
            
            last_K = next_K;
            std::vector<event_track::DASHEventMessageBoxv1> in_emsg_list;

            if (t_d < ad_end_last && t_d >= ad_start_last - opts.avail_dur_)
            {
                
                auto ev = generate_event_message_splice_insert(
                    (uint32_t) last_L,
                    (uint64_t) ad_start_last,
                    (uint64_t) opts.avail_dur_,
                    (uint32_t) opts.timescale_
                );
                std::cout << " === ad slot is active == " << std::endl;
           
                in_emsg_list.push_back(ev);
            }

            if (t_d < ad_end_next && t_d >= ad_start_next - opts.avail_dur_)
            {
                std::cout << " == ad slot is active, pushing segment with markers == " << std::endl;

                auto ev = generate_event_message_splice_insert(
                    (uint32_t)last_L + 1,
                    (uint64_t)ad_start_next ,
                    (uint64_t)opts.avail_dur_,
                    opts.timescale_
                );
                // ev.print();

                in_emsg_list.push_back(ev);
            }

            std::vector<uint8_t> segment_bytes;

            event_track::get_meta_segment_bytes(
				in_emsg_list, 
				seg_start, 
				seg_end, 
				opts.track_id_, 
				opts.timescale_, 
				segment_bytes
			);
            
            if (opts.dry_run_) {
                oft.write((const char*)&segment_bytes[0], segment_bytes.size());
                oft.flush();
            }
            else {
                PostCurlIngestConnection::send_curl_post(uri, segment_bytes);
            }
        }
    }
    return 0;
}
