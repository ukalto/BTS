/* 
@file myexpand.c
@author ukalto
@details This file is used to compile the project
@date 12.10.2022 
valgrind --tool=memcheck --leak-check=yes ./myexpand < t1.txt = to check if memory leaks exist
*/

#include <stdio.h>
#include <stdlib.h> // is for EXIT_SUCCESS, EXIT_FAILURE
#include <unistd.h> // is for getopt handling
#include <assert.h>
#include <sys/stat.h> // checks if file exist


/**
 * @brief buffer size for stdin
 * 
 */
#define SIZE 256

/**
 * @brief tabstop amount if not changed
 * 
 */
static unsigned int tabstop = 8;
/**
 * @brief input and output File pointer
 * 
 */
FILE *out_fp, *in_fp;

/**
 * @brief closes all open File pointers if the programm is exited with an error occuring
 * 
 */
static void close_all(){
    if(in_fp != NULL) fclose(in_fp);
    if(out_fp != NULL) fclose(out_fp);
}

/**
 * @brief checks if incoming string has only valid digits starting with 1-9 and no characters exist
 * 
 * @param s the given char pointer which represents a string
 * @return 1 if it is a valid number otherwise 0
 */
static int digits_only(const char *s){
    if(*s == '0') return 0;
    while (*s) {
        if(*s>='0' && *s<='9') s++;
        else return 0;
    }
    return 1;
}

/**
 * @brief checks if the file does exist in the system
 * 
 * @param filename is the name of the file which is a char pointer which represents a string
 * @return 1 if it exists 0 if not
 */
static int checkIfFileExists(const char* filename){
    struct stat buffer;
    int exist = stat(filename,&buffer);
    if(exist == 0) return 1;
    else return 0;
}


/**
 * @brief expands the given input file or stdin depeding on the in_fp pointer and changes 
 *        its tabs with spaces and write it in the outfile or in stdout depending on the 
 *        out_fp pointer 
 * 
 * @param in_fp is a pointer which leads to the file it should read from or stdin
 * @param out_fp is a pointer which leads to the file it should print the expanded text which is either a output file oder stdout
 */
static void myexpand(FILE *in_fp, FILE *out_fp){ 
    char c;
    int x = 0;
    while((c = fgetc(in_fp)) != EOF){
        if(c == '\t'){
            int p = x%tabstop;
            x = 0;
            for(int i = 0; i < tabstop-p; i++){
                fprintf(out_fp, " ");
            }
        } else if(c == '\n') {
            x = 0;
            fprintf(out_fp, "\n" );
        } else {
            fprintf(out_fp, "%c",c);
            x++;
        }
    }
    if(out_fp == stdout) fprintf(stdout,"\n");
}

/**
 * @brief the main entry point of the program
 * 
 * @param argc number of command-line paramters in argv
 * @param argv array of command-line paramaters
 * @return EXIT_SUCCESS = 1 which means the programm worked succesfully or EXIT_FAILURE = 0 which means it didnt executed fully and breaked in the meantime
 */
int main(int argc, char *argv[]) {
    if(atexit(close_all)<0){
        fprintf(stderr, "Fatal error.\n");
        exit(EXIT_FAILURE);
    }
    if (argc > 0){
        int opt;
        FILE *in_fp = stdin, *out_fp = stdout;
        // ':' has to be infront of the string so the program can 
        // distinguish between '?' and ':'  
        while ((opt = getopt(argc, argv, ":t:o:")) != -1) {
            switch (opt) {
                case 't':
                    if(digits_only(optarg)){
                        tabstop = (int) strtol(optarg, NULL, 0);
                    } else {
                        exit(EXIT_FAILURE);
                    }
                    break;
                case 'o':
                    out_fp = fopen(optarg, "w");
                    break;
                case ':': 
                    fprintf(stderr, "Option needs a value\n");
                    exit(EXIT_FAILURE);
                case '?': 
                    fprintf(stderr, "Unknown option: %c\n", optopt);
                    exit(EXIT_FAILURE);
                default:
                    // code is unreachable so in debug mode you can understand when this code is acutally 
                    // reached which signals that the program doesn't do what it should do
                    assert(0);
            }
        }
        while (argc-optind > 0){
            if(!checkIfFileExists(argv[optind])){
                exit(EXIT_FAILURE);
            }
            in_fp = fopen(argv[optind], "r");
            
            myexpand(in_fp, out_fp);
            optind++;
        }
        if(in_fp == stdin){
            myexpand(in_fp, out_fp);
        }
        fclose(in_fp);
        fclose(out_fp);
        exit(EXIT_SUCCESS);
    }
    else{
        fprintf(stderr, "Fatal error.\n");
        exit(EXIT_FAILURE);
    }
}