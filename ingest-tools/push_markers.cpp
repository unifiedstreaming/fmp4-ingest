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
#include <iostream>
#include <fstream>

// puslishing point url
// avail_duration

struct PostCurlIngestConnection 
{
    CURL* curl;

    CURLcode send_curl_post(std::string &post_url, std::vector<uint8_t> &data) {
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
            std::fprintf(stderr, "---- connection with server sucessfull %s\n",
                curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            return res;
        }
        catch (...) 
        {
            std::cout << "unkown error occured" << std::endl;
        }
    }
};

event_track::DASHEventMessageBoxv1 generate_event_message(
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
    fmp4_stream::gen_splice_insert(ev.message_data_, ev.id_, ev.event_duration_ * 90);
    return ev;
}

int main()
{
    // CMAF metadata track characteristics 
    uint32_t seg_timescale = 1000;
    double seg_duration = 1.92;
    uint32_t track_id = 77;
    double last_K = 0.0;

    // ad break and event characteristics
    double ad_break_duration = 10;
    double avail_interval = 60;
    double last_L = 0;
    double next_L = 0;

    std::ofstream ot("out_track.cmfm", std::ios::binary);
    event_track::write_event_track_cmaf_header(track_id, seg_timescale, ot);
    std::cout << "wrote the cmaf header of the metadata track" << std::endl;

    while (1) {

        std::time_t t = std::time(0);  // t is an integer type
        double t_d = (double)t;
        double next_K = std::ceil(t_d / seg_duration); // calculate the next segment boundary

        last_L = std::floor(t_d / avail_interval);
        
        // the previious/current ad break 
        double ad_start_last = last_L * avail_interval;
        double ad_end_last = last_L * avail_interval + ad_break_duration;

        // the next ad break
        double ad_start_next = (last_L + 1) * avail_interval;
        double ad_end_next = (last_L + 1) * avail_interval + ad_break_duration;

        if (next_K != last_K) {
            uint64_t seg_start = (uint64_t) (seg_timescale * next_K * seg_duration);
            uint64_t seg_end = (uint64_t) (seg_timescale * seg_duration + seg_start);

            std::cout << t << " seconds since 01-Jan-1970\n";
            std::cout << "next_K is: " << next_K << std::endl;
            std::cout << "next segment boundary is after: " << next_K * seg_duration - t_d << " seconds" << std::endl;
            std::cout << "next tfdt is at " << std::endl;
            last_K = next_K;
            std::vector<event_track::DASHEventMessageBoxv1> in_emsg_list;

            if (t_d < ad_end_last && t_d >= ad_start_last - ad_break_duration)
            {
                std::cout << " == ad slot is active == " << std::endl;

                auto ev = generate_event_message(
                    (uint32_t)last_L,
                    (uint64_t)ad_start_last * seg_timescale,
                    (uint64_t)ad_break_duration * seg_timescale,
                    seg_timescale
                );
                ev.print();
                
                in_emsg_list.push_back(ev);
            }
            if (t_d < ad_end_next && t_d >= ad_start_next - ad_break_duration)
            {
                std::cout << " == ad slot is active == " << std::endl;

                auto ev = generate_event_message(
                    (uint32_t)last_L + 1,
                    (uint64_t)ad_start_next * seg_timescale,
                    (uint64_t)ad_break_duration * seg_timescale,
                    seg_timescale
                );
                ev.print();

                in_emsg_list.push_back(ev);
            }
            
            std::vector<event_track::EventSample> samples_in_segment;
                
            find_event_samples(
                    in_emsg_list,
                    samples_in_segment,
                    seg_start,
                    seg_end
            );
            
            event_track::write_evt_samples_as_fmp4_fragment(
                    samples_in_segment,
                    ot,
                    seg_start,
                    track_id,
                    seg_end);
          
       
        }
    }
    return 0;
}