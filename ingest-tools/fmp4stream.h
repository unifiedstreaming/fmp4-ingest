/*******************************************************************************
Supplementary software media ingest specification:
https://github.com/unifiedstreaming/fmp4-ingest

Copyright (C) 2009-2018 CodeShop B.V.
http://www.code-shop.com

 - parse fmp4/cmaf file for media ingest in init and media fragment
 - shows some of the functionalities, code is non-optimized, not production level code

******************************************************************************/

#ifndef fMP4Stream_H
#define fMP4Stream_H

#include <iostream>
#include <fstream>
#include <vector>
#include <stdint.h>
#include <string>
#include <bitset>
#include <limits>
#include <memory>
#include <iomanip>

using namespace std;
#define IS_BIG_ENDIAN (*(uint16_t *)"\0\xff" < 0x100)

namespace fMP4Stream {

	//----------------- structures for storing an fmp4 stream defined in ISOBMMF fMP4 ----------------------
	struct box
	{
		uint32_t m_size;
		uint64_t m_largesize;
		string m_btype;
		uint8_t m_extended_type[16];
		vector<uint8_t> m_box_data;
		bool m_is_large;
		bool m_has_uuid;

		box() : m_size(0), m_largesize(0), m_btype(""),m_is_large(false), m_has_uuid(false) {};
		virtual uint64_t size();
		virtual void print();
		virtual bool read(istream *istr);
		virtual void parse(char * ptr);
	
	};

	struct full_box : public box
	{
		uint8_t m_version;
		unsigned int m_flags;
		uint32_t m_magic_conf;
		
		//
		virtual void parse(char *ptr);
		virtual void print();
		virtual uint64_t size() { return box::size() + 4; };
	};


	struct mvhd : public full_box
	{
		uint64_t m_creationTime;
		uint64_t m_modificationTime;

		uint32_t m_time_scale;
		uint64_t m_duration;

		float    m_rate;
		float    m_volume;

		vector<uint32_t>  m_matrix;
		uint32_t m_nextTrackId;

		void parse(char *ptr);
	};

	struct tkhd : public full_box
	{
		uint64_t m_creation_time;
		uint64_t m_modification_time;
		uint32_t m_track_ID;
		uint32_t m_reserved;
		uint64_t m_duration;

		uint32_t m_width;
		uint32_t m_height;

		uint32_t m_box_size; // use the box size we want to skip
	};

	struct mfhd : public full_box
	{
		uint32_t m_seq_nr;
		void parse(char * ptr);
		mfhd() : full_box() { m_btype = string("mfhd"); };
		void print(); 
		uint64_t size() { return full_box::size() + 4; };
	};


	struct traf : public box
	{
		traf() : box() { m_btype = string("traf"); };
	};

	struct tfhd : public full_box
	{
		bool m_base_data_offset_present;
		bool m_sample_description_index_present;
		bool m_default_sample_duration_present;
		bool m_default_sample_size_present;
		bool m_default_sample_flags_present;
		bool m_duration_is_empty;
		bool m_default_base_is_moof;

		uint32_t m_track_id;
		uint64_t m_base_data_offset;
		uint32_t m_sample_description_index;
		uint32_t m_default_sample_duration;
		uint32_t m_default_sample_size;
		uint32_t m_default_sample_flags;

		tfhd() { m_btype = string("tfhd"); };

		virtual void parse(char * ptr);
		virtual uint64_t size();
		virtual void print();
		//uint32_t size(return )
	};

	struct tfdt : public full_box
	{
		uint64_t m_basemediadecodetime;
		tfdt() { m_btype = string("tfdt"); };

		virtual uint64_t size();
		virtual void parse(char * ptr);
		virtual void print();
	};

	struct trun : public full_box
	{
		struct sample_entry
		{
			uint32_t m_sample_duration;
			uint32_t m_sample_size;
			uint32_t m_sample_flags;
			uint32_t m_sample_composition_time_offset_v0;
			int32_t  m_sample_composition_time_offset_v1;
			virtual void print()
			{
				//cout << "trun sample entry: "; 
				cout << setw(33) << left << " sample duration: " << m_sample_duration << endl;
				cout << setw(33) << left << " sample size: " << m_sample_size << endl;
				//cout << "" << sample_flags << endl;
				//cout << "" << sample_composition_time_offset_v0 <<endl;
				//cout << "" << sample_composition_time_offset_v1 << endl;
			};
		};

		uint32_t m_sample_count;
		// optional fields
		int32_t m_data_offset;
		uint32_t m_first_sample_flags;

		//flags
		bool m_data_offset_present;
		bool m_first_sample_flags_present;
		bool m_sample_duration_present;
		bool m_sample_size_present;
		bool m_sample_flags_present;
		bool m_sample_composition_time_offsets_present;
		//entries
		vector<sample_entry> m_sentry;
		
		//trun methods
		virtual uint64_t size();
		virtual void parse(char * ptr);
		virtual void print();
		
		trun() { this->m_btype = "trun"; }
	};

	// other boxes in the moof box
	struct senc 
	{};

	struct saio
	{};
	
	struct saiz
	{};

	struct sbgp
	{};

	struct sgpd
	{};

	struct subs
	{};

	struct meta
	{};

	struct mdat : public box
	{
		mdat() { this->m_btype = string("mdat"); };
		vector<uint8_t> m_bytes;
	};

	// dash emsg box
	struct emsg : public full_box
	{
		//emsg data
		string m_scheme_id_uri;
		string m_value;
		uint32_t m_timescale;
		uint32_t m_presentation_time_delta;
		uint64_t m_presentation_time;
		uint32_t m_event_duration;
		uint32_t m_id;
		vector<uint8_t> m_message_data;

		// emsg methods
		emsg() { this->m_btype = string("emsg"); }
		virtual uint64_t size();
		virtual void parse(char * ptr, unsigned int data_size);
		virtual void print();
		uint32_t write(ostream *ostr);
		void write_emsg_as_fmp4_fragment(ostream *ostr, uint64_t timestamp, uint32_t track_id, uint64_t next_tdft);
		void write_emsg_as_mpd_event(ostream *ostr, uint64_t base_time);
	};	

	const uint8_t empty_mfra[8] = {
		0x00, 0x00, 0x00, 0x08, 'm', 'f', 'r', 'a'
	};

	const uint8_t sparse_ftyp[20]
	{
		0x00, 0x00, 0x00, 0x14, 'f', 't', 'y', 'p','c','m','f','m', 0x00,0x00,0x00,0x00,'c','m','f','c'
	};

	//-------------------------------- SCTE 35 parsing and detection -------------------
	struct sc35_splice_info
	{
		// scte 35 splice info fields
		uint8_t m_table_id;
		bool m_section_syntax_indicator;
		bool m_private_indicator;
		uint16_t m_section_length;
		uint8_t m_protocol_version;
		bool m_encrypted_packet;
		uint8_t m_encryption_algorithm;
		uint64_t m_pts_adjustment;
		uint8_t m_cw_index;
		uint32_t m_tier;
		uint32_t m_splice_command_length;
		uint8_t m_splice_command_type;
		uint16_t m_descriptor_loop_length;

		// information for the splice insert command
		uint32_t m_splice_insert_event_id;
		bool m_splice_event_cancel_indicator;

		void print(bool verbose = false);
		void parse(uint8_t *ptr, unsigned int size);

	};

	//--------------------------- fmp4 ingest stream definition --------------------------------------------
	struct init_fragment
	{
		box m_ftyp_box;
		box m_moov_box;

		uint32_t get_time_scale();
	};

	struct media_fragment
	{
		box m_styp;
		box m_prft;
		emsg m_emsg;
		bool e_msg_is_in_mdat;
		//
		box m_moof_box;
		box m_mdat_box;
		
		// as cmaf only has one traf box we directly store the entries of it
		mfhd m_mfhd;
		tfhd m_tfhd;
		tfdt m_tfdt;
		trun m_trun;

		// see what is in the fragment and store the sub boxes
		void parse_moof();
		void print();
		uint64_t get_duration();
	};

	struct ingest_stream
	{
		init_fragment m_init_fragment;
		vector<media_fragment> m_media_fragment;
		box m_sidx_box, m_meta_box, m_mfra_box;
		int load_from_file(istream *input_file);
		int write_init_to_file(string &out_file);
		int write_to_sparse_emsg_file(string &out_file, uint32_t track_id, uint32_t announce, string &urn);
		uint64_t get_init_segment_data(vector<uint8_t> &init_seg_dat);
		uint64_t get_media_segment_data(long index, vector<uint8_t> &media_seg_dat);
		void write_to_dash_event_stream(string &out_file);
		void print();
	};

	//------------------ helpers for processing the bitstream ------------------------
	static uint16_t fmp4_endian_swap16(uint16_t in) {
		return ((in & 0x00FF) << 8) | ((in & 0xFF00) >> 8);
	};

	static uint32_t fmp4_endian_swap32(uint32_t in) {
		return  ((in & 0x000000FF) << 24) | \
			((in & 0x0000FF00) << 8) | \
			((in & 0x00FF0000) >> 8) | \
			((in & 0xFF000000) >> 24);
	};

	static uint64_t fmp4_endian_swap64(uint64_t in) {
		return  ((in & 0x00000000000000FF) << 56) | \
			((in & 0x000000000000FF00) << 40) | \
			((in & 0x0000000000FF0000) << 24) | \
			((in & 0x00000000FF000000) << 8) | \
			((in & 0x000000FF00000000) >> 8) | \
			((in & 0x0000FF0000000000) >> 24) | \
			((in & 0x00FF000000000000) >> 40) | \
			((in & 0xFF00000000000000) >> 56);
	};

	static uint16_t fmp4_read_uint16(char *pt)
	{
		return IS_BIG_ENDIAN ? *((uint16_t *)pt) : fmp4_endian_swap16(*((uint16_t *)pt));
	};

	static uint32_t fmp4_read_uint32(char *pt)
	{
		return IS_BIG_ENDIAN ? *((uint32_t *)pt) : fmp4_endian_swap32(*((uint32_t *)pt));
	};

	static uint64_t fmp4_read_uint64(char *pt)
	{
		return IS_BIG_ENDIAN ? *((uint64_t *)pt) : fmp4_endian_swap64(*((uint64_t *)pt));
	};

	static uint16_t fmp4_write_uint16(uint16_t in, char *pt)
	{
		return IS_BIG_ENDIAN ?  ((uint16_t *)pt)[0] = in : ((uint16_t *)pt)[0] = fmp4_endian_swap16(in);
	};

	static uint32_t fmp4_write_uint32(uint32_t in, char *pt)
	{
		return IS_BIG_ENDIAN ? ((uint32_t *)pt)[0] = in : ((uint32_t *)pt)[0] = fmp4_endian_swap32(in);
	};

	static uint64_t fmp4_write_uint64(uint64_t in,char *pt)
	{
		return IS_BIG_ENDIAN ? ((uint64_t *)pt)[0] = in : ((uint64_t *)pt)[0] = fmp4_endian_swap64(in);
	};

	
}
#endif
