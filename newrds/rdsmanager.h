#ifndef RDS_MANAGER_H
#define RDS_MANAGER_H

#include <stdint.h>
// #include <time.h>
#include <functional>

// Example: We can send ~10 groups/sec
#define RDS_SEND_INTERVAL_MS 100
#define MAX_RT_CHARS 64

// A single RDS group = 4 × 16-bit blocks
struct RDSMessage
{
    uint16_t blocks[4];
 };

// We'll store multiple RDS groups in a buffer. We mark the end with a sentinel.
class RDSManager
{
public:

    using TransmitFunc = std::function<void(const RDSMessage*, const int)>;

    void setTransmitter(TransmitFunc func);

    // Constructor: allocate 'bufferSize' RDS groups
    explicit RDSManager(int bufferSize = 64);
    ~RDSManager();    

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



    // Clears the entire buffer and sets a sentinel at position 0 (meaning “end”).
    // So, no groups will be sent until you add something.
    void clearBuffer();

    // ------------------------------------------------
    // shuffleBuffer
    //
    // Finds the first sentinel => that defines how many valid entries we have.
    // Then do a Fisher-Yates shuffle on [0..count-1], leaving the sentinel in place.
    // Finally, reset nextSendIndex_ = 0 so we start from the newly shuffled order.
    // For consistancy, it would be adviced to only do shuffle when nextSendIndex_ is 
    // naturally 0, means we just finished with the previous buffer
    void shuffleBuffer();

    // Adds an entire "station name" in Group 0A form, which typically takes 4 RDS groups
    // for an 8-char PS (2 chars per group). Also sets bits for TA, MS, etc.
    //   - piCode: 16-bit PI code
    //   - ta: Traffic Announcement (0 or 1)
    //   - ms: Music/Speech flag (0 or 1)
    //   - ps8: exactly 8 chars of station name (if shorter, you can pad externally)
    //   - startIndex: where to begin writing in the buffer
    // Returns the index *after* the last written group (i.e., next free spot),
    // or -1 if there's not enough space.
    int addGroup0A(
        uint16_t piCode,
        uint8_t tp,
        uint8_t pty,
        uint8_t ta,
        uint8_t ms,
        const char* ps8,
        int startIndex
    );

    // Adds one Group 4A (Clock-Time and Date) if you want. Typically, you'd call this repeatedly
    // if you want the time to appear often in your “playlist.”
    //   - piCode: station PI
    //   - year, month, day, hour, minute: used to compute MJD and time offset
    //   - localTimeOffsetHalfHours: local time offset in half-hour increments (e.g., +1.0 hour = 2, +5.5 hours = 11)
    //   - startIndex: buffer position
    // Returns the index after this group is written (startIndex+1) or -1 if out of space.
    int addGroup4A(
        uint16_t piCode,
        int year,
        int month,
        int day,
        int hour,
        int minute,
        int localTimeOffsetHalfHours,
        int startIndex
    );

    /**
     * High-level function that constructs up to 16 RDS Group 2A messages
     * for a complete RadioText of up to 64 characters.
     *
     *  - Splits the text into 16 segments (4 chars each).
     *  - Applies ISO-8859-1→RDS mapping.
     *  - Calls buildGroup2ASub for each segment.
     *
     * @param piCode     PI code
     * @param tp         Traffic Program
     * @param pty        Program Type
     * @param textAB     Text A/B flag
     * @param radiotext  Null-terminated string, up to 64 chars
     * @param startIndex Index in the rds buffer to put these messages.
     * @return           Number of groups used (1..16).
     */
    int addGroup2A(
        uint16_t piCode,
        uint8_t  tp,
        uint8_t  pty,
        uint8_t  textAB,
        const char *radiotext,
        int startIndex
    );

    // The main loop function. Pass current time in ms (monotonic).
    // If enough time has passed, sends the next group in the buffer.
    // If that group is the sentinel, wrap back to 0.
    void update();
    uint16_t parseHex(const char* str);

    /**
     * Convert an ISO-8859-1 / Windows-1252 character into the RDS character set 
     * as per EN 50067 Annex E Table E.1.
     *
     * - If the character exists in the RDS encoding, it is mapped.
     * - If the character does not exist, it is replaced with '.' (fallback).
     */
    uint8_t iso8869ToRDSChar(unsigned char c);    

private:
    RDSMessage* buffer_;      // The array of RDS groups
    int bufferSize_;          // How many RDS groups are allocated

    int nextSendIndex_;       // Which buffer index we send next
    uint64_t lastSendMs_;     // Last time we sent a group

    TransmitFunc transmit = nullptr;

private:
    // Builds one subgroup of 0A for the station name (2 chars out of 8).
    //   chunkIndex in [0..3], each chunk transmits 2 characters.
    void buildGroup0ASub(
        RDSMessage* outMsg,
        uint16_t piCode,
        uint8_t tp,
        uint8_t pty,
        uint8_t ta,
        uint8_t ms,
        const char* ps8,
        int chunkIndex
    );

    // Builds one 4A group for clock-time & date. (Simplified demonstration.)
    void buildGroup4A(
        RDSMessage* outMsg,
        uint16_t piCode,
        int year,
        int month,
        int day,
        int hour,
        int minute,
        int localTimeOffsetHalfHours
    );

    /**
     * Build RDS Group 2A blocks for RadioText, with correct Block B bit layout
     * per EN 50067. 
     *
     *  - Each "Group 2A" has 4 blocks (A,B,C,D), each 16 bits:
     *        Block A = PI Code (16 bits)
     *        Block B = [bits15..11: GroupTypeCode=4 for '2A'] +
     *                  [bit10: TP] + [bits9..5: PTY] +
     *                  [bit4: TextAB] + [bits3..0: Segment Address]
     *        Block C,D = 2 characters each from the RDS code table.
     *
     *  - Up to 64 chars of RadioText => 16 segments => 16 groups of 4 chars each.
     *  - We store each 16-bit block in the lower 16 bits of a 32-bit word 
     *    (for your "4×4 bytes" chunk style).
     *     
     * @param outMsg : target array/buffer
     * @param piCode    : 16-bit PI code
     * @param tp        : 1-bit Traffic Program flag (0 or 1)
     * @param pty       : 5-bit Program Type (0..31)
     * @param textAB    : 1-bit Text A/B flag (toggled when text changes), 0=A, 1=B
     * @param rt : Null-terminated string (up to 64 chars used)     
     * @return Number of RDS groups produced (1..16).
     */
    void buildGroup2ASub(
        RDSMessage* outMsg,
        uint16_t piCode,
        uint8_t tp,
        uint8_t pty,
        uint8_t textAB,
        uint8_t  segmentAddr,
        uint8_t  c0,
        uint8_t  c1,
        uint8_t  c2,
        uint8_t  c3
    );

    uint64_t getCurrentTimeMs(void);

    // Helper to compute Modified Julian Date (MJD) from Y/M/D (simple version).
    // RDS uses MJD in group 4A.
    uint16_t computeMJD(int year, int month, int day);

    // Write a sentinel at buffer[index] if valid
    void writeSentinel(int index);

    // Check if buffer[index] is sentinel
    bool isSentinel(int index);    

    // Actually sends out an RDS message over I2C (stub).
    void sendRDSOverI2C(const RDSMessage* msg);
    

    uint8_t rt_abflag = 0;	// ab flag in RT to signal changes

    uint8_t iso8859ToRDSChar(unsigned char c);
};

#endif // RDS_MANAGER_H
