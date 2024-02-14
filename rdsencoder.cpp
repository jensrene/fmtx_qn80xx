#include "rdsencoder.h"
#include <ctype.h>
#include <cstdio>
//#include <cstdint.h>

uint16_t rds_encoder::parse_hex(const char* str) {
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


rds_encoder::rds_encoder() {
//    printf("RDS encoder init\n");
}

void rds_encoder::rmlClear() {
    rds_encoder::rds_message_list* current = rds_encoder::rmlHead;
    rds_encoder::rds_message_list* next;
    while (current != NULL) {
	next = current->next;
	free(current);
	current = next;
    }
}

rds_encoder::~rds_encoder() {
//    printf("RDS encoder destroy\n");
        rmlClear();  
	if (rt != NULL) {			// NOT in rmlClear, because that is called on other places just to clear the rds message list (rml)
	    free(rt);
	}

}

void rds_encoder::set_pi(const char* picode) {
    this->picode = this->parse_hex(picode);
}

void rds_encoder::set_pty(pty_codes_eu pty){
    this->pty = pty;
}

void rds_encoder::set_ps(const char* stationname){
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

void rds_encoder::set_rt(const char* radiotext){
    if (radiotext == NULL) {
	if (rt != NULL) free(rt);
	return;
    }
    if (rt != NULL) free (rt);
    rt = strndup(radiotext,64); //rt is max 64 byte
    rt_new = true;
}

//int func for simplification -> check if 
unsigned char safeCharOrCR(const char* rt, size_t index, size_t length) {
    if (index > length || rt[index] == '\0') {
//	return 0x00;
        return 0x0D; // Return CR if index is out of bounds or character is null terminator.
    } else {
        return (unsigned char)rt[index];
    }
}


rds_encoder::rds_message_list* rds_encoder::get_rt_msg(){
    if (rt == NULL) return NULL; // we need an RT to send, otherwise return NULL.
    rds_message_list *current = NULL;
    rmlClear(); // danger: if we are not done with some RDS, and call that, its lost and probably broken
    uint8_t group_type = group_types::RADIO_A_2A; // RT grp 2A (version code is included in these constants)
    uint8_t rt_len = strnlen(rt,64);
    uint8_t iterationsNeeded = ((rt_len + (4-1))/4); // each 2A msg can contain 4 chars --> rtlen/4. Also: Always round up, even for shorter messages we need afull block.
    unsigned char first, second, third, fourth;
//    if (rt_new) rt_abflag ^= 1; // flip between 0 and 1
    rt_abflag ^= 1; // somehow, flipping each time gives better results with most FM recievers.
    for (int i = 0; i < iterationsNeeded; i++) {
	rds_message_list *new_node = (rds_message_list* )malloc(sizeof(rds_message_list));
	if (!new_node) {
	    printf("Memory allocation failed!\n");
	    return NULL; // return nothing, do nothing.
	}
	//generate RDS message
	new_node->rds_message16[0] = this->picode;   // PI Code (CRC is done by controller)
	new_node->rds_message16[1] = 	((group_type & 0b11111) << 11) | 	// Group Type (5 bits)
					((this->tp & 0b1) << 10) |		// TP
					((this->pty & 0b11111) << 5) |		// PTY
					((rt_abflag & 0b1) << 4) |		// rt A/B toggle flag (->new RT message)
					((i & 0b1111));			// Segment Address Code

	//letter handling RDS message:
	first = safeCharOrCR(rt, (i * 4) + 0, rt_len);
        second = safeCharOrCR(rt, (i * 4) + 1, rt_len);
        third = safeCharOrCR(rt, (i * 4) + 2, rt_len);
        fourth = safeCharOrCR(rt, (i * 4) + 3, rt_len);
	
        new_node->rds_message16[2] = (first << 8) | second;
        new_node->rds_message16[3] = (third << 8) | fourth;
	// buffer handling, append new element.
	new_node->next = NULL;
	if (rds_encoder::rmlHead == NULL) {
	    rmlHead = new_node;
	    current = rds_encoder::rmlHead;
	} else {
	    current->next = new_node;
	    current = new_node;
	}
    }    
    rt_new = false;
    return rmlHead; // return it
}

rds_encoder::rds_message_list* rds_encoder::get_ps_msg(){
    // Function to generate a linked list of RDS PS messages without CRC
//    rds_message_list *head = NULL;
    rds_message_list *current = NULL;
    rmlClear();

    // Set the group type for PS messages
    uint8_t group_type = group_types::PI_0B;
//    uint8_t version_code = 0; // field B0, Basic tuning and switching has no Version B. (0=A,1=B)
    uint8_t di = decoder_information::stereo || decoder_information::not_compressed || decoder_information::static_pty;

    for (int i = 0; i < 4; i++) {
        rds_message_list *new_node = (rds_message_list *)malloc(sizeof(rds_message_list));
	// allocate memory for new node.
        if (!new_node) {
            printf("Memory allocation error\n");
            exit(1);
        }

        new_node->rds_message16[0] = this->picode;
        new_node->rds_message16[1] = ((group_type & 0b11111) << 11) | // Group Type (5 bits)
//                   ((version_code & 0b1) << 11) |   // B0 (Version Code)
                   ((this->tp & 0b1) << 10) | // Traffic Program (TP)
                   ((this->pty & 0b11111) << 5) | // Program Type (PTY) (5 bits)
                   ((this->ta & 0b1) << 4) | // Traffic Announcement (TA)
                   ((this->ms & 0b1) << 3) | // Music/Speech (MS)
                   (((di >> (3 - i)) & 0b1) << 2) | // DI (1 bit, inverse order)
                   (i & 0b11);                               // Address (2 bits)
        new_node->rds_message16[2] = this->picode;
        new_node->rds_message16[3] = (this->ps[i * 2] << 8) | this->ps[i * 2 + 1];

        new_node->next = NULL;

        if (rds_encoder::rmlHead == NULL) {
            rmlHead = new_node;
            current = rds_encoder::rmlHead;
        } else {
            current->next = new_node;
            current = new_node;
        }
    }
    return rmlHead;
}


void rds_encoder::set_tp(bool traffic_program) {
    this->tp = traffic_program;
}

void rds_encoder::set_music() {
    this->ms = false;
}

void rds_encoder::set_speech() {
    this->ms = true;
}

void rds_encoder::enable_traffic() {
    this->ta = true;
}

void rds_encoder::disable_traffic() {
    this->ta = false;
}

/*
    rds_encoder();
    ~rdc_encoder();


    void set_pi(const char* picode);
    void set_pty(pty_codes_eu pty);
    void set_ps(const char* stationname);

    rds_message_node* get_ps_msg ();

private:

    uint16_t picode;  // PI code of the station (kinda UID of the station)
    pty_codes_eu pty; //also uint8_t with static_cast<uint8_t>(PtyCodesEU::News);
    char[8] ps;
    static int parse_hex(const char* str);

*/
