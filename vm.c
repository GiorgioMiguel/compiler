/*
Title: Homework 4 Extended P-Machine
Class: COP 3402-25 by Euripides Montagne
Author: Giorgio Miguel Torregrosa and Yasmeen Moufakkir
Date: 7/13/2025
*/

// headers used
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// global variable initializations
#define ARRAY_SIZE 500
int pas[ARRAY_SIZE] = {};

// defining Instruction Record
typedef struct IR {
    int op;
    int L;
    int M;
} IR;

// prototyping functions
int base( int BP, int L);
void printState(int pc, int sp, int bp, struct IR ir);

// start main
int main(int argc, char* argv[]) {
    
    // initializing the IR
    IR ir;
    ir.op = 0;
    ir.L = 0;
    ir.M = 0;

    // check if a file was given
    if (argc < 2) {
        printf("No input file given. Please retype the execution command with a file name included.");
        return EXIT_FAILURE;
    }// end parameter check for file

    // variable inilizations
    int BP = 499;
    int SP = BP + 1;
    int PC = 10;
    int eop = 1;

    // file pointer
    FILE* fp = fopen(argv[1], "r");
    if(fp == NULL){
        printf("Error Opening File.");
        return 1;
    }// end file pointer assignment

    // loading the instruction set into the pas
    int i = PC;
    while(fscanf(fp, "%d %d %d", &ir.op, &ir.L, &ir.M) == 3) {
        pas[i] = ir.op;
        pas[i + 1] = ir.L;
        pas[i + 2] = ir.M;
        i += 3;
    }// end loading the pas
    fclose(fp); // close file

    // initial print state
    printf("\t\tPC\tBP\tSP\tstack\n");
    printf("Initial values:\t%d\t%d\t%d\n\n", PC, BP, SP);

    // begin fetch/execute cycle
    while(eop) {

        // assign op code, L and M
        ir.op = pas[PC];
        ir.L = pas[PC + 1];
        ir.M = pas[PC + 2];

        // advance program counter
        PC += 3;

        // begin switch cases for op codes
        switch(ir.op) {
           
            // increment op code (INC)
            case 1:
                SP -= ir.M;
            break;// end case 1
            
            // arithmetic op code or (RTN)
            case 2:
                // return
                if(ir.M == 0) {
                    SP = BP + 1;
                    BP = pas[SP - 2];
                    PC = pas[SP - 3];
                }// end subcase 0
                // addition 
                else if(ir.M == 1) {
                    pas[SP + 1] = pas[SP + 1] + pas[SP];
                    SP++;
                }// end subcase 1
                // subtraction 
                else if(ir.M == 2) { 
                    pas[SP + 1] = pas[SP + 1] - pas[SP];
                    SP++;
                }// end subcase 2
                // multiplication
                else if(ir.M == 3) {
                    pas[SP + 1] = pas[SP + 1] * pas[SP];
                    SP++;
                }// end subcase 3
                // division
                else if(ir.M == 4) {
                    pas[SP + 1] = pas[SP + 1] / pas[SP];
                    SP++;
                }// end subcase 4
                // is equal to
                else if(ir.M == 5) {
                    pas[SP + 1] = (pas[SP + 1] == pas[SP]);
                    SP++;
                }// end subcase 5
                // is not equal to
                else if(ir.M == 6) {
                    pas[SP + 1] = (pas[SP + 1] != pas[SP]);
                    SP++;
                }// end subcase 6
                // is less than
                else if(ir.M == 7) {
                    pas[SP + 1] = pas[SP + 1] < pas[SP];
                    SP++;
                }// end subcase 7
                // is less than or equal to
                else if(ir.M == 8) {
                    pas[SP + 1] = pas[SP + 1] <= pas[SP];
                    SP++;
                }// end subcase 8
                // is greater than
                else if(ir.M == 9) {
                    pas[SP + 1] = pas[SP + 1] > pas[SP];
                    SP++;
                }// end subcase 9
                // is greater than or equal to
                else if(ir.M == 10) {
                    pas[SP + 1] = pas[SP + 1] >= pas[SP];
                    SP++;
                }// end subcase 10
                // modulo
                else if(ir.M == 11) {
                    pas[SP + 1] = pas[SP + 1] % pas[SP];
                    SP++;
                }// end subcase 11
                // default to error if any other value encountered
                else {
                    printf("Invalid arithmetic code encountered. Terminating program...");
                    return EXIT_FAILURE;
                }// end else
            break;// end case 2
            
            // load op code (LOD)
            case 3:
                SP = SP -1;
                pas[SP] = pas[base(BP, ir.L) - ir.M];
            break;// end case 3
            
            // store op code (STO)
            case 4:
                pas[base(BP, ir.L) - ir.M] = pas[SP];
                SP = SP + 1;
            break;// end case 4
            
            // call op code (CAL)
            case 5:
                pas[SP - 1] = base(BP, ir.L); // static link (SL)
                pas[SP - 2] = BP; // dynamic link (DL)
                pas[SP - 3] = PC; // return address (RA)
                BP = SP - 1;
                PC = ir.M;
            break;// end case 5
            
            // literal op code (LIT)
            case 6:
                SP = SP - 1;
                pas[SP] = ir.M;
            break;// end case 6
            
            // jump op code (JMP)
            case 7:
                PC = ir.M;
            break;// end case 7
            
            // jump conditionally op code (JMC)
            case 8:
                if(pas[SP] == 0) {
                    PC = ir.M;
                }// end if
                SP = SP + 1;
            break;// end case 8
            
            // system op code (SYS)
            case 9:
                // output value and pop the stack
                if(ir.M == 1) {
                    printf("output result is: %d\n", pas[SP]);
                    SP++;
                }// end if
                // read in an integer and store it at the top of the stack
                else if(ir.M == 2) {
                    int temp;
                    printf("Please Enter an integer: ");
                    scanf("%d", &temp);
                    SP--;
                    pas[SP] = temp;
                }// end else if
                // halt the program
                else if(ir.M == 3) {
                    eop = 0;
                }// end else if
                // default to error if any other value is encountered
                else {
                    printf("Invalid system call encountered. Terminating program...");
                    return EXIT_FAILURE;
                }// end else
            break;// end case 9
            // default case (invalid OP code)
            default:
                printf("Invalid OP code encountered. Terminating program...");
                return EXIT_FAILURE;
            break;// end invalid scenario
        }// end switch cases

        // printing the current state before next cycle
        printState(PC, SP, BP, ir);
    }// end while loop
}// end main

// Provided function: Find base L levels down
int base( int BP, int L) {
    int arb = BP; // arb = activation record base
    
    while ( L > 0) { //find base L levels down
    arb = pas[arb];
    L--;
    }// end while loop

    return arb;
}// end provided function

// print utility function
void printState(int pc, int sp, int bp, struct IR ir) {
    
    // numbers to op code for easier printing
    char opCodes[11][4] = {"NUL\0", "INC\0", "RTN\0", "LOD\0", "STO\0", "CAL\0", "LIT\0", "JMP\0", "JPC\0", "SYS\0"};

    // if op code is arithmetic, change string to the correct mnemonic
    if(ir.op == 2) {
        switch(ir.M) {
            // RTN case, change nothing
            case 0:
            break;
            // ADD case
            case 1:
            strcpy(opCodes[2], "ADD\0");
            break;
            // SUB case
            case 2:
            strcpy(opCodes[2], "SUB\0");
            break;
            // MUL case
            case 3:
            strcpy(opCodes[2], "MUL\0");
            break;
            // DIV case
            case 4:
            strcpy(opCodes[2], "DIV\0");
            break;
            // EQL case
            case 5:
            strcpy(opCodes[2], "EQL\0");
            break;
            // NEQ case
            case 6:
            strcpy(opCodes[2], "NEQ\0");
            break;
            // LSS case
            case 7:
            strcpy(opCodes[2], "LSS\0");
            break;
            // LEQ case
            case 8:
            strcpy(opCodes[2], "LEQ\0");
            break;
            // GTR case
            case 9:
            strcpy(opCodes[2], "GTR\0");
            break;
            // GEQ case
            case 10:
            strcpy(opCodes[2], "GEQ\0");
            // MOD case
            case 11:
            strcpy(opCodes[2], "MOD\0");
            break;
        }// end switch
    }//end if

    // formatted op code and registers print to the screen
    printf("%5s  %d   %d\t%d\t%d\t%d\t", opCodes[ir.op], ir.L, ir.M, pc, bp, sp);
    
    // logic to determine where "|"s should be placed by building a boolean array of 1s and 0s
    int ARs[ARRAY_SIZE] = {0};
    int currentBP = bp;
    while (currentBP != 0 && currentBP < 499) {
        ARs[currentBP] = 1;
        currentBP = pas[currentBP - 1];
    }// end while

    // Print stack contents
    for (int i = ARRAY_SIZE - 1; i >= sp; i--) {
        // Add separator between activation records
        if (i > sp && ARs[i]) {
            printf("| ");
        }// end if
        printf("%d ", pas[i]);
    }// end for loop
    printf("\n");
}// end print utility