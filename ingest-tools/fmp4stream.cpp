/*******************************************************************************
Supplementary software media ingest specification:

https://github.com/unifiedstreaming/fmp4-ingest

Copyright (C) 2009-2018 CodeShop B.V.
http://www.code-shop.com

******************************************************************************/

#include "fmp4stream.h"
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <iomanip> 
#include <memory>
#include <base64.h>

namespace /* anonymous */ {

	inline bool is_big_endian() {
		return  (*(uint16_t *)"\0\xff" < 0x100);
	}

	//------------------ helpers for processing the bitstream ------------------------
	uint16_t fmp4_endian_swap16(uint16_t in) {
		return ((in & 0x00FF) << 8) | ((in & 0xFF00) >> 8);
	};

	uint32_t fmp4_endian_swap32(uint32_t in) {
		return  ((in & 0x000000FF) << 24) | \
			((in & 0x0000FF00) << 8) | \
			((in & 0x00FF0000) >> 8) | \
			((in & 0xFF000000) >> 24);
	}

	uint64_t fmp4_endian_swap64(uint64_t in) {
		return  ((in & 0x00000000000000FF) << 56) | \
			((in & 0x000000000000FF00) << 40) | \
			((in & 0x0000000000FF0000) << 24) | \
			((in & 0x00000000FF000000) << 8) | \
			((in & 0x000000FF00000000) >> 8) | \
			((in & 0x0000FF0000000000) >> 24) | \
			((in & 0x00FF000000000000) >> 40) | \
			((in & 0xFF00000000000000) >> 56);
	};

	uint16_t fmp4_read_uint16(char const*pt)
	{
		return is_big_endian() ? *((uint16_t *)pt) : fmp4_endian_swap16(*((uint16_t *)pt));
	}

	uint32_t fmp4_read_uint32(char const *pt)
	{
		return is_big_endian() ? *((uint32_t *)pt) : fmp4_endian_swap32(*((uint32_t *)pt));
	}

	uint64_t fmp4_read_uint64(char const *pt)
	{
		return is_big_endian() ? *((uint64_t *)pt) : fmp4_endian_swap64(*((uint64_t *)pt));
	}

	uint32_t fmp4_write_uint32(uint32_t in, char const *pt)
	{
		return is_big_endian() ? ((uint32_t *)pt)[0] = in : ((uint32_t *)pt)[0] = fmp4_endian_swap32(in);
	}

	uint64_t fmp4_write_uint64(uint64_t in, char const *pt)
	{
		return is_big_endian() ? ((uint64_t *)pt)[0] = in : ((uint64_t *)pt)[0] = fmp4_endian_swap64(in);
	}

} // anonymous


namespace fMP4Stream
{

	// base 64 sparse movie header
	std::string moov_64_enc("AAACNG1vb3YAAABsbXZoZAAAAAAAAAAAAAAAAAAAAAEAAAAAAAEAAAEAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAIAAAGYdHJhawAAAFx0a2hkAAAABwAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAABNG1kaWEAAAAgbWRoZAAAAAAAAAAAAAAAAAAAAAEAAAAAVcQAAAAAADFoZGxyAAAAAAAAAABtZXRhAAAAAAAAAAAAAAAAVVNQIE1ldGEgSGFuZGxlcgAAAADbbWluZgAAAAxubWhkAAAAAAAAACRkaW5mAAAAHGRyZWYAAAAAAAAAAQAAAAx1cmwgAAAAAQAAAKNzdGJsAAAAV3N0c2QAAAAAAAAAAQAAAEd1cmltAAAAAAAAAAEAAAA3dXJpIAAAAABodHRwOi8vd3d3LnVuaWZpZWQtc3RyZWFtaW5nLmNvbS9kYXNoL2Vtc2cAAAAAEHN0dHMAAAAAAAAAAAAAABBzdHNjAAAAAAAAAAAAAAAUc3RzegAAAAAAAAAAAAAAAAAAABBzdGNvAAAAAAAAAAAAAAAobXZleAAAACB0cmV4AAAAAAAAAAEAAAABAAAAAAAAAAAAAAAA");


	void box::parse(char const* ptr)
	{
		size_ = fmp4_read_uint32(ptr);
		box_type_ = std::string(ptr + 4, ptr + 8);
		if (size_ == 1) {
			large_size_ = fmp4_read_uint64(ptr + 8);
		}
	}

	// only the size of the beginning of the box structure
	uint64_t box::size()  const {
		uint64_t l_size = 8;
		if (is_large_)
			l_size += 8;
		if (has_uuid_)
			l_size += 16;
		return l_size;
	};

	void box::print() const
	{
		std::cout << "=================" << box_type_ << "==================" << std::endl;
		std::cout << std::setw(33) << std::left << " box size: " << size_ << std::endl;
	}

	void mvhd::parse(char const *ptr)
	{
		this->version_ = (unsigned int)ptr[8];

		if (version_ == 1)
			this->time_scale_ = fmp4_read_uint32(ptr + 12 + 8 + 8);
		else
			this->time_scale_ = fmp4_read_uint32(ptr + 12 + 8);

		return;
	}

	bool box::read(std::istream &istr)
	{
		box_data_.resize(9);
		large_size_ = 0;

		istr.read((char*)&box_data_[0], 8);
		size_ = fmp4_read_uint32((char *)&box_data_[0]);
		box_data_[8] = (uint8_t) '\0';
		box_type_ = std::string((char *)&box_data_[4]);
		if (box_type_.compare("uuid") == 0)
			has_uuid_ = true;

		// detect large size box 
		if (size_ == 1)
		{
			is_large_ = true;
			box_data_.resize(16);
			istr.read((char *)&box_data_[8], 8);
			large_size_ = fmp4_read_uint64((char *)&box_data_[8]);
		}
		else 
		{
			large_size_ = size_;
			box_data_.resize(8);
		}

		// read all the bytes from the stream
		if (size_) 
		{
			// write the offset bytes of the box
			uint8_t offset = (uint8_t)box_data_.size();
			box_data_.resize(large_size_);
			if ((large_size_ - offset) > 0)
				istr.read((char*)&box_data_[offset], large_size_ - offset);
			return true;
		}

		return false;
	};

	void full_box::parse(char const *ptr)
	{
		box::parse(ptr);
		// read the version and flags
		uint64_t offset = box::size();
		magic_conf_ = fmp4_read_uint32(ptr + offset);
		this->version_ = *((const uint8_t *)((ptr + offset)));
		this->flags_ = 0x00FFFFFF & fmp4_read_uint32(ptr + offset);
	}

	void full_box::print() const
	{
		box::print();
		std::cout << std::setw(33) << std::left << "version: " << this->version_ << std::endl;
		std::cout << std::setw(33) << std::left << "flags: " << flags_ << std::endl;
	}

	void mfhd::parse(char const * ptr)
	{
		seq_nr_ = fmp4_read_uint32(ptr + 12);
	}

	void mfhd::print() const
	{
		std::cout << "=================mfhd==================" << std::endl;
		std::cout << std::setw(33) << std::left << " sequence number is: " << seq_nr_ << std::endl;
	};

	void tfhd::parse(const char * ptr)
	{
		full_box::parse(ptr);
		track_id_ = fmp4_read_uint32(ptr + 12);
		//cout << "track_id " << track_id << endl;
		base_data_offset_present_ = !!(0x00000001 & flags_);
		//cout << "base_data_offset_present " << base_data_offset_present << endl;
		sample_description_index_present_ = !!(0x00000002 & flags_);
		//cout << "sample_description_index_present " << sample_description_index_present << endl;
		default_sample_duration_present_ = !!(0x00000008 & flags_);
		//cout << "default_sample_duration_present " << default_sample_duration_present << endl;
		default_sample_size_present_ = !!(0x00000010 & flags_);
		//cout << "default_sample_size_present " << default_sample_size_present << endl;
		default_sample_flags_present_ = !!(0x00000020 & flags_);
		//cout << "default_sample_flags_present " << default_sample_flags_present << endl;
		duration_is_empty_ = !!(0x00010000 & flags_);
		default_base_is_moof_ = !!(0x00020000 & flags_);

		unsigned int offset = 16;

		if (base_data_offset_present_)
		{
			base_data_offset_ = fmp4_read_uint32(ptr + offset);
			offset += 4;
		}
		if (sample_description_index_present_)
		{
			sample_description_index_ = fmp4_read_uint32(ptr + offset);
			offset += 4;
		}
		if (default_sample_duration_present_)
		{
			default_sample_duration_ = fmp4_read_uint32(ptr + offset);
			offset += 4;
		}
		if (default_sample_size_present_)
		{
			default_sample_size_ = fmp4_read_uint32(ptr + offset);
			offset += 4;
		}
		if (default_sample_flags_present_)
		{
			default_sample_flags_ = (uint32_t)fmp4_read_uint32(ptr + offset);
			offset += 4;
		}
	}

	uint64_t tfhd::size() const
	{
		uint64_t l_size = full_box::size() + 4;
		if (base_data_offset_present_)
			l_size += 8;
		if (sample_description_index_)
			l_size += 4;
		if (default_sample_duration_present_)
			l_size += 4;
		if (default_sample_size_present_)
			l_size += 4;
		if (default_sample_flags_present_)
			l_size += 4;

		return l_size;
	};

	void tfhd::print() const {
		std::cout << "=================tfhd==================" << std::endl;
		//cout << setw(33) << left << " magic conf                 " << m_magic_conf << endl;
		std::cout << std::setw(33) << std::left << " vflags                  " << flags_ << std::endl;
		std::cout << std::setw(33) << std::left << " track id:                  " << track_id_ << std::endl;
		if (base_data_offset_present_)
			std::cout << std::setw(33) << std::left << " base_data_offset: " << base_data_offset_ << std::endl;
		if (sample_description_index_present_)
			std::cout << std::setw(33) << std::left << " sample description: " << sample_description_index_ << std::endl;
		if (default_sample_duration_present_)
			std::cout << std::setw(33) << std::left << " sample duration: " << default_sample_duration_ << std::endl;
		if (default_sample_size_present_)
			std::cout << std::setw(33) << std::left << " default sample size: " << default_sample_size_ << std::endl;
		if (default_sample_flags_present_)
			std::cout << std::setw(33) << std::left << " default sample flags:  " << default_sample_flags_ << std::endl;
		std::cout << std::setw(33) << std::left << " duration is empty " << duration_is_empty_ << std::endl;
		std::cout << std::setw(33) << std::left << " default base is moof" << default_base_is_moof_ << std::endl;
		//cout << "............." << std::endl;
	};

	uint64_t tfdt::size() const
	{
		return version_ ? full_box::size() + 8 : full_box::size() + 4;
	};

	void tfdt::print() const {
		std::cout << "=================tfdt==================" << std::endl;
		std::cout << std::setw(33) << std::left << " basemediadecodetime: " << base_media_decode_time_ << std::endl;
	};

	void tfdt::parse(const char* ptr)
	{
		full_box::parse(ptr);
		base_media_decode_time_ = version_ ? \
			fmp4_read_uint64((char *)(ptr + 12)) : \
			fmp4_read_uint32((char *)(ptr + 12));

	};

	uint64_t trun::size() const
	{
		uint64_t l_size = full_box::size() + 4;
		if (data_offset_present_)
			l_size += 4;
		if (first_sample_flags_present_)
			l_size += 4;
		if (sample_duration_present_)
			l_size += (sample_count_ * 4);
		if (sample_size_present_)
			l_size += (sample_count_ * 4);
		if (sample_flags_present_)
			l_size += (sample_count_ * 4);
		if (this->sample_composition_time_offsets_present_)
			l_size += (sample_count_ * 4);
		return l_size;
	};

	void trun::print() const
	{
		std::cout << "==================trun=================" << std::endl;
		std::cout << std::setw(33) << std::left << " magic conf                 " << magic_conf_ << std::endl;
		std::cout << std::setw(33) << std::left << " sample count:      " << sample_count_ << std::endl;
		if (data_offset_present_)
			std::cout << std::setw(33) << std::left << " data_offset:        " << data_offset_ << std::endl;
		if (first_sample_flags_present_)
			std::cout << " first sample flags:        " << first_sample_flags_ << std::endl;

		std::cout << std::setw(33) << std::left << " first sample flags present:  " << first_sample_flags_present_ << std::endl;
		std::cout << std::setw(33) << std::left << " sample duration present: " << sample_duration_present_ << std::endl;
		std::cout << std::setw(33) << std::left << " sample_size_present: " << sample_size_present_ << std::endl;
		std::cout << std::setw(33) << std::left << " sample_flags_present: " << sample_flags_present_ << std::endl;
		std::cout << std::setw(33) << std::left << " sample_ct_offsets_present: " << sample_composition_time_offsets_present_ << std::endl;

		for (unsigned int i = 0; i < m_sentry.size(); i++)
			m_sentry[i].print();
	};

	void trun::parse(const char * ptr)
	{
		full_box::parse(ptr);

		sample_count_ = fmp4_read_uint32(ptr + 12);

		data_offset_present_ = !!(0x00000001 & flags_);
		//cout << "data_offset_present " << data_offset_present << endl;
		first_sample_flags_present_ = !!(0x00000004 & flags_);
		//cout << "first_sample_flags_present " << first_sample_flags_present << endl;
		sample_duration_present_ = !!(0x00000100 & flags_);
		//cout << "sample_duration_present " << sample_duration_present << endl;
		sample_size_present_ = !!(0x00000200 & flags_);
		//cout << "sample_size_present " << sample_size_present << endl;
		sample_flags_present_ = !!(0x00000400 & flags_);
		sample_composition_time_offsets_present_ = !!(0x00000800 & flags_);

		//sentry.resize(sample_count);
		unsigned int offset = 16;
		if (data_offset_present_)
		{
			this->data_offset_ = fmp4_read_uint32(ptr + offset);
			offset += 4;
		}if (first_sample_flags_present_) {
			this->first_sample_flags_ = fmp4_read_uint32(ptr + offset);
			offset += 4;
		}
		// write all the entries to the trun box
		for (unsigned int i = 0; i < sample_count_; i++)
		{
			sample_entry ent = {};
			if (sample_duration_present_)
			{
				ent.sample_duration_ = fmp4_read_uint32(ptr + offset);
				offset += 4;
			}
			if (sample_size_present_)
			{
				ent.sample_size_ = fmp4_read_uint32(ptr + offset);
				offset += 4;
			}
			if (sample_flags_present_)
			{
				ent.sample_flags_ = fmp4_read_uint32(ptr + offset);
				offset += 4;
			}
			if (sample_composition_time_offsets_present_)
			{
				if (version_ == 0)
					ent.sample_composition_time_offset_v0_ = fmp4_read_uint32(ptr + offset);
				else
					ent.sample_composition_time_offset_v1_ = fmp4_read_uint32(ptr + offset);
				offset += 4;
			}
			m_sentry.push_back(ent);
		}
	}

	uint64_t media_fragment::get_duration()
	{
		uint64_t duration = 0;

		for (unsigned int i = 0; i != trun_.sample_count_; ++i)
		{
			uint32_t sample_duration = tfhd_.default_sample_duration_;
			if (trun_.sample_duration_present_)
			{
				sample_duration = trun_.m_sentry[i].sample_duration_;
			}

			duration += sample_duration;
		}
		return duration;
	}

	void sc35_splice_info::print(bool verbose) const
	{
		if (verbose) {
			std::cout << std::setw(33) << std::left << " table id: " << (unsigned int)table_id_ << std::endl;
			std::cout << std::setw(33) << std::left << " section_syntax_indicator: " << section_syntax_indicator_ << std::endl;
			std::cout << std::setw(33) << std::left << " private_indicator: " << private_indicator_ << std::endl;
			std::cout << std::setw(33) << std::left << " section_length: " << (unsigned int)section_length_ << std::endl;
			std::cout << std::setw(33) << std::left << " protocol_version: " << (unsigned int)protocol_version_ << std::endl;
			std::cout << std::setw(33) << std::left << " encrypted_packet: " << encrypted_packet_ << std::endl;
			std::cout << std::setw(33) << std::left << " encryption_algorithm: " << (unsigned int)encryption_algorithm_ << std::endl;
			std::cout << std::setw(33) << std::left << " pts_adjustment: " << pts_adjustment_ << std::endl;
			std::cout << std::setw(33) << std::left << " cw_index: " << (unsigned int)cw_index_ << std::endl;
			std::cout << std::setw(33) << std::left << " tier: " << tier_ << std::endl;
			std::cout << std::setw(33) << std::left << " splice_command_length: " << splice_command_length_ << std::endl;
			std::cout << std::setw(33) << std::left << " splice_command_type: " << (unsigned int)splice_command_type_ << std::endl;
			std::cout << std::setw(33) << std::left << " descriptor_loop_length: " << descriptor_loop_length_ << std::endl;
		}
		std::cout << std::setw(33) << std::left << " pts_adjustment: " << pts_adjustment_ << std::endl;
		std::cout << std::setw(33) << std::left << " Command type: ";

		switch (splice_command_type_)
		{
		case 0x00: std::cout << 0x00; // prints "1"
			std::cout << std::setw(33) << std::left << " splice null " << std::endl;
			break;       // and exits the switch
		case 0x04:
			std::cout << std::setw(33) << std::left << " splice schedule " << std::endl;
			break;
		case 0x05:
			std::cout << std::setw(33) << std::left << " splice insert " << std::endl;
			std::cout << std::setw(33) << std::left << " event_id: " << splice_insert_event_id_ << std::endl;
			std::cout << std::setw(33) << std::left << " cancel indicator: " << splice_event_cancel_indicator_ << std::endl;
			break;
		case 0x06:
			std::cout << std::setw(33) << std::left << "time signal " << std::endl;
			break;
		case 0x07:
			std::cout << std::setw(33) << std::left << "bandwidth reservation " << std::endl;
			break;
		}
	}

	void sc35_splice_info::parse(const uint8_t *ptr, unsigned int size)
	{
		table_id_ = *ptr++;

		if (table_id_ != 0xFC)
		{
			std::cout << " error parsing table id, tableid != 0xFC " << std::endl;
			return;
		}
		//else
		//	cout << "table id ok " << endl;

		std::bitset<8> b(*ptr);
		section_syntax_indicator_ = b[7];
		private_indicator_ = b[6];
		section_length_ = 0x0FFF & fmp4_read_uint16((char *)ptr);
		ptr += 2;
		protocol_version_ = (uint8_t)*ptr++;

		b = std::bitset<8>(*ptr);

		encrypted_packet_ = b[7];
		bool pts_highest_bit = b[0];

		b[0] = 0;
		b[7] = 0;
		encryption_algorithm_ = uint8_t(b.to_ullong() >> 1);
		ptr++;

		pts_adjustment_ = (uint64_t)fmp4_read_uint32((char *)ptr);
		if (pts_highest_bit)
		{
			pts_adjustment_ += (uint64_t)std::numeric_limits<uint32_t>::max();
		}
		ptr = ptr + 4;
		cw_index_ = *ptr++;

		uint32_t wd = fmp4_read_uint32((char *)ptr);
		std::bitset<32> a(wd);
		tier_ = (wd & 0xFFF00000) >> 20;
		splice_command_length_ = (0x000FFF00 & wd) >> 8;
		splice_command_type_ = (uint8_t)(0x000000FF & wd);
		ptr += 4;
		descriptor_loop_length_ = fmp4_read_uint16((char *)ptr);

		ptr += descriptor_loop_length_;
		if (splice_command_type_ == 0x05) {
			splice_insert_event_id_ = fmp4_read_uint32((char *)ptr);
			ptr += 4;
			std::bitset<8> bb(*ptr);
			splice_event_cancel_indicator_ = bb[7];
		}
		// we don't support the cancel indicator yet (it should be added)
	}

	uint64_t emsg::size() const
	{
		uint64_t l_size = full_box::size();
		if (version_ == 1)
			l_size += 4;
		l_size += 16; // presentation time duration
		l_size += scheme_id_uri_.size() + 1;
		l_size += value_.size() + 1;
		l_size += message_data_.size();
		return l_size;
	}

	void emsg::parse(const char *ptr, unsigned int data_size)
	{
		full_box::parse(ptr);
		uint64_t offset = full_box::size();
		if (version_ == 0)
		{
			scheme_id_uri_ = std::string(ptr + offset);
			//cout << "scheme_id_uri: " << scheme_id_uri << endl;
			offset = offset + scheme_id_uri_.size() + 1;
			value_ = std::string(ptr + offset);
			//cout << "value: " << value << endl;
			offset = offset + value_.size() + 1;
			timescale_ = fmp4_read_uint32(ptr + offset);
			offset += 4;
			//cout << "timescale: " << timescale << endl;
			presentation_time_delta_ = fmp4_read_uint32(ptr + offset);
			offset += 4;
			//cout << "presentation_time_delta: " << presentation_time_delta << endl;
			event_duration_ = fmp4_read_uint32(ptr + offset);
			offset += 4;
			//cout << "event_duration: " << event_duration << endl;
			id_ = fmp4_read_uint32(ptr + offset);
			//cout << "id: " << id << endl;
			offset += 4;
		}
		else
		{
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
		}
		for (unsigned int i = (unsigned int)offset; i < data_size; i++)
		{
			message_data_.push_back(*(ptr + (size_t)offset));
			offset++;
		}
	}

	//! emsg to mpd event, always to base64 encoding
	void emsg::write_emsg_as_mpd_event(std::ostream &ostr, uint64_t base_time) const
	{
		ostr << "<Event "
			<< "presentationTime=" << '"' << (this->version_ ? presentation_time_ : base_time + presentation_time_delta_) << '"' << " "  \
			<< "duration=" << '"' << event_duration_ << '"' << " "  \
			<< "id=" << '"' << id_ << '"';
		if (this->scheme_id_uri_.compare("urn:scte:scte35:2013:bin") == 0) // write binary scte as xml + bin as defined by scte-35
		{
			ostr << '>' << std::endl << "  <Signal xmlns=" << '"' << "http://www.scte.org/schemas/35/2016" << '"' << '>' << std::endl \
				<< "    <Binary>" << base64_encode(this->message_data_.data(), (unsigned int)this->message_data_.size()) << "</Binary>" << std::endl
				<< "  </Signal>" << std::endl;
		}
		else {
			ostr << " " << "contentEncoding=" << '"' << "base64" << '"' << '>' << std::endl
				<< base64_encode(this->message_data_.data(), (unsigned int)this->message_data_.size()) << std::endl;
		}
		ostr << "</Event>" << std::endl;
	}

	//!
	void emsg::print() const
	{
		std::cout << "=================emsg==================" << std::endl;
		std::cout << std::setw(33) << std::left << " e-msg box version: " << (unsigned int)version_ << std::endl;
		std::cout << " scheme_id_uri:     " << scheme_id_uri_ << std::endl;
		std::cout << std::setw(33) << std::left << " value:             " << value_ << std::endl;
		std::cout << std::setw(33) << std::left << " timescale:         " << timescale_ << std::endl;
		if (version_ == 1)
			std::cout << std::setw(33) << std::left << " presentation_time: " << presentation_time_ << std::endl;
		else
			std::cout << std::setw(33) << std::left << " presentation_time_delta: " << presentation_time_delta_ << std::endl;
		std::cout << std::setw(33) << std::left << " event duration:    " << event_duration_ << std::endl;
		std::cout << std::setw(33) << std::left << " event id           " << id_ << std::endl;
		std::cout << std::setw(33) << std::left << " message data size  " << message_data_.size() << std::endl;

		//print_payload

		if (message_data_[0] == 0xFC)
		{
			// splice table
			//cout << " scte splice info" << endl;
			sc35_splice_info l_splice;
			l_splice.parse((uint8_t*)&message_data_[0], (unsigned int)message_data_.size());
			std::cout << "=============splice info==============" << std::endl;
			l_splice.print();
		}
	}

	// write an emsg box to a stream
	uint32_t emsg::write(std::ostream &ostr) const
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
		ostr.put((uint8_t)version_);
		ostr.put(0u);
		ostr.put(0u);
		ostr.put(0u);
		bytes_written += 4;

		if (version_ == 1)
		{
			fmp4_write_uint32(timescale_, int_buf);
			ostr.write(int_buf, 4);
			bytes_written += 4;
			fmp4_write_uint64(presentation_time_, long_buf);
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
		}
		else
		{
			ostr.write(scheme_id_uri_.c_str(), scheme_id_uri_.size() + 1);
			bytes_written += (uint32_t)scheme_id_uri_.size() + 1;
			ostr.write(value_.c_str(), value_.size() + 1);
			bytes_written += (uint32_t)value_.size() + 1;
			fmp4_write_uint32(timescale_, int_buf);
			ostr.write(int_buf, 4);
			bytes_written += 4;
			fmp4_write_uint32(presentation_time_delta_, int_buf);
			ostr.write(int_buf, 4);
			bytes_written += 4;
			fmp4_write_uint32(event_duration_, int_buf);
			ostr.write(int_buf, 4);
			bytes_written += 4;
			fmp4_write_uint32(id_, int_buf);
			ostr.write(int_buf, 4);
			bytes_written += 4;
		}
		if (message_data_.size())
			ostr.write((char *)&message_data_[0], message_data_.size());
		bytes_written += (uint32_t)message_data_.size();
		return bytes_written;
	}

	void emsg::write_emsg_as_fmp4_fragment(std::ostream &ostr, uint64_t timestamp_tfdt, uint32_t track_id,
		uint64_t next_tfdt, uint8_t target_version)
	{
		if (scheme_id_uri_.size())
		{
			if ((version_ == 1) && (target_version == 0)) {
				this->presentation_time_delta_ = 0; /* should be: presentation_time_ - timestamp_tfdt; */
				this->version_ = target_version;
			}
			else if ((version_ == 0) && (target_version == 1))
			{
				this->presentation_time_ = timestamp_tfdt + presentation_time_delta_;
				this->version_ = target_version;
			}

			// --- init mfhd
			mfhd l_mfhd = {};
			l_mfhd.seq_nr_ = 0;
			uint64_t l_mfhd_size = l_mfhd.size();

			//uint32_t l_announce = 8 * this->timescale_; // following the method of push input stream (i do not think this is correct)

			// --- init tfhd
			tfhd l_tfhd = {};
			l_tfhd.magic_conf_ = 131106u;
			l_tfhd.track_id_ = track_id;
			l_tfhd.sample_description_index_ = 1u;
			l_tfhd.default_sample_flags_ = 37748800u;
			l_tfhd.default_sample_flags_ = 37748800u;
			l_tfhd.base_data_offset_present_ = false;
			l_tfhd.default_base_is_moof_ = true;
			l_tfhd.duration_is_empty_ = false;
			l_tfhd.sample_description_index_present_ = true;
			l_tfhd.default_sample_duration_present_ = false;
			l_tfhd.default_sample_flags_present_ = true;
			l_tfhd.default_sample_size_present_ = false;
			uint64_t l_tfhd_size = l_tfhd.size();

			// --- init tfdt
			tfdt l_tfdt = {};
			l_tfdt.version_ = 1u;
			l_tfdt.base_media_decode_time_ = this->presentation_time_;
			uint64_t l_tfdt_size = l_tfdt.size(); //

			// --- init trun
			trun l_trun = {};
			l_trun.magic_conf_ = 769u;
			l_trun.sample_count_ = 1;
			l_trun.data_offset_present_ = true;
			l_trun.first_sample_flags_present_ = false;
			l_trun.sample_duration_present_ = true;
			l_trun.sample_size_present_ = true;
			l_trun.sample_flags_present_ = false;
			l_trun.sample_composition_time_offsets_present_ = false;

			//-- init sentry in trun write 2 samples
			l_trun.m_sentry.resize(1);
			//l_trun.m_sentry[0].sample_size_ = 0;
			//l_trun.m_sentry[0].sample_duration_ = 0; // presentation_time_delta_ ? this->presentation_time_delta_ : (presentation_time_ - timestamp_tfdt);
			l_trun.m_sentry[0].sample_size_ = (uint32_t)size();
			l_trun.m_sentry[0].sample_duration_ = this->event_duration_;


			//--- initialize the box sizes
			uint64_t l_trun_size = l_trun.size();
			uint64_t l_traf_size = 8 + l_trun_size + l_tfdt_size + l_tfhd_size;
			uint64_t l_moof_size = 8 + l_traf_size + l_mfhd_size; // l_traf_size + 8 + l_mfhd_size;
			l_trun.data_offset_ = (int32_t)l_moof_size + 8;

			// write the fragment 
			char int_buf[4];
			char long_buf[8];

			//--- write the sparse fragment to a file stream
			// write 4 bytes
			fmp4_write_uint32((uint32_t)l_moof_size, int_buf);
			ostr.write(int_buf, 4);
			// write 4 bytes, total 8 bytes
			ostr.put('m');
			ostr.put('o');
			ostr.put('o');
			ostr.put('f');
			// write 16 bytes total 24 bytes
			fmp4_write_uint32((uint32_t)l_mfhd_size, int_buf);
			ostr.write(int_buf, 4);
			//ostr->write("mfhd", 4);
			ostr.put('m');
			ostr.put('f');
			ostr.put('h');
			ostr.put('d');
			fmp4_write_uint32((uint32_t)0u, int_buf);
			ostr.write(int_buf, 4);
			fmp4_write_uint32((uint32_t)l_mfhd.seq_nr_, int_buf);
			ostr.write(int_buf, 4);

			// write traf 8 bytes total 32 bytes
			fmp4_write_uint32((uint32_t)l_traf_size, int_buf);
			ostr.write(int_buf, 4);
			//ostr->write("traf", 4);
			ostr.put('t');
			ostr.put('r');
			ostr.put('a');
			ostr.put('f');

			// write tfhd 24 bytes total 56 bytes
			fmp4_write_uint32((uint32_t)l_tfhd_size, int_buf);
			ostr.write(int_buf, 4);
			//ostr->write("tfhd", 4);
			ostr.put('t');
			ostr.put('f');
			ostr.put('h');
			ostr.put('d');

			fmp4_write_uint32((uint32_t)l_tfhd.magic_conf_, int_buf);
			ostr.write(int_buf, 4);
			fmp4_write_uint32((uint32_t)l_tfhd.track_id_, int_buf);
			ostr.write(int_buf, 4);
			fmp4_write_uint32((uint32_t)l_tfhd.sample_description_index_, int_buf);
			ostr.write(int_buf, 4);
			fmp4_write_uint32((uint32_t)l_tfhd.default_sample_flags_, int_buf);
			ostr.write(int_buf, 4);

			// write tfdt 20 bytes total 76 bytes
			fmp4_write_uint32((uint32_t)l_tfdt_size, int_buf);
			ostr.write(int_buf, 4);
			ostr.put('t');
			ostr.put('f');
			ostr.put('d');
			ostr.put('t');
			ostr.put(1u); // version
			ostr.put(0u);
			ostr.put(0u);
			ostr.put(0u);
			fmp4_write_uint64((uint64_t)l_tfdt.base_media_decode_time_, long_buf);
			ostr.write(long_buf, 8);

			fmp4_write_uint32((uint32_t)l_trun_size, int_buf);
			ostr.write(int_buf, 4);
			//ostr->write("trun", 4);
			ostr.put('t');
			ostr.put('r');
			ostr.put('u');
			ostr.put('n');
			fmp4_write_uint32((uint32_t)l_trun.magic_conf_, int_buf);
			ostr.write(int_buf, 4);
			fmp4_write_uint32((uint32_t)l_trun.sample_count_, int_buf);
			ostr.write(int_buf, 4);
			fmp4_write_uint32((uint32_t)l_trun.data_offset_, int_buf);
			ostr.write(int_buf, 4);

			// write the duration and the sample size
			fmp4_write_uint32((uint32_t)l_trun.m_sentry[0].sample_duration_, int_buf);
			ostr.write(int_buf, 4);
			fmp4_write_uint32((uint32_t)l_trun.m_sentry[0].sample_size_, int_buf);
			ostr.write(int_buf, 4);
			//fmp4_write_uint32((uint32_t)l_trun.m_sentry[1].sample_duration_, int_buf);
			//ostr.write(int_buf, 4);
			//fmp4_write_uint32((uint32_t)l_trun.m_sentry[1].sample_size_, int_buf);
			//ostr.write(int_buf, 4);
			//fmp4_write_uint32((uint32_t)l_trun.m_sentry[2].sample_duration_, int_buf);
			//ostr.write(int_buf, 4);
			//fmp4_write_uint32((uint32_t)l_trun.m_sentry[2].sample_size_, int_buf);
			//ostr.write(int_buf, 4);

			uint32_t mdat_size = (uint32_t)size() + 8; // mdat box + embe box + this event message box
			fmp4_write_uint32(mdat_size, int_buf);
			ostr.write(int_buf, 4);
			ostr.put('m');
			ostr.put('d');
			ostr.put('a');
			ostr.put('t');

			//ostr.write((char *)embe, 8);

			// write the emsg as an mdat box
			this->write(ostr);

		}
		return;
	};

	uint32_t init_fragment::get_time_scale()
	{
		if (moov_box_.box_data_.size() > 30) {
			char * ptr = (char *)moov_box_.box_data_.data();
			if (std::string((char *)(ptr + 12)).compare("mvhd") == 0)
			{
				unsigned int version = (unsigned int)ptr[16];
				return version ? fmp4_read_uint32(ptr + 36) : fmp4_read_uint32(ptr + 28);
			}
			else {
				std::cout << "provide mvhd in beginning of file for correct timescale" << std::endl;
				return 0;
			}
		}
		else {
			std::cout << "provide mvhd in beginning of file for correct timescale" << std::endl;
			return 0;
		}
	}

	void media_fragment::parse_moof()
	{
		if (!moof_box_.size_)
			return;

		uint64_t box_size = 0;
		uint64_t offset = 8;
		uint8_t * ptr = &moof_box_.box_data_[0];

		while (moof_box_.box_data_.size() > offset)
		{
			unsigned int temp_off = 0;
			box_size = (uint64_t)fmp4_read_uint32((char *)&moof_box_.box_data_[offset]);

			char name[5] = { (char)ptr[offset + 4],(char)ptr[offset + 5],(char)ptr[offset + 6],(char)ptr[offset + 7],'\0' };

			if (box_size == 1) // the box_size is a large size (should not happen in fmp4)
			{
				temp_off = 8;
				box_size = fmp4_read_uint64((char *)& ptr[offset + temp_off]);
			}

			if (std::string(name).compare("mfhd") == 0)
			{
				mfhd_.parse((char *)& ptr[offset + temp_off]);
				offset += (uint64_t)box_size;
				//cout << "mfhd size" << box_size << endl;
				continue;
			}

			if (std::string(name).compare("trun") == 0)
			{
				trun_.parse((char *)& ptr[offset + temp_off]);
				offset += (uint64_t)box_size;
				continue;
			}

			if (std::string(name).compare("tfdt") == 0)
			{
				tfdt_.parse((char *)& ptr[offset + temp_off]);
				offset += (uint64_t)box_size;
				continue;
			}

			if (std::string(name).compare("tfhd") == 0)
			{
				tfhd_.parse((char *)& ptr[offset + temp_off]);
				offset += (uint64_t)box_size;
				continue;
			}

			// cmaf style only one traf box per moof so we skip it
			if (std::string(name).compare("traf") == 0)
			{
				offset += 8;
			}
			else
				offset += (unsigned int)box_size;
		}
	};

	void media_fragment::patch_tfdt(uint64_t patch, uint32_t seq_nr)
	{
		if (!moof_box_.size_)
			return;

		uint64_t box_size = 0;
		uint64_t offset = 8;

		if (seq_nr)
			this->mfhd_.seq_nr_ = seq_nr;

		this->tfdt_.base_media_decode_time_ = this->tfdt_.base_media_decode_time_ + patch;

		// find the tfdt box and overwrite it
		while (moof_box_.box_data_.size() > offset)
		{
			unsigned int temp_off = 0;
			box_size = (uint64_t)fmp4_read_uint32((char *)&moof_box_.box_data_[offset]);
			uint8_t * ptr = &moof_box_.box_data_[0];
			char name[5] = { (char)ptr[offset + 4],(char)ptr[offset + 5],(char)ptr[offset + 6],(char)ptr[offset + 7],'\0' };

			if (box_size == 1) // the box_size is a large size (should not happen in fmp4)
			{
				temp_off = 8;
				box_size = fmp4_read_uint64((char *)& ptr[offset + temp_off]);
			}

			if (std::string(name).compare("mfhd") == 0)
			{
				
				if (seq_nr) 
					fmp4_write_uint32(seq_nr, (char *)&ptr[offset + temp_off + 12]);
				//mfhd_.parse((char *)& ptr[offset + temp_off]);
				offset += (uint64_t)box_size;
				//cout << "mfhd size" << box_size << endl;
				continue;
			}

			if (std::string(name).compare("trun") == 0)
			{
				//trun_.parse((char *)& ptr[offset + temp_off]);
				offset += (uint64_t)box_size;
				continue;
			}

			if (std::string(name).compare("tfdt") == 0)
			{
				
				this->tfdt_.version_ ? \
					fmp4_write_uint64(this->tfdt_.base_media_decode_time_, (char *)&ptr[offset + temp_off + 12]) : \
					fmp4_write_uint32((uint32_t)this->tfdt_.base_media_decode_time_, (char *)&ptr[offset + temp_off + 12]);

				offset += (uint64_t)box_size;
				continue;
			}

			if (std::string(name).compare("tfhd") == 0)
			{
				//tfhd_.parse((char *)& ptr[offset + temp_off]);
				offset += (uint64_t)box_size;
				continue;
			}

			// cmaf style only one traf box per moof so we skip it
			if (std::string(name).compare("traf") == 0)
			{
				offset += 8;
			}
			else
				offset += (unsigned int)box_size;
		}
	};

	// function to support the ingest, gets the init fragment data
	uint64_t ingest_stream::get_init_segment_data(std::vector<uint8_t> &init_seg_dat)
	{
		uint64_t ssize = init_fragment_.ftyp_box_.large_size_ + init_fragment_.moov_box_.large_size_;
		init_seg_dat.resize(ssize);

		// hard copy the init segment data to the output vector, as ftyp and moviefragment box need to be combined
		std::copy(init_fragment_.ftyp_box_.box_data_.begin(), init_fragment_.ftyp_box_.box_data_.end(), init_seg_dat.begin());
		std::copy(init_fragment_.moov_box_.box_data_.begin(), init_fragment_.moov_box_.box_data_.end(), init_seg_dat.begin() + init_fragment_.ftyp_box_.large_size_);

		return ssize;
	};

	// function to support the ingest of segments 
	uint64_t ingest_stream::get_media_segment_data(std::size_t index, std::vector<uint8_t> &media_seg_dat)
	{
		if (!(media_fragment_.size() > index))
			return 0;

		uint64_t ssize = media_fragment_[index].moof_box_.large_size_ + media_fragment_[index].mdat_box_.large_size_;
		media_seg_dat.resize(ssize);

		// hard copy the init segment data to the output vector as moviefragmentbox and mdat box need to be combined
		std::copy(media_fragment_[index].moof_box_.box_data_.begin(), media_fragment_[index].moof_box_.box_data_.end(), media_seg_dat.begin());
		std::copy(media_fragment_[index].mdat_box_.box_data_.begin(), media_fragment_[index].mdat_box_.box_data_.end(), media_seg_dat.begin() + media_fragment_[index].moof_box_.large_size_);

		return ssize;
	};

	//
	void media_fragment::print() const
	{
		if (emsg_.scheme_id_uri_.size() && !this->e_msg_is_in_mdat_)
			emsg_.print();

		moof_box_.print(); // moof box size
		mfhd_.print(); // moof box header
		tfhd_.print(); // track fragment header
		tfdt_.print(); // track fragment decode time
		trun_.print(); // trun box
		mdat_box_.print(); // mdat 

		if (emsg_.scheme_id_uri_.size() && this->e_msg_is_in_mdat_)
			emsg_.print();
	}

	// parse an fmp4 file for media ingest
	int ingest_stream::load_from_file(std::istream &infile, bool init_only)
	{
		try
		{
			if (infile)
			{
				std::vector<box> ingest_boxes;

				while (infile.good()) // read box by box in a vector
				{
					box b = {};
					if (b.read(infile))
						ingest_boxes.push_back(b);
					else  // break when we have boxes of size zero
						break;
					//if (b.m_btype.compare("moov") == 0)
					//	for (long k = 0; k < b.m_box_data.size; k++)
					//		putchar(b.m_box_data[0]);
					// we use the mfra to quit, hence we assume mfra box at the end of the file
					if (b.box_type_.compare("mfra") == 0)
						break;
				}

				// organize the boxes in init fragments and media fragments 
				for (auto it = ingest_boxes.begin(); it != ingest_boxes.end(); ++it)
				{
					if (it->box_type_.compare("ftyp") == 0)
					{
						//cout << "|ftyp|";
						init_fragment_.ftyp_box_ = *it;
					}
					if (it->box_type_.compare("moov") == 0)
					{
						//cout << "|moov|";
						init_fragment_.moov_box_ = *it;
						if (init_only)
							return 0;
					}

					if (it->box_type_.compare("moof") == 0) // in case of moof box we push both the moof and following mdat
					{
						media_fragment m = {};
						m.moof_box_ = *it;
						bool mdat_found = false;
						auto prev_box = (it - 1);
						// see if there is an emsg before
						if (prev_box->box_type_.compare("emsg") == 0)
						{
							m.emsg_.parse((char *)& prev_box->box_data_[0], (unsigned int)prev_box->box_data_.size());
							//cout << "|emsg|";
							std::cout << "found inband dash emsg box" << std::endl;
						}
						//cout << "|moof|";
						while (!mdat_found)
						{
							it++;

							if (it->box_type_.compare("mdat") == 0)
							{
								// find event messages embedded in 

								m.mdat_box_ = *it;
								mdat_found = true;
								m.parse_moof();

								uint32_t index = 8;

								while (index < m.mdat_box_.box_data_.size())
								{
									// seek to next box in 
									uint8_t name[9] = {};
									for (uint32_t i = 0; i < 8; i++)
										name[i] = m.mdat_box_.box_data_[index + i];
									name[8] = '\0';
									uint32_t l_size = fmp4_read_uint32((char *)name);

									if (l_size > m.mdat_box_.box_data_.size())
										break;

									std::string enc_box_name((char *)&name[4]);

									if (enc_box_name.compare("emsg") == 0) // right now we can only parse a single emsg, todo update to parse multiple emsg
									{
										m.emsg_.parse((char *)&m.mdat_box_.box_data_[index], (unsigned int)l_size);
										m.e_msg_is_in_mdat_ = true;
										index += l_size;
										continue;
									}
									if (enc_box_name.compare("embe") == 0)
									{
										index += l_size;
										continue;
									}
									break; // skip on all other mdat content only embe and emsg allowed
								}

								media_fragment_.push_back(m);

								break;
							}
						}
					}
					if (it->box_type_.compare("mfra") == 0)
					{
						this->mfra_box_ = *it;
					}
					if (it->box_type_.compare("sidx") == 0)
					{
						this->sidx_box_ = *it;
						std::cout << "|sidx|";
					}
					if (it->box_type_.compare("meta") == 0)
					{
						this->meta_box_ = *it;
						std::cout << "|meta|";
					}
				}
			}
			std::cout << std::endl;
			std::cout << "***  finished reading fmp4 fragments  ***" << std::endl;
			std::cout << "***  read  fmp4 init fragment         ***" << std::endl;
			std::cout << "***  read " << media_fragment_.size() << " fmp4 media fragments ***" << std::endl;

			return 0;
		}
		catch (std::exception e)
		{
			std::cout << e.what() << std::endl;
			return 0;
		}
	}

	// archival function, write init segment to a file
	int ingest_stream::write_init_to_file(std::string &ofile, unsigned int nfrags)
	{
		// write the stream to an output file
		std::ofstream out_file(ofile, std::ofstream::binary);

		if (out_file.good())
		{
			std::vector<uint8_t> init_data;
			get_init_segment_data(init_data);
			out_file.write((char *)init_data.data(), init_data.size());
			for (unsigned int k = 0; k < nfrags; k++) {
				if (k < media_fragment_.size()) {
					init_data.clear();
					get_media_segment_data(k, init_data);
					out_file.write((char *)init_data.data(), init_data.size());
				}
			}
			out_file.close();
			std::cout << " done written init segment to file: " << ofile << std::endl;
		}
		else
		{
			std::cout << " error writing stream to file " << std::endl;
		}

		return 0;
	}

	// carefull only use with the testes=d pre-encoded moov boxes to write streams
	bool setTrackID(std::vector<uint8_t> &moov_in, uint32_t track_id)
	{
		bool set_tkhd = false;
		for (std::size_t k = 0; k < moov_in.size() - 16; k++)
		{
			if (std::string((char *)&moov_in[k]).compare("tkhd") == 0)
			{
				//cout << "tkhd found" << endl;
				if ((uint8_t)moov_in[k + 4] == 1)
				{
					// version 1 
					fmp4_write_uint32(track_id, (char*)&moov_in[k + 24]);
					set_tkhd = true;
				}
				else {
					// version 0
					fmp4_write_uint32(track_id, (char*)&moov_in[k + 16]);
					set_tkhd = true;
				}
			}
			if (std::string((char *)&moov_in[k]).compare("trex") == 0)
			{
				//cout << "tkhd found" << endl;
				if ((uint8_t)moov_in[k + 4] == 1)
				{
					// version 1 
					fmp4_write_uint32(track_id, (char*)&moov_in[k + 24]);
					set_tkhd = true;
				}
				else {
					// version 0
					fmp4_write_uint32(track_id, (char*)&moov_in[k + 16]);
					set_tkhd = true;
				}
			}
		}
		return true;
	}

	// carefull only use with the tested pre-encoded moov boxes to write streams and update the urn in them
	void setSchemeURN(std::vector<uint8_t> &moov_in, const std::string& urn)
	{
		int32_t size_diff = 0;

		std::vector<uint8_t> l_first;
		std::vector<uint8_t> l_last;

		// find the uri box containing the description of the urn, caerfull only use for the enclosed mdat box in the source code
		for (std::size_t k = 0; k < moov_in.size() - 16; k++)
		{
			if (std::string((char *)&moov_in[k]).compare("urim") == 0)
			{
				//cout << "urim box found" << endl;
				std::string or_urn = std::string((char *)&moov_in[k + 24]);
				size_diff = (int32_t)(or_urn.size() - urn.size());

				if (size_diff == 0)
				{
					for (std::size_t l = 0; l < urn.size(); l++)
						moov_in[k + 24 + l] = urn[l];
				}
				else
				{
					// first part of the string
					l_first = std::vector<uint8_t>(moov_in.begin(), moov_in.begin() + k + 24);
					// last part of the string
					l_last = std::vector<uint8_t>(moov_in.begin() + k + 24 + or_urn.size() + 1, moov_in.end());
				}
				break;
			}
		}

		// new string representing the moov_box 
		if (size_diff != 0)
		{
			// cout << "scheme size difference is: " << size_diff << endl;

			moov_in.resize(l_first.size());
			moov_in.reserve(moov_in.size() + urn.size() + l_last.size() + 1);

			for (std::size_t i = 0; i < urn.size(); i++)
				moov_in.push_back(urn[i]);
			moov_in.push_back('\0');
			for (std::size_t i = 0; i < l_last.size(); i++)
				moov_in.push_back(l_last[i]);

			for (std::size_t i = 0; i < moov_in.size(); i++)
			{
				if (std::string((char *)&moov_in[i]).compare("stsd") == 0)
				{
					uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
					or_size = (uint32_t)(or_size - size_diff);
					fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
				}
				if (std::string((char *)&moov_in[i]).compare("stbl") == 0)
				{
					uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
					or_size = (uint32_t)(or_size - size_diff);
					fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
				}
				if (std::string((char *)&moov_in[i]).compare("uri ") == 0)
				{
					uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
					or_size = (uint32_t)(or_size - size_diff);
					fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
				}
				if (std::string((char *)&moov_in[i]).compare("urim") == 0)
				{
					uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
					or_size = (uint32_t)(or_size - size_diff);
					fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
				}
				if (std::string((char *)&moov_in[i]).compare("minf") == 0)
				{
					uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
					or_size = (uint32_t)(or_size - size_diff);
					fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
				}
				if (std::string((char *)&moov_in[i]).compare("mdia") == 0)
				{
					uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
					or_size = (uint32_t)(or_size - size_diff);
					fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
				}
				if (std::string((char *)&moov_in[i]).compare("trak") == 0)
				{
					uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
					or_size = (uint32_t)(or_size - size_diff);
					fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
				}
				if (std::string((char *)&moov_in[i]).compare("moov") == 0)
				{
					uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
					or_size = (uint32_t)(or_size - size_diff);
					fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
				}
			}
		}
	};


	bool get_sparse_moov(const std::string& urn, uint32_t timescale, uint32_t track_id, std::vector<uint8_t> &sparse_moov)
	{
		sparse_moov = base64_decode(moov_64_enc);
		setTrackID(sparse_moov, track_id);
		if (urn.size())
			setSchemeURN(sparse_moov, urn);

		// write back the timescale mvhd
		fmp4_write_uint32(timescale, (char *)&sparse_moov[28]);

		// mdhd 
		fmp4_write_uint32(timescale, (char *)&sparse_moov[244]);

		return true;
	};

	void emsg::convert_emsg_to_sparse_fragment(std::vector<uint8_t> &sparse_frag_out, uint32_t tfdt, uint32_t track_id, uint32_t timescale,  uint8_t target_emsg_version)
	{
		std::ostringstream res(std::ios::binary);
		sparse_frag_out.clear();
		this->write_emsg_as_fmp4_fragment(res, tfdt, track_id, tfdt + 4 * timescale_, 0);
		std::string r = res.str();
		for (int i = 0; i < r.size(); i++)
			sparse_frag_out.push_back(r[i]);
		return;
	};

	// writes sparse emsg file, set the track, the scheme
	int ingest_stream::write_to_sparse_emsg_file(const std::string& out_file,
		uint32_t track_id, uint32_t announce, const std::string& urn, uint32_t timescale, uint8_t target_emsg_version)
	{
		//ifstream moov_s_in("sparse_moov.inc", ios::binary);

		std::vector<uint8_t> sparse_moov = base64_decode(moov_64_enc);
		setTrackID(sparse_moov, track_id);
		if (urn.size())
			setSchemeURN(sparse_moov, urn);

		// write back the timescale mvhd
		fmp4_write_uint32(timescale, (char *)&sparse_moov[28]);

		// mdhd 
		fmp4_write_uint32(timescale, (char *)&sparse_moov[244]);

		std::fstream ot(out_file, std::ios::binary);
		//cout << sparse_moov.size() << endl;

		if (ot.good())
		{
			// write the ftyp header
			ot.write((char *)&sparse_ftyp[0], 20);
			ot.write((const char *)&sparse_moov[0], sparse_moov.size());

			// write each of the event messages as moof mdat combinations in sparse track 
			for (auto it = this->media_fragment_.begin(); it != this->media_fragment_.end(); ++it)
			{
				//it->print();
				if (it->emsg_.scheme_id_uri_.size())
				{
					uint64_t next_tfdt = 0;
					//find the next tdft 
					if ((it + 1) != this->media_fragment_.end())
						next_tfdt = (it + 1)->tfdt_.base_media_decode_time_;
					//cout << " writing emsg fragment " << endl;
					it->emsg_.write_emsg_as_fmp4_fragment(ot, it->tfdt_.base_media_decode_time_, track_id, next_tfdt, target_emsg_version);
				}
			}

			//ot.write((const char *)empty_mfra, 8);
			ot.close();
			std::cout << "*** wrote sparse track file: " << out_file << "  ***" << std::endl;
		}
		else
			std::cout << "error" << std::endl;
		return 0;
	};

	//  
	void ingest_stream::write_to_dash_event_stream(std::string &out_file)
	{
		std::ofstream ot(out_file);


		if (ot.good()) {

			uint32_t time_scale = init_fragment_.get_time_scale();
			std::string scheme_id_uri = "";

			if (media_fragment_.size() > 0)
				scheme_id_uri = media_fragment_[0].emsg_.scheme_id_uri_;

			ot << "<EventStream " << std::endl;
			if (scheme_id_uri.compare("urn:scte:scte35:2013:bin") == 0) // convert binary scte 214 to xml + bin
			{
				ot << "schemeIdUri=" << '"' << "urn:scte:scte35:2014:xml+bin" << '"' << std::endl;
			}
			else {
				ot << "schemeIdUri=" << '"' << scheme_id_uri << '"' << std::endl;
			}
			ot << " timescale=" << '"' << time_scale << '"' << ">" << std::endl;

			// write each of the event messages as moof mdat combinations in sparse track 
			for (auto it = this->media_fragment_.begin(); it != this->media_fragment_.end(); ++it)
			{
				//it->print();
				if (it->emsg_.scheme_id_uri_.size())
				{
					uint64_t l_presentation_time = it->emsg_.version_ ? it->emsg_.presentation_time_ : it->tfdt_.base_media_decode_time_ + it->emsg_.presentation_time_delta_;
					it->emsg_.write_emsg_as_mpd_event(ot, it->tfdt_.base_media_decode_time_);
				}
			}

			ot << "</EventStream> " << std::endl;
		}
		ot.close();
	}

	// dump the contents of the sparse track to screen
	void ingest_stream::print() const
	{
		for (auto it = media_fragment_.begin(); it != media_fragment_.end(); it++)
		{
			it->print();
		}
	}

	// method to patch all timestamps from a VoD (0) base to live (epoch based)
	void ingest_stream::patch_tfdt(uint64_t time_anchor, bool applytimescale)
	{
		if (applytimescale) {
			uint32_t timescale = 0;
			timescale = this->init_fragment_.get_time_scale();
			if(timescale)
			   time_anchor = time_anchor * timescale; // offset to add to the timestamps
		}

		uint32_t seq_nr = 0;
		
		if (media_fragment_.size())
			seq_nr = 1 + this->media_fragment_[media_fragment_.size()-1].mfhd_.seq_nr_;

		for (uint32_t i = 0; i < this->media_fragment_.size(); i++)
		{
			if (seq_nr)
				this->media_fragment_[i].patch_tfdt(time_anchor, seq_nr + i);
			else
				this->media_fragment_[i].patch_tfdt(time_anchor);
		}	
	}

	uint64_t ingest_stream::get_duration() 
	{
		if (media_fragment_.size() > 2) 
		{
			// todo add aditional checks for timing behavior
			return media_fragment_[media_fragment_.size() - 1].tfdt_.base_media_decode_time_
				- media_fragment_[0].tfdt_.base_media_decode_time_
				+ media_fragment_[media_fragment_.size() - 1].get_duration();
		}
		if (media_fragment_.size() == 1)
		{
			// todo add aditional checks for timing behavior
			return media_fragment_[media_fragment_.size() - 1].get_duration();
		}
		return 0;
	}
	uint64_t ingest_stream::get_start_time() 
	{
		if (media_fragment_.size() > 1)
		{
			// todo add aditional checks for timing behavior
			return media_fragment_[0].tfdt_.base_media_decode_time_;
		}
		return 0;
	}
}
