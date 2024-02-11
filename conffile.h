#ifndef CONFFILE_H
#define CONFFILE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "strarray.h"

class conffile {
public:

    const char inputImpedanceStr[4][9] = {"10 kOhm", "20 kOhm", "40 kOhm", "80 kOhm"}; // translation table for output
    conffile(const char* filename);
    ~conffile();
//    void save(bool var1, uint8_t var2, double var3);
    int load(); // returns 0 if successful.
    int save(); // returns 0 if successful.
    strarray* getStrArray(const char* key);
    const char* getString(const char* key, const char* defaultValue);
    int getInt(const char* key, int defaultValue);
    double getFloat(const char* key, double defaultValue);
    bool getBool(const char* key, bool defaultValue);
    void removeKey(const char* key);
    void setString(const char* key, const char* value);
    void setInt(const char* key, int value);
    void setFloat(const char* key, double value);
    void setBool(const char* key, bool value);
private:
    typedef struct ConfigPair {
	char* key;
	strarray* value;
	struct ConfigPair* next;
    } ConfigPair;

    char* filename;
    ConfigPair* configData;
};

#endif // CONFFILE_H

