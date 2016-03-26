

#ifndef   REG_FILE_H_  
#define  REG_FILE_H_

#define NUMBER_OF_REGS 34


extern int32_t RegFile[NUMBER_OF_REGS];
extern void initRegFile(int32_t val);
extern void printRegFile();


#endif


