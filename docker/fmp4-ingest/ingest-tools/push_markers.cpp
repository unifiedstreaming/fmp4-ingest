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

#include  <ctime>
#include  <cstring>
#include  <chrono>
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
        : url_("http://localhost/test01/test01.isml")
        , dry_run_(false)
        , avail_interval_(60000)
        , avail_dur_(10000)
        , seg_dur_(2000)
        , timescale_(1000)
        , track_id_(1)
        , announce_(0)
        , send_vtt_(false)
        , send_event_(true)
        , media_string_("")
        , init_string_("")
    {
    }

    static void print_options()
    {
        printf("Usage: push_markers [options] \n");
        printf(
            " [-u url]                       Publishing Point URL without /Streams() extension\n"
            " [--avail]                      signal an advertisment slot every arg1 ms with duration of arg2 ms (default is 60000 and 10000) \n"
            " [--seg_dur]                    segment duration of avail segments in the timed metadata track in ms (default = 2000ms) \n"                 
            " [--track_id]                   Track id to put in the track (default is 1), event track will use track id, vtt track track_id + 1"
            " [--dry_run]                    Do a dry run and write the output files to disk directly for checking file and box integrity default is false\n"
            " [--announce]                   specify the number of milliseconds seconds in advance to presenation time to send an avail (default is 0)"
            " [--vtt]                        send vtt time codes in mp4 segments (default = false)"
            " [--no_event]                   dont send event track MPEG-B part 18 (default = true)"
            " [--initialization]             SegmentTemplate@initialization sets the relative path for init segments, shall include $RepresentationID$ \n"
            " [--media]                      SegmentTemplate@media sets the relative path for media segments, shall include $RepresentationID$ and $Time$ or $Number$ \n"
            "\n");
    }

    void parse_options(int argc, char* argv[])
    {
        if (argc > 1)
        {
            for (int i = 1; i < argc; i++)
            {
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
                if (t.compare("--vtt") == 0) { send_vtt_ = true; continue; }
                if (t.compare("--no_event") == 0) { send_event_ = false; continue; }
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
    bool     send_vtt_;
    bool     send_event_;

    std::string init_string_;
    std::string media_string_;
};

// use curl to push the segment/header using HTTP post over HTTP 1.1.
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
               
                return res;
            }
            catch (...)
            {
                std::cout << "unkown error occured" << std::endl;
                return CURLcode::CURLE_INTERFACE_FAILED;
            }
            return CURLcode::CURLE_OK;
       
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

// wrapper generates splice insert message
event_track::DASHEventMessageBoxv1 generate_event_message_vtt_time(
    uint32_t id,
    uint64_t presentation_time,
    uint64_t duration,
    uint32_t timescale,
    uint32_t off)
{

    event_track::DASHEventMessageBoxv1 ev;
    ev = {};
    ev.id_ = id;
    ev.presentation_time_ = presentation_time;
    ev.event_duration_ = (uint32_t)duration;
    ev.timescale_ = timescale;
    ev.scheme_id_uri_ = "urn:webvtt";

    //static std::string getTimeStamp(time_t epochTime, const char* format = "%Y-%m-%d %H:%M:%S")
    //{
    char timestamp[64] = { 0 };
    const char* format = "%Y-%m-%d %H:%M:%S";
    std::time_t epochTime = (time_t) presentation_time / timescale;
    std::strftime(timestamp, sizeof(timestamp), format, gmtime(&epochTime));
    
    for (int k = 0; k < 64; k++)
        if(timestamp[k] != '\0')
           ev.message_data_.push_back(timestamp[k]);

    return ev;
}

std::string get_path_from_template(
    std::string& template_string,
    std::string& file_name,
    uint64_t time,
    uint64_t number)
{
    std::string out_string;

    const std::string number_str = "$Number$";
    const std::string time_str = "$Time$";
    const std::string rep_str = "$RepresentationID$";

    std::string rep_name;

    if (file_name.size() > 0)
    {
        size_t poss = file_name.find_last_of(".");
        if (poss != std::string::npos)
            rep_name = file_name.substr(0, poss);
    }

    size_t rep_pos = template_string.find_first_of(rep_str);

    if (rep_pos != std::string::npos)
    {
        std::string b, c;

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
        std::string b, c;

        if (time_pos != 0)
            b = out_string.substr(0, time_pos);

        if (time_pos + time_str.size() < out_string.size())
            c = out_string.substr(time_pos + time_str.size());

        out_string = b + std::to_string(time) + c;
    }
    else if (num_pos != std::string::npos)
    {
        std::string b, c;

        if (num_pos != 0)
            b = out_string.substr(0, num_pos);

        if (num_pos + number_str.size() < out_string.size())
            c = out_string.substr(num_pos + number_str.size());

        out_string = b + std::to_string(number) + c;
    }

    return out_string;
}

int main(int argc, char* argv[])
{
    push_marker_options_t opts;
    opts.parse_options(argc, argv);

    if (!opts.send_event_ && !opts.send_vtt_) 
    {
        std::cout << "not configured to send event track or vtt track, use option --vtt when setting option --no_event" << std::endl;
        return 0;
    }
    
    uint64_t last_K = 0;
    std::string uri = opts.url_ + "/Streams(out_meta.cmfm)";
    std::string uri_vtt = opts.url_ + "/Streams(out_vtt.cmft)";
    // ad break and event characteristics
   
    uint64_t last_L = 0;
    uint64_t next_L = 0;

    std::string event_track = "events";
    std::string vtt_track = "vtt_time";

    std::fstream oft(event_track + ".cmfm" , std::ios::binary | std::ios::out);
    std::fstream oft_vtt(vtt_track + ".cmft", std::ios::binary | std::ios::out);

    std::vector<uint8_t> header_bytes;
    std::vector<uint8_t> header_bytes_vtt;

    if(opts.send_event_)
        event_track::get_meta_header_bytes(opts.track_id_, opts.timescale_, header_bytes, false);
    if(opts.send_vtt_)
        event_track::get_meta_header_bytes(opts.track_id_ + 1, opts.timescale_, header_bytes_vtt, true);
    
    if (opts.dry_run_) {
        if(opts.send_event_)
            oft.write((const char*)&header_bytes[0], header_bytes.size());
        if(opts.send_vtt_)
            oft_vtt.write((const char*)&header_bytes_vtt[0], header_bytes_vtt.size());
    }
    else {
        if (opts.send_event_)
            if (opts.init_string_.size() < 2) {
                std::cout << "posting event segment to: " << uri << std::endl;
                if (PostCurlIngestConnection::send_curl_post(uri, header_bytes) != CURLcode::CURLE_OK)
                    std::exit(1);
            }
            else 
            {
                std::string out_meta = "events";
                uri = opts.url_ + "/" + get_path_from_template(opts.init_string_, out_meta, 0, 0);
                std::cout << "posting event segment to: " << uri << std::endl;
                if (PostCurlIngestConnection::send_curl_post(uri, header_bytes) != CURLcode::CURLE_OK)
                    std::exit(1);
            }
        if (opts.send_vtt_)
            if (opts.init_string_.size() < 2) {
                std::cout << "posting 'vttt segment to: " << uri_vtt << std::endl;
                if (PostCurlIngestConnection::send_curl_post(uri_vtt, header_bytes_vtt) != CURLcode::CURLE_OK)
                    std::exit(1);
            }
            else {
               
                std::cout << "posting 'vttt segment to: " << uri_vtt << std::endl;
                uri_vtt = opts.url_ + "/" + get_path_from_template(opts.init_string_, vtt_track, 0, 0);

                if (PostCurlIngestConnection::send_curl_post(uri_vtt, header_bytes_vtt) != CURLcode::CURLE_OK)
                    std::exit(1);
            }
    }
    while (1) {
        auto t = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        //std::time_t t = std::time(0);  // t is an integer type
        uint64_t t_d = (uint64_t)t + opts.announce_;

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
            std::vector<event_track::DASHEventMessageBoxv1> in_vtt_list;
            
            if (opts.send_vtt_) {
                
                for (unsigned int off = 0; off < opts.seg_dur_; off += 100) 
                {
                    auto ev_vtt = generate_event_message_vtt_time(
                        (uint32_t) (next_K * opts.seg_dur_ + off),
                        (uint64_t)next_K * opts.seg_dur_ + off,
                        (uint64_t)100,
                        (uint32_t)1000,
                        off
                    );

                    in_vtt_list.push_back(ev_vtt);
                }
            }
            if (opts.send_event_) {
                if (t_d < ad_end_last && t_d >= ad_start_last - opts.avail_dur_)
                {

                    auto ev = generate_event_message_splice_insert(
                        (uint32_t)last_L,
                        (uint64_t)ad_start_last,
                        (uint64_t)opts.avail_dur_,
                        (uint32_t)opts.timescale_
                    );
                    std::cout << " === ad slot is active == " << std::endl;

                    in_emsg_list.push_back(ev);
                }

                if (t_d < ad_end_next && t_d >= ad_start_next - opts.avail_dur_)
                {
                    std::cout << " == ad slot is active, pushing segment with markers == " << std::endl;

                    auto ev = generate_event_message_splice_insert(
                        (uint32_t)last_L + 1,
                        (uint64_t)ad_start_next,
                        (uint64_t)opts.avail_dur_,
                        opts.timescale_
                    );
                    // ev.print();

                    in_emsg_list.push_back(ev);
                }
            }
            std::vector<uint8_t> segment_bytes;
            std::vector<uint8_t> vtt_bytes;

            if (opts.send_event_) {
                event_track::get_meta_segment_bytes(
                    in_emsg_list,
                    seg_start,
                    seg_end,
                    opts.track_id_,
                    opts.timescale_,
                    segment_bytes,
                    (uint32_t)next_K,
                    false
                );
            }

            if (opts.send_vtt_) {
                event_track::get_meta_segment_bytes(
                    in_vtt_list,
                    seg_start,
                    seg_end,
                    opts.track_id_ + 1,
                    opts.timescale_,
                    vtt_bytes,
                    (uint32_t)next_K,
                    true
                );
            }

            // in case of dry run just write the output files
            if (opts.dry_run_) {
                if (opts.send_event_) {
                    oft.write((const char*)&segment_bytes[0], segment_bytes.size());
                    oft.flush();
                }
                if (opts.send_vtt_) {
                    oft_vtt.write((const char*)&vtt_bytes[0], vtt_bytes.size());
                    oft_vtt.flush();
                }
            }
            else {
                if (opts.send_event_) {
                    
                    std::string uri_h = uri;
                    
                    if (opts.media_string_.size() > 2) 
                    {  
                        uri = opts.url_ + "/" + get_path_from_template(opts.media_string_, event_track, next_K * opts.seg_dur_, next_K);
                        uri_h = opts.url_ + "/" + get_path_from_template(opts.init_string_, event_track, 0, 0);
                    }

                    std::cout << "posting event track segment to " << uri << std::endl;
                    if (PostCurlIngestConnection::send_curl_post(uri, segment_bytes) == CURLcode::CURLE_OK)
                        std::cout << ("posted segment with timed metadata bytes") << std::endl;
                    
                    else {
                        std::cout << ("error posting segment with webvtt bytes, retry logic not implemented yet") << std::endl;

                        std::cout << "posting event track init segment to " << uri_h << std::endl;
                        if (PostCurlIngestConnection::send_curl_post(uri_h, header_bytes) != CURLcode::CURLE_OK) {
                            std::exit(1);
                        }
                        else {
                            std::cout << "posting event track  segment to " << uri << std::endl;
                            if (PostCurlIngestConnection::send_curl_post(uri, segment_bytes) == CURLcode::CURLE_OK)
                                std::cout << "retry logic succeeded, segment retransmitted" << std::endl;
                            else
                                std::exit(1);
                        }
                    }
                }
                if (opts.send_vtt_) {

                    std::string uri_vtt_h = uri_vtt;

                    if (opts.media_string_.size() > 0)
                    {
                        uri_vtt = opts.url_ + "/" + get_path_from_template(opts.media_string_, vtt_track, next_K * opts.seg_dur_, next_K);
                        uri_vtt_h = opts.url_ + "/" + get_path_from_template(opts.init_string_, vtt_track, 0, 0);
                    }
                    std::cout << "posting vtt track  segment to " << uri_vtt << std::endl;
                    if (PostCurlIngestConnection::send_curl_post(uri_vtt, vtt_bytes) == CURLcode::CURLE_OK)
                        std::cout << ("posted segment with vtt bytes") << std::endl;
                    else {
                        std::cout << ("error posting segment with webvtt bytes, retry logic not implemented yet") << std::endl;
                
                        std::cout << "posting vtt track  init segment to " << uri_vtt_h << std::endl;
                        if (PostCurlIngestConnection::send_curl_post(uri_vtt_h, header_bytes_vtt) != CURLcode::CURLE_OK) {
                            std::exit(1);
                        }
                        else {
                            std::cout << "posting vtt track  segment to " << uri_vtt << std::endl;
                            if (PostCurlIngestConnection::send_curl_post(uri_vtt, vtt_bytes) == CURLcode::CURLE_OK) {
                                std::cout << "retry logic succeeded, vtt segment retransmitted" << std::endl;
                            }
                            else {
                                std::exit(1);
                            }
                        }
                    }
                }
            }
        }
    }
    std::exit(1);
}
