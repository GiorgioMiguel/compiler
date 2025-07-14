/*
 Title: Homework 4 Extended PL/0 Compiler
 Class: COP 3402-25 by Euripides Montagne
 Authors: Giorgio Miguel Torregrosa and Yasmeen Moufakkir
 Date: 7/13/2025
 */

// headers used
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

// definitions
#define RESERVEDLENGTH 11
#define MAXTOKENLENGTH 10000
#define MAXSYMBOLTABLESIZE 500

// enumeration (changed skipsym to oddsym)
typedef enum { 
    modsym = 1, identsym, numbersym, plussym, minussym, 
    multsym, slashsym, fisym, eqlsym, neqsym, lessym, leqsym, 
    gtrsym, geqsym, lparentsym, rparentsym, commasym, semicolonsym, 
    periodsym, becomessym, beginsym, endsym, ifsym, thensym, 
    whensym, dosym, callsym, constsym, varsym, procsym, writesym, readsym, elsesym
} token_type;

// global array variables
// reserved word array
char *reservedWords[RESERVEDLENGTH] = {"const", "var", "begin", "end", "if", "fi", "then", "when", "do", "read", "write", "procedure", "call"};
// parallel token array for the 11 reserved words
token_type wordToken[RESERVEDLENGTH] = {constsym, varsym, beginsym, endsym, ifsym, fisym, thensym, whensym, dosym, readsym, writesym};
// single-character symbol table
token_type symbolToken[256] = {['%'] = modsym, ['+'] = plussym, ['-'] = minussym, ['*'] = multsym, ['/'] = slashsym, ['('] = lparentsym, [')'] = rparentsym, ['='] = eqlsym, [','] = commasym, [';'] = semicolonsym, ['.'] = periodsym, ['<'] = lessym, ['>'] = gtrsym};

// struct for token data
typedef struct {
    token_type type;
    char lexeme[12];
    int value;
} token;

// struct for assembly code data
typedef struct {
    int op;
    int l;
    int m;
} instruction;

// struct for symbol table
typedef struct {
    int kind;      // const = 1, var = 2
    char name[12]; // name up to 11 chars plus a null terminator
    int value;     // number (ASCII value)
    int level;     // L level
    int address;   // M address
    int mark;      // to indicate unavailable or deleted
} symbol;

// global variables
// symbol table continued
symbol symbolTable[MAXSYMBOLTABLESIZE];
int tableIdx = 0;
// token array for token list
token tokenList[MAXTOKENLENGTH] = {0};
int tokenCount = 0;
// Parser tracking variables
instruction code[500];
int codeIdx = 0;
int tokenIdx = 0;
int numVars = 0;
FILE* elf;


// prototype functions
void parseExpression();
void parseStatement();

// start main
int main(int argc, char *argv[]) {

    // check if a file was given
    if (argc < 2) {
        printf("No input file was given. Please retype the execution command with a file name included.\n");
        return EXIT_FAILURE;
    } // end parameter check for file

    // file pointer
    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("Error opening file.\n");
        return EXIT_FAILURE;
    } // end file pointer assignment

    elf = fopen("elf.txt", "w");
    if(elf == NULL) {
        printf("Error creating output file for parser.\n");
        return EXIT_FAILURE;
    }// end outfile check

    // local variables
    // flag if an error was found, get reset every iteration of the scanner
    int flagError = 0;
    // flag if an error was found, does not get reset
    int flagErrorTokenList = 0;

    // begin scanner
    int c;
    while ((c = fgetc(fp)) != EOF) {

        // skip spaces
        if (isspace(c)) {
            continue;
        } // end if

        // identified a character
        if (isalpha(c)) {
            char lexeme[12] = {0};
            int length = 0;
            int nullTermAddress = 0;

            // keep reading in characters until we reach a symbol
            do {
                if (length < 11) {
                    lexeme[length] = c;
                    nullTermAddress++;
                } // end if

                length++;
                c = fgetc(fp);

            } while (isalpha(c) || isdigit(c));

            // null terminate the string
            lexeme[nullTermAddress] = '\0';

            // error name too long
            if (length > 11) {
                flagError = 1;
                flagErrorTokenList = 1;
            } // end name too long error

            // move the fp back one address
            ungetc(c, fp);

            // try to match the string to any reserved words, if one is found assign it to token
            token_type token = identsym;
            for (int i = 0; i < RESERVEDLENGTH; i++) {
                if (strcmp(lexeme, reservedWords[i]) == 0) {
                    token = wordToken[i];
                    break;
                } // end if
            } // end for

            // Printing a row in lexeme table
            if (flagError == 0) {
                strcpy(tokenList[tokenCount].lexeme, lexeme);
                tokenList[tokenCount].type = token;
                tokenList[tokenCount].value = 0;
                tokenCount++;
            } // end if
            else {
                printf("Error : identifier %s is longer than 11 characters.\n", lexeme);
                flagError = 0;
            } // end else
            continue;
        } // end if alpha

        // identified a number
        if (isdigit(c)) {
            char number[6] = {0};
            int length = 0;
            int nullTermAddress = 0;

            // keep reading in numbers
            do {

                if (length < 5) {
                    number[length] = c;
                    nullTermAddress++;
                }

                length++;
                c = fgetc(fp);

            } while (isdigit(c));

            // null terminate the string
            number[nullTermAddress] = '\0';

            // error number too long
            if (length > 5) {
                flagError = 1;
                flagErrorTokenList = 1;
            } // end if

            ungetc(c, fp);
            // convert the string into a value
            int value = atoi(number);

            // printing a row in lexeme table
            if (flagError == 0) {
                strcpy(tokenList[tokenCount].lexeme, number);
                tokenList[tokenCount].type = numbersym;
                tokenList[tokenCount].value = value;
                tokenCount++;
            } // end if
            else {
                printf("Error : %d number is too long.\n", value);
                flagError = 0;
            } // end else

            continue;
        } // end if number

        // identified a symbol
        token_type symToken = symbolToken[c];
        char lexemeSym[3] = {c, '\0', '\0'};
        switch (c) {

        // comment case
        case '/':
            if ((c = fgetc(fp)) == '*') {

                int continueScanning = 1;
                // consume all characters after comment intialization, end after we read in a closing "*/", if we never read a closing comment, end at EOF
                while (continueScanning) {
                    c = fgetc(fp);
                    if ((c == '*' && (c = fgetc(fp)) == '/') || c == EOF) {
                        continueScanning = 0;
                    } // end if
                } // end while
                continue;
            } // end if

            ungetc(c, fp);

            break;
        // ':=' case or ':' error case
        case ':':
            if ((c = fgetc(fp)) == '=') {
                lexemeSym[1] = '=';
                symToken = becomessym;
            } // end if
            else {
                flagError = 1;
                flagErrorTokenList = 1;
                ungetc(c, fp);
            } // end else
            break;
        // '<' or '<=' or '<>' case
        case '<':
            if ((c = fgetc(fp)) == '=') {
                lexemeSym[1] = '=';
                symToken = leqsym;
            } // end if
            else if (c == '>') {
                lexemeSym[1] = '>';
                symToken = neqsym;
            } // end else if
            else {
                symToken = lessym;
                ungetc(c, fp);
            } // end else
            break;
        // '>' or '>=' case
        case '>':
            if ((c = fgetc(fp)) == '=') {
                lexemeSym[1] = '=';
                symToken = geqsym;
            } // end if
            else {
                symToken = gtrsym;
                ungetc(c, fp);
            } // end else
            break;
        // invalid symbols
        default:
            if (symToken == 0) {
                flagError = 1;
                flagErrorTokenList = 1;
            } // end if
            break;
        } // end switch symbol case

        // Printing a row in lexeme table
        if (flagError == 0) {
            strcpy(tokenList[tokenCount].lexeme, lexemeSym);
            tokenList[tokenCount].type = symToken;
            tokenList[tokenCount].value = 0;
            tokenCount++;
        } // end if
        else {
            printf("Error : Invalid symbol %s found.\n", lexemeSym);
            flagError = 0;
        } // end else
    } // end scanner

    // close PL/0 file
    fclose(fp);

    // begin Parser
    // scanner detected an error, halt compilation
    if (flagErrorTokenList == 1) {
        return EXIT_FAILURE;
    } // end if

    // Initializing
    code[codeIdx].op = 7;
    code[codeIdx].l = 0;
    code[codeIdx].m = 0;
    codeIdx++;

    // parse constant declaration block
    if (tokenIdx < tokenCount && tokenList[tokenIdx].type == constsym) {
        // consume current token
        tokenIdx++;

        do {
            // identifier check
            if (tokenIdx >= tokenCount || tokenList[tokenIdx].type != identsym) {
                printf("Error : const, var and read keywords must be followed by an identifier.\n");
                return EXIT_FAILURE;
            } // end identifier check

            // record identifier
            char constName[12] = "";
            strcpy(constName, tokenList[tokenIdx].lexeme);
            tokenIdx++;

            // check for "="
            if (tokenIdx >= tokenCount || tokenList[tokenIdx].type != eqlsym) {
                printf("Error : constants must be assigned with \"=\".\n");
                return EXIT_FAILURE;
            } // end "=" check
            tokenIdx++;

            // check for number
            if (tokenIdx >= tokenCount || tokenList[tokenIdx].type != numbersym) {
                printf("Error : constants must be assigned an integer value.\n");
                return EXIT_FAILURE;
            } // end number check

            // checking for duplicates before adding to symbol table
            for (int i = 0; i < tableIdx; i++) {
                if (strcmp(symbolTable[i].name, constName) == 0 && symbolTable[i].mark == 0) {
                    printf("Error : symbol name has already been declared.\n");
                    return EXIT_FAILURE;
                } // end if
            } // end duplicate check

            // all checks passed, add to symbol table
            strcpy(symbolTable[tableIdx].name, constName);
            symbolTable[tableIdx].kind = 1;
            symbolTable[tableIdx].value = tokenList[tokenIdx].value;
            symbolTable[tableIdx].level = 0;
            symbolTable[tableIdx].address = 0;
            symbolTable[tableIdx].mark = 0;
            tableIdx++;
            tokenIdx++;

            // checking for more const declarations
            if (tokenIdx >= tokenCount || tokenList[tokenIdx].type != commasym) {
                break;
            } // end loop

            tokenIdx++;
        } while (1);

        // semicolon check
        if (tokenIdx >= tokenCount || tokenList[tokenIdx].type != semicolonsym) {
            printf("Error : constant and variable declarations must be followed by a semicolon.\n");
            return EXIT_FAILURE;
        } // end semicolon check

        tokenIdx++;
    } // end const block

    // parse variable declaration block
    if (tokenIdx < tokenCount && tokenList[tokenIdx].type == varsym) {
        tokenIdx++;

        do {
            // identifier check
            if (tokenIdx >= tokenCount || tokenList[tokenIdx].type != identsym) {
                printf("Error : const, var and read keywords must be followed by an identifier.\n");
                return EXIT_FAILURE;
            } // end identifier check

            // record identifier
            char varName[12] = "";
            strcpy(varName, tokenList[tokenIdx].lexeme);

            // check for duplicates
            for (int i = 0; i < tableIdx; i++) {
                if (strcmp(symbolTable[i].name, varName) == 0 && symbolTable[i].mark == 0)
                {
                    printf("Error : variable name has already been declared.\n");
                    return EXIT_FAILURE;
                } // end if
            } // end duplicate check

            // add to symbol table
            strcpy(symbolTable[tableIdx].name, varName);
            symbolTable[tableIdx].kind = 2;
            symbolTable[tableIdx].value = 0;
            symbolTable[tableIdx].level = 0;
            symbolTable[tableIdx].address = numVars + 3;
            symbolTable[tableIdx].mark = 0;
            tableIdx++;
            numVars++;
            tokenIdx++;

            // checking for more var declarations
            if (tokenIdx >= tokenCount || tokenList[tokenIdx].type != commasym) {
                break;
            } // end if

            tokenIdx++;
        } while (1);

        // semicolon check
        if (tokenIdx >= tokenCount || tokenList[tokenIdx].type != semicolonsym) {
            printf("Error : constant and variable declarations must be followed by a semicolon.\n");
            return EXIT_FAILURE;
        } // end semicolon check

        tokenIdx++;
    } // end var block

    // allocating space for variables
    code[codeIdx].op = 6;
    code[codeIdx].l = 0;
    code[codeIdx].m = numVars + 3; // DL, SL, RA plus any additonal variables
    codeIdx++;
    code[0].m = 13; 

    // statement / expression parsing
    parseStatement();

    if (tokenIdx >= tokenCount || tokenList[tokenIdx].type != periodsym) {
        printf("Error : program must end with a period.\n");
        return EXIT_FAILURE;
    } // end if

    // halting program
    code[codeIdx].op = 9;
    code[codeIdx].l = 0;
    code[codeIdx].m = 3;
    codeIdx++;

    // mark all variables as out of scope
    for (int i = 0; i < tableIdx; i++) {
        symbolTable[i].mark = 1;
    } // end for

    // print assembly code
    printf("No errors, program is syntactically correct.\n\n");
    printf("Assembly Code:\n\n");
    printf("Line    OP    L    M\n");
    for (int i = 0; i < codeIdx; i++) {
        printf("%-6d ", i);
        switch (code[i].op)
        {
        case 1:
            printf("LIT");
            fprintf(elf, "1");
            break;
        case 2:
            printf("OPR");
            fprintf(elf, "2");
            break;
        case 3:
            printf("LOD");
            fprintf(elf, "3");
            break;
        case 4:
            printf("STO");
            fprintf(elf, "4");
            break;
        case 5:
            printf("CAL");
            fprintf(elf, "5");
            break;
        case 6:
            printf("INC");
            fprintf(elf, "6");
            break;
        case 7:
            printf("JMP");
            fprintf(elf, "7");
            break;
        case 8:
            printf("JPC");
            fprintf(elf, "8");
            break;
        case 9:
            printf("SYS");
            fprintf(elf, "9");
            break;
        default:
            // undefined op code
            printf("JNK"); 
            break;
        } // end switch
        printf("    %d    %d\n", code[i].l, code[i].m);
        fprintf(elf, " %d %d\n", code[i].l, code[i].m);
    } // end for

    // print symbol table
    printf("\nSymbol Table:\n\n");
    printf("Kind | Name     | Value | Level | Address | Mark\n");
    printf("-------------------------------------------------\n");
    for (int i = 0; i < tableIdx; i++) {
        printf("   %d |        %s |     %d |     %d |       %d |     %d\n", symbolTable[i].kind, symbolTable[i].name, symbolTable[i].value, symbolTable[i].level, symbolTable[i].address, symbolTable[i].mark);
    } // end for

    // print newline (mainly for eustis) and close file pointer
    printf("\n");
    fprintf(elf, "\n");
    fclose(elf);

    return EXIT_SUCCESS;
} // end main

// parse expression helper function
// GRAMMAR RULES
// EXPRESSION: expression ::= term { ("+"|"-") term }
// First parse a term
// TERM: term ::= factor { ("*"|"/") factor }
// Parse first factor
// FACTOR: factor ::= ident | number | "(" expression ")"
void parseExpression() {
    
    if (tokenIdx >= tokenCount) {
        printf("Error : arithmetic equations must contain operands, parentheses, numbers or symbols.\n");
        exit(EXIT_FAILURE);
    }// end if

    // parse first factor
    if (tokenList[tokenIdx].type == numbersym) {
        code[codeIdx].op = 1;
        code[codeIdx].l = 0;
        code[codeIdx].m = tokenList[tokenIdx].value;
        codeIdx++;
        tokenIdx++;
    }// end if
    else if (tokenList[tokenIdx].type == identsym) {
        int factorSymIdx = -1;
        for (int i = 0; i < tableIdx; i++) {
            if (strcmp(symbolTable[i].name, tokenList[tokenIdx].lexeme) == 0 && symbolTable[i].mark == 0) {
                factorSymIdx = i;
                break;
            }// end if
        }// end if

        if (factorSymIdx == -1) {
            printf("Error : undeclared identifier %s.\n", tokenList[tokenIdx].lexeme);
            exit(EXIT_FAILURE);
        }// end if

        if (symbolTable[factorSymIdx].kind == 1) {
            code[codeIdx].op = 1;
            code[codeIdx].l = 0;
            code[codeIdx].m = symbolTable[factorSymIdx].value;
            codeIdx++;
        }// end if
        else {                         
            code[codeIdx].op = 3; 
            code[codeIdx].l = 0;
            code[codeIdx].m = symbolTable[factorSymIdx].address;
            codeIdx++;
        }// end else
        tokenIdx++;
    }// end identsym
    else if (tokenList[tokenIdx].type == lparentsym) {
        tokenIdx++;        
        parseExpression();

        // parantheses check
        if (tokenIdx >= tokenCount || tokenList[tokenIdx].type != rparentsym) {
            printf("Error : right parenthesis must follow left parenthesis.\n");
            exit(EXIT_FAILURE);
        }// end if
        tokenIdx++;
    }// end else if
    else {
        printf("Error : arithmetic equations must contain operands, parentheses, numbers or symbols.\n");
        exit(EXIT_FAILURE);
    }// end else

    // check for invalid expression continuation
    if (tokenIdx < tokenCount && (tokenList[tokenIdx].type == numbersym || tokenList[tokenIdx].type == identsym || tokenList[tokenIdx].type == lparentsym)) {
        printf("Error : arithmetic equations must contain operands, parentheses, numbers or symbols.\n");
        exit(EXIT_FAILURE);
    }// end if

    // Now handle multiplication and division in the current term
    while (tokenIdx < tokenCount && (tokenList[tokenIdx].type == multsym || tokenList[tokenIdx].type == slashsym)) {
        token_type mulOp = tokenList[tokenIdx].type;
        tokenIdx++;

        // Parse next factor
        if (tokenIdx >= tokenCount) {
            printf("Error : arithmetic equations must contain operands, parentheses, numbers or symbols.\n");
            exit(EXIT_FAILURE);
        }// end if

        if (tokenList[tokenIdx].type == numbersym) {
            code[codeIdx].op = 1;
            code[codeIdx].l = 0;
            code[codeIdx].m = tokenList[tokenIdx].value;
            codeIdx++;
            tokenIdx++;
        }// end if
        else if (tokenList[tokenIdx].type == identsym) {
            int factorSymIdx = -1;
            for (int i = 0; i < tableIdx; i++) {
                if (strcmp(symbolTable[i].name, tokenList[tokenIdx].lexeme) == 0 && symbolTable[i].mark == 0) {
                    factorSymIdx = i;
                    break;
                }// end if
            }// end for

            if (factorSymIdx == -1) {
                printf("Error : undeclared identifier %s.\n", tokenList[tokenIdx].lexeme);
                exit(EXIT_FAILURE);
            }// end if

            if (symbolTable[factorSymIdx].kind == 1) {         
                code[codeIdx].op = 1; 
                code[codeIdx].l = 0;
                code[codeIdx].m = symbolTable[factorSymIdx].value;
                codeIdx++;
            }// end if
            else {  
                code[codeIdx].op = 3;
                code[codeIdx].l = 0;
                code[codeIdx].m = symbolTable[factorSymIdx].address;
                codeIdx++;
            }// end else
            tokenIdx++;
        }// end else if
        else if (tokenList[tokenIdx].type == lparentsym) {
            tokenIdx++;        
            parseExpression(); 

            // checking for closing parenthesis
            if (tokenIdx >= tokenCount || tokenList[tokenIdx].type != rparentsym) {
                printf("Error : right parenthesis must follow left parenthesis.\n");
                exit(EXIT_FAILURE);
            }// end if
            tokenIdx++;
        }// end else if
        else {
            printf("Error : arithmetic equations must contain operands, parentheses, numbers or symbols.\n");
            exit(EXIT_FAILURE);
        }// end else

        // check for invalid expression continuation
        if (tokenIdx < tokenCount && (tokenList[tokenIdx].type == numbersym || tokenList[tokenIdx].type == identsym || tokenList[tokenIdx].type == lparentsym)) {
            printf("Error : arithmetic equations must contain operands, parentheses, numbers or symbols.\n");
            exit(EXIT_FAILURE);
        }// end if

        // generate multiply or divide instruction
        code[codeIdx].op = 2; 
        code[codeIdx].l = 0;
        code[codeIdx].m = (mulOp == multsym) ? 3 : 4; 
        codeIdx++;
    }// end while

    // parse addition and subtraction at the expression level
    while (tokenIdx < tokenCount && (tokenList[tokenIdx].type == plussym || tokenList[tokenIdx].type == minussym)) {
        token_type addOp = tokenList[tokenIdx].type;
        tokenIdx++;

        // parse the next term
        if (tokenIdx >= tokenCount) {
            printf("Error : arithmetic equations must contain operands, parentheses, numbers or symbols.\n");
            exit(EXIT_FAILURE);
        }// end if

        // parse first factor of new term
        if (tokenList[tokenIdx].type == numbersym) {
            code[codeIdx].op = 1; // LIT
            code[codeIdx].l = 0;
            code[codeIdx].m = tokenList[tokenIdx].value;
            codeIdx++;
            tokenIdx++;
        }// end if
        else if (tokenList[tokenIdx].type == identsym) {
            int factorSymIdx = -1;
            for (int i = 0; i < tableIdx; i++) {
                if (strcmp(symbolTable[i].name, tokenList[tokenIdx].lexeme) == 0 && symbolTable[i].mark == 0) {
                    factorSymIdx = i;
                    break;
                }// end if
            }// end for

            if (factorSymIdx == -1) {
                printf("Error : undeclared identifier %s.\n", tokenList[tokenIdx].lexeme);
                exit(EXIT_FAILURE);
            }// end if

            if (symbolTable[factorSymIdx].kind == 1) {
                code[codeIdx].op = 1; 
                code[codeIdx].l = 0;
                code[codeIdx].m = symbolTable[factorSymIdx].value;
                codeIdx++;
            }// end if
            else {
                code[codeIdx].op = 3; 
                code[codeIdx].l = 0;
                code[codeIdx].m = symbolTable[factorSymIdx].address;
                codeIdx++;
            }// end else
            tokenIdx++;
        }// end else if
        else if (tokenList[tokenIdx].type == lparentsym) {
            tokenIdx++;
            // recursively parse the expression inside        
            parseExpression(); 

            // check for closing parenthesis
            if (tokenIdx >= tokenCount || tokenList[tokenIdx].type != rparentsym) {
                printf("Error : right parenthesis must follow left parenthesis.\n");
                exit(EXIT_FAILURE);
            }// end if
            tokenIdx++; // consume ')'
        }// end else if
        else {
            printf("Error : arithmetic equations must contain operands, parentheses, numbers or symbols.\n");
            exit(EXIT_FAILURE);
        }// end else

        // check for invalid expression continuation
        if (tokenIdx < tokenCount && (tokenList[tokenIdx].type == numbersym || tokenList[tokenIdx].type == identsym || tokenList[tokenIdx].type == lparentsym)) {
            printf("Error : arithmetic equations must contain operands, parentheses, numbers or symbols.\n");
            exit(EXIT_FAILURE);
        }// end if

        // parse any multiplication/division in this new term
        while (tokenIdx < tokenCount && (tokenList[tokenIdx].type == multsym || tokenList[tokenIdx].type == slashsym)) {
            token_type mulOp = tokenList[tokenIdx].type;
            tokenIdx++;

            if (tokenIdx >= tokenCount) {
                printf("Error : arithmetic equations must contain operands, parentheses, numbers or symbols.\n");
                exit(EXIT_FAILURE);
            }// end if

            if (tokenList[tokenIdx].type == numbersym) {
                code[codeIdx].op = 1; // LIT
                code[codeIdx].l = 0;
                code[codeIdx].m = tokenList[tokenIdx].value;
                codeIdx++;
                tokenIdx++;
            }// end if
            else if (tokenList[tokenIdx].type == identsym) {
                int factorSymIdx = -1;
                for (int i = 0; i < tableIdx; i++) {
                    if (strcmp(symbolTable[i].name, tokenList[tokenIdx].lexeme) == 0 && symbolTable[i].mark == 0) {
                        factorSymIdx = i;
                        break;
                    }// end if
                }// end for

                if (factorSymIdx == -1) {
                    printf("Error : undeclared identifier %s.\n", tokenList[tokenIdx].lexeme);
                    exit(EXIT_FAILURE);
                }// end

                if (symbolTable[factorSymIdx].kind == 1) {
                    code[codeIdx].op = 1; 
                    code[codeIdx].l = 0;
                    code[codeIdx].m = symbolTable[factorSymIdx].value;
                    codeIdx++;
                }// end if
                else {      
                    code[codeIdx].op = 3; 
                    code[codeIdx].l = 0;
                    code[codeIdx].m = symbolTable[factorSymIdx].address;
                    codeIdx++;
                }// end else
                tokenIdx++;
            }// end identsym
            else if (tokenList[tokenIdx].type == lparentsym) {
                tokenIdx++;        
                parseExpression(); 

                // check for closing parenthesis
                if (tokenIdx >= tokenCount || tokenList[tokenIdx].type != rparentsym) {
                    printf("Error : right parenthesis must follow left parenthesis.\n");
                    exit(EXIT_FAILURE);
                }// end if
                tokenIdx++; // consume ')'
            }// end else if
            else {
                printf("Error : arithmetic equations must contain operands, parentheses, numbers or symbols.\n");
                exit(EXIT_FAILURE);
            }// end else

            // check for invalid expression continuation
            if (tokenIdx < tokenCount && (tokenList[tokenIdx].type == numbersym || tokenList[tokenIdx].type == identsym || tokenList[tokenIdx].type == lparentsym)) {
                printf("Error : arithmetic equations must contain operands, parentheses, numbers or symbols.\n");
                exit(EXIT_FAILURE);
            }// end if

            // generate multiply or divide instruction
            code[codeIdx].op = 2; // OPR
            code[codeIdx].l = 0;
            code[codeIdx].m = (mulOp == multsym) ? 3 : 4;
            codeIdx++;
        }// end while

        // generate add or subtract instruction
        code[codeIdx].op = 2; // OPR
        code[codeIdx].l = 0;
        code[codeIdx].m = (addOp == plussym) ? 1 : 2;
        codeIdx++;
    }// end while
}// end parseExpression

// main logic 
void parseStatement() {
    // empty statement
    if (tokenIdx >= tokenCount) {
        return;
    }// end if

    // assignment statement
    if (tokenList[tokenIdx].type == identsym) {
        char identName[12] = "";
        strcpy(identName, tokenList[tokenIdx].lexeme);

        // lookup in symbol table
        int symIdx = -1;
        for (int i = 0; i < tableIdx; i++) {
            if (strcmp(symbolTable[i].name, identName) == 0 && symbolTable[i].mark == 0) {
                symIdx = i;
                break;
            }// end if
        }// end for

        if (symIdx == -1) {
            printf("Error : undeclared identifier %s.\n", identName);
            exit(EXIT_FAILURE);
        }// end if

        if (symbolTable[symIdx].kind != 2) {
            printf("Error : only variable values may be altered.\n");
            exit(EXIT_FAILURE);
        }// end if

        tokenIdx++;

        if (tokenIdx >= tokenCount || tokenList[tokenIdx].type != becomessym) {
            printf("Error : assignment statements must use \":=\".\n");
            exit(EXIT_FAILURE);
        }// end if

        tokenIdx++;
        parseExpression();

        // check if there's an unexpected expression component
        if (tokenIdx < tokenCount && (tokenList[tokenIdx].type == numbersym || tokenList[tokenIdx].type == identsym || tokenList[tokenIdx].type == lparentsym)) {
        printf("Error : arithmetic equations must contain operands, parentheses, numbers or symbols.\n");
        exit(EXIT_FAILURE);
        }// end if

        // store assembly
        code[codeIdx].op = 4; 
        code[codeIdx].l = 0;
        code[codeIdx].m = symbolTable[symIdx].address;
        codeIdx++;
    }// end if
    // begin/end block
    else if (tokenList[tokenIdx].type == beginsym) {
        tokenIdx++;

        while (1) {
            // recursively parse any statement
            parseStatement(); 

            if (tokenIdx >= tokenCount) {
                printf("Error : begin must be followed by end.\n");
                exit(EXIT_FAILURE);
            }// end if

            if (tokenList[tokenIdx].type == endsym) {
                tokenIdx++;
                break;
            }// end if

            if (tokenList[tokenIdx].type != semicolonsym) {
                printf("Error : begin must be followed by end.\n");
                exit(EXIT_FAILURE);
            }// end if

            tokenIdx++;
        }// end while
    }// end else if
    // ifsym statement
    else if (tokenList[tokenIdx].type == ifsym) {
        tokenIdx++; // consume 'if'

        // parse oddsym expression
        if (tokenIdx < tokenCount && tokenList[tokenIdx].type == modsym) {
            tokenIdx++;
            parseExpression();
            code[codeIdx].op = 2; 
            code[codeIdx].l = 0;
            code[codeIdx].m = 11; 
            codeIdx++;
        }// end if
        else {
            parseExpression();

            // operator check
            if (tokenIdx >= tokenCount || (tokenList[tokenIdx].type != eqlsym && tokenList[tokenIdx].type != neqsym && tokenList[tokenIdx].type != lessym && tokenList[tokenIdx].type != leqsym && tokenList[tokenIdx].type != gtrsym && tokenList[tokenIdx].type != geqsym)) {
                printf("Error : condition must contain comparison operator.\n");
                exit(EXIT_FAILURE);
            }// end operator check

            token_type relOp = tokenList[tokenIdx].type;
            tokenIdx++;
            parseExpression();

            // store operator
            code[codeIdx].op = 2; 
            code[codeIdx].l = 0;
            if (relOp == eqlsym) {
                code[codeIdx].m = 5; 
            }// end eqsym
            else if (relOp == neqsym) {
                code[codeIdx].m = 6; 
            }// end neqsym
            else if (relOp == lessym) {
                code[codeIdx].m = 7; 
            }// end lessym
            else if (relOp == leqsym) {
                code[codeIdx].m = 8; 
            }// end leqsym
            else if (relOp == gtrsym) {
                code[codeIdx].m = 9; 
            }// end gtrsym
            else if (relOp == geqsym) {
                code[codeIdx].m = 10;
            }// end geqsym
            codeIdx++;
        }// end else

        if (tokenIdx >= tokenCount || tokenList[tokenIdx].type != thensym) {
            printf("Error : if must be followed by then.\n");
            exit(EXIT_FAILURE);
        }// end if
        tokenIdx++; 

        // store assembly
        int jpcIdx = codeIdx;
        code[codeIdx].op = 8;
        code[codeIdx].l = 0;
        code[codeIdx].m = 0;
        codeIdx++;

        // parse any statement
        parseStatement(); 

        code[jpcIdx].m = codeIdx;

        if (tokenIdx >= tokenCount || tokenList[tokenIdx].type != fisym) {
            printf("Error : if must be followed by fi.\n");
            exit(EXIT_FAILURE);
        }// end if
        tokenIdx++; // consume 'fi'
    }// end else if
    // when statement
    else if (tokenList[tokenIdx].type == whensym) {
        tokenIdx++; // consume 'when'

        int loopIdx = codeIdx;

        // Check for "mod" expression
        if (tokenIdx < tokenCount && tokenList[tokenIdx].type == modsym) {
            tokenIdx++;
            parseExpression();

            // store assembly
            code[codeIdx].op = 2; // OPR
            code[codeIdx].l = 0;
            code[codeIdx].m = 11; // ODD operation
            codeIdx++;
        }// end if
        else {
            parseExpression();

            // op check
            if (tokenIdx >= tokenCount || (tokenList[tokenIdx].type != eqlsym && tokenList[tokenIdx].type != neqsym && tokenList[tokenIdx].type != lessym && tokenList[tokenIdx].type != leqsym && tokenList[tokenIdx].type != gtrsym && tokenList[tokenIdx].type != geqsym)) {
                printf("Error : condition must contain comparison operator.\n");
                exit(EXIT_FAILURE);
            }// end op check

            token_type relOp = tokenList[tokenIdx].type;
            tokenIdx++;
            parseExpression();

            code[codeIdx].op = 2; 
            code[codeIdx].l = 0;
            if (relOp == eqlsym) {
                code[codeIdx].m = 5; 
            }// end eqsym
            else if (relOp == neqsym) {
                code[codeIdx].m = 6; 
            }// end neqsym
            else if (relOp == lessym) {
                code[codeIdx].m = 7; 
            }// end lessym
            else if (relOp == leqsym) {
                code[codeIdx].m = 8; 
            }// end leqsym
            else if (relOp == gtrsym) {
                code[codeIdx].m = 9; 
            }// end gtrsym
            else if (relOp == geqsym) {
                code[codeIdx].m = 10;
            }// end geqsym
            codeIdx++;
        }// end else

        if (tokenIdx >= tokenCount || tokenList[tokenIdx].type != dosym) {
            printf("Error : when must be followed by do.\n");
            exit(EXIT_FAILURE);
        }// end if
        tokenIdx++; 

        // store assembly
        int jpcIdx = codeIdx;
        code[codeIdx].op = 8;
        code[codeIdx].l = 0;
        code[codeIdx].m = 0;
        codeIdx++;

        // parse any statement
        parseStatement(); 

        // store assembly
        code[codeIdx].op = 7;
        code[codeIdx].l = 0;
        code[codeIdx].m = loopIdx;
        codeIdx++;

        code[jpcIdx].m = codeIdx;
    }// end whensym
    // read statement
    else if (tokenList[tokenIdx].type == readsym) {
        tokenIdx++;

        if (tokenIdx >= tokenCount || tokenList[tokenIdx].type != identsym) {
            printf("Error : const, var and read keywords must be followed by an identifier.\n");
            exit(EXIT_FAILURE);
        }// end if

        char identName[12] = "";
        strcpy(identName, tokenList[tokenIdx].lexeme);
        int symIdx = -1;
        for (int i = 0; i < tableIdx; i++) {
            if (strcmp(symbolTable[i].name, identName) == 0 && symbolTable[i].mark == 0) {
                symIdx = i;
                break;
            }// end if
        }// end for

        if (symIdx == -1) {
            printf("Error : undeclared identifier %s.\n", identName);
            exit(EXIT_FAILURE);
        }// end if

        if (symbolTable[symIdx].kind != 2) {
            printf("Error : only variable values may be altered.\n");
            exit(EXIT_FAILURE);
        }// end if
        tokenIdx++;

        // store assembly 
        code[codeIdx].op = 9; 
        code[codeIdx].l = 0;
        code[codeIdx].m = 2; 
        codeIdx++;

        code[codeIdx].op = 4; 
        code[codeIdx].l = 0;
        code[codeIdx].m = symbolTable[symIdx].address;
        codeIdx++;
    }// end readsym
    // write statement
    else if (tokenList[tokenIdx].type == writesym) {
        tokenIdx++;
        parseExpression();

        // store assembly
        code[codeIdx].op = 9; 
        code[codeIdx].l = 0;
        code[codeIdx].m = 1; 
        codeIdx++;
    }// end else if
}// end parseStatement