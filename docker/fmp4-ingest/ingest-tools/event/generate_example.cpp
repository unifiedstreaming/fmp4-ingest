/*
    C++ Function to generate examples of event message track samples 	
	for ISO/IEC 23001-18 clause 9
	
    All Rights Reserved CodeShop B.V. 2021 - 
*/

#include "event_track.h"
#include <iostream>

using namespace event_track;

int main()
{
	std::vector<DASHEventMessageBoxv1> emsgs_in;

	for (int i = 0; i < 10; i++) 
	{
		DASHEventMessageBoxv1 e = generate_random_event();
		
		if (i % 5 == 0) 
			e = generate_random_event(true);

		//if (i % 19 == 0) // introduce indefinete duration event
		//	e.event_duration = 0xFFFFFFFF;

		e.id_ = (unsigned int) i;
		emsgs_in.push_back(e);
	}

	std::vector<EventSample> samples;
	find_event_samples(emsgs_in, samples, 0, 150);

	// sort the events by presentation time
	std::sort(emsgs_in.begin(), emsgs_in.end(),
		[](const DASHEventMessageBoxv1 & a, const DASHEventMessageBoxv1 & b) -> bool
	{
		return a.presentation_time_ < b.presentation_time_;
	});


	for (unsigned int i = 0; i < emsgs_in.size(); i++)
	{
		std::cout << " Input DASH Event: id="    << emsgs_in[i].id_;
		std::cout << " presentation_time=" << emsgs_in[i].presentation_time_;
		std::cout << " event_duration="    << emsgs_in[i].event_duration_;
		std::cout << std::endl;
	}

	std::cout << std::endl;
	std::cout << std::endl;

	for (unsigned int i = 0; i < samples.size(); i++)
	{
		std::cout << " Event Message Track Sample "<< i + 1 <<":";
		std::cout << " sample_presentation_time=" << samples[i].sample_presentation_time_;
		std::cout << " sample_duration=" << samples[i].sample_duration_;
		std::cout << " sample_data=";

		if (samples[i].instance_boxes_.size() == 0)
			std::cout << std::endl << " EventMessageEmtpyBox";
		else {
			//std::cout << std::endl;
			for (unsigned int k = 0; k < samples[i].instance_boxes_.size(); k++)
			{
				EventMessageInstanceBox b = samples[i].instance_boxes_[k];
				std::cout << std::endl << "  EventMessageInstanceBox(" << "id=" << b.id_ << ",presentation_time_delta=" << b.presentation_time_delta_ << ",event_duration=" << b.event_duration_ << ") ";
			}
		}
		std::cout << std::endl;
	}

	return 0;
}