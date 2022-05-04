/*
        C++ Function to calculate sample boundaries and content 
	of the event message track ISO/IEC 23001-18 based on clause 9.2
	
	Input: a list of dash event messages, segment start and end times 
	Output: a list of event message track samples with event message instance boxes (emib) 
	
        All Rights Reserved CodeShop B.V. 2021 - 
*/

#ifndef find_event_samples_HPP
#define find_event_samples_HPP

#include <vector>
#include <algorithm>   
#include <string>

// used only for testing and generating random events
#include <random>

namespace event_track {
	
	//! struct to store a DASHEventMessageBoxv1
	struct DASHEventMessageBoxv1
	{
		// version omitted
		// flags ommited
		uint32_t		timescale;
		uint64_t		presentation_time;
		uint32_t		event_duration;
		uint32_t		id;
		std::string 	        scheme_id_uri;
		std::string		value;
		std::vector<uint8_t>    message_data;
	};
    
	//! struct to store a EventMessageInstanceBox 
	struct EventMessageInstanceBox
	{
		int64_t			presentation_time_delta;
		uint32_t		event_duration;
		uint32_t		id;
		std::string 	        scheme_id_uri;
		std::string		value;
		std::vector<uint8_t>	message_data;

		EventMessageInstanceBox(DASHEventMessageBoxv1 e, uint64_t pres_time)
			: event_duration(e.event_duration), \
			id(e.id), scheme_id_uri(e.scheme_id_uri), \
			value(e.value), message_data(e.message_data)
		{
			presentation_time_delta = e.presentation_time - pres_time;
		}
	};
    
	//! struct to store sample (meta-data)
	struct EventSample
	{
		uint64_t sample_presentation_time;
		uint32_t sample_duration;
		std::vector<EventMessageInstanceBox> instance_boxes;
		bool is_emeb;
	};


	//! algorithm to find all sample boundaries between segment_start and segment_end (0=infinite)
	uint32_t find_sample_boundaries(
		const std::vector<DASHEventMessageBoxv1> &emsgs_in,
		std::vector<uint64_t> &sample_boundaries,
		uint64_t segment_start = 0,
		uint64_t segment_end = 0)
	{

		sample_boundaries = std::vector<uint64_t>();
		sample_boundaries.push_back(segment_start);

		if (segment_end != 0)
			sample_boundaries.push_back(segment_end);

		for (uint32_t k = 0; k < emsgs_in.size(); k++)
		{
			uint64_t pt = emsgs_in[k].presentation_time;
			uint64_t pt_du = emsgs_in[k].presentation_time + emsgs_in[k].event_duration;

			if (emsgs_in[k].event_duration == 0)
				pt_du += 1;  // set the sample duration to a single tick

			// boundaries due to events starting
			if (pt >= segment_start)  // event starts  
				if (segment_end == 0 || pt < segment_end)
					sample_boundaries.push_back(pt);

			// boundaries due to event ending
			if (segment_end != 0)
				if (pt_du >= segment_start && pt_du < segment_end)
					sample_boundaries.push_back(pt_du);

			if (segment_end == 0)
				sample_boundaries.push_back(pt_du);
		}

		if (segment_end != 0)
			sample_boundaries.push_back(segment_end);

		sort(sample_boundaries.begin(), sample_boundaries.end());
		sample_boundaries.erase(std::unique(sample_boundaries.begin(),
			sample_boundaries.end()), sample_boundaries.end());

		if (segment_end == 0)
			sample_boundaries.push_back(sample_boundaries[sample_boundaries.size() - 1] + 1);

		return (uint32_t)sample_boundaries.size();
	};

	//! algorithm to find all event message track samples between segment_start and segment_end (0=infinite)
	uint32_t find_event_samples(
		const std::vector<DASHEventMessageBoxv1> &emsgs_in,
		std::vector<EventSample> &samples_out,
		uint64_t segment_start = 0,
		uint64_t segment_end = 0)
	{

		std::vector<uint64_t> sample_boundaries = std::vector<uint64_t>();
		find_sample_boundaries(emsgs_in, sample_boundaries, segment_start, segment_end);

		for (int i = 0; i < (int)sample_boundaries.size() - 1; i++)
		{
			EventSample s;
			s.sample_presentation_time = sample_boundaries[i];
			s.sample_duration = (int32_t)((int64_t) sample_boundaries[i + 1] - (int64_t) sample_boundaries[i]);
			s.is_emeb = true;

			for (unsigned int k = 0; k < emsgs_in.size(); k++)
			{
				uint64_t pt = emsgs_in[k].presentation_time;
				uint64_t pt_du = emsgs_in[k].presentation_time + emsgs_in[k].event_duration;

				if (emsgs_in[k].event_duration == 0)
					pt_du += 1;

				// event is active during sample duration 
				if (pt < sample_boundaries[i + 1] && pt_du > sample_boundaries[i])
				{
					s.instance_boxes.push_back(EventMessageInstanceBox(emsgs_in[k], s.sample_presentation_time));
					s.is_emeb = false;
				}
			}
			samples_out.push_back(s);
		}

		return (uint32_t)samples_out.size();
	};

	// helper function generate random events for testing and example generation
	// first arg set duration to zero, second and third distribution of presentation time and duration
	DASHEventMessageBoxv1 generate_random_event(bool set_duration_to_zero=false, uint32_t max_p = 150, uint32_t max_d = 20)
	{
		std::default_random_engine generator;
		std::random_device dev;
		std::mt19937 rng(dev());

		std::uniform_int_distribution<std::mt19937::result_type> distP(1, max_p);
		std::uniform_int_distribution<std::mt19937::result_type> distD(1, max_d);

		DASHEventMessageBoxv1 e;

		if(!set_duration_to_zero)
		    e.event_duration = distD(rng);
		else
		    e.event_duration = 0;

		e.presentation_time = distP(rng);
		e.scheme_id_uri = "random-1-100";
		e.value = "";
		e.timescale = 1;
		e.message_data.resize(2);
		e.message_data[0] = (uint8_t)e.presentation_time;
		e.message_data[1] = (uint8_t)e.event_duration;

		return e;
	}
}
#endif
