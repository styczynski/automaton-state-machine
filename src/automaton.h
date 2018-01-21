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
 * Transition graph cotnains values:
 *
 *   - q0  initial state
 *   - A   the size of the alphabet: the alphabet is the set {a,...,x}, where 'x'-'a' = A-1
 *   
 *
 */
 N is the number of lines of the input;
A is the size of the alphabet: the alphabet is the set {a,...,x}, where 'x'-'a' = A-1;
Q is the number of states: the states are the set {0,...,Q-1};
U is the number of universal states: universal states = {0, .., U-1}, existential states = {U, .., Q-1};
F is the number of final states;
q,r,p denotes some states;
and a is a letter of the alphabet


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
 *
 * @param[in] element : ListData
 * @return ListIterator
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

TransitionGraph newTransitionGraph() {
    TransitionGraph tg = MALLOCATE(TransitionGraphImpl);
    initTransitionGraph(tg);
    return tg;
}

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
    
    //fprintf(stderr, "GRAPH HEAD %d %d %d %d %d\n", N, tg->A, tg->Q, tg->U, tg->F);
    
    strGetline(line_buf_p, line_buf_s, input);
    sscanf(line_buf, "%d", &(tg->q0));
    
    //fprintf(stderr, "GRAPH Q0 %d\n", tg->q0);
    
    strGetline(line_buf_p, line_buf_s, input);
    pos = 0;
    for(int i=0;i<tg->F;++i) {
        sscanf(line_buf+pos, "%d%n", &q, &npos);
        //fprintf(stderr, "STATE GOOD Q %d\n", q);
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
               
               //fprintf(stderr, "CONNECT %d -[%c]-> %d\n", q, a, r);
               
               //fprintf(stderr, "set [%d][%c][%d]\n", q, a, tg->size[q][a]);
               tg->graph[q][(int)(a-'a')][tg->size[q][(int)(a-'a')]++] = r;
               if(*((char*)(line_buf+pos)) == '\0') break;
           }
       }
    }
    
    FREE(line_buf);
    
}


#endif // __AUTOMATON_H__