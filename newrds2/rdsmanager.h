// rds.h
#ifndef RDSMANAGER_H
#define RDSMANAGER_H

#include "rdsencoder.h"

class rdsmanager : public rdsencoder {
public:
   
    // ENUMS

     enum ptyCodesEu : uint8_t {
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

    // STRUCTSS

    // Holds info about a group code + scheduling weight
    struct RDSGroupConfig
    {
        uint8_t  groupCode;  // e.g. 0x0A, 0x2A, 0x4A
        uint8_t  weight;     // Frequency weighting
    };

    // FUNCS

    explicit rdsmanager(int bufferSize = 64);
    explicit rdsmanager();
    ~rdsmanager();
    
    /**
    * @brief Add or Update a group with given weight
    *
    * The "weight" concept is how frequently we want to schedule this group
    * relative to others. For example, if we have:
    *   - 0A with weight = 5
    *   - 2A with weight = 2
    *   - 4A with weight = 1
    * Then the total weight is 8. In a weighted round-robin approach, we pick
    * from these groups with probabilities 5/8, 2/8, and 1/8 respectively.
    * The higher the weight, the more often the group is sent.
    *
    * In many RDS setups, 0A is given a higher weight since it carries essential
    * station info (PI, PS, etc.). 2A might be used for RadioText (moderate
    * frequency), and 4A might be used for Clock Time (less frequent).
    * This is just an example usage of weighting.
    *
    * If the group already exists, we update its weight (and totalWeight).
    * If weight=0, we remove the group entirely.
    *
    * Common RDS Example:
    *   addGroup(GROUP_0A, 5);  // Very frequent
    *   addGroup(GROUP_2A, 3);  // Moderately frequent
    *   addGroup(GROUP_4A, 1);  // Less frequent
    *
    */     
    void addGroup(uint8_t groupCode, uint8_t weight);

    /** 
    * remove an existing group from the schedule
    */
    void rdsmanager::delGroup(uint8_t groupCode);

    /**
    * Weighted round-robin - get the next message to send.
    * (maybe be private if we implement an more call based method with sender func)
    */
    RDSMessage rdsmanager::getNextMessage();

   /** 
    * @brief set transmitter function to be used to send RDS out.
    * 
    * gets called every time we want to send out a single RDS message.
    * function structure needed : void(const RDSMessage*) 
    * (See RDSManager::TransmitFunc)
    */
    void setTransmitter(TransmitFunc func);

    void setTransmitter(TransmitFunc func, bool useBufferTransmitter);


 /**
     * @brief Sets the Program Identification (PI) code.
     * 
     * The PI code is a unique identifier for the radio station, allowing receivers
     * to recognize and switch to the same station when using auto-tuning or AF (Alternative Frequencies).
     * 
     * @param picode A pointer to a character array (C-string) representing the PI code.
     *               
     */
    void setPi(const char* picode);

    /**
     * @brief Sets the Program Type (PTY) code.
     * 
     * PTY defines the genre or content type of the broadcast (e.g., News, Pop Music, Weather, etc.).
     * This is based on the standardized PTY codes for the RDS system.
     * 
     * @param pty The PTY code as defined in the `pty_codes_eu` enumeration.
     */
    void setPty(ptyCodesEu pty);

    /**
     * @brief Sets the Program Service (PS) name.
     * 
     * The PS name is an 8-character station identifier displayed on RDS-compatible receivers.
     * If the station name is shorter than 8 characters, it should be padded with spaces.
     * 
     * @param stationname A pointer to a character array (C-string) containing the station name.
     */
    void setPs(const char* stationname);

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
    void setTp(bool traffic_program);

    /**
     * @brief Sets the RadioText (RT) message.
     * 
     * RadioText is a scrolling text message that provides additional station information,
     * such as the currently playing song, station announcements, or promotions.
     * 
     * @param radiotext A pointer to a character array (C-string) containing the radio text message.
     *                  The pointer must remain valid while the message is in use.
     */
    void setRt(const char* radiotext);

    /**
     * @brief Marks the broadcast as music.
     * 
     * RDS allows marking the type of content being broadcast. This function sets the
     * Music/Speech flag to indicate that the station is currently transmitting music.
     */
    void setMusic();

    /**
     * @brief Marks the broadcast as speech.
     * 
     * This function sets the Music/Speech flag to indicate that the station is currently
     * transmitting speech (e.g., news, talk shows, spoken-word content).
     */
    void setSpeech();

    /**
     * @brief Enables the Traffic Announcement (TA) flag.
     * 
     * When this flag is set, RDS-compatible receivers that are set to prioritize
     * traffic information will temporarily switch to this station.
     * The flag should be set only when an actual traffic announcement is being made.
     */
    void enableTraffic();

    /**
     * @brief Disables the Traffic Announcement (TA) flag.
     * 
     * This function clears the Traffic Announcement flag, signaling to receivers
     * that the station is no longer broadcasting an active traffic message.
     */
    void disableTraffic();


private:
    TransmitFunc p_transmit = nullptr;    
    bool p_oldtransmit = false;

    uint16_t p_picode = 0;  // PI code of the station (kinda UID of the station)
    ptyCodesEu p_pty = ptyCodesEu::Education; //also uint8_t with static_cast<uint8_t>(PtyCodesEU::News);
    char p_ps[9] = "MY RADIO"; // really only 8 letters but compiler always adds \0
    bool p_tp = false; 		// we support/do traffic program
    bool p_ms = false;		// false=music, true=speech
    bool p_ta = false;		// currently a traffic announcement is runnin
    bool p_rds_changed = true;// do we have changes, do we need to regenerate RDS messages?
    char* p_rt = NULL;		// Radiotext string    

    // For 2A (RadioText) segmentation
    // Each 2A group can carry 4 text chars => 16 segments max for 64 chars
    uint8_t  p_2ASegmentCount;  // total segments in the current RT
    uint8_t  p_2ASegmentIndex;  // which segment we send next

    // For 0A (RadioText) segmentation
    // Each 0A group can carry 2 text chars => 4 segments max for 8 chars
    uint8_t  p_0ASegmentCount;  // total segments in the current RT
    uint8_t  p_0ASegmentIndex;  // which segment we send next

    // Weighted scheduler data
    RDSGroupConfig* p_groups;     // dynamically allocated array
    size_t          p_groupCount;
    size_t          p_groupCapacity;
    uint16_t        p_totalWeight;
    uint16_t        p_schedulerCounter;

    void rdsmanager::ensureCapacity(size_t minCapacity);

};

#endif // RDSMANAGER_H