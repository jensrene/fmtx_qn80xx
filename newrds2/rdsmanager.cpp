#include <string.h>

#include "rdsmanager.h"



// Constructor
rdsmanager::rdsmanager(int bufferSize) : rdsencoder(bufferSize)
{
    
}

// default constructor => Assume Buffer of 1 since we don't really use that buffer anymore.
// EXCEPTION: if we plan to use the rdsencoder sender/addGroup functions.
rdsmanager::rdsmanager() : rdsencoder(1)
{

}

// Destructor
rdsmanager::~rdsmanager()
{

}



// -----------------------------------------
// Ensure capacity for at least minCapacity
// -----------------------------------------
void rdsmanager::ensureCapacity(size_t minCapacity)
{
    if (minCapacity <= p_groupCapacity)
    {
        return;
    }

    size_t newCap = p_groupCapacity;
    while (newCap < minCapacity)
    {
        newCap *= 2;
    }

    RDSGroupConfig* newPtr = (RDSGroupConfig*)malloc(newCap * sizeof(RDSGroupConfig));
    if (!newPtr) {
        // handle error or throw
        return;
    }

    memcpy(newPtr, p_groups, p_groupCount * sizeof(RDSGroupConfig));
    memset(newPtr + p_groupCount, 0, (newCap - p_groupCount) * sizeof(RDSGroupConfig));

    free(p_groups);
    p_groups = newPtr;
    p_groupCapacity = newCap;
}


void rdsmanager::addGroup(uint8_t groupCode, uint8_t weight)
{
    if (weight == 0)
    {
        // weight=0 => remove
        delGroup(groupCode);
        return;
    }

    // Check if it exists
    for (size_t i = 0; i < p_groupCount; i++)
    {
        if (p_groups[i].groupCode == groupCode)
        {
            // Update totalWeight
            p_totalWeight = (uint16_t)(p_totalWeight - p_groups[i].weight + weight);
            p_groups[i].weight = weight;
            return;
        }
    }

    // Not found => add new
    ensureCapacity(p_groupCount + 1);
    p_groups[p_groupCount].groupCode = groupCode;
    p_groups[p_groupCount].weight    = weight;
    p_groupCount++;

    p_totalWeight = (uint16_t)(p_totalWeight + weight);
}

// -----------------------------------------
// delGroup() - remove a group
// -----------------------------------------
void rdsmanager::delGroup(uint8_t groupCode)
{
    for (size_t i = 0; i < p_groupCount; i++)
    {
        if (p_groups[i].groupCode == groupCode)
        {
            p_totalWeight = (uint16_t)(p_totalWeight - p_groups[i].weight);

            // shift down
            for (size_t j = i; j < p_groupCount - 1; j++)
            {
                p_groups[j] = p_groups[j+1];
            }
            p_groupCount--;
            return;
        }
    }
}

// -----------------------------------------
// getNextMessage() - Weighted round-robin
// -----------------------------------------
RDSMessage rdsmanager::getNextMessage()
{
    if (p_groupCount == 0 || p_totalWeight == 0)
    {
        // fallback
        return buildGroup0A();
    }

    p_schedulerCounter++;
    if (p_schedulerCounter >= p_totalWeight)
    {
        p_schedulerCounter = 0;
    }

    uint16_t runningSum = 0;
    for (size_t i = 0; i < p_groupCount; i++)
    {
        runningSum += p_groups[i].weight;
        if (p_schedulerCounter < runningSum)
        {
            return buildGroup(p_groups[i].groupCode); // BUILD NEW BUILDER CLASSES HANDLING SEGMENTING INTERNALLY(!)
        }
    }

    // Fallback
    return buildGroup0A();  // BUILD NEW BUILDER CLASSES HANDLING SEGMENTING INTERNALLY(!)
}


void rdsmanager::setTransmitter(TransmitFunc func) 
{
    rdsmanager::p_transmit = func;
}

/** @brief extended setTransmitter, allows to also use "old"/rdsencoder transmitter.
 *  Sets the Transmitter function called when there is new data to transmit. 
 */
void rdsmanager::setTransmitter(TransmitFunc func, bool useBufferTransmitter) 
{
    if (useBufferTransmitter) {
        rdsencoder::setTransmitter(func);
        p_oldtransmit = true;
    } else {
        rdsmanager::setTransmitter(func);
        p_oldtransmit = false;
    }    
}

void rdsmanager::setPi(const char* picode) {
    this->p_picode = this->parseHex(picode);
}

void rdsmanager::setPty(ptyCodesEu pty){
    this->p_pty = pty;
}

void rdsmanager::setPs(const char* stationname){
    if (stationname == NULL) {
        strncpy(p_ps,"        \0",9); //null->empty
        return;
    }
    uint8_t len = strnlen(stationname,8);
    strncpy(p_ps,stationname,len);
    if (len<8) {
	memset(p_ps+len,' ',8-len);
	p_ps[8] = '\0';
    }
}

void rdsmanager::setRt(const char* radiotext){
    if (radiotext == NULL) {
	if (p_rt != NULL) free(p_rt);
	return;
    }
    if (p_rt != NULL) free (p_rt);
    p_rt = strndup(radiotext,64); //rt is max 64 byte
}

void rdsmanager::setTp(bool traffic_program) {
    this->p_tp = traffic_program;
}

void rdsmanager::setMusic() {
    this->p_ms = false;
}

void rdsmanager::setSpeech() {
    this->p_ms = true;
}

void rdsmanager::enableTraffic() {
    this->p_ta = true;
}

void rdsmanager::disableTraffic() {
    this->p_ta = false;
}