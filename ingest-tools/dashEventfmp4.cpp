/*******************************************************************************
Supplementary software media ingest specification:
https://github.com/unifiedstreaming/fmp4-ingest

Copyright (C) 2009-2018 CodeShop B.V.
http://www.code-shop.com

******************************************************************************/

#include "fmp4stream.h"
#include "base64.h"
#include "tinyxml2.h"
#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>

using namespace fMP4Stream;
using namespace std;
extern string moov_64_enc;

// struct to hold mpd events, encapsulate event and eventstream information
struct event_t 
{
	int64_t presentation_time_;   // presentation time of event
	unsigned int duration_;       // duration of event
	bool base64_;                 // codes if the payload is base64 coded
	string message_data_;         // message data (attribute or value)
	string scheme_id_uri_;        // scheme if uri
	string value_;                // string of the subscheme
	uint32_t id_;                      // id value of the event
	uint32_t time_scale_;              // timescale, inherited from eventstream
	int64_t pto_;                 //  pto in eventstream

	event_t()
		: presentation_time_(0), duration_(0), base64_(false), message_data_(), scheme_id_uri_(), value_(), id_(0) , time_scale_(1)
	{
	};

	bool to_emsg(emsg &em) 
	{
		em.scheme_id_uri_ = scheme_id_uri_;
		em.value_ = value_;
		em.timescale_ = time_scale_;
		em.presentation_time_delta_=0;
		em.presentation_time_=presentation_time_;
		em.event_duration_= duration_;
		em.id_ = id_;
		em.version_ = 1;

		// copy the message data
		if (!base64_) 
		{
			em.message_data_.resize(message_data_.size());
			for (unsigned int k = 0; k < message_data_.size(); k++)
				em.message_data_[k] = (uint8_t) message_data_[k];
		}
		else
		{
			em.message_data_ = base64_decode(message_data_);
		}

		return true;
	}
};

// struct to parse the event streams
struct event_parser_t : public tinyxml2::XMLVisitor
{
	virtual bool VisitEnter(const tinyxml2::XMLElement &el, const tinyxml2::XMLAttribute *at) 
	{
		string el_name = el.Value();

		if ((el_name.compare("dash:EventStream") == 0) || (el_name.compare("EventStream") == 0))
			cout << "eventstream found" << endl;
		else
			return true;
		
		event_t l_root_event;

		// parse attributes for the eventstream
		if (el.QueryUnsignedAttribute ("timescale", &l_root_event.time_scale_) != tinyxml2::XMLError::XML_SUCCESS)
		{
			l_root_event.time_scale_ = 1; // no timescale defined
		}

		l_root_event.scheme_id_uri_.resize(50000);
		const char *ids[1] = {};
		ids[0] = l_root_event.scheme_id_uri_.c_str();

		if (el.QueryStringAttribute("schemeIdUri", ids) != tinyxml2::XMLError::XML_SUCCESS)
		{
			cout << "error parsing schemeIdUri, it is mandatory for each eventStream to have a URI" << endl;
		}
		else
		{
			l_root_event.scheme_id_uri_ = string(ids[0]);
			cout << l_root_event.scheme_id_uri_ << endl;
		}
		if (el.QueryStringAttribute("value", ids) == tinyxml2::XMLError::XML_SUCCESS)
		{
			l_root_event.value_ = string(ids[0]);
			cout << l_root_event.value_ << endl;
		}
		if (el.QueryInt64Attribute("presentationTimeOffset", &l_root_event.pto_) != tinyxml2::XMLError::XML_SUCCESS)
		{
			l_root_event.pto_ = 0;
		}

		// parse the children ach of 
		auto child = el.FirstChildElement();
		
		while (child != nullptr) 
		{
			event_t l_new_event = l_root_event; // copy information from the eventstre,a;

			char messageData[500000] = {};
			char value_c[100] = {};
			const char* md[1] = { messageData };
			const char* val[1] = { value_c };
			bool attr_md = false;

			// query the event attributes
			if (child->QueryInt64Attribute("presentationTime", &l_new_event.presentation_time_) != tinyxml2::XML_SUCCESS)
			{
				l_new_event.presentation_time_ = 0;
			}
			if (child->QueryUnsignedAttribute("duration", &l_new_event.duration_) != tinyxml2::XML_SUCCESS)
			{
				l_new_event.duration_ = 0;
			}
			if (child->QueryStringAttribute("value", val) == tinyxml2::XML_SUCCESS)
			{
				l_new_event.value_ = string(val[0]);
			}
			if (child->QueryUnsignedAttribute("id", &l_new_event.id_) != tinyxml2::XML_SUCCESS)
			{
				l_new_event.id_ = 0; // unknown
			}
			if (child->QueryStringAttribute("messageData", md) == tinyxml2::XML_SUCCESS)
			{
				l_new_event.message_data_ = string(md[0]);
				attr_md = true;
			}
			if (!attr_md)
			{
				if(child->GetText() != nullptr)
				    l_new_event.message_data_ = string(child->GetText());
				else if (l_new_event.scheme_id_uri_.compare("urn:scte:scte35:2014:xml+bin") == 0)
				{
					auto bin_dat = child->FirstChildElement()->FirstChildElement()->GetText();
					if (bin_dat != nullptr) {
						l_new_event.message_data_ = string(bin_dat);
						l_new_event.base64_ = true;
						l_new_event.scheme_id_uri_ = "urn:scte:scte35:2013:bin"; // use the 2013 binary scheme with base64 encoding
					}
				}
				else 
					l_new_event.message_data_ = string();
			}
			events_.push_back(l_new_event);
			child = child->NextSiblingElement();
		}
		return true;
	}
    
	vector<event_t> events_;
};

// program for convering dash event to fmp4 metatrack
int main(int argc, char *argv[])
{
	string out_file = "out_sparse_fmp4.cmfm";
	string scheme_prefix;

	if (argc > 2)
		out_file = string(argv[2]);

	if (argc > 1)
	{
		string in_file_name(argv[1]);
		tinyxml2::XMLDocument doc;
		doc.LoadFile(in_file_name.c_str());

		event_parser_t evt;
		doc.Accept(&evt);

		cout << "done printing events: " << evt.events_.size()<< endl;

		ingest_stream l_ingest_stream;
		
		for (int i = 0; i < evt.events_.size(); i++)
		{
			media_fragment m;
			evt.events_[i].to_emsg(m.emsg_);
			m.tfdt_.base_media_decode_time_ = evt.events_[i].presentation_time_;
			l_ingest_stream.media_fragment_.push_back(m);
		}

		string event_urn = "urn:mpeg:dash:event";
		l_ingest_stream.write_to_sparse_emsg_file(out_file, 1, 0, event_urn);

		return 1;
	}
	else
	{
		cout << "usage: fmp4meta infile(dash event xml) outfile(sparse_meta_emsg, cmfc)" << endl;
	}
}