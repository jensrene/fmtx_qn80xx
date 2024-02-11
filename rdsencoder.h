#ifndef RDSENCODER_H
#define RDSENCODER_H


#include <stdint.h>
#include <cstring>
#include <stdlib.h>
//#include <ctime>
//#include <cstdlib>

class rds_encoder {

public:
    enum pty_codes_eu : uint8_t {
    NoProgrammeTypeDefined = 0,
    News = 1,
    CurrentAffairs = 2,
    Information = 3,
    Sport = 4,
    Education = 5,
    Drama = 6,
    Culture = 7,
    Science = 8,
    Varied = 9,
    Pop = 10,
    Rock = 11,
    EasyListening = 12,
    LightClassical = 13,
    SeriousClassical = 14,
    OtherMusic = 15,
    Weather = 16,
    Finance = 17,
    ChildrensProgrammes = 18,
    SocialAffairs = 19,
    Religion = 20,
    PhoneIn = 21,
    Travel = 22,
    Leisure = 23,
    JazzMusic = 24,
    CountryMusic = 25,
    NationalMusic = 26,
    OldiesMusic = 27,
    FolkMusic = 28,
    Documentary = 29,
    AlarmTest = 30,
    Alarm = 31
};

    enum group_types : uint8_t {
    BTS_0A = 0,      /* Basic tuning and switching information, contains PS */
    PI_0B = 1,             /* Program Identification */
    AF_1A = 2,             /* Alternative Frequencies */
    PIN_SL_1B = 3,         /* Program Item Number (PIN) and Slow labeling */
    RADIO_A_2A = 4,        /* Radiotext A */
    RADIO_B_2B = 5,        /* Radiotext B */
    CT_DATE_3A = 6,        /* Clock-time and date */
    OPEN_DATA_3B = 7,      /* Open data */
    PIN_4A = 8,            /* Program Item Number (PIN) */
    TA_4B = 9,             /* Traffic Announcement (TA) */
    MUSIC_SPEECH_5A = 10,  /* Music/speech flag */
    OPEN_DATA_5B = 11,     /* Open data */
    CT_DATE_6A = 12,       /* Clock-time and date */
    OPEN_DATA_6B = 13,     /* Open data */
    RP_ODA_7A = 14,        /* Radio Paging (RP) and ODA group */
    OPEN_DATA_7B = 15,     /* Open data */
    TMC_8A = 16,           /* Traffic Message Channel (TMC) */
    OPEN_DATA_8B = 17,     /* Open data */
    OPEN_DATA_9A = 18,     /* Open data */
    OPEN_DATA_9B = 19,     /* Open data */
    PTY_TP_10A = 20,       /* Program Type (PTY) and Traffic Program (TP) */
    OPEN_DATA_10B = 21,    /* Open data */
    OPEN_DATA_11A = 22,    /* Open data */
    OPEN_DATA_11B = 23,    /* Open data */
    OPEN_DATA_12A = 24,    /* Open data */
    OPEN_DATA_12B = 25,    /* Open data */
    EON_13A = 26,          /* Enhanced other Networks information (EON) */
    OPEN_DATA_13B = 27,    /* Open data */
    OPEN_DATA_14A = 28,    /* Open data */
    OPEN_DATA_14B = 29,    /* Open data */
    OPEN_DATA_15A = 30,    /* Open data */
    OPEN_DATA_15B = 31     /* Open data */
};
    enum decoder_information {
    mono = 0x00, // Bit d0, set to 0: Mono
    stereo = 0x01, // Bit d0, set to 1: Stereo
    not_artificial_head = 0x00, // Bit d1, set to 0: Not Artificial Head
    artificial_head = 0x02, // Bit d1, set to 1: Artificial Head
    not_compressed = 0x00, // Bit d2, set to 0: Not compressed
    compressed = 0x04, // Bit d2, set to 1: Compressed 
    static_pty = 0x00, // Bit d3, set to 0: Static PTY
    dynamic_pty = 0x08 // Bit d3, set to 1: Indicates that the PTY code on the tuned service, or referenced in EON variant 13, is dynamically switched
};

    // RDS message node structure (some RDS messages consist of several followed up messages)
    struct rds_message_list {
	union {
	    uint16_t rds_message16[4];
	    uint8_t rds_message8[8];
	};
	struct rds_message_list *next;
    };

    rds_encoder();
    ~rds_encoder();

    void rmlClear();
    void set_pi(const char* picode);
    void set_pty(pty_codes_eu pty);
    void set_ps(const char* stationname);
    void set_tp(bool traffic_program); // sets the flag we transmit traffic program/info
    void set_music();
    void set_speech();
    void enable_traffic();
    void disable_traffic();


    rds_message_list* get_ps_msg ();

private:

    uint16_t picode = 0;  // PI code of the station (kinda UID of the station)
    pty_codes_eu pty = pty_codes_eu::Education; //also uint8_t with static_cast<uint8_t>(PtyCodesEU::News);
    char ps[9] = "MY RADIO"; // really only 8 letters but compiler always adds \0
    bool tp = false; 		// we support/do traffic program
    bool ms = false;		// false=music, true=speech
    bool ta = false;		// currently a traffic announcement is runnin
    uint16_t parse_hex(const char* str);
    rds_message_list* rmlHead = NULL;  // to save the head position
    
};

#endif // RDSENCODER_H

