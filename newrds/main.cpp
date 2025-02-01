#include "rdsmanager.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

void consoleTransmit(const RDSMessage* msg, int BufferIndex) {
        printf("Sending RDS [%d]: %04X %04X %04X %04X\n", BufferIndex,
        msg->blocks[0], msg->blocks[1], msg->blocks[2], msg->blocks[3]);
}

int main()
{
    srand((unsigned)time(NULL));

    // Make an RDSManager with space for 32 groups
    RDSManager rds(32);
    rds.setTransmitter(consoleTransmit);

    // 1) Clear the buffer
    rds.clearBuffer();

    // 2) Add a station name (Group 0A) at position 0
    //    Suppose 8-char PS = "ROCK 101"
    int index = rds.addGroup0A(rds.parseHex("1234"), /*TP=*/1,/*PTY=*/0x00,/*TA=*/0, /*MS=*/1, "ROCK 101", 0);
    // 'index' now should be 4, plus the sentinel at 4 => next free is 4

    // 3) Add a single 4A group (time), after that
    index = rds.addGroup4A(0x1234, 2025, 1, 31, 12, 0, /*offset*/2, index);
    // 'index' is now 5, plus sentinel at 5 => next free is 5

    // Let's add another 4A group (like a second time or weighting)
    index = rds.addGroup4A(0x1234, 2025, 1, 31, 12, 5, /*offset*/2, index);
    // 'index' is now 6, sentinel at 6 => next free is 6

    index = rds.addGroup2A(0x1235,0,0x00, 0,"This is my station",index);

    rds.shuffleBuffer();

    // In total, the buffer has:
    //   [0] 0A chunk0
    //   [1] 0A chunk1
    //   [2] 0A chunk2
    //   [3] 0A chunk3
    //   [4] sentinel
    //   [4] 4A
    //   [5] sentinel
    //   [5] 4A
    //   [6] sentinel
    //
    // so effectively we have:
    //   - Four 0A subgroups
    //   - Then 1 4A group
    //   - Then 1 more 4A group
    //   - Then sentinel
    // Actually we overwrote the sentinel each time we added a group, so it looks like:
    //   idx=0..3 => 0A
    //   idx=4 => 4A
    //   idx=5 => 4A
    //   idx=6 => sentinel
    //
    // We'll cycle over 0..6 in update() sending 0A0,0A1,0A2,0A3,4A,4A, sentinel => wrap

    while (1)
    {
        rds.update();

        // Sleep ~10 ms
#ifdef _WIN32
        Sleep(10);

#else
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 10 * 1000000;
        nanosleep(&ts, NULL);
#endif
    }

    return 0;
}
