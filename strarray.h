#ifndef STRARRAY_H
#define STRARRAY_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>

class strarray {
public:
    typedef struct type {
	char* content;
	struct type* next;
    } type;

    strarray();
    ~strarray();
    
    void  append(const char* value);
    char* toString(const char delimiter);
    void  clear(); // empty this, including mem alloc
    void  clonefrom(const strarray* source);
    strarray* dup() const;

    type* get();

//    void save(bool var1, uint8_t var2, double var3);

private:
    type**  datachain;  // OLD address to the address of datachain. change at this address the address to the new datachain
    type*   head; // start (address) of the concat list
    char*  returnVarChar; // address to clean

};

#endif // STRARRAY_H

