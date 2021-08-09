#include "event_track.h"
#include <base64.h>
using namespace fmp4_stream;

// base 64 sparse movie header
std::string moov_64_enc("AAACNG1vb3YAAABsbXZoZAAAAAAAAAAAAAAAAAAAAAEAAAAAAAEAAAEAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAIAAAGYdHJhawAAAFx0a2hkAAAABwAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAABNG1kaWEAAAAgbWRoZAAAAAAAAAAAAAAAAAAAAAEAAAAAVcQAAAAAADFoZGxyAAAAAAAAAABtZXRhAAAAAAAAAAAAAAAAVVNQIE1ldGEgSGFuZGxlcgAAAADbbWluZgAAAAxubWhkAAAAAAAAACRkaW5mAAAAHGRyZWYAAAAAAAAAAQAAAAx1cmwgAAAAAQAAAKNzdGJsAAAAV3N0c2QAAAAAAAAAAQAAAEd1cmltAAAAAAAAAAEAAAA3dXJpIAAAAABodHRwOi8vd3d3LnVuaWZpZWQtc3RyZWFtaW5nLmNvbS9kYXNoL2Vtc2cAAAAAEHN0dHMAAAAAAAAAAAAAABBzdHNjAAAAAAAAAAAAAAAUc3RzegAAAAAAAAAAAAAAAAAAABBzdGNvAAAAAAAAAAAAAAAobXZleAAAACB0cmV4AAAAAAAAAAEAAAABAAAAAAAAAAAAAAAA");

//! algorithm to find all sample boundaries between segment_start and segment_end (0=infinite)
uint32_t event_track::find_sample_boundaries(
	const std::vector<DASHEventMessageBoxv1> &emsgs_in,
	std::vector<uint64_t> &sample_boundaries,
	uint64_t segment_start,
	uint64_t segment_end )
{

	sample_boundaries = std::vector<uint64_t>();
	sample_boundaries.push_back(segment_start);

	if (segment_end != 0)
		sample_boundaries.push_back(segment_end);

	for (uint32_t k = 0; k < emsgs_in.size(); k++)
	{
		uint64_t pt = emsgs_in[k].presentation_time_;
		uint64_t pt_du = emsgs_in[k].presentation_time_ + emsgs_in[k].event_duration_;

		if (emsgs_in[k].event_duration_ == 0)
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
uint32_t event_track::find_event_samples(
	const std::vector<event_track::DASHEventMessageBoxv1> &emsgs_in,
	std::vector<event_track::EventSample> &samples_out,
	uint64_t segment_start ,
	uint64_t segment_end )
{

	std::vector<uint64_t> sample_boundaries = std::vector<uint64_t>();
	find_sample_boundaries(emsgs_in, sample_boundaries, segment_start, segment_end);

	for (int i = 0; i < (int)sample_boundaries.size() - 1; i++)
	{
		event_track::EventSample s;
		s.sample_presentation_time_ = sample_boundaries[i];
		s.sample_duration_ = (int32_t)((int64_t)sample_boundaries[i + 1] - (int64_t)sample_boundaries[i]);
		
		s.is_emeb_ = true;

		for (unsigned int k = 0; k < emsgs_in.size(); k++)
		{
			uint64_t pt = emsgs_in[k].presentation_time_;
			uint64_t pt_du = emsgs_in[k].presentation_time_ + emsgs_in[k].event_duration_;

			if (emsgs_in[k].event_duration_ == 0)
				pt_du += 1;

			// event is active during sample duration 
			if (pt < sample_boundaries[i + 1] && pt_du > sample_boundaries[i])
			{
				s.instance_boxes_.push_back(event_track::EventMessageInstanceBox(emsgs_in[k], s.sample_presentation_time_));
				s.is_emeb_ = false;
			}
		}
		samples_out.push_back(s);
	}

	return (uint32_t)samples_out.size();
};

void event_track::write_evt_samples_as_fmp4_fragment(
	std::vector<event_track::EventSample> &samples_in,
	std::ostream &ostr, 
	uint64_t timestamp_tfdt, 
	uint32_t track_id,
	uint64_t next_tfdt)
{
	if (samples_in.size())
	{
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
		l_tfdt.base_media_decode_time_ = samples_in[0].sample_presentation_time_;
		uint64_t l_tfdt_size = l_tfdt.size(); //

		// --- init trun
		trun l_trun = {};
		l_trun.magic_conf_ = 769u;
		l_trun.sample_count_ = (uint32_t) samples_in.size();
		l_trun.data_offset_present_ = true;
		l_trun.first_sample_flags_present_ = false;
		l_trun.sample_duration_present_ = true;
		l_trun.sample_size_present_ = true;
		l_trun.sample_flags_present_ = false;
		l_trun.sample_composition_time_offsets_present_ = false;

		//-- init sentry in trun write 2 samples
		l_trun.m_sentry.resize(samples_in.size());
		//l_trun.m_sentry[0].sample_size_ = 0;
		//l_trun.m_sentry[0].sample_duration_ = 0; 
		uint32_t t_size = 0;

		for (unsigned int i = 0; i < samples_in.size(); i++) 
		{
			t_size += (uint32_t) samples_in[i].size(); // implement the size function 
			l_trun.m_sentry[i].sample_size_ = (uint32_t)samples_in[i].size();
			l_trun.m_sentry[i].sample_duration_ = samples_in[i].sample_duration_;
		}

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
		fmp4_write_uint32((uint32_t)track_id, int_buf);
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
		for (int k = 0; k < l_trun.m_sentry.size(); k++)
		{
			fmp4_write_uint32((uint32_t)l_trun.m_sentry[k].sample_duration_, int_buf);
			ostr.write(int_buf, 4);
			fmp4_write_uint32((uint32_t)l_trun.m_sentry[k].sample_size_, int_buf);
			ostr.write(int_buf, 4);
		}

		uint32_t mdat_size = (uint32_t)t_size + 8; // mdat box + embe box + this event message box
		fmp4_write_uint32(mdat_size, int_buf);
		ostr.write(int_buf, 4);

		ostr.put('m');
		ostr.put('d');
		ostr.put('a');
		ostr.put('t');

		// write the emsg as an mdat box
		for (int i = 0; i < samples_in.size(); i++)
			samples_in[i].write(ostr);

	}
	return;
};

bool event_track::get_sparse_moov(
	const std::string& urn, 
	uint32_t timescale, 
	uint32_t track_id, 
	std::vector<uint8_t> &sparse_moov)
{

	sparse_moov = base64_decode(moov_64_enc);
	set_track_id(sparse_moov, track_id);
	
	// write back the timescale mvhd
	fmp4_write_uint32(timescale, (char *)&sparse_moov[28]);

	// mdhd 
	fmp4_write_uint32(timescale, (char *)&sparse_moov[244]);

	set_evte(sparse_moov);

	return true;
};

bool compare_4cc(char *in, std::string &in_4cc)
{
	if (in[0] == in_4cc[0] && in[1] == in_4cc[1] && in[2] == in_4cc[2] && in[3] == in_4cc[3])
		return true;
	else
		return false;
}

// carefull only use with the tested pre-encoded moov boxes to write streams and update the urn in them
void set_scheme_id_uri(std::vector<uint8_t> &moov_in, 
	const std::string& urn)
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
			if (compare_4cc((char *)&moov_in[i], std::string("stsd")))
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = (uint32_t)(or_size - size_diff);
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
			if (compare_4cc((char *)&moov_in[i], std::string("stbl")))
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = (uint32_t)(or_size - size_diff);
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
			if (compare_4cc((char *)&moov_in[i], std::string("uri ")))
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = (uint32_t)(or_size - size_diff);
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
			if (compare_4cc((char *)&moov_in[i], std::string("urim")))
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = (uint32_t)(or_size - size_diff);
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
			if (compare_4cc((char *)&moov_in[i], std::string("minf")))
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = (uint32_t)(or_size - size_diff);
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
			if (compare_4cc((char *)&moov_in[i], std::string("mdia")))
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = (uint32_t)(or_size - size_diff);
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
			if (compare_4cc((char *)&moov_in[i], std::string("trak")))
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = (uint32_t)(or_size - size_diff);
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
			if (compare_4cc((char *)&moov_in[i], std::string("moov")))
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = (uint32_t)(or_size - size_diff);
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
		}
	}
};

// carefull only use with the tested pre-encoded moov boxes to write streams and update the urn in them
void event_track::set_evte(std::vector<uint8_t>& moov_in)
{
	uint32_t size_diff = 0;

	std::vector<uint8_t> l_first;
	std::vector<uint8_t> l_last;

	// find the uri box containing the description of the urn, caerfull only use for the enclosed mdat box in the source code
	for (std::size_t k = 0; k < moov_in.size() - 16; k++)
	{
		if (compare_4cc((char *)&moov_in[k], std::string("urim")))
		{
			//cout << "urim box found" << endl;
			uint32_t or_size = fmp4_read_uint32((char *)&moov_in[k - 4]);
			fmp4_write_uint32(16, (char *)&moov_in[k - 4]);

			moov_in[k]     = 'e';
			moov_in[k + 1] = 'v';
			moov_in[k + 2] = 't';
			moov_in[k + 3] = 'e';

			size_diff = (uint32_t)(or_size - 16);

			// first part of the string
			l_first = std::vector<uint8_t>(moov_in.begin(), moov_in.begin() + k + 12);
			// last part of the string
			l_last = std::vector<uint8_t>(moov_in.begin() + k + or_size - 4, moov_in.end());

			uint32_t l_diff = moov_in.size() - l_first.size() - l_last.size() - size_diff;

			break;
		}
	}

	// new string representing the moov_box 
	if (size_diff != 0)
	{
		// cout << "scheme size difference is: " << size_diff << endl;

		moov_in.resize(l_first.size());
		moov_in.reserve(moov_in.size() + l_last.size() + 1);

		for (std::size_t i = 0; i < l_last.size(); i++)
			moov_in.push_back(l_last[i]);

		for (std::size_t i = 0; i < moov_in.size(); i++)
		{

			if (compare_4cc((char *)&moov_in[i], std::string("stsd")))
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = (uint32_t)(or_size - size_diff);
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
			if (compare_4cc((char *)&moov_in[i], std::string("stbl")))
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = (uint32_t)(or_size - size_diff);
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
			if (compare_4cc((char *)&moov_in[i], std::string("uri ")))
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = (uint32_t)(or_size - size_diff);
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}

			if (compare_4cc((char *)&moov_in[i], std::string("minf")))
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = (uint32_t)(or_size - size_diff);
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
			if (compare_4cc((char *)&moov_in[i], std::string("mdia")))
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = (uint32_t)(or_size - size_diff);
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
			if (compare_4cc((char *)&moov_in[i], std::string("trak")))
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = (uint32_t)(or_size - size_diff);
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
			if (compare_4cc((char *)&moov_in[i], std::string("moov")))
			{
				uint32_t or_size = fmp4_read_uint32((char *)&moov_in[i - 4]);
				or_size = (uint32_t)(or_size - size_diff);
				fmp4_write_uint32(or_size, (char *)&moov_in[i - 4]);
			}
		}
	}
};

// writes sparse emsg file, set the track, the scheme
int event_track::write_to_segmented_event_track_file(
	const std::string& out_file,
	std::vector<event_track::DASHEventMessageBoxv1> &in_emsg_list,
	uint32_t track_id, 
	uint64_t pt_off_start, 
	uint64_t pt_off_end, 
	const std::string& urn,
	uint64_t seg_duration,
	uint32_t timescale)
{
	std::vector<uint8_t> sparse_moov = base64_decode(moov_64_enc);
	set_track_id(sparse_moov, track_id);
	
	// write back the timescale mvhd / mhd
	fmp4_write_uint32(timescale, (char *)&sparse_moov[28]);
	fmp4_write_uint32(timescale, (char *)&sparse_moov[244]);

	event_track::set_evte(sparse_moov);

	std::ofstream ot(out_file, std::ios::binary);
	//cout << sparse_moov.size() << endl;

	if (seg_duration == 0)
		seg_duration = pt_off_end - pt_off_start;

	if (ot.good())
	{
		// write the ftyp header
		ot.write((char *)&sparse_ftyp[0], 20);
		ot.write((const char *)&sparse_moov[0], sparse_moov.size());
		uint64_t current_tfdt = 0;
		uint64_t it_time = pt_off_start;

		while (it_time < pt_off_end)
		{
			uint64_t actual_duration = seg_duration; 
			if (it_time + seg_duration > pt_off_end)
				actual_duration = pt_off_end - it_time;

			std::vector<event_track::EventSample> samples_in_segment;
			find_event_samples(in_emsg_list, samples_in_segment, it_time, it_time + actual_duration);
			event_track::write_evt_samples_as_fmp4_fragment(samples_in_segment, ot, it_time,track_id , it_time + actual_duration);
			it_time +=actual_duration;
		}
		if(ot.good())
		   ot.close();

		std::cout << "*** wrote sparse track file: " << out_file << "  ***" << std::endl;
	}
	return 0;
};

// write back the dash event streams in an ingest format
void event_track::ingest_event_stream::write_to_dash_event_stream(std::string &out_file)
{
	std::ofstream ot(out_file);

	if (ot.good()) 
	{

		uint32_t time_scale = init_fragment_.get_time_scale();
		std::string scheme_id_uri = "";

		for (auto it = this->events_list_.begin(); it != this->events_list_.end(); ++it)
		{
			scheme_id_uri = it->first;
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
			for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++)
			{
				ot << "<Event "
					<< "presentationTime=" << '"' << it2->second.presentation_time_ << '"' << " "  \
					<< "duration=" << '"' << it2->second.event_duration_<< '"' << " "  \
					<< "id=" << '"' << it2->second.id_ << '"';
				if (it2->second.scheme_id_uri_.compare("urn:scte:scte35:2013:bin") == 0) // write binary scte as xml + bin as defined by scte-35
				{
					ot << '>' << std::endl << "  <Signal xmlns=" << '"' << "http://www.scte.org/schemas/35/2016" << '"' << '>' << std::endl \
						<< "    <Binary>" << base64_encode(it2->second.message_data_.data(), (unsigned int) it2->second.message_data_.size()) << "</Binary>" << std::endl
						<< "  </Signal>" << std::endl;
				}
				else {
					ot << " " << "contentEncoding=" << '"' << "base64" << '"' << '>' << std::endl
						<< base64_encode(it2->second.message_data_.data(), (unsigned int)it2->second.message_data_.size()) << std::endl;
				}
				ot << "</Event>" << std::endl;
			}
			}
			ot << "</EventStream> " << std::endl;
		}

	    ot.close();
}

// parse an fmp4 file for media ingest
int event_track::ingest_event_stream::load_from_file(std::istream &infile, bool init_only)
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
					m.parse_moof();
					bool mdat_found = false;
					
					it++;

					// only support default base is moof and mdat immediately following (CMAF)
					if (m.tfhd_.default_base_is_moof_ && (m.trun_.data_offset_ - 8) == m.moof_box_.size_) {

						if (it->box_type_.compare("mdat") == 0)
						{
							m.mdat_box_ = *it;
							uint64_t pres_time = m.tfdt_.base_media_decode_time_;
							uint32_t data_offset = 0;

							if (m.trun_.sample_duration_present_ && m.trun_.sample_size_present_)
							{
								for (int i = 0; i < m.trun_.m_sentry.size(); i++)
								{
									uint32_t ss = m.trun_.m_sentry[i].sample_size_;
									std::vector<uint8_t> sample_data(ss);
									
									for (unsigned int j = 0; j < ss; j++)
										sample_data[j] = m.mdat_box_.box_data_[8 + data_offset + j];
				
									//parse_event_sample(sample_data,uint64_t presentation_time, )
									unsigned int bts = 0;
									if (ss > 8)
									{
										while (bts < sample_data.size()) {
											EventMessageInstanceBox im_box;
											bts += im_box.parse((char *)sample_data.data() + bts, sample_data.size());
											auto e = im_box.to_emsg_v1(pres_time);
											events_list_[im_box.scheme_id_uri_][im_box.id_] = e;
										}
									}

									data_offset += ss;
									pres_time += m.trun_.m_sentry[i].sample_duration_;
								}
							}
						}
					}
					else 
					{
						std::cout << "error only CMAF based metadata tracks supported with default_base_is_moof and mdat immediately following" << std::endl;
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

// parse an fmp4 file for media ingest
int event_track::ingest_event_stream::print_samples_from_file(std::istream &infile, bool init_only)
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
					m.parse_moof();
					bool mdat_found = false;
					std::cout << "======  found movie fragment with base media decode time ===== " << m.tfdt_.base_media_decode_time_ << std::endl;
					it++;

					// only support default base is moof and mdat immediately following (CMAF)
					if (m.tfhd_.default_base_is_moof_ && (m.trun_.data_offset_ - 8) == m.moof_box_.size_) {

						if (it->box_type_.compare("mdat") == 0)
						{
							m.mdat_box_ = *it;
							uint64_t pres_time = m.tfdt_.base_media_decode_time_;
							uint32_t data_offset = 0;

							if (m.trun_.sample_duration_present_ && m.trun_.sample_size_present_)
							{
								for (int i = 0; i < m.trun_.m_sentry.size(); i++)
								{
									uint32_t ss = m.trun_.m_sentry[i].sample_size_;
									std::vector<uint8_t> sample_data(ss);
									std::cout << "======  found sample in movie fragment, presentation time "<< pres_time <<
										" duration " \
										<< m.trun_.m_sentry[i].sample_duration_ <<  "sample size" << m.trun_.m_sentry[i].sample_size_ << "=====" << std::endl;
									for (unsigned int j = 0; j < ss; j++)
										sample_data[j] = m.mdat_box_.box_data_[8 + data_offset + j];

									//parse_event_sample(sample_data,uint64_t presentation_time, )
									unsigned int bts = 0;
									if (ss > 8)
									{
										while (bts < sample_data.size()) {
											EventMessageInstanceBox im_box;
											bts += im_box.parse((char *)sample_data.data() + bts, sample_data.size());
											im_box.print();
											// auto e = im_box.to_emsg_v1(pres_time);
											// events_list_[im_box.scheme_id_uri_][im_box.id_] = e;
										}
									}
									else 
									{
										std::cout << "empty sample (embe/emeb): " << std::endl;
									}

									data_offset += ss;
									pres_time += m.trun_.m_sentry[i].sample_duration_;
								}
							}
						}
					}
					else
					{
						std::cout << "error only CMAF based metadata tracks supported with default_base_is_moof and mdat immediately following" << std::endl;
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