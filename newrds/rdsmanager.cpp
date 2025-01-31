#include "rdsmanager.h"

//#include <cstdlib>  // malloc, free
//#include <cstring>  // memset
//#include <stdio.h>   // optional for debug

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

// A convenient sentinel pattern: 0xFFFF in all blocks
static const uint16_t SENTINEL_VAL = 0xFFFF;

RDSManager::RDSManager(int bufferSize)
{
    bufferSize_ = bufferSize;
    if (bufferSize_ < 2)
        bufferSize_ = 2; // minimal

    buffer_ = (RDSMessage*)malloc(sizeof(RDSMessage) * bufferSize_);
    if (buffer_)
    {
        // Initialize everything as sentinel
        for (int i = 0; i < bufferSize_; i++)
        {
            for (int b = 0; b < 4; b++)
                buffer_[i].blocks[b] = SENTINEL_VAL;
        }
    }

    nextSendIndex_ = 0;
    lastSendMs_ = getCurrentTimeMs();
}

RDSManager::~RDSManager()
{
    if (buffer_)
    {
        free(buffer_);
        buffer_ = NULL;
    }
}

// ------------------------------------------------
// clearBuffer
//  Set the very first slot to sentinel => means “empty.”
void RDSManager::clearBuffer()
{
    if (!buffer_) return;
    for (int i = 0; i < bufferSize_; i++)
    {
        for (int b = 0; b < 4; b++)
            buffer_[i].blocks[b] = SENTINEL_VAL;
    }
    nextSendIndex_ = 0;
}

// ------------------------------------------------
// isSentinel
bool RDSManager::isSentinel(int index)
{
    if (!buffer_) return true; // treat as sentinel
    if (index < 0 || index >= bufferSize_) return true;
    return (buffer_[index].blocks[0] == SENTINEL_VAL &&
            buffer_[index].blocks[1] == SENTINEL_VAL &&
            buffer_[index].blocks[2] == SENTINEL_VAL &&
            buffer_[index].blocks[3] == SENTINEL_VAL);
}

// ------------------------------------------------
// writeSentinel
void RDSManager::writeSentinel(int index)
{
    if (!buffer_) return;
    if (index < 0 || index >= bufferSize_) return;

    buffer_[index].blocks[0] = SENTINEL_VAL;
    buffer_[index].blocks[1] = SENTINEL_VAL;
    buffer_[index].blocks[2] = SENTINEL_VAL;
    buffer_[index].blocks[3] = SENTINEL_VAL;
}

// ------------------------------------------------
// buildGroup0ASub
//  Build one 0A subgroup (2 chars out of an 8-char PS)
void RDSManager::buildGroup0ASub(
    RDSMessage* outMsg,
    uint16_t piCode,
    int ta,
    int ms,
    const char* ps8,
    int chunkIndex
)
{
    // Zero it
    outMsg->blocks[0] = 0;
    outMsg->blocks[1] = 0;
    outMsg->blocks[2] = 0;
    outMsg->blocks[3] = 0;

    // block0 = PI
    outMsg->blocks[0] = piCode;

    // block1: group code = 0, version A => bits [15..12]=0, bit 11=0
    // bit 10 => TP=0 (just an example), bits [9..5] => PTY=0,
    // bit4 => MS, bit3 => TA
    // bits [2..1] => address (which chunk?), bit0 => textAB=0 for simplicity
    uint16_t b1 = 0x0000;
    if (ms) { b1 |= (1 << 4); } // MS
    if (ta) { b1 |= (1 << 3); } // TA

    // chunkIndex goes in [2..1]
    // e.g. chunk 0 => address=0, chunk1 =>1, chunk2=>2, chunk3=>3
    // So let's do:
    uint16_t address = (chunkIndex & 0x03);
    b1 |= (address << 1);

    outMsg->blocks[1] = b1;

    // block2: for a full 0A, might carry AF or something else.
    // We'll just set it to 0 for simplicity
    outMsg->blocks[2] = 0x0000;

    // block3: 2 characters from ps8
    int c1Index = chunkIndex * 2;
    int c2Index = c1Index + 1;
    if (ps8)
    {
        unsigned char c1 = (unsigned char)ps8[c1Index];
        unsigned char c2 = (unsigned char)ps8[c2Index];
        outMsg->blocks[3] = (c1 << 8) | c2;
    }
    else
    {
        outMsg->blocks[3] = 0x2020; // two spaces
    }
}

// ------------------------------------------------
// addGroup0A
//  Writes 4 subgroups for an 8-char station name using group 0A.
//  Returns the index after the last written group. Also writes a sentinel there.
int RDSManager::addGroup0A(
    uint16_t piCode,
    int ta,
    int ms,
    const char* ps8,
    int startIndex
)
{
    if (!buffer_) return -1;
    if (!ps8) return -1;
    if (startIndex < 0 || startIndex >= bufferSize_) return -1;

    // We need space for 4 subgroups + 1 sentinel => 5 slots
    if (startIndex + 4 >= bufferSize_)
    {
        // Not enough space to fit all 4 plus the sentinel
        return -1;
    }

    // Build 4 subgroups
    for (int i = 0; i < 4; i++)
    {
        buildGroup0ASub(&buffer_[startIndex + i], piCode, ta, ms, ps8, i);
    }

    int nextPos = startIndex + 4;
    // Write sentinel at nextPos
    writeSentinel(nextPos);

    return nextPos;
}


// ------------------------------------------------
// computeMJD
//
// A simplistic function to compute the Modified Julian Date for RDS group 4A
// from a given Y/M/D. 
// Proper formula from the RDS spec is: 
//   MJD = (int)(JD - 2400000.5)
//   JD is Julian Date. 
// For brevity, this is a short approximation. 
// You might use a more robust approach if needed.
uint16_t RDSManager::computeMJD(int year, int month, int day)
{
    // A standard formula from RDS references:
    //  If month <= 2, then year--, month+=12
    if (month <= 2)
    {
        year -= 1;
        month += 12;
    }
    long a = year / 100;
    long b = 2 - a + (a / 4);

    // Julian Date for 0h UT
    long jd = (long)(365.25 * (year + 4716)) +
              (long)(30.6001 * (month + 1)) +
              day + b - 1524;

    // MJD = JD - 2400000.5 ~ JD - 2400001
    // We'll do integer math:
    long mjd = jd - 2400001;
    return (uint16_t)(mjd & 0xFFFF);
}

// ------------------------------------------------
// buildGroup4A
//   Build a single 4A group with clock-time & date (MJD).
void RDSManager::buildGroup4A(
    RDSMessage* outMsg,
    uint16_t piCode,
    int year,
    int month,
    int day,
    int hour,
    int minute,
    int localTimeOffsetHalfHours
)
{
    // Clear
    outMsg->blocks[0] = 0;
    outMsg->blocks[1] = 0;
    outMsg->blocks[2] = 0;
    outMsg->blocks[3] = 0;

    // block0 = PI
    outMsg->blocks[0] = piCode;

    // block1: group code=4 => bits[15..12]=4 => 0100, version A => bit11=0
    // For simplicity: TP=0, PTY=0 => bits[10..5]=0
    // => block1 = 0x4000
    outMsg->blocks[1] = 0x4000;

    // block2 & block3 carry date/time
    // According to RDS spec for Group 4A:
    //   block2 bits [15..0] = MJD[15..0]
    //   block3 bits [15..11] = MJD[4..0], bits [10..6] = hour, bits [5..0] = minute
    // Actually the spec is more intricate: top bits of block3 also hold local offset.
    // We do a simplified approach:

    // 1) Compute MJD
    uint16_t mjd = computeMJD(year, month, day);

    // Store entire MJD in block2
    outMsg->blocks[2] = mjd;

    // block3 structure (per RBDS/RDS 4A):
    //   bits [15..11] = MJD[4..0]  (the lower 5 bits of MJD)
    //   bits [10..6]  = hour (0..23)
    //   bits [5..0]   = minute (0..59)
    // But we also have a 5-bit offset in bits [4..0] of block3, usually.
    // The spec merges minute and offset. Let's do a simplified partial approach:
    // We'll place the entire MJD's low 5 bits in [15..11],
    // hour in [10..6], minute in [5..0] - localTimeOffsetHalfHours is typically in bits [4..0], 
    // so let's do an approximate approach:
    uint16_t lowMJD = (mjd & 0x1F); // 5 bits
    uint16_t b3 = (lowMJD << 11);

    // hour => bits [10..6]
    if (hour < 0) hour = 0;
    if (hour > 23) hour = 23;
    b3 |= ((hour & 0x1F) << 6);

    // minute => bits [5..0]
    // But we also have to consider localTimeOffset. In the real spec, bits [4..0] are offset,
    // and bits [5..0] are the minutes. We'll do a partial approach:
    // Let's store minute in bits [5..0], ignoring offset for brevity. 
    if (minute < 0) minute = 0;
    if (minute > 59) minute = 59;
    b3 |= (minute & 0x3F);

    outMsg->blocks[3] = b3;

    // A truly correct 4A build would also store localTimeOffset in bits [4..0]
    // and shift minute up by 1 bit, etc. We skip that for brevity.
    (void)localTimeOffsetHalfHours; // not fully used in this example
}

// ------------------------------------------------
// addGroup4A
//  Writes one single 4A group. Returns next index = startIndex+1, plus sentinel, or -1 if no space.
int RDSManager::addGroup4A(
    uint16_t piCode,
    int year,
    int month,
    int day,
    int hour,
    int minute,
    int localTimeOffsetHalfHours,
    int startIndex
)
{
    if (!buffer_) return -1;
    if (startIndex < 0 || startIndex >= bufferSize_) return -1;

    // We need 1 slot for the group plus 1 for the sentinel => 2 total
    if (startIndex + 1 >= bufferSize_)
    {
        return -1; // no space
    }

    buildGroup4A(&buffer_[startIndex], piCode, year, month, day, hour, minute, localTimeOffsetHalfHours);

    int nextPos = startIndex + 1;
    // place sentinel
    writeSentinel(nextPos);

    return nextPos;
}

// ------------------------------------------------
// sendRDSOverI2C
//  Stub for actual I2C writes to the QN8066 (or similar)
void RDSManager::sendRDSOverI2C(const RDSMessage* msg)
{
    // In practice, you'd do:
    //   1) Write msg->blocks[i] to QN8066 RDS registers
    //   2) Trigger RDS send
    //
    // For demonstration, we just might do:
     printf("Sending RDS: %04X %04X %04X %04X\n",
            msg->blocks[0], msg->blocks[1], msg->blocks[2], msg->blocks[3]);
}

uint64_t RDSManager::getCurrentTimeMs(void) 
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); 
    // Convert to milliseconds
    return (uint64_t)(ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL);
}


// ------------------------------------------------
// update
//  Non-blocking: if enough time has passed, send buffer_[nextSendIndex_] 
//  then nextSendIndex_++. If we see sentinel, wrap to 0. 
void RDSManager::update()
{
    if (!buffer_) return;

    uint64_t currentTimeMs = getCurrentTimeMs();
    // Check rate limiting
    if ((currentTimeMs - lastSendMs_) < RDS_SEND_INTERVAL_MS)
        return;

    lastSendMs_ = currentTimeMs;

    // Check sentinel => if so, wrap
    if (isSentinel(nextSendIndex_))
    {
        // Loop back to 0 if the buffer’s first slot is not also sentinel
        nextSendIndex_ = 0;
        if (isSentinel(nextSendIndex_))
        {
            // Means the buffer is truly empty; nothing to send
            return;
        }
    }

    // Send the group
    sendRDSOverI2C(&buffer_[nextSendIndex_]);

    // Move forward
    nextSendIndex_++;
    if (nextSendIndex_ >= bufferSize_)
    {
        // wrap around
        nextSendIndex_ = 0;
    }
}
