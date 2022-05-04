/*******************************************************************************

Supplementary software media ingest specification:
https://github.com/unifiedstreaming/fmp4-ingest

Copyright (C) 2009-2021 CodeShop B.V.
http://www.code-shop.com

convert DASH Events in an XML file to a timed metadata track

******************************************************************************/

#include "fmp4stream.h"
#include "event_track.h"
#include "base64.h"
#include "tinyxml2.h"
#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <algorithm>

using namespace fmp4_stream;
using namespace event_track;
extern std::string moov_64_enc;
/*
// struct to hold mpd events, encapsulate event and eventstream information
struct event_t
{
	uint64_t presentation_time_;   // presentation time of event
	uint32_t duration_;           // duration of event
	bool base64_;                 // codes if the payload is base64 coded
	std::string message_data_;    // message data (attribute or value)
	std::string scheme_id_uri_;   // scheme if uri
	std::string value_;           // string of the subscheme
	uint32_t id_;                 // id value of the event
	uint32_t time_scale_;         // timescale, inherited from eventstream
	int64_t pto_;                 //  pto in eventstream

	event_t()
		: presentation_time_(0), duration_(0), base64_(false), message_data_(), scheme_id_uri_(), value_(), id_(0), time_scale_(1)
	{
	};

	bool to_emsg(emsg &em)
	{
		em.scheme_id_uri_ = scheme_id_uri_;
		em.value_ = value_;
		em.timescale_ = time_scale_;
		em.presentation_time_delta_ = 0;
		em.presentation_time_ = presentation_time_;
		em.event_duration_ = duration_;
		em.id_ = id_;
		em.version_ = 1;

		// copy the message data
		if (!base64_)
		{
			em.message_data_.resize(message_data_.size());
			for (unsigned int k = 0; k < message_data_.size(); k++)
				em.message_data_[k] = (uint8_t)message_data_[k];
		}
		else
		{
			em.message_data_ = base64_decode(message_data_);
		}

		return true;
	}
};*/

struct event_parser_t : public tinyxml2::XMLVisitor
{
	virtual bool VisitEnter(const tinyxml2::XMLElement &el, const tinyxml2::XMLAttribute *at)
	{
		
		std::string el_name = el.Value();

		if ((el_name.compare("dash:EventStream") == 0) || (el_name.compare("EventStream") == 0))
			std::cout << "*** eventstream found ***" << std::endl;
		   // query the event attributes
		else
			return true;

		event_track::DASHEventMessageBoxv1 l_root_event;

		// parse attributes for the eventstream
		if (el.QueryUnsignedAttribute("timescale", &l_root_event.timescale_) != tinyxml2::XMLError::XML_SUCCESS)
		{
			l_root_event.timescale_ = 1; // no timescale defined use default
		}

		l_root_event.scheme_id_uri_.resize(50000);
		const char *ids[1] = {};
		ids[0] = l_root_event.scheme_id_uri_.c_str();

		if (el.QueryStringAttribute("schemeIdUri", ids) != tinyxml2::XMLError::XML_SUCCESS)
		{
			std::cout << "error parsing schemeIdUri, it is mandatory for each eventStream to have a URI" << std::endl;
			return false;
		}
		else
		{
			l_root_event.scheme_id_uri_ = std::string(ids[0]);
			//std::cout << l_root_event.scheme_id_uri_ << std::endl;
		}
		if (el.QueryStringAttribute("value", ids) == tinyxml2::XMLError::XML_SUCCESS)
		{
			l_root_event.value_ = std::string(ids[0]);
			//std::cout << l_root_event.value_ << std::endl;
		}
		if (el.QueryInt64Attribute("presentationTimeOffset", &pto_) == tinyxml2::XMLError::XML_SUCCESS)
		{
			pt_start_time_ = (uint64_t) pto_;
		}
		else 
		{
			pt_start_time_ = 0;
		}
		int64_t pt_end=0;
		if (el.QueryInt64Attribute("endTime", &pt_end) == tinyxml2::XMLError::XML_SUCCESS)
		{
			this->pt_end_time_ = (uint64_t) pt_end;
		}
		else 
		{
			this->pt_end_time_ = 0;
		}

		// parse the children of the EventStream
		auto child = el.FirstChildElement();

		while (child != nullptr)
		{
			event_track::DASHEventMessageBoxv1 l_new_event = l_root_event; // copy information from the eventstream

			char messageData[50000] = {}; // only up to 50k message data supported in mpd events
			char value_c[100] = {};
			const char* md[1] = { messageData };
			const char* val[1] = { value_c };
			bool attr_md = false;

			// query the event attributes
			if (child->QueryUnsigned64Attribute("presentationTime", &l_new_event.presentation_time_) != tinyxml2::XML_SUCCESS)
				l_new_event.presentation_time_ = 0;
			if (child->QueryUnsignedAttribute("duration", &l_new_event.event_duration_) != tinyxml2::XML_SUCCESS)
				l_new_event.event_duration_ = 0;
			if (child->QueryStringAttribute("value", val) == tinyxml2::XML_SUCCESS)
				l_new_event.value_ = std::string(val[0]);
			if (child->QueryUnsignedAttribute("id", &l_new_event.id_) != tinyxml2::XML_SUCCESS)
				l_new_event.id_ = 0; // unknown
			if (child->QueryStringAttribute("messageData", md) == tinyxml2::XML_SUCCESS)
			{
				std::string data = std::string(md[0]);
				if (child->QueryStringAttribute("contentEncoding", md) == tinyxml2::XML_SUCCESS)
				{
					std::string contentEncoding(md[0]);
					if (contentEncoding.compare("Base64") == 0)
						l_new_event.message_data_ = base64_decode(data);
					else if (contentEncoding.compare("base64") == 0)
						l_new_event.message_data_ = base64_decode(data);
					else
						for (int i = 0; i < data.size(); i++)
							l_new_event.message_data_.push_back(data[i]);
				}
			}		
			else 
			{
				if (child->GetText() != nullptr) {
					std::string data = std::string(child->GetText());
					if (child->QueryStringAttribute("contentEncoding", md) == tinyxml2::XML_SUCCESS)
					{
						std::string contentEncoding(md[0]);
						if (contentEncoding.compare("Base64") == 0)
							l_new_event.message_data_ = base64_decode(data);
						else if (contentEncoding.compare("base64") == 0)
							l_new_event.message_data_ = base64_decode(data);
						else
							for (int i = 0; i < data.size(); i++)
								l_new_event.message_data_.push_back(data[i]);
					}
				}
				else if (l_new_event.scheme_id_uri_.compare("urn:scte:scte35:2014:xml+bin") == 0)
				{
					std::string data = std::string(child->FirstChildElement()->FirstChildElement()->GetText());
					if (data.size()!=0) 
					{
						l_new_event.message_data_ = base64_decode(data);
						l_new_event.scheme_id_uri_ = "urn:scte:scte35:2013:bin"; // use the 2013 binary scheme with base64 encoding
					}
				}
				else
				{
					std::cout << "warning could not interpet event payload, \
						xml payload not supported except for scte-214" << std::endl;
					l_new_event.message_data_ = std::vector<uint8_t>(0);
				}
			}

			events_.push_back(l_new_event);
			child = child->NextSiblingElement();
		}
		return true;
	}

	uint64_t pt_start_time_;
	uint64_t pt_end_time_;
	int64_t  pto_;
	uint32_t timescale_;

	std::vector<event_track::DASHEventMessageBoxv1> events_;
};

int main(int argc, char *argv[])
{
	std::string out_file = "out_sparse_event_track.cmfm";
	uint32_t seg_duration_ticks = 0; // segmentation duration 0 = entire track
	uint32_t track_id = 99; // default track_id

	if (argc > 4)
		seg_duration_ticks = atoi(argv[4]);
	if (argc > 3)
		track_id = atoi(argv[3]);
	if (argc > 2)
		out_file = std::string(argv[2]);
	if (argc > 1)
	{
		std::string in_file_name(argv[1]);

		tinyxml2::XMLDocument doc(true, tinyxml2::Whitespace::COLLAPSE_WHITESPACE);
		doc.LoadFile(in_file_name.c_str());

		event_parser_t evt;
		evt.pt_end_time_ = 0;
		evt.pt_start_time_ = 0;
		doc.Accept(&evt);

		if (evt.events_.size() < 1)
		{
			std::cout << "no events found in the manifest, no track written" << std::endl;
			return 0;
		}

		evt.timescale_ = evt.events_[0].timescale_;
		for (int i = 0; i < evt.events_.size(); i++)
		{
			if (evt.timescale_ != evt.events_[i].timescale_)
			{
				std::cout << "only events with same timescale are supported, no output track written" << std::endl;
				return 0;
			}
		}
		
		// if end time is not set, use the latest event expiration as the end time.
		if (evt.pt_end_time_ == 0) 
		{
			for (unsigned int i = 0; i < evt.events_.size(); i++) 
			{
				if (evt.events_[i].presentation_time_ + evt.events_[i].event_duration_ > evt.pt_end_time_)
					evt.pt_end_time_ = evt.events_[i].presentation_time_ + evt.events_[i].event_duration_;
			}
		}

		event_track::write_to_segmented_event_track_file(
			     out_file, evt.events_,
			     track_id, evt.pt_start_time_,  
			     evt.pt_end_time_, "test_urn", 
			     seg_duration_ticks,evt.timescale_);

		return 0;
	}
	else
	{
		std::cout << " Tool for converting XML Dash Events to an event message track " << std::endl;
		std::cout << " Events are first represented as DashEventMessageBoxes " << std::endl;
		std::cout << " DashEventMessageBoxes are then stored in mdat in samples " << std::endl;
		std::cout << " These samples place the events on the media timeline " << std::endl;
		std::cout << " Only tracks with a common timescale are supported    " << std::endl;
		std::cout << " Format is under consideration for standardisation in MPEG as event message track " << std::endl;
		std::cout << std::endl;
		std::cout << std::endl;
		std::cout << " Usage: dashEventfmp4 infile(dash/smil event xml) outfile(fmp4 emsg_track) [track_id(id)] [segment duration]" << std::endl;
	}
}
