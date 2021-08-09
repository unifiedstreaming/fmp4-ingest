#include <vector>
#include <algorithm>   
#include <string>

// used only for testing and generating random events
#include <random>

// sample boundaries (create sample boundaries due to change in active event and segment boundary)
// de-duplication (convert back to event list) 
// pre-announce  
// example to write to fragmented mp4 file

struct DASHEventMessageBoxv1 
{
	// version omitted
	// flags ommited
	uint32_t		timescale;
	uint64_t		presentation_time;
	uint32_t		event_duration;
	uint32_t		id;
	std::string 	scheme_id_uri;
	std::string		value;
	std::vector<uint8_t>   message_data;
};

struct DASHEventMessageInstanceBox
{
	int64_t			presentation_time_delta;
	uint32_t		event_duration;
	uint32_t		id;
	std::string 	scheme_id_uri;
    std::string		value;
	std::vector<uint8_t>	message_data;

	DASHEventMessageInstanceBox(DASHEventMessageBoxv1 e, uint64_t pres_time)
		: event_duration(e.event_duration), \
		id(e.id), scheme_id_uri(e.scheme_id_uri), \
		value(e.value), message_data(e.message_data)
	{
		presentation_time_delta = e.presentation_time - pres_time;
	}
};

struct EventSample 
{
	uint64_t sample_presentation_time;
	uint32_t sample_duration;
	std::vector<DASHEventMessageInstanceBox> instance_boxes;
	bool is_emeb;
};


// algorithm to find samples 
// in a segment from segment_start to segment_end
// if both are zero cover entire range
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
	
	return (uint32_t) sample_boundaries.size();
};

// find the event message track samples and append them to samples_out
uint32_t find_event_samples(
	    const std::vector<DASHEventMessageBoxv1> &emsgs_in,
	    std::vector<EventSample> &samples_out,
		uint64_t segment_start = 0,
		uint64_t segment_end = 0) 
    {
	
	std::vector<uint64_t> sample_boundaries = std::vector<uint64_t>();
	find_sample_boundaries(emsgs_in, sample_boundaries, segment_start, segment_end);

	for (int i = 0; i < (int) sample_boundaries.size() - 1; i++)
	{
		EventSample s;
		s.sample_presentation_time = sample_boundaries[i];
		s.sample_duration = sample_boundaries[i + 1] - sample_boundaries[i];
		s.is_emeb = true;

		for (unsigned int k = 0; k < emsgs_in.size(); k++) 
		{
			uint64_t pt = emsgs_in[k].presentation_time;
			uint64_t pt_du = emsgs_in[k].presentation_time + emsgs_in[k].event_duration;

			// active 
			if (pt < sample_boundaries[i + 1] && pt_du >= sample_boundaries[i]) 
			{
				s.instance_boxes.push_back(DASHEventMessageInstanceBox(emsgs_in[k], s.sample_presentation_time));
				s.is_emeb = false;
			}
					
		}
		samples_out.push_back(s);
	}

    return (uint32_t)samples_out.size();
};

DASHEventMessageBoxv1 generate_random_event()
{
	std::default_random_engine generator;
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist100(1, 200); // distribution in range [1, 6]
	std::uniform_int_distribution<std::mt19937::result_type> dist40(1, 20); // distribution in range [1, 6] 

	DASHEventMessageBoxv1 e; 
	e.event_duration = dist40(rng);
	e.presentation_time = dist100(rng);
	e.scheme_id_uri = "random-1-100";
	e.value = "";
	e.timescale = 1;
	e.message_data.resize(2);
	e.message_data[0] = (uint8_t)e.presentation_time;
	e.message_data[1] = (uint8_t)e.event_duration;
    
	return e;
}

int main()
{
	std::vector<DASHEventMessageBoxv1> emsgs_in;

	for (int i = 0; i < 30; i++) 
	{
		DASHEventMessageBoxv1 e = generate_random_event();
		e.id = (unsigned int) i;
		emsgs_in.push_back(e);
	}

	std::vector<EventSample> samples;
	find_event_samples(emsgs_in, samples, 0, 200);

	return 0;
}
