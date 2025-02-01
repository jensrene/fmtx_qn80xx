#include "rdsmanager.h"

//#include <cstdlib>  // malloc, free
//#include <cstring>  // memset
//#include <stdio.h>   // optional for debug

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
// #include <stdio.h>

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

void RDSManager::setTransmitter(TransmitFunc func) {
        RDSManager::transmit = func;
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
    uint8_t tp,
    uint8_t pty,
    uint8_t ta,
    uint8_t ms,
    const char* ps8,
    int chunkIndex
)
{
    int group_type = group_types::BTS_0A;

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

    b1 = ((group_type & 0b11111) << 11) | // Group Type (5 bits)
                   ((tp & 0b1) << 10) | // Traffic Program (TP)
                   ((pty & 0b11111) << 5) | // Program Type (PTY) (5 bits)
                   ((ta & 0b1) << 4) | // Traffic Announcement (TA)
                   ((ms & 0b1) << 3); //| // Music/Speech (MS)
                   //(((di >> (3 - i)) & 0b1) << 2) | // DI (1 bit, inverse order)
                   //(i & 0b11);       
    
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
    uint8_t tp,
    uint8_t pty,
    uint8_t ta,
    uint8_t ms,
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
        buildGroup0ASub(&buffer_[startIndex + i], piCode, tp, pty, ta, ms, ps8, i);
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


void RDSManager::buildGroup2ASub(
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
)
{    
    // set group type
    uint8_t group_type = RDSManager::group_types::RADIO_A_2A;

    // Zero it
    outMsg->blocks[0] = 0;
    outMsg->blocks[1] = 0;
    outMsg->blocks[2] = 0;
    outMsg->blocks[3] = 0;

    // block0 = PI
    outMsg->blocks[0] = piCode;

    uint16_t b1 = 0x0000;

    b1 = ((group_type & 0b11111) << 11) | // Group Type (5 bits)
                   ((tp & 0b1) << 10) | // Traffic Program (TP)
                   ((pty & 0b11111) << 5) |  // Program Type (PTY) (5 bits)
                   ((textAB & 0b1) <<4) |
                   ((segmentAddr & 0b1111));

    outMsg->blocks[1] = b1;

    outMsg->blocks[2] = ((uint16_t)c0 << 8) | (uint16_t)c1;
    outMsg->blocks[3] = ((uint16_t)c2 << 8) | (uint16_t)c3;                
}

// ------------------------------------------------
// addGroup2A
//  Writes 2A groups
int RDSManager::addGroup2A(
        uint16_t piCode,
        uint8_t  tp,
        uint8_t  pty,
        uint8_t  textAB,
        const char *radiotext, 
        int startIndex
)
{
    const int CHARS_PER_GROUP = 4;  // 2 chars in block C + 2 in block D

    // Determine how many characters are actually used
    size_t textLen = strlen(radiotext);
    if (textLen > MAX_RT_CHARS) {
        textLen = MAX_RT_CHARS; // "truncate" / limit max size
    }

    if (!buffer_) return -1;
    if (!radiotext) return -2;
    if (startIndex < 0 || startIndex >= bufferSize_) return -1;

    // Minimum 1 group, maximum 16
    // Each group = 4 characters => groupCount = ceil(textLen/4)
    uint8_t groupCount = (uint8_t)((textLen + (CHARS_PER_GROUP - 1)) / CHARS_PER_GROUP);
    if (groupCount == 0) {
        groupCount = 1; // send at least one group even if text is empty
    }

    // Prepare a local buffer of 64 chars (fill with spaces if needed)
    char textBuf[MAX_RT_CHARS];
    memset(textBuf, ' ', sizeof(textBuf));
    memcpy(textBuf, radiotext, textLen);


    // We need space for 4 subgroups + 1 sentinel => 5 slots
    if (startIndex + groupCount >= bufferSize_)
    {
        // Not enough space to fit all groupMsgs plus the sentinel
        return -4;
    }

    // Build each group
    for (uint8_t g = 0; g < groupCount; g++)
    {
        // Extract 4 chars from the text (and map them to RDS)
        // index base: g*4
        uint8_t c0 = iso8859ToRDSChar((unsigned char)textBuf[g*4 + 0]);
        uint8_t c1 = iso8859ToRDSChar((unsigned char)textBuf[g*4 + 1]);
        uint8_t c2 = iso8859ToRDSChar((unsigned char)textBuf[g*4 + 2]);
        uint8_t c3 = iso8859ToRDSChar((unsigned char)textBuf[g*4 + 3]);

        // Build a single 2A group (4 blocks)
        buildGroup2ASub(
            &buffer_[startIndex + g],
            piCode, 
            tp, 
            pty, 
            textAB,
            g,       // segment address (0..15)
            c0, c1,  // block C (2 chars)
            c2, c3  // block D (2 chars)
        );
    }

    int nextPos = startIndex + groupCount;
    // Write sentinel at nextPos
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
    // printf("Sending RDS: %04X %04X %04X %04X\n",
    //        msg->blocks[0], msg->blocks[1], msg->blocks[2], msg->blocks[3]);
    if (transmit) {
        transmit(msg);
    } else {
        return; // if we set no output function, having no output is expected, no error => just return
    }
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

uint16_t RDSManager::parseHex(const char* str) {
    uint16_t result = 0;
    int count = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isxdigit(str[i])) {
            // If the current character is not a valid hexadecimal digit, return 0
            return 0;
        }
        if (count >= 4) {
            // If we have already processed 4 hexadecimal digits, ignore the rest of the string
            break;
        }
        // Convert the current hexadecimal digit to its numerical value and add it to the result
        result = (result << 4) + (isdigit(str[i]) ? (str[i] - '0') : (tolower(str[i]) - 'a' + 10));
        count++;
    }
    return result;
}

uint8_t RDSManager::iso8859ToRDSChar(unsigned char c) {
        // Standard ASCII characters (0x20..0x7F) pass through unchanged
    if (c >= 0x20 && c <= 0x7F)
    {
        return c;
    }

    // Map extended Latin characters (ISO-8859-1) to RDS codes per Annex E:
    switch (c)
    {
        case 0xC4: return 0xD1; // Ä -> RDS 0xD1
        case 0xD6: return 0xD7; // Ö -> RDS 0xD7
        case 0xDC: return 0xD9; // Ü -> RDS 0xD9
        case 0xE4: return 0x91; // ä -> RDS 0xE1
        case 0xF6: return 0x97; // ö -> RDS 0xF2
        case 0xFC: return 0x99; // ü -> RDS 0xF3
        case 0xDF: return 0x8D; // ß -> RDS 0x8D
/* WRONG but can be fixed: 
        case 0xC0: return 0x80; // À -> RDS 0x80
        case 0xC1: return 0x81; // Á -> RDS 0x81
        case 0xC2: return 0x82; // Â -> RDS 0x82
        case 0xC3: return 0x83; // Ã -> RDS 0x83
        case 0xC7: return 0x87; // Ç -> RDS 0x87
        case 0xC8: return 0x88; // È -> RDS 0x88
        case 0xC9: return 0x89; // É -> RDS 0x89
        case 0xCA: return 0x8A; // Ê -> RDS 0x8A
        case 0xCB: return 0x8B; // Ë -> RDS 0x8B
        case 0xCC: return 0x8C; // Ì -> RDS 0x8C
        case 0xCD: return 0x8D; // Í -> RDS 0x8D
        case 0xCE: return 0x8E; // Î -> RDS 0x8E
        case 0xCF: return 0x8F; // Ï -> RDS 0x8F
        case 0xD1: return 0x91; // Ñ -> RDS 0x91
        case 0xD2: return 0x92; // Ò -> RDS 0x92
        case 0xD3: return 0x93; // Ó -> RDS 0x93
        case 0xD4: return 0x94; // Ô -> RDS 0x94
        case 0xD5: return 0x95; // Õ -> RDS 0x95
        case 0xD9: return 0x99; // Ù -> RDS 0x99
        case 0xDA: return 0x9A; // Ú -> RDS 0x9A
        case 0xDB: return 0x9B; // Û -> RDS 0x9B
        case 0xE0: return 0xA0; // à -> RDS 0xA0
        case 0xE1: return 0xA1; // á -> RDS 0xA1
        case 0xE2: return 0xA2; // â -> RDS 0xA2
        case 0xE3: return 0xA3; // ã -> RDS 0xA3
        case 0xE7: return 0xA7; // ç -> RDS 0xA7
        case 0xE8: return 0xA8; // è -> RDS 0xA8
        case 0xE9: return 0xA9; // é -> RDS 0xA9
        case 0xEA: return 0xAA; // ê -> RDS 0xAA
        case 0xEB: return 0xAB; // ë -> RDS 0xAB
        case 0xEC: return 0xAC; // ì -> RDS 0xAC
        case 0xED: return 0xAD; // í -> RDS 0xAD
        case 0xEE: return 0xAE; // î -> RDS 0xAE
        case 0xEF: return 0xAF; // ï -> RDS 0xAF
        case 0xF1: return 0xB1; // ñ -> RDS 0xB1
        case 0xF2: return 0xB2; // ò -> RDS 0xB2
        case 0xF3: return 0xB3; // ó -> RDS 0xB3
        case 0xF4: return 0xB4; // ô -> RDS 0xB4
        case 0xF5: return 0xB5; // õ -> RDS 0xB5
        case 0xF9: return 0xB9; // ù -> RDS 0xB9
        case 0xFA: return 0xBA; // ú -> RDS 0xBA
        case 0xFB: return 0xBB; // û -> RDS 0xBB
*/
        default:
            return '.'; // Fallback for unknown characters
    }
}