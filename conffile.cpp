#include "conffile.h"

conffile::conffile(const char* filename) {  // constructor
//    this->filename = (char*)malloc(strlen(filename)+1);
//    strcpy(this->filename,filename);
    this->filename = strdup(filename);
    configData = NULL;
}

conffile::~conffile() { //destructor
    // clean memory:

    ConfigPair* current = configData;
    ConfigPair* next;

    while (current != NULL) {
        next = current->next;
        free(current->key);
        free(current->value);
        free(current);
        current = next;
    }
    free(this->filename);
}

int conffile::load() {
    if (this->filename == NULL) return -2;
//    printf("opening %s",this->filename);
    FILE* file = fopen(this->filename, "r");
//    printf("opened %s",this->filename);
    if (!file) {
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
    if (line[0] == '#' || line[0] == '\n') { // ignore comment lines (#)
        continue;
    }
    ConfigPair* pair = (ConfigPair*)malloc(sizeof(ConfigPair));
    pair->key = (char*)malloc(128);
    pair->value = (char*)malloc(128);

    char* equals = strchr(line, '=');
    if (equals) {
        *equals = '\0';
        strncpy(pair->key, line, 127);
        pair->key[127] = '\0';

        char* value = equals + 1;
        while (isspace((unsigned char)*value)) value++; // Skip leading whitespace

        if (*value == '"') {
            // Value starts with a quote, so find the matching quote
            char* endquote = strchr(value + 1, '"');
            if (endquote) {
                *endquote = '\0';
                strncpy(pair->value, value + 1, 127);
            } else {
                // No matching quote, so just copy the rest of the line
                strncpy(pair->value, value + 1, 127);
            }
        } else {
            // No starting quote, so just copy the rest of the line
            strncpy(pair->value, value, 127);
        }
        pair->value[127] = '\0';
    } else {
        // No equals sign, so just copy the whole line into the key and leave the value empty
        strncpy(pair->key, line, 127);
        pair->key[127] = '\0';
        pair->value[0] = '\0';
    }

    pair->next = configData;
    configData = pair;
}
    fclose(file);
    return 0;
}

int conffile::save() {
    FILE* file = fopen(filename, "w");
    if (!file) {
        return -1;
    }

    ConfigPair* current = configData;
    while (current) {
        fprintf(file, "%s = %s\n", current->key, current->value);
        current = current->next;
    }

    fclose(file);
    return 0;
}

const char* conffile::getString(const char* key, const char* defaultValue) {
    ConfigPair* current = this->configData;
    while (current) {
        if (strcmp(current->key, key) == 0) {
            return current->value;
        }
        current = current->next;
    }
    return defaultValue;
}

int conffile::getInt(const char* key, int defaultValue) {
    const char* value = this->getString(key, NULL);
    return value ? atoi(value) : defaultValue;
}

double conffile::getFloat(const char* key, double defaultValue) {
    const char* value = this->getString(key, NULL);
    return value ? atof(value) : defaultValue;
}

bool conffile::getBool(const char* key, bool defaultValue) {
    const char* value = this->getString(key, NULL);
    if (value) {
        return strcmp(value, "true") == 0 || strcmp(value, "1") == 0 || strcmp(value, "yes") == 0;
    }
    return defaultValue;
}

void conffile::removeKey(const char* key) {
    ConfigPair* current = this->configData;
    ConfigPair* previous = NULL;

    while (current) {
        if (strcmp(current->key, key) == 0) {
            if (previous) {
                previous->next = current->next;
            } else {
                configData = current->next;
            }
            free(current->key);
            free(current->value);
            free(current);
            return;
        }
        previous = current;
        current = current->next;
    }
}

void conffile::setString(const char* key, const char* value) {
    ConfigPair* current = this->configData;

    while (current) {
        if (strcmp(current->key, key) == 0) {
            free(current->value);
            current->value = strdup(value);
            return;
        }
        current = current->next;
    }

    // If the key was not found, create a new key-value pair and add it to the list
    ConfigPair* newPair = (ConfigPair*)malloc(sizeof(ConfigPair));
    newPair->key = strdup(key);
    newPair->value = strdup(value);
    newPair->next = this->configData;
    this->configData = newPair;
}

void conffile::setInt(const char* key, int value) {
    char valueStr[32];
    snprintf(valueStr, sizeof(valueStr), "%d", value);
    this->setString(key, valueStr);
}

void conffile::setFloat(const char* key, double value) {
    char valueStr[32];
    snprintf(valueStr, sizeof(valueStr), "%.3f", value);
    this->setString(key, valueStr);
}

void conffile::setBool(const char* key, bool value) {
    this->setString(key, value ? "true" : "false");
}
