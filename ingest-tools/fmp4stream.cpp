/*******************************************************************************
Supplementary software media ingest specification: https://github.com/unifiedstreaming/fmp4-ingest

Copyright (C) 2009-2018 CodeShop B.V.
http://www.code-shop.com

******************************************************************************/


#include "fmp4stream.h"
#include <iostream>
#include <stdint.h>
#include <iomanip> 
#include <memory>
#include <base64.h>

using namespace fMP4Stream;


// base 64 sparse movie header
string moov_64_enc("AAACNG1vb3YAAABsbXZoZAAAAAAAAAAAAAAAAAAAAAEAAAAAAAEAAAEAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAIAAAGYdHJhawAAAFx0a2hkAAAABwAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAABNG1kaWEAAAAgbWRoZAAAAAAAAAAAAAAAAAAAAAEAAAAAVcQAAAAAADFoZGxyAAAAAAAAAABtZXRhAAAAAAAAAAAAAAAAVVNQIE1ldGEgSGFuZGxlcgAAAADbbWluZgAAAAxubWhkAAAAAAAAACRkaW5mAAAAHGRyZWYAAAAAAAAAAQAAAAx1cmwgAAAAAQAAAKNzdGJsAAAAV3N0c2QAAAAAAAAAAQAAAEd1cmltAAAAAAAAAAEAAAA3dXJpIAAAAABodHRwOi8vd3d3LnVuaWZpZWQtc3RyZWFtaW5nLmNvbS9kYXNoL2Vtc2cAAAAAEHN0dHMAAAAAAAAAAAAAABBzdHNjAAAAAAAAAAAAAAAUc3RzegAAAAAAAAAAAAAAAAAAABBzdGNvAAAAAAAAAAAAAAAobXZleAAAACB0cmV4AAAAAAAAAAEAAAABAAAAAAAAAAAAAAAA");


//constexpr moov_chars

void box::parse(char * ptr)
{
	m_size = fmp4_read_uint32(ptr);
	char name[5] = { ptr[4],ptr[5],ptr[6],ptr[7],'\0' };
	m_btype = string(name);
	if (m_size == 1) {
		m_largesize = fmp4_read_uint64(ptr + 8);
	}
}

uint64_t box::size() {
	uint64_t l_size = 8;
	if (m_is_large)
		l_size += 8;
	if (m_has_uuid)
		l_size += 16;
	return l_size;
};

void box::print()
{
	cout << "=================" << m_btype << "==================" << endl;
	cout << setw(33) << left << " box size: " << m_size << endl;
}

void mvhd::parse(char *ptr)
{
	this->m_version = (unsigned int) ptr[8];
	if (m_version == 1)
		this->m_time_scale = fmp4_read_uint32(ptr + 12 + 8 + 8);
	else
		this->m_time_scale = fmp4_read_uint32(ptr + 12 + 8);

	return;
}

bool box::read(istream *istr)
{
	m_box_data.resize(9);
	m_largesize = 0;

	istr->read((char*)&m_box_data[0], 8);
	m_size = fmp4_read_uint32((char *)&m_box_data[0]);
	m_box_data[8] = (uint8_t) '\0';
	m_btype = string((char *)&m_box_data[4]);
	if (m_btype.compare("uuid") == 0)
		m_has_uuid = true;

	// detect large size box 
	if (m_size == 1)
	{
		m_is_large = true;
		m_box_data.resize(16);
		istr->read((char *)&m_box_data[8], 8);
		m_largesize = fmp4_read_uint64((char *)&m_box_data[8]);
	}
	else {
		m_largesize = m_size;
		m_box_data.resize(8);
	}

	// read all the bytes from the stream
	if (m_size) {
		// write the offset bytes of the box
		uint8_t offset = (uint8_t)m_box_data.size();
		m_box_data.resize(m_largesize);
		istr->read((char*)&m_box_data[offset], m_largesize - offset);
		return true;
	}

	return false;
};

void full_box::parse(char *ptr)
{
	box::parse(ptr);
	// read the version and flags
	uint64_t offset = box::size();
	m_magic_conf = fmp4_read_uint32((char *)(ptr + offset));
	this->m_version = *((uint8_t *)((ptr + offset)));
	this->m_flags = 0x00FFFFFF & fmp4_read_uint32((char *)(ptr + offset));
}

void full_box::print()
{
	box::print();
	cout << setw(33) << left << "version: " << this->m_version << endl;
	cout << setw(33) << left << "flags: " << m_flags << endl;
}

void mfhd::parse(char * ptr)
{
	m_seq_nr = fmp4_read_uint32(ptr + 12);
}

void mfhd::print() {
	cout << "=================mfhd==================" << endl;
	cout << setw(33) << left << " sequence number is: " << m_seq_nr << endl;
};

void tfhd::parse(char * ptr)
{
	full_box::parse(ptr);
	m_track_id = fmp4_read_uint32((char *)(ptr + 12));
	//cout << "track_id " << track_id << endl;
	m_base_data_offset_present = !!(0x00000001 & m_flags);
	//cout << "base_data_offset_present " << base_data_offset_present << endl;
	m_sample_description_index_present = !!(0x00000002 & m_flags);
	//cout << "sample_description_index_present " << sample_description_index_present << endl;
	m_default_sample_duration_present = !!(0x00000008 & m_flags);
	//cout << "default_sample_duration_present " << default_sample_duration_present << endl;
	m_default_sample_size_present = !!(0x00000010 & m_flags);
	//cout << "default_sample_size_present " << default_sample_size_present << endl;
	m_default_sample_flags_present = !!(0x00000020 & m_flags);
	//cout << "default_sample_flags_present " << default_sample_flags_present << endl;
	m_duration_is_empty = !!(0x00010000 & m_flags);
	m_default_base_is_moof = !!(0x00020000 & m_flags);

	unsigned int offset = 16;

	if (m_base_data_offset_present)
	{
		m_base_data_offset = fmp4_read_uint32((char *)(ptr + offset));
		offset += 4;
	}
	if (m_sample_description_index_present)
	{
		m_sample_description_index = fmp4_read_uint32((char *)(ptr + offset));
		offset += 4;
	}
	if (m_default_sample_duration_present)
	{
		m_default_sample_duration = fmp4_read_uint32((char *)(ptr + offset));
		offset += 4;
	}
	if (m_default_sample_size_present)
	{
		m_default_sample_size = fmp4_read_uint32((char *)(ptr + offset));
		offset += 4;
	}
	if (m_default_sample_flags_present)
	{
		m_default_sample_flags = (uint32_t)fmp4_read_uint32(ptr + offset);
		offset += 4;
	}
}

uint64_t tfhd::size()
{
	uint64_t l_size = full_box::size() + 4;
	if (m_base_data_offset_present)
		l_size += 8;
	if (m_sample_description_index)
		l_size += 4;
	if (m_default_sample_duration_present)
		l_size += 4;
	if (m_default_sample_size_present)
		l_size += 4;
	if (m_default_sample_flags_present)
		l_size += 4;

	return l_size;
};
void tfhd::print() {
	cout << "=================tfhd==================" << endl;
	//cout << setw(33) << left << " magic conf                 " << m_magic_conf << endl;
	cout << setw(33) << left << " vflags                  " << m_flags << endl;
	cout << setw(33) << left << " track id:                  " << m_track_id << endl;
	if (m_base_data_offset_present)
		cout << setw(33) << left << " base_data_offset: " << m_base_data_offset << endl;
	if (m_sample_description_index_present)
		cout << setw(33) << left << " sample description: " << m_sample_description_index << endl;
	if (m_default_sample_duration_present)
		cout << setw(33) << left << " sample duration: " << m_default_sample_duration << endl;
	if (m_default_sample_size_present)
		cout  << setw(33) << left << " default sample size: " << m_default_sample_size << endl;
	if (m_default_sample_flags_present)
		cout << setw(33) << left << " default sample flags:  " << m_default_sample_flags << endl;
	cout << setw(33) << left << " duration is empty " << m_duration_is_empty << endl;
	cout << setw(33) << left << " default base is moof" << m_default_base_is_moof << endl;
	//cout << "............." << std::endl;
};

uint64_t tfdt::size()
{
	return m_version ? full_box::size() + 8 : full_box::size() + 4;
};

void tfdt::print() {
	cout << "=================tfdt==================" << endl;
	cout << setw(33) << left << " basemediadecodetime: " << m_basemediadecodetime << endl;
};

void tfdt::parse(char* ptr)
{
	full_box::parse(ptr);
	m_basemediadecodetime = m_version ? \
		fmp4_read_uint64((char *)(ptr + 12)) : \
		fmp4_read_uint32((char *)(ptr + 12));

};



uint64_t trun::size()
{
	uint64_t l_size = full_box::size() + 4;
	if (m_data_offset_present)
		l_size += 4;
	if (m_first_sample_flags_present)
		l_size += 4;
	if (m_sample_duration_present)
		l_size += (m_sample_count * 4);
	if (m_sample_size_present)
		l_size += (m_sample_count * 4);
	if (m_sample_flags_present)
		l_size += (m_sample_count * 4);
	if (this->m_sample_composition_time_offsets_present)
		l_size += (m_sample_count * 4);
	return l_size;
};

void trun::print()
{
	cout << "==================trun=================" << endl;
	cout << setw(33) << left << " magic conf                 " << m_magic_conf << endl;
	cout << setw(33) << left << " sample count:      " << m_sample_count << endl;
	if (m_data_offset_present)
		cout << setw(33) << left << " data_offset:        " << m_data_offset << endl;
	if (m_first_sample_flags_present)
		cout << " first sample flags:        " << m_first_sample_flags << endl;
	cout << setw(33) << left << " first sample flags present:  " << m_first_sample_flags_present << endl;
	//if(sample_duration_present)
	cout << setw(33) << left << " sample duration present: " << m_sample_duration_present << endl;
	//if(sample_size_present)
	cout << setw(33) << left << " sample_size_present: " << m_sample_size_present << endl;
	//if(sample_flags_present)
	cout << setw(33) << left << " sample_flags_present: " << m_sample_flags_present << endl;
	//if(sample_composition_time_offsets_present)
	cout << setw(33) << left << " sample_ct_offsets_present: " << m_sample_composition_time_offsets_present << endl;

	/*
	cout << " flags " << sample_count << endl;
	cout << " data_offset_present: " << data_offset_present << endl;;
	cout << "first_sample_flags_present: " << first_sample_flags_present << endl;;
	cout << "sample_duration_present: " << sample_duration_present << endl;;
	cout << "sample_size_present: " << sample_size_present << endl;;
	cout << "sample_flags_present: " << sample_flags_present << endl;;
	cout << "sample_composition_time_offsets_present: " << sample_composition_time_offsets_present<< endl;*/
	for (unsigned int i = 0; i < m_sentry.size(); i++)
		m_sentry[i].print();
};

void trun::parse(char * ptr)
{
	full_box::parse(ptr);

	m_sample_count = fmp4_read_uint32((char *)(ptr + 12));

	m_data_offset_present = !!(0x00000001 & m_flags);
	//cout << "data_offset_present " << data_offset_present << endl;
	m_first_sample_flags_present = !!(0x00000004 & m_flags);
	//cout << "first_sample_flags_present " << first_sample_flags_present << endl;
	m_sample_duration_present = !!(0x00000100 & m_flags);
	//cout << "sample_duration_present " << sample_duration_present << endl;
	m_sample_size_present = !!(0x00000200 & m_flags);
	//cout << "sample_size_present " << sample_size_present << endl;
	m_sample_flags_present = !!(0x00000400 & m_flags);
	m_sample_composition_time_offsets_present = !!(0x00000800 & m_flags);

	//sentry.resize(sample_count);
	unsigned int offset = 16;
	if (m_data_offset_present)
	{
		this->m_data_offset = fmp4_read_uint32((char *)(ptr + offset));
		offset += 4;
	}if (m_first_sample_flags_present) {
		this->m_first_sample_flags = fmp4_read_uint32((char *)(ptr + offset));
		offset += 4;
	}
	// write all the entries to the trun box
	for (unsigned int i = 0; i < m_sample_count; i++)
	{
		sample_entry ent = {};
		if (m_sample_duration_present)
		{
			ent.m_sample_duration = fmp4_read_uint32((char *)(ptr + offset));
			offset += 4;
		}
		if (m_sample_size_present)
		{
			ent.m_sample_size = fmp4_read_uint32((char *)(ptr + offset));
			offset += 4;
		}
		if (m_sample_flags_present)
		{
			ent.m_sample_flags = fmp4_read_uint32((char *)(ptr + offset));
			offset += 4;
		}
		if (m_sample_composition_time_offsets_present)
		{
			if (m_version == 0)
				ent.m_sample_composition_time_offset_v0 = fmp4_read_uint32((char *)(ptr + offset));
			else
				ent.m_sample_composition_time_offset_v1 = fmp4_read_uint32((char *)(ptr + offset));
			offset += 4;
		}
		m_sentry.push_back(ent);
	}
}

// duration from the trun box
uint64_t media_fragment::get_duration()
{
	uint64_t duration = 0;

	for (unsigned int i = 0; i != m_trun.m_sample_count; ++i)
	{
		uint32_t sample_duration = m_tfhd.m_default_sample_duration;
		if (m_trun.m_sample_duration_present)
		{
			sample_duration = m_trun.m_sentry[i].m_sample_duration;
		}

		duration += sample_duration;
	}
	return duration;
}

void sc35_splice_info::print(bool verbose )
{
	if (verbose) {
		cout << setw(33) << left << " table id: " << (unsigned int)m_table_id << endl;
		cout << setw(33) << left << " section_syntax_indicator: " << m_section_syntax_indicator << endl;
		cout << setw(33) << left << " private_indicator: " << m_private_indicator << endl;
		cout << setw(33) << left << " section_length: " << (unsigned int)m_section_length << endl;
		cout << setw(33) << left << " protocol_version: " << (unsigned int)m_protocol_version << endl;
		cout << setw(33) << left << " encrypted_packet: " << m_encrypted_packet << endl;
		cout << setw(33) << left << " encryption_algorithm: " << (unsigned int)m_encryption_algorithm << endl;
		cout << setw(33) << left << " pts_adjustment: " << m_pts_adjustment << endl;
		cout << setw(33) << left << " cw_index: " << (unsigned int)m_cw_index << endl;
		cout << setw(33) << left << " tier: " << m_tier << endl;
		cout << setw(33) << left << " splice_command_length: " << m_splice_command_length << endl;
		cout << setw(33) << left << " splice_command_type: " << (unsigned int)m_splice_command_type << endl;
		cout << setw(33) << left << " descriptor_loop_length: " << m_descriptor_loop_length << endl;
	}
	cout << setw(33) << left << " pts_adjustment: " << m_pts_adjustment << endl;
	cout << setw(33) << left << " Command type: ";

	switch (m_splice_command_type)
	{
	case 0x00: cout << 0x00; // prints "1"
		cout << setw(33) << left << " splice null " << endl;
		break;       // and exits the switch
	case 0x04:
		cout << setw(33) << left << " splice schedule " << endl;
		break;
	case 0x05:
		cout << setw(33) << left << " splice insert " << endl;
		cout << setw(33) << left << " event_id: " << m_splice_insert_event_id << endl;
		cout << setw(33) << left << " cancel indicator: " << m_splice_event_cancel_indicator << endl;
		break;
	case 0x06:
		cout << setw(33) << left << "time signal " << endl;
		break;
	case 0x07:
		cout << setw(33) << left << "bandwidth reservation " << endl;
		break;
	}
}

void sc35_splice_info::parse(uint8_t *ptr, unsigned int size)
{
	unsigned int pos = 0;
	m_table_id = *ptr++;

	if (m_table_id != 0xFC)
	{
		cout << " error parsing table id, tableid != 0xFC " << endl;
		return;
	}
	//else
	//	cout << "table id ok " << endl;

	bitset<8> b(*ptr);
	m_section_syntax_indicator = b[7];
	m_private_indicator = b[6];
	m_section_length = 0x0FFF & fmp4_read_uint16((char *)ptr);
	ptr += 2;
	m_protocol_version = (uint8_t)*ptr++;

	b = bitset<8>(*ptr);

	m_encrypted_packet = b[7];
	bool pts_highest_bit = b[0];

	b[0] = 0;
	b[7] = 0;
	m_encryption_algorithm = uint8_t(b.to_ullong() >> 1);
	ptr++;

	m_pts_adjustment = (uint64_t)fmp4_read_uint32((char *)ptr);
	if (pts_highest_bit)
	{
		m_pts_adjustment += (uint64_t)numeric_limits<uint32_t>::max();
	}
	ptr = ptr + 4;
	m_cw_index = *ptr++;

	uint32_t wd = fmp4_read_uint32((char *)ptr);
	bitset<32> a(wd);
	m_tier = (wd & 0xFFF00000) >> 20;
	m_splice_command_length = (0x000FFF00 & wd) >> 8;
	m_splice_command_type = (uint8_t)(0x000000FF & wd);
	ptr += 4;
	m_descriptor_loop_length = fmp4_read_uint16((char *)ptr);

	ptr += m_descriptor_loop_length;
	if (m_splice_command_type == 0x05) {
		m_splice_insert_event_id = fmp4_read_uint32((char *)ptr);
		ptr += 4;
		bitset<8> bb(*ptr);
		m_splice_event_cancel_indicator = bb[7];
	}
	// we don't support the cancel indicator yet (it should be added)
}


uint64_t emsg::size()
{
	uint64_t l_size = full_box::size();
	if (m_version == 1)
		l_size += 4;
	l_size += 16;
	l_size += m_scheme_id_uri.size() + 1;
	l_size += m_value.size() + 1;
	l_size += m_message_data.size();
	return l_size;
}

//
void emsg::parse(char * ptr, unsigned int data_size)
{
	full_box::parse(ptr);
	unsigned int offset = full_box::size();
	if (m_version == 0)
	{
		m_scheme_id_uri = string((ptr + offset));
		//cout << "scheme_id_uri: " << scheme_id_uri << endl;
		offset = offset + m_scheme_id_uri.size() + 1;
		m_value = string((ptr + offset));
		//cout << "value: " << value << endl;
		offset = offset + m_value.size() + 1;
		m_timescale = fmp4_read_uint32(ptr + offset);
		offset += 4;
		//cout << "timescale: " << timescale << endl;
		m_presentation_time_delta = fmp4_read_uint32(ptr + offset);
		offset += 4;
		//cout << "presentation_time_delta: " << presentation_time_delta << endl;
		m_event_duration = fmp4_read_uint32(ptr + offset);
		offset += 4;
		//cout << "event_duration: " << event_duration << endl;
		m_id = fmp4_read_uint32(ptr + offset);
		//cout << "id: " << id << endl;
		offset += 4;
	}
	else
	{
		m_timescale = fmp4_read_uint32(ptr + offset);
		//cout << "timescale: " << timescale << endl;
		offset += 4;
		m_presentation_time = fmp4_read_uint64(ptr + offset);
		//cout << "presentation time: " << presentation_time << endl;
		offset += 8;
		m_event_duration = fmp4_read_uint32(ptr + offset);
		//cout << "event duration: " << event_duration << endl;
		offset += 4;
		m_id = fmp4_read_uint32(ptr + offset);
		offset += 4;
		//cout << "id: " << id << endl;
		m_scheme_id_uri = string((ptr + offset));
		offset = offset + m_scheme_id_uri.size() + 1;
		//cout << "scheme_id_uri: " << scheme_id_uri << endl;
		m_value = string((ptr + offset));
		//cout << "value: " << value << endl;
		offset = offset + m_value.size() + 1;
	}
	for (unsigned int i = offset; i < data_size; i++)
	{
		m_message_data.push_back(*(ptr + offset));
		offset++;
	}

	//if (message_data[0] == 0xFC) {}
	//	cout << "program splice table detected " << endl;
}

void emsg::write_emsg_as_mpd_event(ostream *ostr, uint64_t base_time)
{
	*ostr << "<Event" << endl \
		<< "presentationTime=" << '"' << (this->m_version ? m_presentation_time : base_time + m_presentation_time_delta) << '"' << endl \
		<< "duration=" << '"' << m_event_duration << '"' << endl \
		<< "id=" << '"' << m_id << "'" << '>' << endl \
		<< base64_encode(this->m_message_data.data(), this->m_message_data.size()) << endl
		<< "</Event>" << endl;
}

//
void emsg::print()
{
	cout << "=================emsg==================" << endl;
	cout << setw(33) << left << " e-msg box version: " << (unsigned int)m_version << endl;
	cout << " scheme_id_uri:     " << m_scheme_id_uri << endl;
	cout << setw(33) << left << " value:             " << m_value << endl;
	cout << setw(33) << left << " timescale:         " << m_timescale << endl;
	if (m_version == 1)
		cout << setw(33) << left << " presentation_time: " << m_presentation_time << endl;
	else
		cout << setw(33) << left << " presentation_time_delta: " << m_presentation_time_delta << endl;
	cout << setw(33) << left << " event duration:    " << m_event_duration << endl;
	cout << setw(33) << left << " event id           " << m_event_duration << endl;
	cout << setw(33) << left << " message data size  " << m_message_data.size() << endl;

	//print_payload

	if (m_message_data[0] == 0xFC)
	{
		// splice table
		//cout << " scte splice info" << endl;
		sc35_splice_info l_splice;
		l_splice.parse((uint8_t*)&m_message_data[0], m_message_data.size());
		cout << "=============splice info==============" << endl;
		l_splice.print();
	}
}

//! write an emsg box to a stream
uint32_t emsg::write(ostream *ostr)
{
	uint32_t bytes_written = 0;
	uint32_t size = this->size();
	ostr->write((char *)&size, 4);
	bytes_written += 4;
	ostr->put('e');
	ostr->put('m');
	ostr->put('s');
	ostr->put('g');
	bytes_written += 4;
	ostr->put((uint8_t)m_version);
	ostr->put(0u);
	ostr->put(0u);
	ostr->put(0u);
	bytes_written += 4;
	char int_buf[4];
	char long_buf[8];
	if (m_version == 1)
	{
		fmp4_write_uint32(m_timescale, int_buf);
		ostr->write(int_buf, 4);
		bytes_written += 4;
		fmp4_write_uint64(m_presentation_time, long_buf);
		ostr->write(long_buf, 8);
		bytes_written += 8;
		fmp4_write_uint32(m_event_duration, int_buf);
		ostr->write(int_buf, 4);
		bytes_written += 4;
		fmp4_write_uint32(m_id, int_buf);
		ostr->write(int_buf, 4);
		bytes_written += 4;
		ostr->write(m_scheme_id_uri.c_str(), m_scheme_id_uri.size() + 1);
		bytes_written += m_scheme_id_uri.size() + 1;
		ostr->write(m_value.c_str(), m_value.size() + 1);
		bytes_written += m_value.size() + 1;
	}
	else
	{
		ostr->write(m_scheme_id_uri.c_str(), m_scheme_id_uri.size() + 1);
		bytes_written += m_scheme_id_uri.size() + 1;
		ostr->write(m_value.c_str(), m_value.size() + 1);
		bytes_written += m_value.size() + 1;
		fmp4_write_uint32(m_timescale, int_buf);
		ostr->write(int_buf, 4);
		bytes_written += 4;
		fmp4_write_uint32(m_presentation_time_delta, int_buf);
		ostr->write(int_buf, 4);
		bytes_written += 4;
		fmp4_write_uint32(m_event_duration, int_buf);
		ostr->write(int_buf, 4);
		bytes_written += 4;
		fmp4_write_uint32(m_id, int_buf);
		ostr->write(int_buf, 4);
		bytes_written += 4;
	}
	ostr->write((char *)&m_message_data[0], m_message_data.size());
	bytes_written += m_message_data.size();
	return bytes_written;
}

//! write an emsg message as a sparse fragment with advertisement time timestamp announce second before the application time
void emsg::write_emsg_as_fmp4_fragment(ostream *ostr, uint64_t timestamp_tdft, uint32_t track_id = 1,
	uint64_t next_tdft=0 /* announce n seconds in advance*/)
{
	if (m_scheme_id_uri.size())
	{
		cout << "*** writing emsg fragment scheme: " << m_scheme_id_uri << "***" << endl;

		//--------- carefull code still unpolished contains few assertions------------------

		// --- init mfhd
		mfhd l_mfhd = {};
		l_mfhd.m_seq_nr = 0;
		uint64_t l_mfhd_size = l_mfhd.size();
		
		// --- init tfhd
		tfhd l_tfhd = {};
		l_tfhd.m_magic_conf = 131106u;
		l_tfhd.m_track_id = track_id;
		l_tfhd.m_sample_description_index = 1u;
		l_tfhd.m_default_sample_flags = 37748800u;
		l_tfhd.m_default_sample_flags = 37748800u;
		l_tfhd.m_base_data_offset_present = false;
		l_tfhd.m_default_base_is_moof = true;
		l_tfhd.m_duration_is_empty = false;
		l_tfhd.m_sample_description_index_present = true;
		l_tfhd.m_default_sample_duration_present = false;
		l_tfhd.m_default_sample_flags_present = true;
		l_tfhd.m_default_sample_size_present = false;
		uint64_t l_tfhd_size = l_tfhd.size();

		// --- init tfdt
		tfdt l_tfdt = {};
		l_tfdt.m_version = 1;
		l_tfdt.m_basemediadecodetime = timestamp_tdft;
		uint64_t l_tfdt_size = l_tfdt.size(); // size should be 12 + 8 = 20

		// --- init trun
		trun l_trun = {};
		l_trun.m_magic_conf = 769u;
		l_trun.m_sample_count = 3;
		l_trun.m_data_offset_present = true;
		l_trun.m_first_sample_flags_present =false;
		l_trun.m_sample_duration_present = true;
		l_trun.m_sample_size_present = true;
		l_trun.m_sample_flags_present = false;
		l_trun.m_sample_composition_time_offsets_present = false;

		//-- init sentry in trun write 3 samples
		l_trun.m_sentry.resize(3);
		l_trun.m_sentry[0].m_sample_size = 0;
		l_trun.m_sentry[0].m_sample_duration = this->m_presentation_time_delta;   // announce in advance via basemediadecodetime (4 seconds in advance)
		l_trun.m_sentry[1].m_sample_size = (uint32_t)size();
		l_trun.m_sentry[1].m_sample_duration = this->m_event_duration;       // duration is 1 
		l_trun.m_sentry[2].m_sample_size = 0;
		// calculate the gap that needs to be filled up to the next tdft
		int32_t gap_duration = (int32_t) (next_tdft - timestamp_tdft - m_event_duration - m_presentation_time_delta);
		cout << "gap_dur" << gap_duration << endl;
		cout << "next_tdft" << next_tdft << endl;
		l_trun.m_sentry[2].m_sample_duration = next_tdft ? (gap_duration > 0 ? gap_duration :  0) : 0; // if unknown set the gap duration to zero


		//--- initialize the box sizes
		uint64_t l_trun_size = l_trun.size();
		uint64_t l_traf_size = 8 + l_trun_size + l_tfdt_size + l_tfhd_size;
		// warning computing the moof size not always accurate (don't know why), hardcoded the value
		uint64_t l_moof_size = 120; // 8 + l_traf_size + l_mfhd_size; // l_traf_size + 8 + l_mfhd_size;
		l_trun.m_data_offset = l_moof_size + 8;

		// write the fragment 
		char int_buf[4];
		char long_buf[8];

		//--- write the sparse fragment to a file stream
		// write 4 bytes
		fmp4_write_uint32((uint32_t)l_moof_size, int_buf);
		ostr->write(int_buf, 4);
		// write 4 bytes, total 8 bytes
		ostr->put('m');
		ostr->put('o');
		ostr->put('o');
		ostr->put('f');
		// write 16 bytes total 24 bytes
		fmp4_write_uint32((uint32_t)l_mfhd_size, int_buf);
		ostr->write(int_buf, 4);
		//ostr->write("mfhd", 4);
		ostr->put('m');
		ostr->put('f');
		ostr->put('h');
		ostr->put('d');
		fmp4_write_uint32((uint32_t)0u, int_buf);
		ostr->write(int_buf, 4);
		fmp4_write_uint32((uint32_t)l_mfhd.m_seq_nr, int_buf);
		ostr->write(int_buf, 4);

		// write traf 8 bytes total 32 bytes
		fmp4_write_uint32((uint32_t)l_traf_size, int_buf);
		ostr->write(int_buf, 4);
		//ostr->write("traf", 4);
		ostr->put('t');
		ostr->put('r');
		ostr->put('a');
		ostr->put('f');

		// write tfhd 24 bytes total 56 bytes
		fmp4_write_uint32((uint32_t)l_tfhd_size, int_buf);
		ostr->write(int_buf, 4);
		//ostr->write("tfhd", 4);
		ostr->put('t');
		ostr->put('f');
		ostr->put('h');
		ostr->put('d');

		fmp4_write_uint32((uint32_t)l_tfhd.m_magic_conf, int_buf);
		ostr->write(int_buf, 4);
		fmp4_write_uint32((uint32_t)l_tfhd.m_track_id, int_buf);
		ostr->write(int_buf, 4);
		fmp4_write_uint32((uint32_t)l_tfhd.m_sample_description_index, int_buf);
		ostr->write(int_buf, 4);
		fmp4_write_uint32((uint32_t)l_tfhd.m_default_sample_flags, int_buf);
		ostr->write(int_buf, 4);

		// write tfdt 20 bytes total 76 bytes
		fmp4_write_uint32((uint32_t)l_tfdt_size, int_buf);
		ostr->write(int_buf, 4);
		ostr->put('t');
		ostr->put('f');
		ostr->put('d');
		ostr->put('t');
		ostr->put(255u);
		ostr->put(255u);
		ostr->put(255u);
		ostr->put(255u);
		fmp4_write_uint64((uint64_t)l_tfdt.m_basemediadecodetime, long_buf);
		ostr->write(long_buf, 8);


		// write the trun box, 44 bytes total 120 bytes
		cout << "trun size release: " << l_trun_size << endl;
		fmp4_write_uint32((uint32_t)l_trun_size, int_buf);
		ostr->write(int_buf, 4);
		//ostr->write("trun", 4);
		ostr->put('t');
		ostr->put('r');
		ostr->put('u');
		ostr->put('n');
		fmp4_write_uint32((uint32_t)l_trun.m_magic_conf, int_buf);
		ostr->write(int_buf, 4);
		fmp4_write_uint32((uint32_t)l_trun.m_sample_count, int_buf);
		ostr->write(int_buf, 4);
		fmp4_write_uint32((uint32_t)l_trun.m_data_offset, int_buf);
		ostr->write(int_buf, 4);

		// write the duration and the sample size
		fmp4_write_uint32((uint32_t)l_trun.m_sentry[0].m_sample_duration, int_buf);
		ostr->write(int_buf, 4);
		fmp4_write_uint32((uint32_t)l_trun.m_sentry[0].m_sample_size, int_buf);
		ostr->write(int_buf, 4);
		fmp4_write_uint32((uint32_t)l_trun.m_sentry[1].m_sample_duration, int_buf);
		ostr->write(int_buf, 4);
		fmp4_write_uint32((uint32_t)l_trun.m_sentry[1].m_sample_size, int_buf);
		ostr->write(int_buf, 4);
		fmp4_write_uint32((uint32_t)l_trun.m_sentry[2].m_sample_duration, int_buf);
		ostr->write(int_buf, 4);
		fmp4_write_uint32((uint32_t)l_trun.m_sentry[2].m_sample_size, int_buf);
		ostr->write(int_buf, 4);

		uint32_t mdat_size = (uint32_t)size() + 8;
		fmp4_write_uint32(mdat_size, int_buf);
		ostr->write(int_buf, 4);
		ostr->put('m');
		ostr->put('d');
		ostr->put('a');
		ostr->put('t');

		// write the emsg as an mdat box
		uint32_t bw = write(ostr);
	
		// we are done writing the emsg message as a sparse fragment
	}
	return;
};

// carefull very naive version of parsing the timescale from the mvhd
uint32_t init_fragment::get_time_scale()
{
	if (m_moov_box.m_box_data.size() > 30) {
		char * ptr = (char *)m_moov_box.m_box_data.data();
		if (string((char *)(ptr + 12)).compare("mvhd") == 0)
		{
			unsigned int version = (unsigned int) ptr[16];
			return version ? fmp4_read_uint32(ptr + 36) : fmp4_read_uint32(ptr + 28);
		}
		else {
			cout << "provide mvhd in beginning of file for correct timescale" << endl;
			return 0;
		}
	}
	else {
		cout << "provide mvhd in beginning of file for correct timescale" << endl;
		return 0;
	}
}

// see what is in the fragment
void media_fragment::parse_moof()
{
	if (!m_moof_box.m_size)
		return;

	uint64_t box_size = 0;
	uint64_t offset = 8;

	while (m_moof_box.m_box_data.size() > offset)
	{
		unsigned int temp_off = 0;
		box_size = (uint64_t)fmp4_read_uint32((char *)&m_moof_box.m_box_data[offset]);
		uint8_t * ptr = &m_moof_box.m_box_data[0];
		char name[5] = { (char)ptr[offset + 4],(char)ptr[offset + 5],(char)ptr[offset + 6],(char)ptr[offset + 7],'\0' };

		if (box_size == 1) // the box_size is a large size (should not happen in fmp4)
		{
			temp_off = 8;
			box_size = fmp4_read_uint64((char *)& ptr[offset + temp_off]);
		}

		if (string(name).compare("mfhd") == 0)
		{
			m_mfhd.parse((char *)& ptr[offset + temp_off]);
			offset += (uint64_t)box_size;
			//cout << "mfhd size" << box_size << endl;
			continue;
		}

		if (string(name).compare("trun") == 0)
		{
			m_trun.parse((char *)& ptr[offset + temp_off]);
			offset += (uint64_t)box_size;
			continue;
		}

		if (string(name).compare("tfdt") == 0)
		{
			m_tfdt.parse((char *)& ptr[offset + temp_off]);
			offset += (uint64_t)box_size;
			continue;
		}

		if (string(name).compare("tfhd") == 0)
		{
			m_tfhd.parse((char *)& ptr[offset + temp_off]);
			offset += (uint64_t)box_size;
			continue;
		}

		// cmaf style only one traf box per moof so we skip it
		if (string(name).compare("traf") == 0)
		{
			offset += 8;
		}
		else
			offset += (unsigned int)box_size;
	}
};

// function to support the ingest
uint64_t ingest_stream::get_init_segment_data(std::vector<uint8_t> &init_seg_dat)
{
	uint64_t ssize = m_init_fragment.m_ftyp_box.m_largesize + m_init_fragment.m_moov_box.m_largesize;
	init_seg_dat.resize(ssize);

	// hard copy the init segment data to the output vector
	copy(m_init_fragment.m_ftyp_box.m_box_data.begin(), m_init_fragment.m_ftyp_box.m_box_data.end(), init_seg_dat.begin());
	copy(m_init_fragment.m_moov_box.m_box_data.begin(), m_init_fragment.m_moov_box.m_box_data.end(), init_seg_dat.begin() + m_init_fragment.m_ftyp_box.m_largesize);

	return ssize;
};

// function to support the ingest of segments 
uint64_t ingest_stream::get_media_segment_data(long index, std::vector<uint8_t> &media_seg_dat)
{
	if (!(m_media_fragment.size() > index))
		return 0;

	uint64_t ssize = m_media_fragment[index].m_moof_box.m_largesize + m_media_fragment[index].m_mdat_box.m_largesize;
	media_seg_dat.resize(ssize);

	// hard copy the init segment data to the output vector
	copy(m_media_fragment[index].m_moof_box.m_box_data.begin(), m_media_fragment[index].m_moof_box.m_box_data.end(), media_seg_dat.begin());
	copy(m_media_fragment[index].m_mdat_box.m_box_data.begin(), m_media_fragment[index].m_mdat_box.m_box_data.end(), media_seg_dat.begin() + m_media_fragment[index].m_moof_box.m_largesize);

	return ssize;
};

//
void media_fragment::print()
{
	if (m_emsg.m_scheme_id_uri.size() && !this->e_msg_is_in_mdat) {
		m_emsg.print();
	}
	m_moof_box.print(); // moof box size
	m_mfhd.print(); // moof box header
	m_tfhd.print(); // track fragment header
	m_tfdt.print(); // track fragment decode time
	m_trun.print(); // trun box
	m_mdat_box.print(); // mdat 
	if (m_emsg.m_scheme_id_uri.size() && this->e_msg_is_in_mdat) {
		m_emsg.print();
	}

}

// parse an fmp4 file for media ingest
int ingest_stream::load_from_file(istream *infile)
{
	try
	{
		if (infile)
		{
			vector<box> ingest_boxes;

			while (infile->good()) // read box by box in a vector
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
				if (b.m_btype.compare("mfra") == 0)
					break;
			}

			// organize the boxes in init fragments and media fragments 
			for (auto it = ingest_boxes.begin(); it != ingest_boxes.end(); ++it)
			{
				if (it->m_btype.compare("ftyp") == 0)
				{
					//cout << "|ftyp|";
					m_init_fragment.m_ftyp_box = *it;
				}
				if (it->m_btype.compare("moov") == 0)
				{
					//cout << "|moov|";
					m_init_fragment.m_moov_box = *it;
				}

				if (it->m_btype.compare("moof") == 0) // in case of moof box we push both the moof and following mdat
				{
					media_fragment m = {};
					m.m_moof_box = *it;
					bool mdat_found = false;
					auto prev_box = (it - 1);
					// see if there is an emsg before
					if (prev_box->m_btype.compare("emsg") == 0)
					{
						m.m_emsg.parse((char *)& prev_box->m_box_data[0], prev_box->m_box_data.size());
						//cout << "|emsg|";
						cout << "found inband dash emsg box" << std::endl;
					}
					//cout << "|moof|";
					while (!mdat_found)
					{
						it++;
						if (it->m_btype.compare("mdat") == 0)
						{
							m.m_mdat_box = *it;
							mdat_found = true;
							m.parse_moof();


							if (m.m_mdat_box.m_size > 8) {
								uint8_t name[9] = {};
								for (int i = 0; i < 8; i++)
									name[i] = m.m_mdat_box.m_box_data[i + 8];
								name[8] = '\0';
								string enc_box_name((char *)&name[4]);
								if (enc_box_name.compare("emsg") == 0)
								{
									m.m_emsg.parse((char *)&m.m_mdat_box.m_box_data[8], m.m_mdat_box.m_box_data.size() - 8);
									m.e_msg_is_in_mdat = true;
								}
							}

							//cout << "mdat size is " << it->m_size << std::endl;
							m_media_fragment.push_back(m);
							//cout << "|mdat|";
							break;
						}
					}
				}
				if (it->m_btype.compare("mfra") == 0)
				{
					this->m_mfra_box = *it;
				}
				if (it->m_btype.compare("sidx") == 0)
				{
					this->m_sidx_box = *it;
					cout << "|sidx|";
				}
				if (it->m_btype.compare("meta") == 0)
				{
					this->m_meta_box = *it;
					cout << "|meta|";
				}
			}
		}
		cout << endl;
		cout << "***  finished reading fmp4 fragments  ***" << endl;
		cout << "***  read  fmp4 init fragment         ***" << endl;
		cout << "***  read " << m_media_fragment.size() << " fmp4 media fragments ***" << endl;

		return 1;
	}
	catch (std::exception e)
	{
		cout << e.what() << std::endl;
		return 0;
	}
}

//! archival function, write init segment to a file
int ingest_stream::write_init_to_file(string &ofile)
{
	// write the stream to an output file
	ofstream out_file(ofile, ofstream::binary);

	if (out_file.good())
	{
		// write the init fragment 
		//out_file.write((char *)&this->m_init_fragment.m_ftyp_box.m_box_data[0], m_init_fragment.m_ftyp_box.m_largesize);
		//out_file.write((char *)&this->m_init_fragment.m_moov_box.m_box_data[0], m_init_fragment.m_moov_box.m_largesize);
		vector<uint8_t> init_data;
		get_init_segment_data(init_data);
		out_file.write((char *)init_data.data(), init_data.size());

		// write the fragments
		//for (long i = 0; i < this->m_media_fragment.size(); i++)
		//{
		//vector<uint8_t> seg_data;
		//this->getMediaSegmentData(i, seg_data);
		//	out_file.write((char *)seg_data.data(), seg_data.size());
		//	//std::cout << "writing fragment " << m_media_fragment[i].m_moof_box.m_btype << std::endl;
		//	//out_file.write((char *)m_media_fragment[i].m_moof_box.m_box_data.data(), m_media_fragment[i].m_moof_box.m_largesize);
		//	
		//	//std::cout << "writing fragment " << m_media_fragment[i].m_mdat_box.m_btype << std::endl;
		//	//out_file.write((char *)m_media_fragment[i].m_mdat_box.m_box_data.data(), m_media_fragment[i].m_mdat_box.m_largesize);
		//}

		//out_file.write((char *)m_mfra_box.m_box_data.data(), m_mfra_box.m_largesize);
		out_file.close();
		cout << " done written init segment to file: " << ofile << std::endl;
	}
	else
	{
		std::cout << " error writing stream to file " << std::endl;
	}

	return 0;
}

//! carefull only use with the testes=d pre-encoded moov boxes to write streams
void setTrackID(vector<uint8_t> &moov_in, uint32_t track_id)
{
	for (int k = 0; k < moov_in.size() - 16; k++)
	{
		if(string( (char *) &moov_in[k]).compare("tkhd") == 0)
		{
			//cout << "tkhd found" << endl;
			if ((uint8_t)moov_in[k + 4] == 1)
			{
				// version 1 
				//4+4+8+8 = 20 
				fmp4_write_uint32(track_id, (char*)&moov_in[k + 24]);
			}
			else{
			   // version 0
			   //4+4+4+4 = 20 
			   fmp4_write_uint32(track_id, (char*)&moov_in[k + 16]);
			}
        }
	}
}

//! carefull only use with the tested pre-encoded moov boxes to write streams
void setSchemeURN(vector<uint8_t> &moov_in, string &urn)
{
	uint16_t size_diff=0;
	vector<uint8_t> l_first;
	vector<uint8_t> l_last;

	// find the uri box containing the description of the urn, caerfull only use for the enclosed mdat box in the source code
	for (int k = 0; k < moov_in.size() - 16; k++)
	{
		if (string((char *)&moov_in[k]).compare("urim") == 0)
		{
			//cout << "urim box found" << endl;
			string or_urn = string((char *)&moov_in[k + 24]);
			size_diff = or_urn.size() - urn.size();

			if (size_diff == 0) 
			{
				for (int l = 0; l < urn.size(); l++)
					moov_in[k + 24 + l] = urn[l];
			}
			else {
			   // first part of the string
			   l_first = vector<uint8_t>(moov_in.begin(), moov_in.begin() + k + 24);
			   // last part of the string
			    l_last = vector<uint8_t>(moov_in.begin() + k + 24 + or_urn.size() + 1, moov_in.end());
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

		for (int i = 0; i < urn.size(); i++)
			moov_in.push_back(urn[i]);
		moov_in.push_back('\0');
		for (int i = 0; i < l_last.size(); i++)
			moov_in.push_back(l_last[i]);

		for (int i = 0; i < moov_in.size(); i++)
		{
			if (string((char *)&moov_in[i]).compare("stsd") == 0)
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = or_size - size_diff;
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
			if (string((char *)&moov_in[i]).compare("stbl") == 0)
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = or_size - size_diff;
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
			if (string((char *)&moov_in[i]).compare("uri ") == 0)
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = or_size - size_diff;
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
			if (string((char *)&moov_in[i]).compare("urim") == 0)
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = or_size - size_diff;
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
			if (string((char *)&moov_in[i]).compare("minf") == 0)
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = or_size - size_diff;
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
			if (string((char *)&moov_in[i]).compare("mdia") == 0)
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = or_size - size_diff;
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
			if (string((char *)&moov_in[i]).compare("trak") == 0)
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = or_size - size_diff;
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
			if (string((char *)&moov_in[i]).compare("moov") == 0)
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = or_size - size_diff;
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
		}
	}
};

//! writes sparse emsg file, set the track, the scheme
int ingest_stream::write_to_sparse_emsg_file(string &out_file, uint32_t track_id, uint32_t announce, string &urn)
{
	//ifstream moov_s_in("sparse_moov.inc", ios::binary);
	
	unsigned int seq_nr = 1;
	vector<uint8_t> sparse_moov = base64_decode(moov_64_enc);
	setTrackID(sparse_moov, track_id);
	if(urn.size())
	    setSchemeURN(sparse_moov, urn );
	ofstream ot(out_file, ios::binary);
	//cout << sparse_moov.size() << endl;

	if (ot.good())
	{
		// write the ftyp header
		ot.write( (char *) &sparse_ftyp[0], 20);

		ot.write((const char *)&sparse_moov[0], sparse_moov.size() );

		// write each of the event messages as moof mdat combinations in sparse track 
		for (auto it = this->m_media_fragment.begin(); it != this->m_media_fragment.end(); ++it)
		{
			//it->print();
			if (it->m_emsg.m_scheme_id_uri.size())
			{

				uint64_t next_tdft = 0;
				//find the next tdft 
				if ( (it + 1) != this->m_media_fragment.end())
					next_tdft = (it+1)->m_tfdt.m_basemediadecodetime;
				//cout << " writing emsg fragment " << endl;
				it->m_emsg.write_emsg_as_fmp4_fragment(&ot, it->m_tfdt.m_basemediadecodetime, track_id, next_tdft);

			}
		}

		ot.write((char *)empty_mfra, 8);

		ot.close();
		cout << "*** wrote sparse track file: " <<  out_file << "  ***" << std::endl;
	}
	else
		cout << "error" << endl;
	return 0;
};

// 
void ingest_stream::write_to_dash_event_stream(string &out_file)
{
	ofstream ot(out_file);


	if (ot.good()) {

		uint32_t time_scale = m_init_fragment.get_time_scale();
		string scheme_id_uri = "";

		if (m_media_fragment.size() > 0)
			scheme_id_uri = m_media_fragment[0].m_emsg.m_scheme_id_uri;

		ot << "<EventStream " << endl
			<< "schemeIdUri=" << '"' << scheme_id_uri << '"' << endl
			<< "timescale=" << '"' << time_scale << '"' << ">" << endl;

		// write each of the event messages as moof mdat combinations in sparse track 
		for (auto it = this->m_media_fragment.begin(); it != this->m_media_fragment.end(); ++it)
		{
			//it->print();
			if (it->m_emsg.m_scheme_id_uri.size())
			{
				
				//cout << " writing emsg fragment " << endl;
				it->m_emsg.write_emsg_as_mpd_event(&ot, it->m_tfdt.m_basemediadecodetime);

			}
		}

		ot << "</EventStream> " << endl;
	}
	ot.close();
}

//! dump the contents of the sparse track to screen
void ingest_stream::print()
{
	for (auto it = m_media_fragment.begin(); it != m_media_fragment.end(); ++it)
	{
		it->print();
	}
}
