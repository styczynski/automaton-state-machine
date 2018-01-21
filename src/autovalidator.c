/** @file
*
*  Autovalidator is the application to easily launch the server and testers using one command
*  and provide synchronization.
*
*  Synchronization makes it easy to test with valgrind or to verify the results.
*
*  Usage:
*
*     # Spawns server and tester for each given tester file
*     ./autovalidator <server_file> [<tester_file>...]
*
*     # -v flag recursively enable logging
*     ./autovalidator -v <server_file> [<tester_file>...]
*
*
*  Usage with valgrind for detailed leak checks and traced uninitialized variables (may be very slow):
*
*     valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all \
*        --trace-children=yes --track-origins=yes ./autovalidator ../validator.in  ../tester1.in ../tester2.in
*
*
*  @author Piotr Styczyński <piotrsty1@gmail.com>
*  @copyright MIT
*  @date 2018-01-21
*/
#include <stdio.h>
#include <string.h>


#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_TESTERS_PROCS 100

#ifdef FILE_BUF_SIZE
#undef FILE_BUF_SIZE
#endif

#define FILE_BUF_SIZE 10007

pid_t tester_pid[MAX_TESTERS_PROCS];
pid_t server_pid;

int verboseMode = 0;

void redirect(int dest, int src) {
    if(dest == -1) {
        fprintf(stderr, "[AUTOVALIDATOR] Could not open file: %s\n", strerror(errno));
        exit(-1);
    }
    if(dup2(dest, src) == -1) {
        fprintf(stderr, "[AUTOVALIDATOR] Could not redirect stream: %s\n", strerror(errno));
        exit(-1);
    }
    close(dest);
}

void printFile(char* header, char* name) {
    
    FILE* input = fopen(name, "r");
    
    if(input == NULL) {
        fprintf(stderr, "[AUTOVALIDATOR] Failed to load file: %s", strerror(errno));
        exit(-1);
    }
    
    char buffer[FILE_BUF_SIZE];
    size_t contentSize = 1;
    char *content = (char*) malloc(sizeof(char) * FILE_BUF_SIZE);
    if(content == NULL) {
        fclose(input);
        fprintf(stderr, "[AUTOVALIDATOR] Failed to allocate space file buffer: %s", strerror(errno));
        exit(-1);
    }
    content[0] = '\0';
    while(fgets(buffer, FILE_BUF_SIZE, input)) {
        contentSize += strlen(buffer);
        content = (char*) realloc(content, sizeof(char) * contentSize);
        if(content == NULL) {
            fclose(input);
            fprintf(stderr, "[AUTOVALIDATOR] Failed to reallocate space for file buffer: %s", strerror(errno));
            exit(-1);
        }
        strcat(content, buffer);
    }

    if(ferror(input)) {
        free(content);
        fclose(input);
        fprintf(stderr, "[AUTOVALIDATOR] Error reading from file: %s", strerror(errno));
        exit(-1);
    }
    
    fprintf(stdout, "[ %s ]:\n%s\n", header, content);
    fflush(stdout);
    
    fclose(input);
    free(content);
    
}

int main(int argc, char *argv[]) {
    
    char* server_input_file = NULL;
    char* server_output_file = NULL;
    char* tester_input_file[MAX_TESTERS_PROCS];
    char* tester_output_file[MAX_TESTERS_PROCS];
    int testers_count = -1;
    
    for(int i=0;i<MAX_TESTERS_PROCS;++i) {
        tester_input_file[MAX_TESTERS_PROCS] = NULL;
    }
    
    for(int i=1;i<argc;++i) {
        if(strcmp(argv[i], "-v") == 0) {
            verboseMode = 1;
        } else {
            if(testers_count == -1) {
                server_input_file = argv[i];
                server_output_file = (char*) malloc(sizeof(char) * 150);
                sprintf(server_output_file, "server_out.txt");
                ++testers_count;
            } else {
                tester_output_file[testers_count] = (char*) malloc(sizeof(char) * 150);
                tester_input_file[testers_count] = argv[i];
                sprintf(tester_output_file[testers_count], "tester_out_%d.txt", i);
                testers_count++;
            }
        }
    }
    
    if(testers_count < 0 || server_input_file == NULL) {
        fprintf(stdout, "Usage:\n   autovalidator <validator file> [<tester_files>...]\n");
        return 0;
    }
            
            
    switch (server_pid = fork()) {
        case -1: {
            fprintf(stderr, "[AUTOVALIDATOR] Could not start the server: Error in fork(): %s\n", strerror(errno));
            exit(-1);
        }
        case 0: {
            
            int input_desc = open(server_input_file, O_RDONLY);
            redirect(input_desc, 0);
           
            int output_desc = open(server_output_file, O_WRONLY | O_CREAT | O_TRUNC);
            redirect(output_desc, 1);
            
            char* vFlag = (verboseMode)?"-v":NULL;
            
            execlp("./validator", "validator", vFlag, NULL);
            fprintf(stderr, "[AUTOVALIDATOR] Could not start the server: Error in execlp(): %s\n", strerror(errno));
            exit(-1);
        }
    } 
    
    for(int i=0;i<testers_count;++i) {
        switch (tester_pid[i] = fork()) {
            case -1: {
                fprintf(stderr, "[AUTOVALIDATOR] Could not start the tester: Error in fork(): %s\n", strerror(errno));
                exit(-1);
            }
            case 0: {
                
                int input_desc = open(tester_input_file[i], O_RDONLY);
                redirect(input_desc, 0);
                
                int output_desc = open(tester_output_file[i], O_RDWR | O_CREAT | O_TRUNC, 0644);
                redirect(output_desc, 1);
                
                char* vFlag = (verboseMode)?"-v":NULL;
                
                execlp("./tester", "tester", vFlag, NULL);
                fprintf(stderr, "[AUTOVALIDATOR] Could not start the tester: Error in execlp(): %s\n", strerror(errno));
                exit(-1);
            }
        }
    }
    
    int wstatus;
    int cont = 1;
    while(cont) {
        if(wait(&wstatus) == -1) {
            if(errno == ECHILD) {
                break;
            }
        }
    }
    sleep(1);
    
    fprintf(stdout, "\033[0m");
    fprintf(stdout, "Program terminated.\nThe generated output:\n");
    
    printFile("SERVER", server_output_file);
    free(server_output_file);
    for(int i=0;i<testers_count;++i) {
        printFile("TESTER", tester_output_file[i]);
        free(tester_output_file[i]);
    }
    
    return 0;
}