
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "RegFile.h"
#include "Syscall.h"
#include "utils/heap.h"
#include "elf_reader/elf_reader.h"

//Stats

uint32_t DynInstCount = 0;

void write_initialization_vector(uint32_t sp, uint32_t gp, uint32_t start) {
        printf("\n ----- BOOT Sequence ----- \n");
        printf("Initializing sp=0x%08x; gp=0x%08x; start=0x%08x\n", sp, gp, start);
        RegFile[28] = gp;
        RegFile[29] = sp;
        RegFile[31] = start;
        printRegFile();

    }


int main(int argc, char * argv[]) {

    int MaxInst = 0;
    int status = 0;
    uint32_t i;
    uint32_t PC,newPC;
    uint32_t CurrentInstruction;

    if (argc < 2) {
      printf("Input argument missing \n");
      return -1;
    }
    sscanf (argv[2], "%d", &MaxInst);

    //Open file pointers & initialize Heap & Regsiters
    initHeap();
    initFDT();
    initRegFile(0);

    //Load required code portions into Emulator Memory
    status =  LoadOSMemory(argv[1]);
    if(status <0) { return status; }

    //set Global & Stack Pointers for the Emulator
    // & provide startAddress of Program in Memory to Processor
    write_initialization_vector(exec.GSP, exec.GP, exec.GPC_START);

    printf("\n ----- Execute Program ----- \n");
    printf("Max Instruction to run = %d \n",MaxInst);
    PC = exec.GPC_START;
    for(i=0; i<MaxInst ; i++) {
        DynInstCount++;
        CurrentInstruction = readWord(PC,false);
        printRegFile();
        uint32_t opCode = CurrentInstruction >> 26;
        uint32_t funcCode = CurrentInstruction & 63;
        uint32_t rs = 0 + ((CurrentInstruction & ( 31 << (32-11))) >> 21);
        uint32_t rt = (CurrentInstruction & ( 31 << (32-16))) >> 16;
        uint32_t rd = (CurrentInstruction & ( 31 << (32-21))) >> 11 ;
        uint32_t shamt = (CurrentInstruction & ( 31 << (32-26))) >> 6;
        uint32_t imm = CurrentInstruction & ( 65536 - 1);
        uint32_t target = CurrentInstruction & (67108864 - 1);
        int32_t *HI = &RegFile[32];
        int32_t *LO = &RegFile[33];

        printf("%d", funcCode);

        switch(opCode){
          printf("OP=%d\n", opCode);
          case 0:  //Special = 0
            switch(funcCode){
             printf("FUNC=%d\n", funcCode);
              case 0: //0 sll
                RegFile[rd] = RegFile[rt] << shamt;
                break;
              case 2: //10 srl
                RegFile[rd] = RegFile[rt] >> shamt;
                break;
              case 3: //11 sra
                RegFile[rd] = RegFile[rt] << shamt;
                break;
              case 4: //100 sllv
                RegFile[rd] = RegFile[rt] << RegFile[rs];
                break;
              case 6: //110 srlv
                RegFile[rd] = RegFile[rt] >> RegFile[rs];
                break;
              case 7: //111 srav
                RegFile[rd] = RegFile[rt] << RegFile[rs];
                break;
              case 32: //100000 add
                RegFile[rd] = RegFile[rs] + RegFile[rt];
                break;
              case 33: //100001 addu
                RegFile[rd] = RegFile[rs] + RegFile[rt];
                break;
              case 12: //1100: syscall
                SyscallExe(RegFile[2]);
                break;
              case 16: //10000 mfhi
                RegFile[rd] = *HI;
                break;
              case 18: //10010 mflo
                RegFile[rd] = *LO;
                break;
              case 17: //10001 mthi
                *HI = RegFile[rd];
                break;
              case 19: //10011 mflo
                *LO = RegFile[rd];
                break;

              case 34: //100010 sub
                RegFile[rd] = RegFile[rs]-RegFile[rt];
                break;
              case 35: //100011 subu
                RegFile[rd]= RegFile[rs]-RegFile[rt];
                break;
              case 24: // 11000 mult
              {
                uint64_t result = ((uint64_t)RegFile[rs]) * ((uint64_t) RegFile[rt]);
                uint64_t half = 4294967296-1;
                *LO = result & half;
                *HI = result & (half << 32);
                break;
              }
              case 25: // 11001 multu
              {
                uint64_t result = ((uint64_t)RegFile[rs]) * ((uint64_t) RegFile[rt]);
                uint64_t half = 4294967296-1;
                *LO = result & half;
                *HI = result & (half << 32);
                break;
              }
              case 26: //11010 div
                *LO = RegFile[rs] / RegFile[rt];
                *HI = RegFile[rs] % RegFile[rt];
                break;
              case 27: //11011 divu
                *LO = RegFile[rs] / RegFile[rt];
                *HI = RegFile[rs] % RegFile[rt];
                break;
              case 38: //100110 xor
                RegFile[rd] = RegFile[rs] ^ RegFile[rt];
                break;
              case 36: //100100 and
                RegFile[rd] = RegFile[rs] & RegFile[rt];
                break;
              case 37: //100101 or
                RegFile[rd] = RegFile[rs] | RegFile[rt];
                break;
              case 39: //100111 nor
                RegFile[rd] = ~(RegFile[rs] | RegFile[rt]);
                break;
              case 44: //101010 slt
                RegFile[rd] = (RegFile[rs] < RegFile[rt]);
                break;
              case 45: //101011 sltu
                RegFile[rd] = (RegFile[rs] < RegFile[rt]);
                break;
              }
           break;
           case 8: //1000 addi
              RegFile[rt] = RegFile[rs] + imm;
              break;
          case 9: //1001 addiu
             RegFile[rt] = RegFile[rs] + imm;
             break;
          case 10: //1010 slti
            RegFile[rt] = (RegFile[rs] < imm);
            break;
          case 11: //1011 sltui
            RegFile[rt] = (RegFile[rs] < imm);
            break;
          case 12: //1100 andi
              RegFile[rt] = RegFile[rs] & (imm);
              break;
          case 13: //1101 ori
              RegFile[rt] = RegFile[rs] | (imm);
          case 14: //1110 xori
              RegFile[rt] = RegFiles[rs]^(imm);
              break;

        }

        PC += 4;//will be changed
    /********************************/
    //Add your implementation here
    /********************************/

    } //end fori


    //Close file pointers & free allocated Memory
    closeFDT();
    CleanUp();
    return 1;
}
