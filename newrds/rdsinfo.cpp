#include "rdsinfo.h"
#include "rdsmanager.h"

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



/* PUBLIC */

rdsinfo::rdsinfo(int bufferSize) 
{
    myRDSManager = new RDSManager(bufferSize);
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
}

void rdsinfo::set_pty(pty_codes_eu pty){
    this->pty = pty;
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
    
}

void rdsinfo::set_rt(const char* radiotext){
    if (radiotext == NULL) {
	if (rt != NULL) free(rt);
	return;
    }
    if (rt != NULL) free (rt);
    rt = strndup(radiotext,64); //rt is max 64 byte
    rt_new = true;
}