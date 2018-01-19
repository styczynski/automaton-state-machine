#ifndef __AUTOMATON_H__
#define __AUTOMATON_H__

#include "automaton_config.h"

#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "syserr.h"

typedef struct TransitionGraphImpl TransitionGraphImpl;
typedef TransitionGraphImpl* TransitionGraph;


struct TransitionGraphImpl {
    int graph[MAX_Q][MAX_A][MAX_Q];
    int size[MAX_Q][MAX_A];
    int acceptingStates[MAX_A];
    int q0;
    int A;
    int Q;
    int U;
    int F;
};

void printTransitionGraph(const TransitionGraph tg) {
    printf("Transition graph: {\n");
    for(int q=0;q<MAX_Q;++q) {
        for(int a=0;a<MAX_A;++a) {
            const int size = tg->size[q][a];
            if(size > 0) {
                printf("  %d --[%c]--> { ", q, (char)a);
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
    for(int q=0;q<MAX_Q;++q) {
        for(int a=0;a<MAX_A;++a) {
            tg->size[q][a] = 0;
        }
    }
}

TransitionGraph newTransitionGraph() {
    TransitionGraph tg = (TransitionGraph) malloc(sizeof(TransitionGraphImpl));
    initTransitionGraph(tg);
    return tg;
}

char* loadTransitionGraphDesc(FILE* input) {
    fseek(input, 0, SEEK_END);
    long fsize = ftell(input);
    
    fseek(input, 0, SEEK_SET);
    char *buff = malloc(fsize + 1);
    
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
    char* line_buf = (char*) malloc(LINE_BUF_SIZE * sizeof(char));
    size_t line_buf_size = LINE_BUF_SIZE;
    
    strGetline(&line_buf, &line_buf_size, input);
    sscanf(line_buf, "%d %d %d %d %d", &N, &(tg->A), &(tg->Q), &(tg->U), &(tg->F));
    
    strGetline(&line_buf, &line_buf_size, input);
    sscanf(line_buf, "%d", &(tg->q0));
    
    strGetline(&line_buf, &line_buf_size, input);
    pos = 0;
    for(int i=0;i<tg->F;++i) {
        sscanf(line_buf+pos, "%d%n", &q, &npos);
        tg->acceptingStates[q] = 1;
        pos += npos;
    }

    while(strGetline(&line_buf, &line_buf_size, input) > 0) {
       sscanf(line_buf, "%d %c%n", &q, &a, &pos);
       if(*((char*)(line_buf+pos)) == '\0') break;
       while(sscanf(line_buf+pos, "%d%n", &r, &npos)) {
           pos += npos;
           tg->graph[q][(int)a][tg->size[q][(int)a]++] = r;
           if(*((char*)(line_buf+pos)) == '\0') break;
       }
    }
    
    free(line_buf);
    
}


#endif // __AUTOMATON_H__