/** @file
*
*  Automaton helper functions (C99 standard)
*
*  @author Piotr Styczyński <piotrsty1@gmail.com>
*  @copyright MIT
*  @date 2018-01-21
*/
#ifndef __AUTOMATON_H__
#define __AUTOMATON_H__

#include "automaton_config.h"

#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "memalloc.h"

/**
 * Iternal type of the transition graph
 */
typedef struct TransitionGraphImpl TransitionGraphImpl;

/**
 * Type of transition graph (pointer to the actual data structure)
 *
 */
typedef TransitionGraphImpl* TransitionGraph;

/**
 * Strucutre containing the transition graph.
 */
struct TransitionGraphImpl {
    int graph[MAX_Q][MAX_A][MAX_Q]; ///< graph[q][a][i] means that theres edge between states q -> graph[q][a][i] by letter a
    int size[MAX_Q][MAX_A];         ///< size[q][a] is the valid size of graph[q][a][i] (number of q by-letter-a neighbours)
    int acceptingStates[MAX_Q];     ///< accepting states list
    int q0; ///<  initial state
    int A;  ///<  the size of the alphabet: the alphabet is the set {a,...,x}, where 'x'-'a' = A-1
    int Q;  ///<  the number of states: the states are the set {0,...,Q-1}
    int U;  ///<  the number of universal states: universal states = {0, .., U-1}, existential states = {U, .., Q-1}
    int F;  ///<  the number of final states
};

/**
 * Prints the transition graph to the standard output.
 *
 * Using the following format:
 *
 *    Transition graph: {
 *       1 --[a]--> { 0 1 }
 *       0 --[z]--> { 2 }
 *       3 --[c]--> { 2 0 1 }
 *       ...
 *    }
 * 
 * @param[in] tg : Transition graph to be printed
 */
void printTransitionGraph(const TransitionGraph tg) {
    printf("Transition graph: {\n");
    for(int q=0;q<MAX_Q;++q) {
        for(int a=0;a<MAX_A;++a) {
            const int size = tg->size[q][a];
            if(size > 0) {
                printf("  %d --[%c]--> { ", q, (char)(a+'a'));
                for(int r=0;r<size;++r) {
                    printf("%d ", tg->graph[q][a][r]);
                }
                printf("}\n");
            }
        }
    }
    printf("}\n");
}

/**
 * Used to initialize transition graph.
 * This method can be used to clean the graph.
 * 
 * @param[in] tg : Input transition graph
 */
void initTransitionGraph(TransitionGraph tg) {
    tg->q0 = 0;
    tg->A = 0;
    tg->Q = 0;
    tg->U = 0;
    tg->F = 0;
    for(int q=0;q<MAX_Q;++q) {
        tg->acceptingStates[q] = 0;
        for(int a=0;a<MAX_A;++a) {
            tg->size[q][a] = 0;
            for(int p=0;p<MAX_Q;++p) {
                tg->graph[q][a][p] = -1;
            }
        }
    }
}

/**
 * Creates new initialized and empty transition graph.
 * 
 * @returns New empty transition graph
 */
TransitionGraph newTransitionGraph() {
    TransitionGraph tg = MALLOCATE(TransitionGraphImpl);
    initTransitionGraph(tg);
    return tg;
}

/**
 * Loads transition graph textual representation from standard input.
 * For details on valid textual representation of transition graph see: loadTransitionGraph docs.
 * 
 * NOTE: Returned array must be freed.
 *
 * @returns Loaded allocated array with textual graph representation
 */
char* loadTransitionGraphDescFromStdin() {
    char buffer[FILE_BUF_SIZE];
    size_t contentSize = 1;
    char *content = MALLOCATE_ARRAY(char, FILE_BUF_SIZE);
    if(content == NULL) {
        syserr("Failed to allocate content");
    }
    content[0] = '\0';
    while(fgets(buffer, FILE_BUF_SIZE, stdin)) {
        contentSize += strlen(buffer);
        content = MREALLOCATE_ARRAY(char, contentSize, content);
        if(content == NULL) {
            syserr("Failed to reallocate content");
        }
        strcat(content, buffer);
    }

    if(ferror(stdin)) {
        FREE(content);
        syserr("Error reading from stdin.");
    }
    
    return content;
}

/**
 * Loads transition graph textual representation from the given file.
 * For details on valid textual representation of transition graph see: loadTransitionGraph docs.
 *
 * NOTE: Returned array must be freed.
 *
 * @param[in] input : Input file
 * @returns Loaded allocated array with textual graph representation
 */
char* loadTransitionGraphDescFromFile(FILE* input) {
    fseek(input, 0, SEEK_END);
    long fsize = ftell(input);
    
    fseek(input, 0, SEEK_SET);
    char *buff = MALLOCATE_BLOCKS(1, fsize + 1);
    
    fread(buff, fsize, 1, input);
    fclose(input);
    
    buff[fsize] = 0;
    return buff;
}

/**
 * Reads the transition graph from the given text array.
 *
 * Valid graph format is:
 *
 *   N A Q U F\n
 *   q\n
 *   [q]\n
 *   [q a r [p]\n]
 *   
 *   where
 *     N is the number of lines of the input;
 *     A is the size of the alphabet: the alphabet is the set {a,...,x}, where 'x'-'a' = A-1;
 *     Q is the number of states: the states are the set {0,...,Q-1};
 *     U is the number of universal states: universal states = {0, .., U-1}, existential states = {U, .., Q-1};
 *     F is the number of final states;
 *     q, r, p denotes some states
 *     and a is a letter of the alphabet
 *
 *  NOTE:
 *     A sequence [wyr] denotes that the string wyr repeats a finite (greater than or equal to 0) number of times.
 * 
 * @param[in] input : Input text
 * @param[in] tg    : Transition graph to be set
 */
void loadTransitionGraph(char** input, TransitionGraph tg) {
    
    if(input == NULL) return;
    
    int N;
    int q;
    char a;
    int pos;
    int npos;
    int r;
    char* line_buf = MALLOCATE_ARRAY(char, LINE_BUF_SIZE);
    size_t line_buf_size = LINE_BUF_SIZE;
    
    char** line_buf_p = &line_buf;
    size_t* line_buf_s = &line_buf_size;
    
    strGetline(line_buf_p, line_buf_s, input);
    sscanf(line_buf, "%d %d %d %d %d", &N, &(tg->A), &(tg->Q), &(tg->U), &(tg->F));
    
    strGetline(line_buf_p, line_buf_s, input);
    sscanf(line_buf, "%d", &(tg->q0));
    
    strGetline(line_buf_p, line_buf_s, input);
    pos = 0;
    for(int i=0;i<tg->F;++i) {
        sscanf(line_buf+pos, "%d%n", &q, &npos);
        tg->acceptingStates[q] = 1;
        pos += npos;
    }

    while(1) {
       int getline_size = strGetline(line_buf_p, line_buf_s, input);
       if(getline_size == -1) break;

       if(getline_size > 0) {
           if(!sscanf(line_buf, "%d %c%n", &q, &a, &pos)) {
               continue;
           }
           if(*((char*)(line_buf+pos)) == '\0') break;
           while(sscanf(line_buf+pos, "%d%n", &r, &npos)) {
               pos += npos;
               
               tg->graph[q][(int)(a-'a')][tg->size[q][(int)(a-'a')]++] = r;
               if(*((char*)(line_buf+pos)) == '\0') break;
           }
       }
    }
    
    FREE(line_buf);
    
}


#endif // __AUTOMATON_H__