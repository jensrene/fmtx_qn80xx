#ifndef RDSINFO_H
#define RDSINFO_H

#include <stdint.h>
#include <stdlib.h>


class rdsinfo {

     /**
     * @brief Constructor of rdsinfo class
     * 
     * 
     * @param bufferSize the size of the rds internal rdsmassage buffer. Each RDS message requires 1 entry, but some data requires multiple rds messages, so be considerate!
     *               
     */
    explicit rdsinfo(int bufferSize = 64);
    ~rdsinfo();

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


 /**
     * @brief Sets the Program Identification (PI) code.
     * 
     * The PI code is a unique identifier for the radio station, allowing receivers
     * to recognize and switch to the same station when using auto-tuning or AF (Alternative Frequencies).
     * 
     * @param picode A pointer to a character array (C-string) representing the PI code.
     *               
     */
    void set_pi(const char* picode);

    /**
     * @brief Sets the Program Type (PTY) code.
     * 
     * PTY defines the genre or content type of the broadcast (e.g., News, Pop Music, Weather, etc.).
     * This is based on the standardized PTY codes for the RDS system.
     * 
     * @param pty The PTY code as defined in the `pty_codes_eu` enumeration.
     */
    void set_pty(pty_codes_eu pty);

    /**
     * @brief Sets the Program Service (PS) name.
     * 
     * The PS name is an 8-character station identifier displayed on RDS-compatible receivers.
     * If the station name is shorter than 8 characters, it should be padded with spaces.
     * 
     * @param stationname A pointer to a character array (C-string) containing the station name.
     */
    void set_ps(const char* stationname);

    /**
     * @brief Sets the Traffic Program (TP) flag.
     * 
     * This flag indicates whether the station provides traffic announcements.
     * If set to true, RDS receivers may prioritize tuning into this station during traffic updates.
     * 
     * @param traffic_program A boolean value: 
     *                        - `true` if the station provides traffic information.
     *                        - `false` otherwise.
     */
    void set_tp(bool traffic_program);

    /**
     * @brief Sets the RadioText (RT) message.
     * 
     * RadioText is a scrolling text message that provides additional station information,
     * such as the currently playing song, station announcements, or promotions.
     * 
     * @param radiotext A pointer to a character array (C-string) containing the radio text message.
     *                  The pointer must remain valid while the message is in use.
     */
    void set_rt(const char* radiotext);

    /**
     * @brief Marks the broadcast as music.
     * 
     * RDS allows marking the type of content being broadcast. This function sets the
     * Music/Speech flag to indicate that the station is currently transmitting music.
     */
    void set_music();

    /**
     * @brief Marks the broadcast as speech.
     * 
     * This function sets the Music/Speech flag to indicate that the station is currently
     * transmitting speech (e.g., news, talk shows, spoken-word content).
     */
    void set_speech();

    /**
     * @brief Enables the Traffic Announcement (TA) flag.
     * 
     * When this flag is set, RDS-compatible receivers that are set to prioritize
     * traffic information will temporarily switch to this station.
     * The flag should be set only when an actual traffic announcement is being made.
     */
    void enable_traffic();

    /**
     * @brief Disables the Traffic Announcement (TA) flag.
     * 
     * This function clears the Traffic Announcement flag, signaling to receivers
     * that the station is no longer broadcasting an active traffic message.
     */
    void disable_traffic();

private:

    unsigned char safeCharOrCR(const char* rt, size_t index, size_t length);

    uint16_t picode = 0;  // PI code of the station (kinda UID of the station)
    pty_codes_eu pty = pty_codes_eu::Education; //also uint8_t with static_cast<uint8_t>(PtyCodesEU::News);
    char ps[9] = "MY RADIO"; // really only 8 letters but compiler always adds \0
    bool tp = false; 		// we support/do traffic program
    bool ms = false;		// false=music, true=speech
    bool ta = false;		// currently a traffic announcement is runnin
    bool rt_new = true;		// is the RT new/changed?

    char* rt = NULL;		// Radiotext string    

};

#endif
