//#define _GNU_SOURCE

#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
//#include <stdio.h>

//#include <stdio.h>
//#include <stdlib.h>

//#include <unistd.h>
//#include <string.h>

#include "transmitter.h"
#include "conffile.h"
#include "rdsencoder.h"
#include "pwmcontroller.h"
#include "strarray.h"

#define RUNNING_DIR "/tmp"
#define LOCK_FILE "myprogram.lock"
#define LOG_FILE "/var/log/test.log"  //default
#define SOCKET_PATH "/tmp/myprogram.sock"
#define PWM_RPI_PIN 18
#define QN80XX_ADDR 0x21
#define MAXPOWER 103 // this is the maximum power from cfg file/power command. We could go to 10000 or higher, but that only gives us precision, not more power of course ;) 0.1% steps seem precise enought. One step represents 0.0033V on the pi PWM


bool is_daemon = false;
char* logfile = NULL;
double frequency = 87.5;
int power = 0;
bool autooff = true;
bool softclip = true;
uint8_t buffergain = 0;
uint8_t digitalgain = 0;
uint8_t inputimpedance = 0;
strarray* rdssid = NULL;
strarray* rdssiddyn = NULL;  // for additional stuff.
bool rdssiddynactive = false; // not defines if, used to switch buffers from nondyn. false=nondyn, true=dyn
char rdspi[5] = "ABCD"; // pi code in hex


/* reverse:  reverse string s in place */
 void reverse(char* s)
 {
     int i, j;
     char c;

     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
}  

/* itoa:  convert n to characters in s */
 char* itoa(int n, char* s)
 {
     int i, sign;

     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
    return s;
}  


/**
 * Concatenates two strings, dynamically allocating memory for the result.
 * 
 * @param str1 The first string.
 * @param str2 The second string.
 * @return A pointer to the newly allocated string that contains the contents
 *         of str1 followed by the contents of str2. The caller is responsible
 *         for freeing this memory. Returns NULL if memory allocation fails or
 *         either input string is NULL.
 */
char* strcat_copy(const char *str1, const char *str2) {
    if (!str1 || !str2) {
        // Handle null pointers to prevent undefined behavior.
        return NULL;
    }

    size_t str1_len = strlen(str1);
    size_t str2_len = strlen(str2);

    // Allocate memory for the concatenated string plus one for the null terminator.
    char *new_str = (char*)malloc(str1_len + str2_len + 1);
    if (!new_str) {
        // Memory allocation failed, return NULL to indicate failure.
        return NULL;
    }

    // Copy the first string into the new string.
    memcpy(new_str, str1, str1_len);
    // Append the second string to the new string.
    memcpy(new_str + str1_len, str2, str2_len + 1); // Includes null terminator.

    return new_str;
}

bool file_exists(const char *fname)
{
return (access(fname, F_OK) == 0) ;
}

void log_message(const char *message)
{
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[20];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    if (is_daemon) {
        FILE *file = fopen(logfile, "a");
        if (!file) return;
        fprintf(file, "[%s] %s\n", timestamp, message);
        fclose(file);
    } else {
        printf("[%s] %s\n", timestamp, message);
    }
}


// Function to calculate the length of the next word until the next space or '-' or end of string
int nextWordLength(const char* str, int startIndex) {
    int length = 0;
    while (str[startIndex] != ' ' && str[startIndex] != '-' && str[startIndex] != '\0') {
        ++length;
        ++startIndex;
    }
    return length;
}

/*
void titleToPS(const char* musicArtistTitle, strarray& output) {
    char buffer[9]; // RDS PS buffer: 8CHR+\0
    int bufferIndex = 0; // where we are in the bufer
    int i = 0;
//    for (int i=0; musicArtistTitle[i] != '\0'; ++i){ // go through each letter
    while (musicArtistTitle[i] != '\0') {
	if (musicArtistTitle[i] == '-' || bufferIndex == 8 || musicArtistTitle[i] == '\0' ) {	// we need split
	    if ((musicArtistTitle[i] == '-') || ( musicArtistTitle[i] == '\0' && bufferIndex > 0) ) { //split char
		int wordLength = nextWordLength(musicArtistTitle, i+1);
		if (((wordLength < 8) && (bufferIndex + wordLength <= 8)) || (musicArtistTitle[i] == '\0')) { //word is shorter->fill
		    memset(buffer+bufferIndex, ' ', 8);
		    bufferIndex = 8;
		}
	    }
	    buffer[bufferIndex] = '\0';
	    output.append(buffer);
	    bufferIndex = 0;
	    if (musicArtistTitle[i] == '-') ++i;
	    if (musicArtistTitle[i] == '\0') break;
	}

	if (musicArtistTitle[i] != '\0' && bufferIndex < 8) {
	    buffer[bufferIndex++] = musicArtistTitle[i++];
	}
    }
/*    if (bufferIndex > 0) { //append remain
	buffer[bufferIndex] = '\0';
	output.append(buffer);
    }***
}*/

void titleToPS(const char* musicTitle, strarray& output) {
    int i = 0, wordLen = 0;
    char buffer[9]; // Temporary buffer for chunks of up to 8 characters
    int bufferIndex = 0; // Current index in the buffer

    while (musicTitle[i] != '\0') {
        if (musicTitle[i] == ' ' || musicTitle[i] == '-') {
            // If at a space or hyphen, check the length of the next word
            int nextLen = nextWordLength(&musicTitle[i + 1],0);

            // Decide whether to split here or start a new chunk based on the next word's length
            if (bufferIndex + nextLen >= 8 || musicTitle[i] == '-') {
                // Flush the current buffer if it's not empty
                if (bufferIndex > 0) {
                    buffer[bufferIndex] = '\0'; // Null-terminate
                    output.append(buffer);
                    bufferIndex = 0; // Reset for the next word
                }
                if (musicTitle[i] == '-') {
                    i++; // Skip the hyphen for the next iteration
                    continue; // Don't add '-' to the buffer
                }
            } else {
                // If the next word can fit in the current chunk, add the space
                buffer[bufferIndex++] = musicTitle[i];
            }
        } else {
            // Add the current character to the buffer
            if (bufferIndex < 8) {
                buffer[bufferIndex++] = musicTitle[i];
            } else {
                // Buffer full, flush it before adding more characters
                buffer[bufferIndex] = '\0'; // Null-terminate
                output.append(buffer);
                bufferIndex = 0; // Reset buffer index
                buffer[bufferIndex++] = musicTitle[i]; // Start next chunk with current character
            }
        }
        i++; // Move to the next character

        // Handle the last word
        if (musicTitle[i] == '\0' && bufferIndex > 0) {
            buffer[bufferIndex] = '\0'; // Null-terminate
            output.append(buffer);
        }
    }
}

void signal_handler(int sig)
{
    switch(sig) {
    case SIGTERM:
        log_message("terminate signal catched");
        unlink(SOCKET_PATH);
        exit(0);
        break;
    }
}

void daemonize()
{
    int i, lfp;
    char str[10];

    if(getppid() == 1) return;

    i = fork();
    if (i<0) exit(1);
    if (i>0) exit(0);

    setsid();

    for (i=getdtablesize();i>=0;--i) close(i);

    i = open("/dev/null",O_RDWR);
    dup(i);
    dup(i);

    umask(027);

    chdir(RUNNING_DIR);

    lfp = open(LOCK_FILE, O_RDWR|O_CREAT, 0640);
    if (lfp<0) exit(1);
    if (lockf(lfp, F_TLOCK, 0)<0) exit(0);

    sprintf(str, "%d\n", getpid());
    write(lfp, str, strlen(str));

    signal(SIGCHLD, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTERM, signal_handler);
}

void run_daemon()
{
    char buffer[512];
    bool sendRDS = false;
    bool transmitting = false;

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("socket");
        exit(1);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);
    unlink(SOCKET_PATH);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        close(sock);
        exit(1);
    }

    if (listen(sock, 5) == -1)
    {
        perror("listen");
        close(sock);
        exit(1);
    }

    int flags = fcntl(sock,F_GETFL,0);
    if (flags == -1)  {
	log_message("ERROR: failed to get flags for socket\n");
	exit(2);
	}
    flags |= O_NONBLOCK;
    if (fcntl(sock,F_SETFL,flags) == -1) {
        log_message("ERROR: failed to set flags for socket\n");
        exit(3);
    }

/*	int flags = fcntl(client_sock,F_GETFL,0);
	if (flags == -1)  {
	    log_message("ERROR: failed to get flags for socket\n");
	    exit(2);
	}
	flags |= O_NONBLOCK;
	if (fcntl(client_sock,F_SETFL,flags) == -1) {
	    log_message("ERROR: failed to set flags for socket\n");
	    exit(3);
	}
*/


    Transmitter transmitter(QN80XX_ADDR, 5);
    clock_t start_time = clock();
    PWMController *pwmController = NULL;
    if (power >= 0) {
	pwmController = new PWMController(PWM_RPI_PIN);     // Create an instance of PWMController
	float frequency = pwmController->getFrequency();
	pwmController->setSupplyVoltage(MAXPOWER);
        char logbuf_tmp[64];
	snprintf(logbuf_tmp, sizeof(logbuf_tmp)-1, "PWM initialized with frequency of %.3f Hz\n",frequency);
	log_message(logbuf_tmp);
    }
    
    // Set the voltage to 2.5V
//    float voltage = 1.5;
//    pwmController.setVoltage(voltage);


    double rds_time01;
    char* next_loop = NULL; // what to do in the next loop, null otherwise
    strarray::type* currentPS = rdssid->get();
    char currentPSStr[9];
    short PScounter = 2; // start at the number or times we submit it->initialize automatically.
    while(1)
    {
        rds_time01 = (double)(clock()-start_time) / CLOCKS_PER_SEC;
	int client_sock;
	if (next_loop == NULL) client_sock = accept(sock, NULL, NULL);
	else client_sock = 0;

        if (client_sock == -1)
        {
	    if (errno != EWOULDBLOCK){
	    log_message("ERROR: opening client socket failed!\n");
            perror("accept");
            close(sock);
            exit(1);
	    } //else we have no pending connections and dont get any new data.
	    
        } else {
	    int n;	
	    if (next_loop != NULL) {
		log_message("internal command executed.");
		strncpy(buffer, next_loop,511);
		buffer[511] = '\n';
		n = strlen(buffer);
		free(next_loop);
		next_loop = NULL;
	    }
	    else { n = read(client_sock, buffer, sizeof(buffer)-1); }
    	    if (n > 0) {
        	buffer[n] = '\0';
		// begin handling of commands recieved:
        	if (strcmp(buffer, "stop") == 0)
        	{
		    log_message("Received stop command. Resetting transmitter and exiting...");
		    transmitter.reset();
            	    break;
        	} else if (strcmp(buffer, "activate") == 0) {
		    char logbuf[128];		
	    	    int result = transmitter.startup();
		    if (result < 0) {
			log_message(transmitter.get_errmsg());
		    } else {
			
    			if (transmitter.set_frequency(frequency) < 0) {
			    snprintf(logbuf,sizeof(logbuf), "ERROR: failed setting frequency %.2fMHz. Please check for interference on the I2C connection!",frequency);
			} else {
			    snprintf(logbuf,sizeof(logbuf), "Starting transmitter at %.2f MHz.",frequency);
			    log_message(logbuf);
    			    transmitter.set_softclipping(softclip);
    			    transmitter.set_buffergain(buffergain);
    			    transmitter.set_digitalgain(digitalgain);
    			    transmitter.set_inputimpedance(inputimpedance);
			    transmitting = true;
			}
		    }
		} else if (strcmp(buffer, "reset") == 0) {
		    log_message("Resetting transmitter.");
		    transmitting = false;
		    transmitter.reset();
		} else if (strcmp(buffer, "status") == 0) {
//		    log_message("sending status...");
		    int transmitter_status = transmitter.status();
		    if (transmitter_status < 0) {
			log_message("ERROR: failed to read status. Check for interference!!!");
		    } else { // no error 
			
			char buffer[2] = {(char)(transmitter_status & 0xFF),(char)( ((transmitting & 1)<<0) | ((sendRDS & 1)<<1))}; // first buffer entry is transmitterstatus, second is 1bit:transmitting? 2bit:rdson?
			int n = write(client_sock, buffer,2);
			if (n < 0) {
			    log_message("ERROR: sending status failed!");
			} else {
			    log_message("send status to client");
			}
			
		    }
		} else if (strcmp(buffer,"rdson") == 0) {
		    log_message("RDS enabled");
		    sendRDS = true;
		} else if (strcmp(buffer,"rdsoff") == 0) {
		    log_message("RDS disabled");
		    sendRDS = false;
		} else if (strncmp(buffer,"power=",6) == 0) { // first 5 letters are "power="
		    char logbuf[64];
		    if (pwmController != NULL) {
    			char* splitpos = strchr(buffer,'=');
			if (splitpos != NULL) {
			    uint16_t value = atoi(splitpos+1);
			    if (value > MAXPOWER) value = MAXPOWER; // limit to max.
			    pwmController->setVoltage(atoi(splitpos+1));
			    snprintf(logbuf,sizeof(logbuf)-1,"Setting power with PWM to %u/%u",value,MAXPOWER);
			    log_message(logbuf);
			    if ( !transmitting ) log_message ("WARNING: power set, but transmitter not active!");
			}
		    } else {
		    log_message("ERROR: Try to set power with disabled pwm. Please add power=xxx (value 1-1000) into config file.");
		    }
		} else if (strncmp(buffer,"title=",6) == 0) {  // first 6 letters are "title="
		    char logbuf[512];
		    char* splitpos = strchr(buffer,'='); // pos of = in "title="
		    if (splitpos != NULL) { // must be true wit the current version, but better safe
			char* title = splitpos+1;
			if (rdssiddyn == NULL) { rdssiddyn = new strarray; }
			rdssiddyn->clear();
			titleToPS(title,*rdssiddyn);
			snprintf(logbuf,sizeof(logbuf), "Title recieved: \"%s\"",title);
			rdssiddynactive = true;
			currentPS = rdssiddyn->get();
			log_message(logbuf);
		    }
		}
		else {
            	    char logbuf[529];
		    snprintf(logbuf, sizeof(logbuf), "Command recieved %s",buffer);
		    log_message(logbuf);
        	}
	    }
    	    close(client_sock);
	}
	
	if (rds_time01 >= 1.0){ // everything in here will be called ~each second
	    start_time = clock(); // doing it early makes sure we have only 1sec not 1sec+processingtime.
	    transmitter.set_frequency(frequency); // dirty - we just want to be sure the freq fits even if interference to i2c happens.
	    if (sendRDS && transmitting) {  // only try rds when enabled and transmitting already
		char logbuf[64];
		PScounter++;
		if (PScounter > 2) { // change PS to next entry (middle shorters)
		    PScounter = 0;
		    
		    uint8_t len = strnlen(currentPS->content,8);
		    uint8_t pad = (8-len)/2;
		    memset(currentPSStr, ' ',8);
		    strncpy(currentPSStr+pad,currentPS->content,len);
		    currentPSStr[8] = '\0';
		    currentPS = currentPS->next;
		    if (currentPS == NULL) {  // next is empty.
			if ( rdssiddynactive ) {  // dyn array is set->we want that,too!
			    currentPS = rdssiddyn->get(); // switch to rdssiddyn
			    rdssiddynactive = false;
			    
			}
			else {
			    currentPS = rdssid->get(); //loop pointered array: go beginning.
			    rdssiddynactive = (rdssiddyn != NULL); // only do if we actually have that class.
			}
		    }
		}
		snprintf(logbuf,sizeof(logbuf),"sending RDS for PS... PS: '%s', PI: 0x%s",currentPSStr,rdspi);
		log_message(logbuf);
		rds_encoder* myRDS = new rds_encoder;
		rds_encoder::rds_message_list* RDSmsg;
		myRDS->set_pi(rdspi);
		myRDS->set_ps(currentPSStr);
		RDSmsg = myRDS->get_ps_msg();
		rds_encoder::rds_message_list* tmpList;
    		tmpList = RDSmsg;
		int8_t transmit_result = 0;
    		while (tmpList) {
        	    transmit_result = transmitter.transmit_rds(tmpList->rds_message16);
		    if (transmit_result < 0) {	// it is only -1 if it fails at check
			log_message("sending RDS failed, resetting transmitter.");
			//transmitter.reset();
			transmitting = false;
		    }
//		    usleep(2000); // wait 2ms
        	    tmpList = tmpList->next;
    		}
		if (!transmitting) next_loop = strdup("activate");
		delete myRDS;
	    }
	}
    sleep(0.9);
    }
    if (pwmController != NULL) delete(pwmController);
    close(sock);
    unlink(SOCKET_PATH);
}


void introScreen(const char *prgName) {
    printf("%s",prgName);
    printf("\n");
    printf(" FM radio transmitter control for QN80xx fm transmitter ICs.\n");
    printf(" -----------------------------------------------------------\n");
    printf("\n");
    printf(" This program allows to control the QN8036/QN8066 fm transmitter.\n");
    printf(" It uses the I2C bus to communicate and the PWM controller to generate \n");
    printf(" a PWM to control the RF output power of the transmitter.\n");
    printf("\n");
}


void print_help(const char* prog_name) {
    printf("%s [options]\n", prog_name);
    printf("\nUsage:\n");
    printf("  %s -r <raw_command> | -c <config_file> | -e | -d | -s | -a | -p <power_value> | -q\n", prog_name);
    printf("\nOptions:\n");
    printf("  -r, --raw-command <raw_command>    Send a raw command to the program. Valid raw commands include:\n");
    printf("                                     stop, activate, reset, status, rdson, rdsoff, power=<value>.\n");
    printf("                                     'power' value must be between 0 and 103.\n");
    printf("  -c, --config <config_file>         Load program configurations from a specified file.\n");
    printf("  -e, --execute                      Start listener, output to stdout but don't daemonize.\n");
    printf("  -d, --daemon                       Start the program as a daemon listener.\n");
    printf("  -s, --status                       Gets the status of the transmitter from a running daemon.\n");
    printf("  -a, --activate                     Activates the transmission of fm with configured settings.\n");
    printf("  -t, --title                        Submit the currently played item/title.\n");
    printf("  -p, --power <power_value>          Sets the power of the transmitter, basially voltage to the '.\n");
    printf("                                     power transistor. 'power' value must be between 0 and 103.\n");
    printf("  -q, --stop                         Stops the transmission and the daemon exits.\n");
/*    printf("\nRaw Command Details:\n");
    printf("  - stop:        Stop the program.\n");
    printf("  - activate:    Activate the program.\n");
    printf("  - reset:       Reset the program.\n");
    printf("  - status:      Retrieve the status of the program.\n");
    printf("  - rdson:       Turn on RDS.\n");
    printf("  - rdsoff:      Turn off RDS.\n");
    printf("  - power=value: Set the power level of the program. The value must be between 0 and 103.\n");*/
    printf("\nPlease note:\n");
    printf("1. The program will execute the options in the order they are given. If multiple raw commands are given, only the last one will take effect.\n");
    printf("2. When using the --config option, the program will exit if the specified configuration file does not exist.\n");
    printf("3. When using the --daemon option, the program will run in the background as a daemon.\n");
    printf("4. The --execute option will start the listener and output to stdout, but will not daemonize the program.\n");
}

int main(int argc, char *argv[]) 
{
    int opt;
    char *command = NULL;
    char *configFile = NULL;
    bool main_run = false;

    introScreen(argv[0]);

    struct option options[] = {
        {"raw-command", required_argument, NULL, 'r'},  // send raw command
	{"config", required_argument, NULL, 'c'},   // load from config file 
        {"execute", no_argument, NULL, 'e'},	    // start listener, but dont daemonize, out to stdout
	{"daemon",no_argument, NULL, 'd'},	    // start deamon listener.
	{"status",no_argument, NULL, 's'},
	{"activate",no_argument, NULL, 'a'},
	{"power",no_argument, NULL, 'p'},
	{"stop",no_argument, NULL, 'q'},
	{"help",no_argument, NULL, 'h'},
	{"title",required_argument, NULL, 't'},
        {0, 0, 0, 0}
    };
    
    while ((opt = getopt_long(argc, argv, "r:c:edst:ap:qh", options, NULL)) != -1) {
        switch (opt) {
            case 'r':
                command = strdup(optarg);
                break;
	    case 'c':
		configFile = optarg;
		if (file_exists(configFile)) {
    		    printf("  * Using config file: %s \n",configFile);
		    } else {
		    fprintf(stderr, "Configuration file not found: %s\n",configFile);
		    exit(EXIT_FAILURE);
		}
		break;
            case 'd':
		is_daemon = true;
		[[fallthrough]];
	    case 'e':            
		main_run = true;
                break;
	    case 's':
		command = strdup("status");
		break;
	    case 'a':
		command = strdup("activate");
		break;
	    case 'p':
/*		char buffer[30];
		snprintf(buffer,sizeof(buffer)-1,"power=%s",optarg);
		command = strdup(buffer);*/
		command = strcat_copy("power=",optarg);
		break;
	    case 'q':
		command = strdup("stop");
		break;
	    case 'h':
		print_help(argv[0]);
		exit(EXIT_SUCCESS);
	    case 't':
		command = strcat_copy("title=",optarg);
		//printf(" Title recieved: %s \n",optarg);
		break;
            default:
                print_help(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    if (configFile) {
	conffile myCfg(configFile);
        int result = myCfg.load();
        if (result != 0) {
            printf("Error: %s \n",strerror(errno));
            exit (-1);
        }
	logfile = strdup(myCfg.getString("logfile",LOG_FILE));
        frequency = myCfg.getFloat("frequency",87.5);
        power = myCfg.getInt("power",-1);
        autooff = myCfg.getBool("autooff",true);
        softclip = myCfg.getBool("softclipping",true);
        buffergain = myCfg.getInt("buffergain",0);
        digitalgain = myCfg.getInt("digitalgain",0);
        inputimpedance = myCfg.getInt("inputimpedance",0);
	rdssid = myCfg.getStrArray("RDS_SID");
        if (rdssid == NULL) {
	    myCfg.setString("RDS_SID"," RADIO ");
	    rdssid = myCfg.getStrArray("RDS_SID");
	} else {
	rdssid = rdssid->dup(); // get a new instance, independant from cfg.
	}

	strncpy(rdspi,myCfg.getString("picode","ABCD"),4);
	char bufstr[4];
	rdspi[4] = '\0'; // should be, but depends on init, so better save then sorry.
        printf("\n  Loaded data:\n\n");
	printf("   * Logfile:         %s\n\n",logfile);
        printf("   * Frequency:       %.2f MHz\n",frequency);
	printf("   * RDS PS:          %s\n",rdssid->toString(','));
        printf("   * Power:           %s\n",(power<0)?"PWM disabled":itoa(power,bufstr));
        printf("   * Auto-Off:        %s\n", autooff ? "enabled" : "disabled");
        printf("   * Soft-Clipping:   %s\n", softclip ? "On" : "Off");
        printf("   * Buffer gain:     %i dB\n", buffergain);
        printf("   * Input Impedance: %s\n",myCfg.inputImpedanceStr[inputimpedance]);
        printf("\n");
	}
    if (command)    { // we have a command -> send raw
	printf(" Sending command %s ...",command);
        int sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock == -1)
        {
            perror("socket");
            exit(1);
        }
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);

        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        {	    
	    printf(" ERROR\n  * Failed to connect to running process. Is the daemon running?\n");
            close(sock);
	    if (command != NULL) free(command);
            exit(1);
        }
        write(sock, command, strlen(command));
	if (strcmp(command,"status") == 0) {
	    char tmp_status[2];
	    int n = read(sock, tmp_status, 2);
    		if (n > 0) {		// read successful
		    uint8_t fsm = ((uint8_t)tmp_status[0] >> 4);
		    uint8_t peak = ((uint8_t)tmp_status[0] & 0b1111);
		    bool transmitting = (tmp_status[1] >> 0) & 1;
		    bool sendRDS = (tmp_status[1] >> 1) & 1;
		    printf("\n Status of QN80XX:\n");
		    printf("  * transmitter is    : %s\n", transmitting ? "enabled": "disabled");
		    if (sendRDS) printf("  * RDS is enabled\n");
    		    printf("  * fsm (expected 10) : %i\n",fsm);
		    printf("  * audio peak (<=14) : %i\n",peak);
    		    printf("\n");
//        while (true) {
//            draw_bar2_avr(status & 0b1111);
//            status = transmitter.status();
//          printf("%i,",status & 0b1111);
//            usleep(10000);
//        }
		} else {
		printf( " ERROR: No status recieved\n");
		}
	}
        close(sock);
	printf("done!\n\n");
    } else { // we either send raw to an existing process, or do other stuff - never both.
	if (is_daemon) {
	    printf("  * Starting in daemon mode.\n");
	    daemonize();	// prepare daemonization
	}
	if (main_run) {
	    if (rdssid == NULL) {
	
		rdssid = new strarray;
		rdssid->append(" RADIO ");
	    }
	    if (logfile == NULL) logfile = strdup (LOG_FILE);
	    run_daemon();	// run main (daemon) loop.
	}
    }
    if (command != NULL) free(command);
    if (logfile != NULL) free(logfile);
    if (rdssid != NULL) delete rdssid;
    if (rdssiddyn != NULL) delete rdssiddyn;
    log_message("program ended.");
    return 0;
}
