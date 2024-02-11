#include "strarray.h"



strarray::strarray() {
    strarray::returnVarChar = NULL;
    strarray::head = NULL;
}

strarray::type* strarray::get() {
    return strarray::head;
}


/* NOTE: spend a day finding this: strlen is hyperoptimized ( https://stackoverflow.com/questions/31062721/how-dangerous-is-this-faster-strlen ), so it WILL generate an error on valgrind.
         two options: a) ignore the error, have fast strlen.   b) use "myStrLen" function instead which is slow but does not go out of bounds.
*/
int myStrLen(const char* str){
//    char* start = strdup(str);
    int len = 0;
    while (*str != '\0') {
	len++;
	str++;
    }
    return len;
}


void strarray::append(const char* value) {
    strarray::type* newNode = (strarray::type*)malloc(sizeof(strarray::type));
    newNode->content=strdup(value); // duplicate string into new node
/*    newNode->content=(char*)calloc(myStrLen(value)+1,1);
    memcpy(newNode->content,value,myStrLen(value));
    newNode->content[myStrLen(value)] = '\0';*/
//    printf("Added : '%s',   new len: %i\n ",value, strlen(newNode->content));

    newNode->next = NULL;
    if (head == NULL) {
	head = newNode;
    } else {
	strarray::type* current = head;
	while (current->next != NULL) {
	    current = current->next;	// move to the head
	}
    current->next = newNode;
    }
}

/* DANGER: this function tries to be smart but is not: It gives out a new string with newly reserved memory each time its called. To do that, if the memory has been previously reserved for its own return, it frees that. 
   Also it is freed on demand (if used) inside the class destructor.
   While this is comfy and smart for many uses, it posses a risk: If the result data is used somewhere later, without making a copy it (meaning, just passing pointers around to the same memory), that memory might get freed on 
   a second/later call of this function or on class destruction. While the later _might_ be intuitive, the first one sure is not. 
   If you want to be safe, just copy the content of the result immediatly, for example with strdup, immediatly after using this function. */
char* strarray::toString(const char delimiter) {
    if (returnVarChar != NULL) {		//danger, this can be a problem if we want to reuse old return variables. The only memory and fool proof solution would be build another pointer list of pointers and at the end free them all.
	free(returnVarChar);
    }
    
    strarray::type* node = strarray::head;

    // determine how long the string will be for memory reservation.
    int finalSize = 0;
    while (node != NULL) {	//iterate through all nodes
	finalSize++;
//	finalSize = finalSize + 4;
	finalSize = finalSize + (myStrLen(node->content));
	node = node->next;
    }
    //printf("size : %i\n",finalSize);
    returnVarChar = (char*)malloc(finalSize); // REMEMBER CLEANUP!
    
    //now copy
    node = strarray::head;
    int pos = 0;
    int contentLen = 0;
    while (node != NULL) {	//iterate through all nodes
    	contentLen = myStrLen(node->content);
//	contentLen = strlen("asda");
	memcpy(returnVarChar+pos,node->content,contentLen);
	pos = pos + contentLen;
	returnVarChar[pos] = delimiter;
	pos++;
	node = node->next;
    }
    returnVarChar[pos-1] = '\0';
//    returnVarChar = returnStr;
    return returnVarChar;
}

void strarray::clonefrom(const strarray* source) {
	strarray::clear();
        if (!source || !source->head) return; // Check for null source or empty list

        type* srcNode = source->head;
        type** destNode = &head;

        while (srcNode != NULL) {
            *destNode = (type*)malloc(sizeof(type)); // Allocate new node
            if (*destNode == NULL) {
                // Handle malloc failure, for simplicity we just exit here,
                // but in a real application, you would want to handle this more gracefully.
		printf("MALLOC ERROR!");
                exit(1);
            }

            (*destNode)->content = (char*)malloc(myStrLen(srcNode->content) + 1); // Allocate memory for content
            if ((*destNode)->content == NULL) {
                // Handle malloc failure
		printf("MALLOC ERROR2!");
                exit(1);
            }
            strcpy((*destNode)->content, srcNode->content); // Copy content
            (*destNode)->next = NULL; // Set next to null

            srcNode = srcNode->next; // Move to next source node
            destNode = &((*destNode)->next); // Move to next destination node pointer
        }
}

strarray* strarray::dup() const {
        strarray* duplicate = new strarray(); // Allocate new strarray using new
        duplicate->clonefrom(this); // Use clonefrom to clone the current strarray into the new one
        return duplicate;
}

void strarray::clear() {
    strarray::type* node = strarray::head;
    while (node != NULL) {
	strarray::type* nextNode = node->next;
	free(node->content);
	free(node);
	node = nextNode;
    }
    if (returnVarChar != NULL) {
	free(returnVarChar);
    }
    strarray::head = NULL;
}

strarray::~strarray() {
    clear();
}

// Implement other member functions if needed
//void strarray::save(bool var1, uint8_t var2, double var3) {
//    // Implement the save function here
//}
