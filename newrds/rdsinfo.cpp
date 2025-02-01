#include "rdsinfo.h"


#include <string.h>

RDSManager *myRDSManager = NULL;

/* PRIVATE */

//int func for simplification -> check if 
unsigned char rdsinfo::safeCharOrCR(const char* rt, size_t index, size_t length) {
    if (index > length || rt[index] == '\0') {
//	return 0x00;
        return 0x0D; // Return CR if index is out of bounds or character is null terminator.
    } else {
        return (unsigned char)rt[index];
    }
}


void rdsinfo::setTransmitter(RDSManager::TransmitFunc func)
{
    metaTransitFunc = func;
    if (myRDSManager != NULL) {
        myRDSManager->setTransmitter(metaTransitFunc);

    }
}



void rdsinfo::metaTransmit(const RDSMessage* msg, const int bufferIndex) { 
 if (bufferIndex == 0) { //we are at the buffer start but did not send it
    if (rdsinfo::rds_changed) { // rds changed-->needs update...
        // TODO: regenerate RDS data. 
        // TODO FIRST: write a generator function in this class that generates all "wanted" classes. 
        //              (so was we assume a fixed set of classes, but it could later be configurable)
        rdsinfo::rds_changed == false;        
    }
 }



 // called to do transmit
 if (metaTransitFunc != NULL) 
 {
    metaTransitFunc(msg,bufferIndex);
 }
}


/* PUBLIC */

rdsinfo::rdsinfo(int bufferSize) 
{
    myRDSManager = new RDSManager(bufferSize);
    //myRDSManager->setTransmitter(rdsinfo::consoleTransmit);
    myRDSManager->setTransmitter([this](const RDSMessage* msg, const int bufferIndex) { this->metaTransmit(msg,bufferIndex); } );
}

rdsinfo::~rdsinfo()
{
    if (myRDSManager != NULL) delete(myRDSManager);
}


void rdsinfo::set_pi(const char* picode) {
    if (myRDSManager != NULL) 
    {
        this->picode = myRDSManager->parseHex(picode);
    }
    this->rds_changed = true;
}

void rdsinfo::set_pty(pty_codes_eu pty){
    this->pty = pty;
    this->rds_changed = true;
}

void rdsinfo::set_ps(const char* stationname){
    if (stationname == NULL) {
        strncpy(ps,"        \0",9); //null->empty
        return;
    }
    uint8_t len = strnlen(stationname,8);
    strncpy(ps,stationname,len);
    if (len<8) {
	memset(ps+len,' ',8-len);
	ps[8] = '\0';
    }
    this->rds_changed = true;
}

void rdsinfo::set_rt(const char* radiotext){
    if (radiotext == NULL) {
	if (rt != NULL) free(rt);
	return;
    }
    if (rt != NULL) free (rt);
    rt = strndup(radiotext,64); //rt is max 64 byte
    this->rds_changed = true;
}