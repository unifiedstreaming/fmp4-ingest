#ifndef EVENT_TRACK_H
#define EVENT_TRACK_H

#include "fmp4stream.h"
#include "base64.h"
#include <vector>
#include <algorithm>   
#include <string>
#include <set>
#include <map>

// code from https://github.com/RufaelDev/EventMessageTrackSamples 
// used only for testing and generating random events
#include <random>

namespace event_track {

	// empty message cue
	const uint8_t embe[8] = {
		0x00, 0x00, 0x00, 0x08, 'e', 'm', 'b', 'e'
	};

	// empty message cue
	const uint8_t emeb[8] = {
		0x00, 0x00, 0x00, 0x08, 'e', 'm', 'e', 'b'
	};

	//! struct to store a DASHEventMessageBoxv1
	struct DASHEventMessageBoxv1 : fmp4_stream::full_box
	{
		// version omitted
		// flags   ommited
		uint32_t		timescale_;
		uint64_t		presentation_time_;
		uint32_t		event_duration_;
		uint32_t		id_;
		std::string 	scheme_id_uri_;
		std::string		value_;
		std::vector<uint8_t>    message_data_;

		uint32_t parse(const char *ptr, unsigned int data_size, uint64_t presentation_time = 0)
		{
			
			if (data_size > 8)
			{
				if ((uint8_t)*(ptr + 8) == 0x01) {
					std::cout << "emsg v1" << std::endl;

					fmp4_stream::full_box::parse(ptr);
					uint64_t offset = full_box::size();

					timescale_ = fmp4_read_uint32(ptr + offset);
					//cout << "timescale: " << timescale << endl;
					offset += 4;
					presentation_time_ = fmp4_read_uint64(ptr + offset);
					//cout << "presentation time: " << presentation_time << endl;
					offset += 8;
					event_duration_ = fmp4_read_uint32(ptr + offset);
					//cout << "event duration: " << event_duration << endl;
					offset += 4;
					id_ = fmp4_read_uint32(ptr + offset);
					offset += 4;
					//cout << "id: " << id << endl;
					scheme_id_uri_ = std::string(ptr + offset);
					offset = offset + scheme_id_uri_.size() + 1;
					//cout << "scheme_id_uri: " << scheme_id_uri << endl;
					value_ = std::string(ptr + offset);
					//cout << "value: " << value << endl;
					offset = offset + value_.size() + 1;

					for (unsigned int i = (unsigned int)offset; i < size_; i++)
						message_data_.push_back(*(ptr + (size_t)i));
				}
				else 
				{
					fmp4_stream::full_box::parse(ptr);
					uint64_t offset = full_box::size();
                    
					scheme_id_uri_ = std::string(ptr + offset);
					offset = offset + scheme_id_uri_.size() + 1;
					value_ = std::string(ptr + offset);
					offset = offset + value_.size() + 1;
					timescale_ = fmp4_read_uint32(ptr + offset);
					//cout << "timescale: " << timescale << endl;
					offset += 4;
					presentation_time_ = fmp4_read_uint32(ptr + offset) + presentation_time;
					//cout << "presentation time: " << presentation_time << endl;
					offset += 4;
					event_duration_ = fmp4_read_uint32(ptr + offset);
					//cout << "event duration: " << event_duration << endl;
					offset += 4;
					id_ = fmp4_read_uint32(ptr + offset);
					offset += 4;
					//cout << "id: " << id << endl;

					for (unsigned int i = (unsigned int)offset; i < size_; i++)
						message_data_.push_back(*(ptr + (size_t)i));
				}
				return size_;
			}
			return 0;
		}
		
		void print()
		{
			std::cout << "== Event Message instance Box emsg v1 ==" << std::endl;
			std::cout << " reserved (timescale) " << timescale_ << std::endl;
			std::cout << " presentation_time: " << presentation_time_ << std::endl;
			std::cout << " event_duration: " << event_duration_ << std::endl;
			std::cout << " id: " << id_ << std::endl;
			std::cout << " scheme id uri: " << scheme_id_uri_ << std::endl;
			std::cout << " value:  " << value_ << std::endl;
			std::cout << " message data b64: " << base64_encode(message_data_.data(), message_data_.size()) << std::endl;
		}
	};

	//! struct to store a EventMessageInstanceBox 
	struct EventMessageInstanceBox : fmp4_stream::full_box
	{
		uint32_t		reserved_;
		int64_t			presentation_time_delta_;
		uint32_t		event_duration_;
		uint32_t		id_;
		std::string 	scheme_id_uri_;
		std::string		value_;
		std::vector<uint8_t>	message_data_;

		std::size_t size()
		{
			return 8 + 4 + 8 + 4 + 4 + 4 + scheme_id_uri_.size() + value_.size() + 2 + message_data_.size();
		}

		EventMessageInstanceBox()
			: reserved_(0), event_duration_(0), \
			id_(0), scheme_id_uri_(), \
			value_(), message_data_(), presentation_time_delta_(0)
		{
		}

		EventMessageInstanceBox(DASHEventMessageBoxv1 e, uint64_t pres_time)
			: reserved_(e.timescale_), event_duration_(e.event_duration_), \
			id_(e.id_), scheme_id_uri_(e.scheme_id_uri_), \
			value_(e.value_), message_data_(e.message_data_)
		{
			presentation_time_delta_ = e.presentation_time_ - pres_time;
		}

		void print()
		{
			std::cout << "== Event Message instance Box emib ==" << std::endl;
			std::cout << " reserved (timescale) " << reserved_ << std::endl;
			std::cout << " presentation_time_delta_ " << presentation_time_delta_ << std::endl;
			std::cout << " event_duration: " << event_duration_ << std::endl;
			std::cout << " id " << id_ << std::endl;
			std::cout << " scheme id uri " << scheme_id_uri_ << std::endl;
			std::cout << " value  " << value_ << std::endl;
			std::cout << " message data b64 " << base64_encode(message_data_.data(), message_data_.size()) << std::endl;
		}

		size_t write(std::ostream &ostr)
		{
			char int_buf[4];
			char long_buf[8];
			uint32_t bytes_written = 0;

			uint32_t sz = (uint32_t)this->size();
			fmp4_write_uint32(sz, int_buf);
			ostr.write((char *)int_buf, 4);
			bytes_written += 4;

			ostr.put('e');
			ostr.put('m');
			ostr.put('i');
			ostr.put('b');
			bytes_written += 4;
			ostr.put((uint8_t)0);
			ostr.put(0u);
			ostr.put(0u);
			ostr.put(0u);
			bytes_written += 4;

			fmp4_write_uint32(0, int_buf);
			ostr.write(int_buf, 4);
			bytes_written += 4;
			fmp4_write_int64(presentation_time_delta_, long_buf);
			ostr.write(long_buf, 8);
			bytes_written += 8;
			fmp4_write_uint32(event_duration_, int_buf);
			ostr.write(int_buf, 4);
			bytes_written += 4;
			fmp4_write_uint32(id_, int_buf);
			ostr.write(int_buf, 4);
			bytes_written += 4;
			ostr.write(scheme_id_uri_.c_str(), scheme_id_uri_.size() + 1);
			bytes_written += (uint32_t)scheme_id_uri_.size() + 1;
			ostr.write(value_.c_str(), value_.size() + 1);
			bytes_written += (uint32_t)value_.size() + 1;

			if (message_data_.size())
				ostr.write((char *)&message_data_[0], message_data_.size());

			bytes_written += (uint32_t)message_data_.size();
			
			return bytes_written;
		}

		//! enable writing emsg v1 to allow testing with old version of oriign
		size_t write_as_emsg_v1(std::ostream &ostr, uint64_t presentation_time)
		{
			char int_buf[4];
			char long_buf[8];
			uint32_t bytes_written = 0;

			uint32_t sz = (uint32_t)this->size();
			fmp4_write_uint32(sz, int_buf);
			ostr.write((char *)int_buf, 4);
			bytes_written += 4;

			ostr.put('e');
			ostr.put('m');
			ostr.put('s');
			ostr.put('g');
			bytes_written += 4;
			ostr.put((uint8_t)1);
			ostr.put(0u);
			ostr.put(0u);
			ostr.put(0u);
			bytes_written += 4;

			fmp4_write_uint32(reserved_, int_buf);
			ostr.write(int_buf, 4);
			bytes_written += 4;
			fmp4_write_int64(presentation_time_delta_ + presentation_time, long_buf);
			ostr.write(long_buf, 8);
			bytes_written += 8;
			fmp4_write_uint32(event_duration_, int_buf);
			ostr.write(int_buf, 4);
			bytes_written += 4;
			fmp4_write_uint32(id_, int_buf);
			ostr.write(int_buf, 4);
			bytes_written += 4;
			ostr.write(scheme_id_uri_.c_str(), scheme_id_uri_.size() + 1);
			bytes_written += (uint32_t)scheme_id_uri_.size() + 1;
			ostr.write(value_.c_str(), value_.size() + 1);
			bytes_written += (uint32_t)value_.size() + 1;

			if (message_data_.size())
				ostr.write((char *)&message_data_[0], message_data_.size());

			bytes_written += (uint32_t)message_data_.size();

			return bytes_written;
		}

		uint32_t parse(const char *ptr, unsigned int data_size, uint64_t pres_time=0)
		{
			if (data_size > 8) 
			{
				fmp4_stream::full_box::parse(ptr);
				uint64_t offset = full_box::size();

				reserved_ = fmp4_read_uint32(ptr + offset);
				//cout << "timescale: " << timescale << endl;
				offset += 4;
				presentation_time_delta_ = fmp4_read_uint64(ptr + offset);
				//cout << "presentation time: " << presentation_time << endl;
				offset += 8;
				event_duration_ = fmp4_read_uint32(ptr + offset);
				//cout << "event duration: " << event_duration << endl;
				offset += 4;
				id_ = fmp4_read_uint32(ptr + offset);
				offset += 4;
				//cout << "id: " << id << endl;
				scheme_id_uri_ = std::string(ptr + offset);
				offset = offset + scheme_id_uri_.size() + 1;
				//cout << "scheme_id_uri: " << scheme_id_uri << endl;
				value_ = std::string(ptr + offset);
				//cout << "value: " << value << endl;
				offset = offset + value_.size() + 1;

				for (unsigned int i = (unsigned int)offset; i < size_; i++)
					message_data_.push_back(*(ptr + (size_t)i));

				return size_;
			}
			return 0;
		}

		DASHEventMessageBoxv1 to_emsg_v1(uint64_t presentation_time)
		{
			DASHEventMessageBoxv1 e; 
			e.timescale_ = reserved_;
			e.scheme_id_uri_ = scheme_id_uri_;
			e.event_duration_ = event_duration_;
			e.id_ = id_;
			e.presentation_time_ = presentation_time_delta_ + presentation_time;
			e.value_ = value_;
			e.message_data_ = message_data_;
			return e;
		}
	};

	void set_evte(std::vector<uint8_t> &moov_in);

	//! struct to store sample (meta-data) and to write the mdat
	struct EventSample
	{
		uint64_t sample_presentation_time_;
		uint32_t sample_duration_;
		std::vector<EventMessageInstanceBox> instance_boxes_;
		bool is_emeb_;

		std::size_t size()
		{
			if(is_emeb_ || instance_boxes_.size() == 0)
			   return 8;
			else 
			{
				size_t l_size = 0;

				for (uint32_t k = 0; k < instance_boxes_.size(); k++)
					l_size += (uint32_t)instance_boxes_[k].size();

				return l_size;
			}   
		}

		// write sample (mdat/payload)
		size_t write(std::ostream &ostr)
		{
			if (is_emeb_ || instance_boxes_.size() == 0) 
			{
				ostr.write((const char *)&emeb[0], 8);
				return 8;
			}
			else 
			{
				size_t t_size = 0;
				for (unsigned int y = 0; y < instance_boxes_.size(); y++)
					t_size+=instance_boxes_[y].write(ostr);
				return t_size;
			}
		}

		size_t write_as_emsgv1(std::ostream &ostr)
		{
			if (is_emeb_ || instance_boxes_.size() == 0)
			{
				ostr.write((const char *)&emeb[0], 8);
				return 8;
			}
			else
			{
				size_t t_size = 0;
				for (unsigned int y = 0; y < instance_boxes_.size(); y++)
					t_size += instance_boxes_[y].write_as_emsg_v1(ostr,this->sample_presentation_time_);
				return t_size;
			}
		}
	};


	//! algorithm to find all sample boundaries between segment_start and segment_end (0=infinite)
	uint32_t find_sample_boundaries(
		const std::vector<DASHEventMessageBoxv1> &emsgs_in,
		std::vector<uint64_t> &sample_boundaries,
		uint64_t segment_start = 0,
		uint64_t segment_end = 0);

	//! algorithm to find all event message track samples between segment_start and segment_end (0=infinite)
	uint32_t find_event_samples(
		const std::vector<DASHEventMessageBoxv1> &emsgs_in,
		std::vector<EventSample> &samples_out,
		uint64_t segment_start = 0,
		uint64_t segment_end = 0);

	void write_evt_samples_as_fmp4_fragment(std::vector<EventSample> &samples_in,
		std::ostream &ostr,
		uint64_t timestamp_tfdt,
		uint32_t track_id,
		uint64_t next_tfdt);

	int write_to_segmented_event_track_file(
		const std::string& out_file,
		std::vector<DASHEventMessageBoxv1> &in_emsg_list,
		uint32_t track_id,
		uint64_t pt_off_start,
		uint64_t pt_off_end,
		const std::string& urn,
		uint64_t seg_duration,
		uint32_t timescale);
	
   struct ingest_event_stream: public fmp4_stream::ingest_stream
   {
	    typedef std::map<uint32_t,event_track::DASHEventMessageBoxv1> emsg_stream;
		std::map<std::string, emsg_stream> events_list_;

		int load_from_file(std::istream &infile, bool init_only);
		int print_samples_from_file(std::istream &infile, bool init_only);
		void write_to_dash_event_stream(std::string &out_file);
   };

	bool get_sparse_moov(const std::string& urn, 
		          uint32_t timescale, uint32_t track_id, 
		          std::vector<uint8_t> &sparse_moov);
    
	// function for generating track files with avails and scte-35
	int gen_avail_files(uint32_t track_duration = 60000,
		uint32_t seg_duration_ticks_ms = 0,
		uint32_t avail_duration = 30000,
		uint32_t avail_interval = 1800000, 
		uint64_t start_time = 0);		// helper function generate random events for testing and example generation
	// first arg set duration to zero, second and third distribution of presentation time and duration
	DASHEventMessageBoxv1 generate_random_event(bool set_duration_to_zero = false, uint32_t max_p = 150, uint32_t max_d = 20);
	//static void write_embe(std::ostream &ostr, uint64_t timestamp_tfdt, uint32_t track_id, uint32_t duration_in);
}
#endif 
