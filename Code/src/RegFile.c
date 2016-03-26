

#include <stdio.h>
#include <inttypes.h>
#include "RegFile.h"

// 32x32 Register File
int32_t RegFile[NUMBER_OF_REGS];

void initRegFile(int32_t val) {
    int i;
    for(i=0;i<34;i++) {
    RegFile[i] = val;
    }
}

void printRegFile() {
    printf("\n ----- REG DUMP ----- \n");
    int j;
    for ( j=0; j < NUMBER_OF_REGS; j++) {
        printf("REG[%2d]: 0x%08x (%d)",j,RegFile[j],RegFile[j]);
        if(j%2==0){printf("\t\t");}
        else{printf("\n");}
        }
}




