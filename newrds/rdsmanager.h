#ifndef RDS_MANAGER_H
#define RDS_MANAGER_H

#include <stdint.h>
// #include <time.h>

// Example: We can send ~10 groups/sec
#define RDS_SEND_INTERVAL_MS 100

// A single RDS group = 4 × 16-bit blocks
struct RDSMessage
{
    uint16_t blocks[4];
 };

// We'll store multiple RDS groups in a buffer. We mark the end with a sentinel.
class RDSManager
{
public:
    // Constructor: allocate 'bufferSize' RDS groups
    explicit RDSManager(int bufferSize = 64);
    ~RDSManager();

    // Clears the entire buffer and sets a sentinel at position 0 (meaning “end”).
    // So, no groups will be sent until you add something.
    void clearBuffer();

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
        int ta,
        int ms,
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

    // The main loop function. Pass current time in ms (monotonic).
    // If enough time has passed, sends the next group in the buffer.
    // If that group is the sentinel, wrap back to 0.
    void update();

private:
    RDSMessage* buffer_;      // The array of RDS groups
    int bufferSize_;          // How many RDS groups are allocated

    int nextSendIndex_;       // Which buffer index we send next
    uint64_t lastSendMs_;     // Last time we sent a group

private:
    // Builds one subgroup of 0A for the station name (2 chars out of 8).
    //   chunkIndex in [0..3], each chunk transmits 2 characters.
    void buildGroup0ASub(
        RDSMessage* outMsg,
        uint16_t piCode,
        int ta,
        int ms,
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
};

#endif // RDS_MANAGER_H
