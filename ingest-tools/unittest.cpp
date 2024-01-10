#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "event/catch.hpp"
#include "event/fmp4stream.h"
#include "event/base64.h"

// box types obtained from the test files in base64 encoded from  +++ tears-of-steel-avc1-400k.cmfv
// box types
char t_ftyp_b64[] = "AAAAGGZ0eXBjbWZjAAAAAGlzbzZjbWZj";
char t_free_b64[] = "AAAAKGZyZWVVU1AgYnkgQ29kZVNob3AREREREREREREREREREREREQ==";
                 
char t_moof_b64[] = "AAAB6G1vb2YAAAAQbWZoZAAAAAAAAAABAAAB0HRyYWYAAAAcdGZoZAACACoAAAABAAAAAQAAAgABAQDAAAAAFHRmZHQBAAAAAAAAAAAAAAAAAAGYdHJ1bgAAAgUAAABgAAAB8AJAAEAAAASCAAAAIgAAABMAAAAmAAAAQQAAAQcAAAC/AAAArwAAALEAAACFAAAAdwAAAJwAAACQAAAAdgAAAFcAAABYAAAAZQAAAE8AAABaAAAAeQAAAGcAAABgAAAAfgAAAGgAAABeAAABBQAAAHIAAABSAAAAYwAAAHAAAABbAAAAWwAAAHkAAAB1AAAAaAAAAGMAAABrAAAAUQAAAGIAAAPIAAAAZgAAAGIAAACtAAAAmQAAAG8AAAB4AAAAdgAAAFoAAABiAAAAgQAAAG8AAABeAAAAlAAAAG0AAABpAAAAYgAAAGwAAABPAAAAYgAAAIgAAABYAAAAcQAAAIUAAAB3AAAAaQAAAGEAAABrAAAAUAAAAGEAAABvAAAAWgAAAFsAAACLAAAAagAAAGcAAAB2AAAAbAAAAE4AAABiAAAAhQAAAFoAAABiAAAAhwAAAHcAAABoAAAAYwAAAGsAAABPAAAAYQAAAIMAAABeAAAAWgAAAHoAAAB/AAAAYQAAAGw=";
char t_moof2_b64[] = "AAAB6G1vb2YAAAAQbWZoZAAAAAAAAAACAAAB0HRyYWYAAAAcdGZoZAACACoAAAABAAAAAQAAAgABAQDAAAAAFHRmZHQBAAAAAAAAAAAAwAAAAAGYdHJ1bgAAAgUAAABgAAAB8AJAAEAAAAeNAAAAbAAAAF4AAABoAAAAUwAAAGwAAACtAAAAUgAAAEYAAABOAAAAZAAAAEEAAABMAAAAWQAAAEoAAABKAAAAdgAAAFEAAABMAAAAWAAAAFMAAABEAAAASwAAAFcAAABMAAAASwAAAG4AAABTAAAAUQAAAE0AAABZAAAARAAAB7AAAADiAAAAdwAAAFUAAACOAAAAZwAAAE0AAABNAAAAgQAAAEAAAABMAAAAVwAAAFMAAABMAAAAeAAAAFEAAABHAAAAUAAAAGUAAABEAAAASAAAAFoAAABKAAAASgAAAH8AAABRAAAATAAAAFEAAABqAAAAPgAAAE4AAABZAAAATAAAAFgAAAB2AAAAUwAAAEsAAABPAAAAVgAAAEEAAABOAAAAWQAAAEkAAABKAAAAegAAAFEAAABHAAAATwAAAFkAAAA9AAAASwAAAFYAAABJAAAASwAAAHsAAABSAAAARwAAAEwAAABmAAAAPQAAAEwAAABVAAAASQAAAEs=";

char t_moov_b64[] = "AAACx21vb3YAAABsbXZoZAAAAAAAAAAAAAAAAAAAMAAAAAAAAAEAAAEAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAIAAAIXdHJhawAAAFx0a2hkAAAABwAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAQAAAAAEVzmkAfAAAAAABs21kaWEAAAAgbWRoZAAAAAAAAAAAAAAAAAAAMAAAAAAAFccAAAAAADJoZGxyAAAAAAAAAAB2aWRlAAAAAAAAAAAAAAAAVVNQIFZpZGVvIEhhbmRsZXIAAAABWW1pbmYAAAAUdm1oZAAAAAEAAAAAAAAAAAAAACRkaW5mAAAAHGRyZWYAAAAAAAAAAQAAAAx1cmwgAAAAAQAAARlzdGJsAAAAmXN0c2QAAAAAAAAAAQAAAIlhdmMxAAAAAAAAAAEAAAAAAAAAAAAAAAAAAAAAANwAfABIAAAASAAAAAAAAAABCkFWQyBDb2RpbmcAAAAAAAAAAAAAAAAAAAAAAAAAAAAAGP//AAAAM2F2Y0MBQsAU/wEAHGdCwBTbDhHu//DjYLQRAAADAAEAAAMAMB8UKuABAARoyoyyAAAAFGJ0cnQAAAAAADKG6AAGNdgAAAAQcGFzcAAALoAAACTTAAAAEHN0dHMAAAAAAAAAAAAAABBzdHNjAAAAAAAAAAAAAAAUc3RzegAAAAAAAAAAAAAAAAAAABBzdGNvAAAAAAAAAAAAAAAQc3RzcwAAAAAAAAAAAAAAPG12ZXgAAAAUbWVoZAEAAAAAAAAAAImkAAAAACB0cmV4AAAAAAAAAAEAAAABAAAAAAAAAAAAAAAA";

// full box types & inner box types for testing
char t_sidx_b64[] = "AAAIwHNpZHgAAAAAAAAAAQAAMAAAAAAAAAAAAAAAALgAADLNAADAAJAAAAAAADE9AADAAJAAAAAADvYbAADAAJAAAAAABdl4AADAAJAAAAAAAh0EAADAAJAAAAAAAWc1AADAAJAAAAAAAjzBAADAAJAAAAAAAoK1AADAAJAAAAAAAvF9AADAAJAAAAAAAmm / AADAAJAAAAAAA72FAADAAJAAAAAAA / M7AADAAJAAAAAABDmNAADAAJAAAAAAAv3yAADAAJAAAAAAAqzTAADAAJAAAAAAAVdyAADAAJAAAAAAAHlgAADAAJAAAAAAAU + XAADAAJAAAAAAAvRNAADAAJAAAAAABAI5AADAAJAAAAAAAntbAADAAJAAAAAAA3ybAADAAJAAAAAAA4H5AADAAJAAAAAAAgnsAADAAJAAAAAAAnG + AADAAJAAAAAAAoYtAADAAJAAAAAAAyzLAADAAJAAAAAAAyEuAADAAJAAAAAAA55yAADAAJAAAAAAAq74AADAAJAAAAAAA29WAADAAJAAAAAAA6oWAADAAJAAAAAAAycFAADAAJAAAAAAAfjSAADAAJAAAAAAAbc / AADAAJAAAAAAAwYVAADAAJAAAAAAAq7EAADAAJAAAAAAAteEAADAAJAAAAAAA9o7AADAAJAAAAAABCo + AADAAJAAAAAABGleAADAAJAAAAAABGMTAADAAJAAAAAAA + emAADAAJAAAAAAAnUUAADAAJAAAAAAAygPAADAAJAAAAAAAlWUAADAAJAAAAAABLh5AADAAJAAAAAAAs8tAADAAJAAAAAAAy32AADAAJAAAAAAAxwmAADAAJAAAAAAAt / 5AADAAJAAAAAABCWpAADAAJAAAAAABX10AADAAJAAAAAAA4eQAADAAJAAAAAABXrMAADAAJAAAAAAA + WpAADAAJAAAAAAAnqVAADAAJAAAAAAAs9bAADAAJAAAAAAApJmAADAAJAAAAAAAt27AADAAJAAAAAAAntoAADAAJAAAAAAAmrqAADAAJAAAAAAAoAiAADAAJAAAAAAAyzlAADAAJAAAAAAAwoeAADAAJAAAAAABCIsAADAAJAAAAAAA4 + fAADAAJAAAAAABAIAAADAAJAAAAAAA4brAADAAJAAAAAAAzHCAADAAJAAAAAAA0IvAADAAJAAAAAAA5OzAADAAJAAAAAAA1IDAADAAJAAAAAAAzciAADAAJAAAAAAApdVAADAAJAAAAAAA2QNAADAAJAAAAAAAmktAADAAJAAAAAAAkV1AADAAJAAAAAAAgFXAADAAJAAAAAAAQR3AADAAJAAAAAAAgBwAADAAJAAAAAAAUwoAADAAJAAAAAAAcLVAADAAJAAAAAAAyQMAADAAJAAAAAAAlkxAADAAJAAAAAAAnaHAADAAJAAAAAAAnhZAADAAJAAAAAAA06sAADAAJAAAAAAAp / HAADAAJAAAAAAArTDAADAAJAAAAAAAg20AADAAJAAAAAAAdcMAADAAJAAAAAAArw2AADAAJAAAAAAAwjhAADAAJAAAAAABHrHAADAAJAAAAAAA1E4AADAAJAAAAAAAqGQAADAAJAAAAAAApamAADAAJAAAAAAAq3BAADAAJAAAAAAAntuAADAAJAAAAAAA4EpAADAAJAAAAAAAu29AADAAJAAAAAAAq3qAADAAJAAAAAAAvk2AADAAJAAAAAABY9bAADAAJAAAAAAA9LQAADAAJAAAAAAA2DcAADAAJAAAAAAAj1HAADAAJAAAAAAA1LTAADAAJAAAAAAA + zQAADAAJAAAAAAA5hgAADAAJAAAAAAAr68AADAAJAAAAAAArS6AADAAJAAAAAAAxDBAADAAJAAAAAAA33oAADAAJAAAAAAA3ylAADAAJAAAAAABG6mAADAAJAAAAAABhDaAADAAJAAAAAABPR7AADAAJAAAAAABMwtAADAAJAAAAAAAt1jAADAAJAAAAAAApiTAADAAJAAAAAAAxAjAADAAJAAAAAAA8ILAADAAJAAAAAAA3f7AADAAJAAAAAAA7lIAADAAJAAAAAAAnVPAADAAJAAAAAAArWXAADAAJAAAAAAAzo + AADAAJAAAAAAAhGOAADAAJAAAAAAA3EfAADAAJAAAAAAA5wiAADAAJAAAAAAAhNAAADAAJAAAAAAAYmoAADAAJAAAAAAAgz + AADAAJAAAAAAAzaVAADAAJAAAAAAAsqwAADAAJAAAAAAAp2iAADAAJAAAAAAAZ9UAADAAJAAAAAAAhXOAADAAJAAAAAAAmJIAADAAJAAAAAAA1MFAADAAJAAAAAABEdJAADAAJAAAAAABGfEAADAAJAAAAAAA8iaAADAAJAAAAAAAxxBAADAAJAAAAAAAsv0AADAAJAAAAAAAmVhAADAAJAAAAAAA5djAADAAJAAAAAAA / FZAADAAJAAAAAAA6igAADAAJAAAAAAAzA5AADAAJAAAAAAA7WSAADAAJAAAAAAAs3SAADAAJAAAAAAA15gAADAAJAAAAAAAyjUAADAAJAAAAAAAul7AADAAJAAAAAAAYqwAADAAJAAAAAAAlASAADAAJAAAAAAAuINAADAAJAAAAAAA2MbAADAAJAAAAAAA15LAADAAJAAAAAAAy0BAADAAJAAAAAAA7V5AADAAJAAAAAABIqNAADAAJAAAAAABKWZAADAAJAAAAAABCtAAADAAJAAAAAAA / uhAADAAJAAAAAAA + 4xAADAAJAAAAAAA7EUAADAAJAAAAAAA + jmAADAAJAAAAAAA7HzAADAAJAAAAAAA9KrAADAAJAAAAAAA0ggAADAAJAAAAAAAh8FAADAAJAAAAAAAVKAAADAAJAAAAAAAGjeAADAAJAAAAAAAlW2AADAAJAAAAAAA4hmAADAAJAAAAAABAQEAADAAJAAAAAAAz4cAADAAJAAAAAAAvKqAADAAJAAAAAAAXlDAADAAJAAAAAAAA2BAABkAJAAAAA=";

char t_tfdt_b64[] = "AAAAFHRmZHQBAAAAAAAAAAAAAAA="; // 0
char t2_tfdt_b64[] = "AAAAFHRmZHQBAAAAAAAAAAAAwAA="; //  49152
char t3_tfdt_b64[] = "AAAAFHRmZHQBAAAAAAAAAAABgAA="; //98304

//char t_trun_b64[] = "AAABmHRydW4AAAIFAAAAYAAAAfACQABAAAAEggAAACIAAAATAAAAJgAAAEEAAAEHAAAAvwAAAK8AAACxAAAAhQAAAHcAAACcAAAAkAAAAHYAAABXAAAAWAAAAGUAAABPAAAAWgAAAHkAAABnAAAAYAAAAH4AAABoAAAAXgAAAQUAAAByAAAAUgAAAGMAAABwAAAAWwAAAFsAAAB5AAAAdQAAAGgAAABjAAAAawAAAFEAAABiAAADyAAAAGYAAABiAAAArQAAAJkAAABvAAAAeAAAAHYAAABaAAAAYgAAAIEAAABvAAAAXgAAAJQAAABtAAAAaQAAAGIAAABsAAAATwAAAGIAAACIAAAAWAAAAHEAAACFAAAAdwAAAGkAAABhAAAAawAAAFAAAABhAAAAbwAAAFoAAABbAAAAiwAAAGoAAABnAAAAdgAAAGwAAABOAAAAYgAAAIUAAABaAAAAYgAAAIcAAAB3AAAAaAAAAGMAAABrAAAATwAAAGEAAACDAAAAXgAAAFoAAAB6AAAAfwAAAGEAAABs";
//char t2_trun_b64[] = "AAABmHRydW4AAAIFAAAAYAAAAfACQABAAAAHjQAAAGwAAABeAAAAaAAAAFMAAABsAAAArQAAAFIAAABGAAAATgAAAGQAAABBAAAATAAAAFkAAABKAAAASgAAAHYAAABRAAAATAAAAFgAAABTAAAARAAAAEsAAABXAAAATAAAAEsAAABuAAAAUwAAAFEAAABNAAAAWQAAAEQAAAewAAAA4gAAAHcAAABVAAAAjgAAAGcAAABNAAAATQAAAIEAAABAAAAATAAAAFcAAABTAAAATAAAAHgAAABRAAAARwAAAFAAAABlAAAARAAAAEgAAABaAAAASgAAAEoAAAB/AAAAUQAAAEwAAABRAAAAagAAAD4AAABOAAAAWQAAAEwAAABYAAAAdgAAAFMAAABLAAAATwAAAFYAAABBAAAATgAAAFkAAABJAAAASgAAAHoAAABRAAAARwAAAE8AAABZAAAAPQAAAEsAAABWAAAASQAAAEsAAAB7AAAAUgAAAEcAAABMAAAAZgAAAD0AAABMAAAAVQAAAEkAAABL";
//char t3_trun_b64[] = "AAABmHRydW4AAAIFAAAAYAAAAfACQABAAAAEiwAAAEwAAABGAAAAUAAAAGIAAAA9AAAATAAAAFQAAABJAAAASwAAAJQAAABRAAAATgAAAE4AAABXAAAAQgAAAEsAAABYAAAASwAAAEoAAAB5AAAATQAAAE0AAABQAABkPQAAMgYAADB5AAA18AAANZAAADiLAAA38QAANnkAADiYAAA4HQAAN1YAADohAAA63wAAPZkAADkXAAA8KwAAPsEAAD8AAABDUwAAPPIAAEFQAABDLAAAPSMAAD53AAA++gAARvgAAEIJAABEXgAATS0AAEa6AABMNwAATCMAAExZAABI8gAAQZgAAEFvAAA/5wAAQOcAAD77AAA/5QAAQOgAADwVAAA8QAAAPAkAADWtAAA4NwAAMmkAADDEAAAmvQAALyoAACatAAArzgAAKFwAACTFAAAjlwAAKsYAACXFAAAjqAAAIzwAAB/jAAAj5wAAIMsAAB6QAAAedwAAHOwAAB2yAAAdLAAAGLUAABlhAAAbxwAAGRcAABkv";

char trunnetje [] = "AAABmHRydW4AAAIFAAAAYAAAAfACQABAAAAbXAAACREAAAkDAAAIGQAACPkAAAjtAAAH8AAAB/EAAAiyAAAIAgAAB/QAAAfIAAAIXAAAByMAAAdJAAAGzwAAB44AAAbLAAAGTwAABwkAAAaJAAAHcgAAB0AAAAa0AAAGfwAABvwAAAY+AAAGmQAABaoAAAa0AAAGNAAABn8AAAarAAAGFQAABbsAAAZeAAAFvAAABdsAAAZGAAAFuAAABcsAAAW6AAAFVAAABPQAAAVfAAAFbwAABY0AAAUGAAAFbQAABIcAAAZ9AAAE5wAABFcAAAWIAAAFmAAABZ0AAAdiAAAFCAAABS0AAAVOAAAW9QAACjwAAAGOAAACHwAAAX8AAAI1AAAC0gAAAogAAAGjAAADCgAAAtQAAAGTAAAB0wAAAoYAAAKdAAAHXgAAAlMAAAFRAAAB5QAABCQAAAIOAAABlAAAAycAAAKZAAADRAAAA0MAAAJQAAACHgAAAgMAAAG6AAACxQAABeMAAAMkAAADbAAAA1UAAAQr";

char t_mfhd_b64[] = "AAAAEG1maGQAAAAAAAAAAQ==";
char t2_mfhd_b64[] = "AAAAEG1maGQAAAAAAAAAAg==";
char t3_mfhd_b64[] = "AAAAEG1maGQAAAAAAAAAAw==";

char t_tfhd_b64[] = "AAAAHHRmaGQAAgAqAAAAAQAAAAEAAAIAAQEAwA==";
char t2_tfhd_b64[] = "AAAAHHRmaGQAAgAqAAAAAQAAAAEAAAIAAQEAwA==";


char emsg_b64[] = "AAAAWmVtc2cAAAAAdXJuOnNjdGU6c2N0ZTM1OjIwMTM6YmluAAAAADIAAAAAAAADkAAAAAMr/DAhAAAAAAAAAP/wEAUAAAMrf+9//gAaF7DAAAAAAADkYSQC";

using namespace std;

string get_path_from_template(
	string &template_string,
	string &file_name,
	uint64_t time,
	uint64_t number)
{
	string out_string;

	const string number_str = "$Number$";
	const string time_str = "$Time$";
	const string rep_str = "$RepresentationID$";

	string rep_name;

	if (file_name.size() > 0)
	{
		size_t poss = file_name.find_last_of(".");
		if (poss != std::string::npos)
			rep_name = file_name.substr(0, poss);
	}

	size_t rep_pos = template_string.find_first_of(rep_str);

	if (rep_pos != std::string::npos)
	{
		string b, c;

		if (rep_pos != 0)
			b = template_string.substr(0, rep_pos);

		if (rep_pos + rep_str.size() < template_string.size())
			c = template_string.substr(rep_pos + rep_str.size());

		out_string = b + rep_name + c;
	}

	size_t time_pos = out_string.find(time_str);
	size_t num_pos = out_string.find(number_str);

	if (time_pos != std::string::npos)
	{
		string b, c;

		if (time_pos != 0)
			b = out_string.substr(0, time_pos);

		if (time_pos + time_str.size() < out_string.size())
			c = out_string.substr(time_pos + time_str.size());

		out_string = b + std::to_string(time) + c;
	}
	else if (num_pos != std::string::npos)
	{
		string b, c;

		if (num_pos != 0)
			b = out_string.substr(0, num_pos);

		if (num_pos + number_str.size() < out_string.size())
			c = out_string.substr(num_pos + number_str.size());

		out_string = b + std::to_string(number) + c;
	}

	return out_string;
}

TEST_CASE(" test create path from segmentTemplate")
{
	SECTION("number based from mediakind")
	{
		string init = "$RepresentationID$-init.m4s";
		string media = "$RepresentationID$-0-I-$Number$.m4s";
		string file_name = "test.cmfv";
		string res = get_path_from_template(init, file_name, 0, 0);
		string res1 = get_path_from_template(media, file_name, 0, 1);
		
		REQUIRE(res.compare("test-init.m4s") == 0);
		REQUIRE(res1.compare("test-0-I-1.m4s") == 0);
	}

	SECTION("number based from mediakind")
	{
		string init = "$RepresentationID$/init.m4s";
		string media = "$RepresentationID$/$Number$.m4s";
		string file_name = "test.cmfv";
		string res = get_path_from_template(init, file_name, 0, 0);
		string res1 = get_path_from_template(media, file_name, 0, 1);

		REQUIRE(res.compare("test/init.m4s") == 0);
		REQUIRE(res1.compare("test/1.m4s") == 0);
	}
}


TEST_CASE("test fmp4 stream support for box", "[fmp4_stream_box]") {

	SECTION("parse box") 
	{
	    fmp4_stream::box b;
		std::vector<uint8_t> bin_dat = base64_decode(t_ftyp_b64);
		b.parse((const char *)&bin_dat[0]);

		REQUIRE(b.box_type_ .compare("ftyp") == 0);
		REQUIRE(b.read_size() == bin_dat.size());

		b= fmp4_stream::box();
		bin_dat = base64_decode(t_moof_b64);
		b.parse((const char *)&bin_dat[0]);
		REQUIRE(b.box_type_.compare("moof") == 0);
		REQUIRE(b.read_size() == bin_dat.size());

		b = fmp4_stream::box();
		bin_dat = base64_decode(t_moof2_b64);
		b.parse((const char *)&bin_dat[0]);
		REQUIRE(b.box_type_.compare("moof") == 0);
		REQUIRE(b.read_size() == bin_dat.size());

		b = fmp4_stream::box();
		bin_dat = base64_decode(t_moov_b64);
		b.parse((const char *)&bin_dat[0]);
		REQUIRE(b.read_size() == bin_dat.size());
		REQUIRE(b.box_type_.compare("moov") == 0);

		b = fmp4_stream::box();
		bin_dat = base64_decode(t_free_b64);
		b.parse((const char *)&bin_dat[0]);
		REQUIRE(b.box_type_.compare("free") == 0);
		REQUIRE(b.read_size() == bin_dat.size());

		fmp4_stream::full_box b1 = fmp4_stream::full_box();
		bin_dat = base64_decode(t_sidx_b64);
		b1.parse((const char *)&bin_dat[0]);
		REQUIRE(b1.box_type_.compare("sidx") == 0);
		//REQUIRE(b1.read_size() == bin_dat.size());
	}
	SECTION("test parse tfdt") 
	{
		fmp4_stream::tfdt t = fmp4_stream::tfdt();
		std::vector<uint8_t> bin_dat = base64_decode(t_tfdt_b64);
		t.parse((char *)&bin_dat[0]);
		REQUIRE(t.base_media_decode_time_ == 0);

		fmp4_stream::tfdt t1 = fmp4_stream::tfdt();
		bin_dat = base64_decode(t2_tfdt_b64);
		t1.parse((char *)&bin_dat[0]);
		REQUIRE(t1.base_media_decode_time_ == 49152);

		fmp4_stream::tfdt t2 = fmp4_stream::tfdt();
		bin_dat = base64_decode(t3_tfdt_b64);
		t2.parse((char *)&bin_dat[0]);
		REQUIRE(t2.base_media_decode_time_ == 98304);
	}
	SECTION("test parse trun")
	{
		fmp4_stream::trun t = fmp4_stream::trun();
		std::vector<uint8_t> bin_dat = base64_decode(trunnetje);
		t.parse((char *)&bin_dat[0]);

		REQUIRE(t.data_offset_present_ == true);
		REQUIRE(t.sample_composition_time_offsets_present_ == false);
		REQUIRE(t.sample_flags_present_ == false);
		REQUIRE(t.sample_size_present_ == true);
		REQUIRE(t.sample_duration_present_ == false);
		REQUIRE(t.first_sample_flags_present_ == true);	
	
		REQUIRE(t.m_sentry.size() == 96);
		REQUIRE(t.data_offset_ == 496);

	}
	SECTION("tests parse mfhd")
	{
		fmp4_stream::mfhd t = fmp4_stream::mfhd();
		std::vector<uint8_t> bin_dat = base64_decode(t_mfhd_b64);
		t.parse((char *)&bin_dat[0]);

		REQUIRE(t.seq_nr_ == 1);

		fmp4_stream::mfhd t1 = fmp4_stream::mfhd();
		bin_dat = base64_decode(t2_mfhd_b64);
		t1.parse((char *)&bin_dat[0]);
		REQUIRE(t1.seq_nr_ == 2);

		fmp4_stream::mfhd t2 = fmp4_stream::mfhd();
		bin_dat = base64_decode(t3_mfhd_b64);
		t2.parse((char *)&bin_dat[0]);
		REQUIRE(t2.seq_nr_ == 3);
	}
	SECTION("test parse tfhd")
	{
		fmp4_stream::tfhd t = fmp4_stream::tfhd();
		std::vector<uint8_t> bin_dat = base64_decode(t_tfhd_b64);
		t.parse((char *)&bin_dat[0]);

		REQUIRE(t.base_data_offset_present_ == false);
		REQUIRE(t.default_base_is_moof_ == true);
		REQUIRE(t.duration_is_empty_ == false);
		REQUIRE(t.default_sample_size_present_ == false);
		REQUIRE(t.default_sample_flags_present_ == true);
		REQUIRE(t.sample_description_index_ == 1);
		REQUIRE(t.default_sample_duration_present_ == true);
		REQUIRE(t.track_id_ == 1);
		REQUIRE(t.default_sample_duration_ == 512);


		fmp4_stream::tfhd t1 = fmp4_stream::tfhd();
		bin_dat = base64_decode(t2_tfhd_b64);
		t1.parse((char *)&bin_dat[0]);
	}
}


TEST_CASE("test emsg support", "[emsg]") {

	SECTION("parse emsg v1")
	{
		fmp4_stream::emsg t1 = fmp4_stream::emsg();
		std::vector<uint8_t> bin_dat = base64_decode(emsg_b64);
		t1.parse((char *)&bin_dat[0], bin_dat.size());
		REQUIRE(t1.event_duration_ == 233472);
		REQUIRE(t1.id_ == 811);

	}

	SECTION("parse emsg")
	{

		fmp4_stream::emsg t1 = fmp4_stream::emsg();
		std::vector<uint8_t> bin_dat = base64_decode(emsg_b64);
		t1.parse((char *)&bin_dat[0], bin_dat.size());
		REQUIRE(t1.event_duration_ == 233472);
		REQUIRE(t1.id_ == 811);

	}

}

/* todo additional unit tests 
TEST_CASE("test emsg track", "[emsg_track]") {

	SECTION("generate sparse moov")
	{

	}

	SECTION("parse emsg track")
	{

	}
	SECTION("create emsg track")
	{

	}
	SECTION("write to DASH Event Stream")
	{

	}
}


TEST_CASE("test SCTE-35 support", "[scte-35]") {

	SECTION("splice info")
	{

	}

	SECTION("generate splice insert")
	{

	}

}

TEST_CASE("ingest stream support", "[ingest_stream]") {

	SECTION("read from file & write to file")
	{

	}

	SECTION("get init segment")
	{

	}

	SECTION("get media segment")
	{

	}
	SECTION("test duration")
	{

	}
	SECTION("test patching with duration")
	{

	}

} */