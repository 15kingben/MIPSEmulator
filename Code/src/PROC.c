
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
    bool isJumping = false;
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
        i--;
        switchCaseLabel:
        i++;

        DynInstCount++;
        CurrentInstruction = readWord(PC,false);
        printRegFile();
        uint32_t opCode = CurrentInstruction >> 26;
        uint32_t funcCode = CurrentInstruction & 63;
        uint32_t rs = 0 + ((CurrentInstruction & ( 31 << (32-11))) >> 21);
        uint32_t rt = (CurrentInstruction & ( 31 << (32-16))) >> 16;
        uint32_t rd = (CurrentInstruction & ( 31 << (32-21))) >> 11 ;
        uint32_t shamt = (CurrentInstruction & ( 31 << (32-26))) >> 6;
        int32_t imm =  (CurrentInstruction & ( 65535));
        imm =  (imm << 16) >> 16;
        uint32_t target = CurrentInstruction & (67108864 - 1);
        int32_t *HI = &RegFile[32];
        int32_t *LO = &RegFile[33];
        int32_t jumpAdress = CurrentInstruction & 67108863;

        /*printf("IMM=%d\n", imm);
        printf("OPCODE=%d\n", opCode);
        printf("FUNC=%d\n", funcCode);
        printf("ISJumping=%d\n", isJumping);
        if(isJumping){
          printf("newPC=%d\n", newPC);
        }
        printf("PC=%d\n", PC);*/

        //getchar();
        switch(opCode){
          case 0:  //Special = 0
            switch(funcCode){
              case 0: //0 sll
                RegFile[rd] = RegFile[rt] << shamt;
                break;
              case 2: //10 srl
                RegFile[rd] = (int32_t)(((uint32_t)RegFile[rt]) >> shamt);
                break;
              case 3: //11 sra
                RegFile[rd] = RegFile[rt] >> shamt;
                break;
              case 4: //100 sllv
                RegFile[rd] = RegFile[rt] << RegFile[rs];
                break;
              case 6: //110 srlv
                RegFile[rd] = (int32_t)(((uint32_t)RegFile[rt]) >> RegFile[rs]);
                break;
              case 7: //111 srav
                RegFile[rd] = RegFile[rt] >> RegFile[rs];
                break;
              case 8: //1000 jr
                isJumping = true;
                newPC = RegFile[rs];
                PC+=4;
                goto switchCaseLabel;
                break;
              case 9: //1001 jalr
                isJumping = true;
                newPC = RegFile[rs];
                RegFile[rd] = PC + 8;
                PC+=4;
                goto switchCaseLabel;
                break;
              case 32: //100000 add
                RegFile[rd] = RegFile[rs] + RegFile[rt];
                break;
              case 33: //100001 addu
                RegFile[rd] = RegFile[rs] + RegFile[rt];//(int32_t) ((uint32_t)
                break;
              case 12: //1100: syscall
                SyscallExe(RegFile[2]);
                break;
              case 16: //10000 mfhi
                RegFile[rd] = RegFile[32];//*HI;
                break;
              case 18: //10010 mflo
                RegFile[rd] = RegFile[33];//*LO;
                break;
              case 17: //10001 mthi
                RegFile[32] = RegFile[rs];
                break;
              case 19: //10011 mflo
                RegFile[33] = RegFile[rs];
                break;


              case 34: //100010 sub
                RegFile[rd] = RegFile[rs]-RegFile[rt];
                break;
              case 35: //100011 subu
                RegFile[rd]= RegFile[rs]-RegFile[rt];//(uint32_t)
                break;
              case 24: // 11000 mult
              {
                int64_t result = ((int64_t)RegFile[rs]) * ((int64_t) RegFile[rt]);
                int64_t half = 4294967296-1;
                *LO = result & half;
                *HI = result & (half << 32);
                break;
              }
              case 25: // 11001 multu
              {
                uint64_t result = ((int64_t) ((uint32_t)RegFile[rs])) * ((int64_t) ((uint32_t)RegFile[rt]));
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
                *LO = (RegFile[rs]) / (RegFile[rt]); //(uint32_t)
                *HI = (RegFile[rs]) % (RegFile[rt]);
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
              case 42: //101010 slt
                RegFile[rd] = (RegFile[rs] < RegFile[rt]);
                break;
              case 43: //101011 sltu
                RegFile[rd] = ((uint32_t)RegFile[rs] < (uint32_t)RegFile[rt]);
                break;
              }


           break;

          case 8: //1000 addi
              RegFile[rt] = RegFile[rs] + imm;
              break;
          case 9: //1001 addiu
            //("ADDINGUNSIGNEDIMMEDIATE RT=%d RS=%d IMM=%d\n", RegFile[rt], RegFile[rs], imm);
             RegFile[rt] =  RegFile[rs] + imm;//(uint32_t)
             break;
          case 10: //1010 slti
            RegFile[rt] = (RegFile[rs] < imm);
            break;
          case 11: //1011 sltui
            RegFile[rt] = (RegFile[rs] < (uint32_t)imm); //(uint32_t)
            break;
          case 12: //1100 andi
              {uint32_t andi = imm;
              andi = (andi << 16) >> 16;
              RegFile[rt] = RegFile[rs] & (andi);
              break;}
          case 13: //1101 ori    uint32_t andi = imm;
              {uint32_t ori = imm;

              ori = (ori << 16) >> 16;
              RegFile[rt] = RegFile[rs] | (ori);}
              break;
          case 14: //1110 xori     uint32_t andi = imm;
              {uint32_t xori = imm;

              xori = (xori << 16) >> 16;
              RegFile[rt] = RegFile[rs]^(xori);}
              break;
          case 15: //1111 lui
            RegFile[rt] = ((imm) << 16);
            break;


          //LOADS AND STORES
          case 32: // 100000 lb
              {
              int32_t byte = (int32_t) readByte(RegFile[rs] + imm, true);
              byte = (byte << (24)) >> 24; //sign extension
              RegFile[rt] = byte;
              break;
              }
          case 33: // 100001 lh
            {
            int32_t halfWord = (int32_t) readWord(RegFile[rs] + imm, true);
            halfWord = halfWord & (65535 << 16); // half word
            halfWord = (halfWord) >> 16; //sign extension
            RegFile[rt] = halfWord;
            break;
            }
          case 34: // 100010 lwl
            {
              int32_t startByte = (RegFile[rs] + imm )%4;
              int32_t diff = (RegFile[rs] + imm ) - startByte;
              uint32_t read = (uint32_t)readWord(startByte , true);
              read = (read << diff);

              RegFile[rt] = RegFile[rt] | read;

            // uint32_t halfWord = (uint32_t) readWord(RegFile[rs] + imm, false);
            // halfWord = halfWord & (65535 << 16); // half word
            // RegFile[rt] = RegFile[rt] | halfWord;
            break;
            }

          case 35: // 100011 lw
            {
            int32_t word = (int32_t) readWord(RegFile[rs] + imm, true);
            RegFile[rt] = word;
            break;
            }

          case 36: // 100100  lbu
            {
            int32_t byte = (uint32_t) readByte(RegFile[rs] + imm, true);
            RegFile[rt] = byte;
            break;
            }
          case 37: // 100101 lhu
            {
            uint32_t halfWord = (uint32_t) readWord(RegFile[rs] + imm, true);
            halfWord = halfWord & (65535 << 16); // half word
            halfWord = (halfWord) >> 16; //no sign extension
            RegFile[rt] = (int32_t)halfWord;
            break;
          }
          case 38: //100110 lwr

            {
              int32_t startByte = (RegFile[rs] + imm)%4;
              int32_t diff = (RegFile[rs] + imm) - startByte + 1;
              uint32_t read = (uint32_t)readWord(startByte , true);

              read = read >> (4-diff);
              RegFile[rt] = RegFile[rt] | read;
            }
          case 40: //101000 sb
            {
            writeByte(RegFile[rs] + imm, RegFile[rt] & 255, true);
            break;
            }
          case 41: //101001 sh

            {
            writeByte(RegFile[rs] + imm, (RegFile[rt] & (255 << 8)) >> 8, true);
            writeByte(RegFile[rs] + imm + 8, RegFile[rt] & 255, true);
            break;}
          case 42: //101010 swl
            {
              int32_t effAddr = RegFile[rs] + imm;
              int count = 0;
              for(int addr = effAddr; addr < (effAddr + 4) - ((effAddr + 4)%4); addr++){
                writeByte(addr, (uint8_t) (RegFile[rt] >> 8*(3 - count)), true);
                count++;
              }
            break;}
          case 43: //101011 sw
            {writeWord(RegFile[rs] + imm, RegFile[rt], true);
            break;}
          case 46: //101110 swr
            {
              int32_t effAddr = RegFile[rs] + imm;
              int diff = effAddr - ((effAddr)%4);
              int count = 0;
              for(int addr = effAddr - ((effAddr)%4); addr <= effAddr; addr++){
                writeByte(addr, (uint8_t ) (RegFile[rt] >> 8*(diff - 1 - count)), true);
                count++;
              }
            break;}

          //JUMPS
          case 2: // 10 j
            isJumping = true;
            newPC = (jumpAdress << 2) | ((PC + 4) & (4 << 28));
            PC+=4;
            goto switchCaseLabel;
            break;
          case 3: // 11 jal
            isJumping = true;
            newPC = (jumpAdress << 2) | ((PC + 4) & (4 << 28));
            RegFile[31] = PC + 8;
            PC+=4;
            goto switchCaseLabel;
            break;

          //BRANCHES
          case 4: //100 beq
            {isJumping = true;
            int32_t offset = imm;
            offset = (offset << 16) >> 14;

            if(RegFile[rs] == RegFile[rt]){
              newPC = (PC + 4) + offset;
            }else{
              newPC = PC + 8;
            }
            PC+=4;
            goto switchCaseLabel;
            break;
            }

          case 5: //101 bne
            {isJumping = true;
            int32_t offset = imm;
            //offset = (offset << 16) >> 14;
            offset = offset << 2;
            if(RegFile[rs] != RegFile[rt]){
              newPC = (PC+4) + offset;
            }else{
              newPC = PC + 8;
            }
            PC+=4;
            goto switchCaseLabel;

            break;
            }

          case 6: //110 blez
            {isJumping = true;
            int32_t offset = (int32_t) imm;
            offset = (offset << 16) >> 14;

            if(RegFile[rs] <= 0){//RegFile[rt]){
              newPC = (PC + 4) + offset;
            }else{
              newPC = PC + 8;
            }
            PC+=4;
            goto switchCaseLabel;
            break;
            }

          case 7: //111 bgtz
            {isJumping = true;
            int32_t offset = (int32_t) imm;
            offset = (offset << 16) >> 14;

            if(RegFile[rs] > 0){//RegFile[rt]){
              newPC = (PC + 4) + offset;
            }else{
              newPC = PC + 8;
            }
            goto switchCaseLabel;
            break;
            }


          case 20://10100 beql
            {
            isJumping = true;
            int32_t offset = (int32_t) imm;
            offset = (offset << 16) >> 14;

            if(RegFile[rs] == RegFile[rt]){
              newPC = (PC + 4) + offset;
            }else{
              newPC = PC + 8;
            }
            PC+=4;
            goto switchCaseLabel;
            break;
            }

          case 21://10101 bnel
            {
            isJumping = true;
            int32_t offset = (int32_t) imm;
            offset = (offset << 16) >> 14;

            if(RegFile[rs] != RegFile[rt]){
              newPC = (PC + 4) + offset;
            }else{
              newPC = PC + 8;
            }
            PC+=4;
            goto switchCaseLabel;
            break;
            }

          case 22://10110 blezl
            {
            isJumping = true;
            int32_t offset = (int32_t) imm;
            offset = (offset << 16) >> 14;

            if(RegFile[rs] <= 0){//RegFile[rt]){
              newPC = (PC + 4) + offset;
            }else{
              newPC = PC + 8;
            }
            PC+=4;
            goto switchCaseLabel;
            break;
            }


          case 1:
            switch(rt){
              case 0: //bltz
                {
                //printf("bltz RegRS:%d", RegFile[rs]);

                isJumping = true;
                int32_t offset = (int32_t) imm;
                offset = (offset << 16) >> 14;


                if(RegFile[rs] < 0){//RegFile[rt]){
                  newPC = (PC + 4) + offset;
                }else{
                  newPC = PC + 8;
                }
                PC+=4;
                goto switchCaseLabel;
                break;
                }

              case 1://bgez
                {
                isJumping = true;
                int32_t offset = (int32_t) imm;
                offset = (offset << 16) >> 14;

                if(RegFile[rs] >= 0){//RegFile[rt]){
                  newPC = (PC + 4) + offset;
                }else{
                  newPC = PC + 8;
                }
                PC+=4;
                goto switchCaseLabel;
                break;
                }


              case 16://bltzal
                {
                isJumping = true;
                int32_t offset = (int32_t) imm;
                offset = (offset << 16) >> 14;

                if(RegFile[rs] <0){// RegFile[rt]){
                  newPC = (PC + 4) + offset;
                }else{
                  newPC = PC + 8;
                }
                RegFile[31] = PC + 8;
                PC+=4;
                goto switchCaseLabel;
                break;
                }

              case 17: //bgezal
                {
                isJumping = true;
                int32_t offset = (int32_t) imm;
                offset = (offset << 16) >> 14;

                if(RegFile[rs] >= 0){//RegFile[rt]){
                  newPC = (PC + 4) + offset;
                }else{
                  newPC = PC + 8;
                }
                RegFile[31] = PC + 8;
                PC+=4;
                goto switchCaseLabel;
                break;
                }

            }
            break;

        }

        if(isJumping){
          PC = newPC;
          isJumping = false;
        }else{
          PC += 4;
        }

    /********************************/
    //Add your implementation here
    /********************************/

    } //end fori


    //Close file pointers & free allocated Memory
    closeFDT();
    CleanUp();
    return 1;
}
