/*******************************************************************************
Supplementary software media ingest specification:
https://github.com/unifiedstreaming/fmp4-ingest

Copyright (C) 2009-2021 CodeShop B.V.
http://www.code-shop.com

 - parse fmp4/cmaf file for media ingest in init and media fragment
 - shows some of the functionalities, code is non-optimized, not production level code

******************************************************************************/

#ifndef fMP4Stream_H
#define fMP4Stream_H

#include <stdint.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <bitset>
#include <iomanip>

namespace /* anonymous */ {

	//--------- helpers for parsing and reading big endian mp4

	inline bool is_big_endian()
	{
		return  (*(uint16_t *)"\0\xff" < 0x100);
	}

	
	uint16_t fmp4_endian_swap16(uint16_t in)
	{
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

	int32_t fmp4_write_int32(int32_t in, char const *pt)
	{
		return is_big_endian() ? ((int32_t *)pt)[0] = in : ((int32_t *)pt)[0] = fmp4_endian_swap32(in);
	}

	uint64_t fmp4_write_uint64(uint64_t in, char const *pt)
	{
		return is_big_endian() ? ((uint64_t *)pt)[0] = in : ((uint64_t *)pt)[0] = fmp4_endian_swap64(in);
	}

	int64_t fmp4_write_int64(int64_t in, char const *pt)
	{
		return is_big_endian() ? ((int64_t *)pt)[0] = in : ((int64_t *)pt)[0] = fmp4_endian_swap64(in);
	}

} // anonymous


namespace fmp4_stream {

	//----------------- structures for storing an fmp4 stream defined in ISOBMMF (simplified) ----------------------
	struct box
	{
		uint32_t size_;
		uint64_t large_size_;
		std::string box_type_;
		uint8_t extended_type_[16];
		std::vector<uint8_t> box_data_;
		bool is_large_;
		bool has_uuid_;

		box() : size_(0), large_size_(0), box_type_(""), is_large_(false), has_uuid_(false) {  };
		uint64_t read_size() { return (size_ == 1) ? large_size_ : (uint64_t) size_; };

		virtual uint64_t size() const; // warning only beginning of box
		virtual void print() const;
		virtual bool read(std::istream& istr);
		virtual void parse(char const *ptr);
	
	};

	struct full_box : public box
	{
		uint8_t version_;
		uint32_t flags_;
		uint32_t magic_conf_;

		virtual void parse(char const *ptr);
		virtual void print() const;
		virtual uint64_t size() const { return box::size() + 4; };
		full_box() : version_(0), flags_(0), magic_conf_(0) {};
	};


	struct mvhd : public full_box
	{
		uint64_t creation_time_;
		uint64_t modification_time_;

		uint32_t time_scale_;
		uint64_t duration_;

		float    rate_;
		float    volume_;

		std::vector<uint32_t>  matrix_;
		uint32_t next_track_id_;
		
        mvhd() : creation_time_(0), modification_time_(0), time_scale_(0), duration_(0), rate_(0), volume_(0), next_track_id_(0) {};
		
		void parse(char const* ptr);
	};

	struct tkhd : public full_box
	{
		uint64_t creation_time_;
		uint64_t modification_time_;
		uint32_t track_id_;
		uint32_t reserved_;
		uint64_t duration_;

		uint32_t width_;
		uint32_t height_;

		uint32_t box_size_; 
		tkhd() : full_box() { *this = {}; };

	    // todo implement parsing of tkhd
	};

	// --------- boxes in track fragments, in this case parsing is implemented
	struct mfhd : public full_box
	{
		uint32_t seq_nr_;
		
		mfhd() : full_box(), seq_nr_(0) { box_type_ = std::string("mfhd"); };
		void print() const; 
		uint64_t size() const { return full_box::size() + 4; };
		void parse(char const* ptr);
	};


	struct traf : public box
	{
		traf() : box() { box_type_ = std::string("traf"); };
	};

	struct tfhd : public full_box
	{
		bool base_data_offset_present_;
		bool sample_description_index_present_;
		bool default_sample_duration_present_;
		bool default_sample_size_present_;
		bool default_sample_flags_present_;
		bool duration_is_empty_;
		bool default_base_is_moof_;

		uint32_t track_id_;
		uint64_t base_data_offset_;
		uint32_t sample_description_index_;
		uint32_t default_sample_duration_;
		uint32_t default_sample_size_;
		uint32_t default_sample_flags_;

		tfhd() { set_zero();  box_type_ = std::string("tfhd"); };

		void set_zero() 
		{
			base_data_offset_present_=false;
			sample_description_index_present_=false;
			default_sample_duration_present_=false;
			default_sample_size_present_=false;
			default_sample_flags_present_=false;
			duration_is_empty_=false;
			default_base_is_moof_=false;

			track_id_=0;
			base_data_offset_=0;
			sample_description_index_=0;
			default_sample_duration_=0;
			default_sample_size_=0;
			default_sample_flags_=0;
		};

		virtual void parse(char const * ptr);
		virtual uint64_t size() const;
		virtual void print() const;
		//uint32_t size(return )
	};

	struct tfdt : public full_box
	{
		uint64_t base_media_decode_time_;
		tfdt() : full_box() , base_media_decode_time_(0){ box_type_ = std::string("tfdt"); };

		virtual uint64_t size() const;
		virtual void parse(const char * ptr);
		virtual void print() const;

		//static void update_tfdt(uint64_t new_base_media_decode_time, char * box_Data, size_t box_size);
	};

	struct trun : public full_box
	{
		struct sample_entry
		{
			uint32_t sample_duration_;
			uint32_t sample_size_;
			uint32_t sample_flags_;
			uint32_t sample_composition_time_offset_v0_;
			int32_t  sample_composition_time_offset_v1_;
			
			virtual void print() const
			{
				//cout << "trun sample entry: "; 
				std::cout << std::setw(33) << std::left << " sample duration: " << sample_duration_ << std::endl;
				std::cout << std::setw(33) << std::left << " sample size: " << sample_size_ << std::endl;
				//cout << "" << sample_flags << endl;
				//cout << "" << sample_composition_time_offset_v0 <<endl;
				//cout << "" << sample_composition_time_offset_v1 << endl;
			};
			sample_entry() : sample_duration_(0), 
				sample_size_(0), sample_flags_(0), 
				sample_composition_time_offset_v0_(0), 
				sample_composition_time_offset_v1_(0) {};
		};

		uint32_t sample_count_;
		
		// optional fields
		int32_t data_offset_;
		uint32_t first_sample_flags_;

		//flags
		bool data_offset_present_;
		bool first_sample_flags_present_;
		bool sample_duration_present_;
		bool sample_size_present_;
		bool sample_flags_present_;
		bool sample_composition_time_offsets_present_;
		
		//entries
		std::vector<sample_entry> m_sentry;
		
		//trun methods
		virtual uint64_t size() const;
		virtual void parse(char const * ptr);
		virtual void print() const;
		
		trun() : full_box() 
		{
			zero_set();
			this->box_type_ = "trun"; 
		}

		void zero_set() 
		{
			sample_count_=0;

			// optional fields
			data_offset_=0;
			first_sample_flags_=0;

			//flags
			data_offset_present_= false;
			first_sample_flags_present_ = false;
			sample_duration_present_ = false;
			sample_size_present_= false;
			sample_flags_present_= false;
			sample_composition_time_offsets_present_ = false;
		}
	};

	// other boxes in the moof box not yet implemented (todo)
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
		mdat() : box() {  this->box_type_ = std::string("mdat"); };
		std::vector<uint8_t> m_bytes;
	};

	// dash emsg box
	struct emsg : public full_box
	{
		//emsg data
		std::string scheme_id_uri_;
		std::string value_;
		uint32_t timescale_;
		uint32_t presentation_time_delta_;
		uint64_t presentation_time_;
		uint32_t event_duration_;
		uint32_t id_;
		std::vector<uint8_t> message_data_;
		
		// emsg methods
		emsg() : presentation_time_delta_(0)
		{ 
			zero_set();
			this->box_type_ = std::string("emsg"); 
		}

		void zero_set()
		{
			scheme_id_uri_ = {};
			value_ = {};
			timescale_=0;
			presentation_time_delta_=0;
			presentation_time_=0;
			event_duration_=0;
			id_=0;
		};

		virtual uint64_t size() const;
		virtual void parse(char const * ptr, unsigned int data_size);
		virtual void print() const;
		uint32_t write(std::ostream &ostr) const;

		void write_emsg_as_mpd_event(std::ostream &ostr, uint64_t base_time) const;
	};	

	const uint8_t empty_mfra[8] = {
		0x00, 0x00, 0x00, 0x08, 'm', 'f', 'r', 'a'
	};

	const uint8_t sparse_ftyp[20] =
	{
		0x00, 0x00, 0x00, 0x14, 'f', 't', 'y', 'p','c','m','f','m', 0x00,0x00,0x00,0x00,'c','m','f','c'
	};

	//-------------------------------- SCTE 35 parsing and detection -------------------
	struct sc35_splice_info
	{
		// scte 35 splice info fields
		uint8_t table_id_;
		bool section_syntax_indicator_;
		bool private_indicator_;
		uint16_t section_length_;
		uint8_t protocol_version_;
		bool encrypted_packet_;
		uint8_t encryption_algorithm_;
		uint64_t pts_adjustment_;
		uint8_t cw_index_;
		uint32_t tier_;
		uint32_t splice_command_length_;
		uint8_t splice_command_type_;
		uint16_t descriptor_loop_length_;

		// information for the splice insert command
		uint32_t splice_insert_event_id_;
		bool splice_event_cancel_indicator_;

		void print(bool verbose = false) const;
		void parse(uint8_t const *ptr, unsigned int size);

		void zero_set() 
		{
			// scte 35 splice info fields
			table_id_=0;
			section_syntax_indicator_ = 0;;
			private_indicator_ = 0;
			section_length_=0;
			protocol_version_=0;
			encrypted_packet_=0;
			encryption_algorithm_=0;
			pts_adjustment_=0;
			cw_index_=0;
			tier_=0;
			splice_command_length_=0;
			splice_command_type_=0;
			descriptor_loop_length_=0;
		}

	};

	//--------------------------- fmp4 ingest stream definition --------------------------------------------
	struct init_fragment
	{
		box ftyp_box_;
		box moov_box_;

		uint32_t get_time_scale();
	};

	struct media_fragment
	{
		box styp_;
		box prft_;
		std::vector<emsg> emsg_;
		bool e_msg_is_in_mdat_;
		//
		box moof_box_;
		box mdat_box_;

		// as cmaf only has one traf box we directly store the entries of it
		mfhd mfhd_;
		tfhd tfhd_;
		tfdt tfdt_;
		trun trun_;

		// some functions for parsing and updating movie fragments
		void parse_moof();
		void patch_tfdt(uint64_t patch, uint32_t seq_nr = 0);
		void print() const;
		uint64_t get_duration();
		uint64_t get_bmdt() { return tfdt_.base_media_decode_time_; }
	};

	// structure to store an entire ingest stream
	struct ingest_stream
	{
		init_fragment init_fragment_;
		std::vector<media_fragment> media_fragment_;
		box sidx_box_, meta_box_, mfra_box_;
		int load_from_file(std::istream &input_file, bool init_only=false);
		int write_init_to_file(std::string &out_file, unsigned int nfrags=0, bool write_separate=false);
		
		uint64_t get_init_segment_data(std::vector<uint8_t> &init_seg_dat);
		uint64_t get_media_segment_data(std::size_t index, std::vector<uint8_t> &media_seg_dat);

		
		void print() const;

		void patch_tfdt(uint64_t patch, bool apply_timescale=true, uint32_t anchor_scale=1);

		uint64_t get_duration();
		uint64_t get_start_time();
	};

	void gen_splice_insert(std::vector<uint8_t> &out_splice_insert, uint32_t event_id, uint32_t duration);
	bool set_track_id(std::vector<uint8_t> &moov_in, uint32_t track_id);

}
#endif
