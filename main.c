#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

/**
 *  MSTS is Max Symbol Table Size
 *  MPS is Max Program Size
 */

#define MSTS 100
#define MPS 500

typedef struct symbol
{
    int kind;       // const = 1, var = 2, proc = 3
    char name[13];  // name up to 12 chars
    int val;        // number
    int level;      // L level
    int addr;       // M address
} symbol;

typedef struct token
{
    int idNum;
    char ident[13];
    int value;
} token;

typedef struct command
{
    int op;
    int lex;
    int mod;
} command;

enum Token_Name
{
    nulsym = 1,
    identsym = 2,
    numbersym = 3,
    plussym = 4,
    minussym = 5,
    multsym = 6,
    slashsym = 7,
    oddsym = 8,
    eqlsym = 9,
    neqsym = 10,
    lessym = 11,
    leqsym = 12,
    gtrsym = 13,
    geqsym = 14,
    lparentsym = 15,
    rparentsym = 16,
    commasym = 17,
    semicolonsym = 18,
    periodsym = 19,
    becomessym = 20,
    beginsym = 21,
    endsym = 22,
    ifsym = 23,
    thensym = 24,
    whilesym = 25,
    dosym = 26,
    callsym = 27,
    constsym = 28,
    varsym = 29,
    procsym = 30,
    writesym = 31,
    readsym = 32,
    elsesym = 33
};
const char symbolName[34][13] = {"", "nulsym", "identsym", "numbersym", "plussym", "minussym", "multsym", "slashsym", "oddsym", "eqlsym", "neqsym", "lessym", "leqsym", "gtrsym", "geqsym", "lparentsym",
                                 "rparentsym", "commasym", "semicolonsym", "periodsym", "becomessym", "beginsym", "endsym", "ifsym", "thensym", "whilesym", "dosym", "callsym", "constsym", "varsym",
                                 "procsym", "writesym", "readsym", "elsesym"};

/**
 *  Used by the three Ident functions to keep track of what ident is where
 *
 */
symbol symbolTable[MSTS];

/**
 *  Used by the command barker to store the finished program before output
 *
 */
command outputProgram[MPS];

/**
 *  pos is the current position for the end of the symbol table
 *  frameSize determines where new variables will be stored in the stack as well as the size of the stack
 *  commandPos is the current position for the end of the program code
 *  tokenNum is the token number of the current token. Used to tell the user where there is a problem
 *  lexLev is the current lexicographical level we are in
 *  tok is the current token being parsed
 *  InFile and OutFile are the input and output files
 *      They are only opened in Main. They can be closed anywhere when we detect an error.
 */
int pos = 0, frameSize = 4, commandPos = 0, tokenNum = 0, lexLev = 0;
token tok;
FILE *inFile, *outFile;

/**
 *  Non-Terminal Symbols
 *  Used in Tiny PL0 Grammar
 */
void program();
void block();
void constDec();
void varDec();
void statement();
void condition();
void expression();
void term();
void factor();
/**
 *  New Non-Terminal Symbols
 *  Used in PL0 Grammar
 */
void procDec();

/**
 *  These provide functionality to our compiler
 *
 */
void consume(int last);             //Consumes the old token, and gets a new one. Will complain if it gets heartburn (unexpected token)
void bark(int op, int l, int m);    //Barks out command
void rebark(int addr, int m);       //Updates command with new modifier
void emitBark();                    //Outputs program to the file
void ident(int kind);               //Adds ident to symbol table
void getIdent(char * name);         //Finds memory address in symbol table and pushes value to top of stack
void storeIdent(char * name);       //Finds memory address in symbol table and stores top of stack there
/**
 *  New functions
 *
 */
void callIdent();                   //Finds the start of a function and jumps to it's code


int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Error: Not enough arguments.\n\"Compile <inputFile> <outputFile>\" is minimum required command line.\n Cannot continue.\n");
        return 0;
    }
    inFile = fopen(argv[1], "r");
    if (inFile == NULL)
    {
        printf("Error, File not found!\n");
        return 0;
    }
    tok.idNum = 1;
    consume(nulsym);

    program();

    if (inFile!=NULL)
        fclose(inFile);
    printf("No Errors, program syntactically correct.\n");

    outFile = fopen(argv[2], "w");
    emitBark();
    fclose(outFile);

    return 0;
}

void program()
{
    block();
    consume(periodsym);
    bark(9, 0, 2);
}

void block()
{
    int temp, tempframe;

        temp = commandPos;                                  //We need to jump past the child procedures to the body of the function
        bark(7, 0, 0);                                      //Placeholder jump command

    constDec();
    varDec();

        tempframe = frameSize;

    procDec();

        rebark(temp, commandPos);                           //Correct or previous barked jump
        bark(6, 0, tempframe);                              //Set up our stack frame with room for all of our variables

    statement();
    if (lexLev > 0)                                         //If we're not in the bottom lex level
        bark(2, 0, 0);                                      //We need to return from the procedure
}

void constDec()
{
    if (tok.idNum == constsym)
    {
        consume(constsym);
        ident(1);
        /* consume(eqlsym); // Ident will handle this
        number(); */
        while (tok.idNum == commasym)
        {
            consume(commasym);
            ident(1);
        }
        consume(semicolonsym);
    }
}

void varDec()
{
    if (tok.idNum == varsym)
    {
        consume(varsym);
        ident(2);
        while (tok.idNum == commasym)
        {
            consume(commasym);
            ident(2);
        }
        consume(semicolonsym);
    }
}

void procDec()
{
    int temp;
    lexLev++;                           //Everything in here is one lex level higher than outside
    while (tok.idNum == procsym)
    {
        consume(procsym);
        ident(3);
        temp = pos;                     //Store the position of the symbol table
        consume(semicolonsym);
        block();
        consume(semicolonsym);
        pos = temp;                     //Return the symbol table to where we stored it to "delete" the variables for the procedure
    }
    lexLev--;                           //Once we're done in here we need to drop back down to the previous lex level
}

void statement()
{
    char id[13];
    int save, save2;
    switch (tok.idNum)
    {
        case identsym : strcpy(id, tok.ident);  //<ident> := <expression> ** Store the value of the ident token for later
                        consume(identsym);
                        consume(becomessym);
                        expression();
                        storeIdent(id);         //Store the value at the top of the stack into the memory address for the identifier we started with.
                        break;
        case callsym  : consume(callsym);
                        callIdent();
                        break;
        case beginsym : consume(beginsym);      //begin <statement> {; <statement>} end
                        statement();
                        while (tok.idNum == semicolonsym)
                        {
                            consume(semicolonsym);
                            statement();
                        }
                        consume(endsym);
                        break;
        case ifsym    : consume(ifsym);         //if <condition> then <statement>
                        condition();
                        save = commandPos;      //Save the current command position so we can rebark it later
                        bark(8, 0, 0);          //Bark out a jump if condition resolved to 0
                        consume(thensym);
                        statement();
                        if (tok.idNum == elsesym)
                        {
                            consume(elsesym);
                            save2 = commandPos; //Save position of the jump to skip the else
                            bark(7, 0, 0);      //If we hit this jump, the above condition was true and we do not want to do the else code
                            rebark(save, commandPos);   //If the condition was false, we wish to jump here now
                            statement();        //Perform the else statement
                            rebark(save2, commandPos);  //If we jumped past the else, this is where we will end up.
                        }else
                        {
                            rebark(save, commandPos);   //Update the mod of our jump command to go to the next instruction after the body of the then.
                        }
                        break;
        case whilesym : consume(whilesym);      //while <condition> do <statement>
                        save2 = commandPos;
                        condition();
                        save = commandPos;      //Save the current command position so we can rebark it later
                        bark(8, 0, 0);          //Bark out a jump if condition resolved to 0
                        consume(dosym);
                        statement();
                        bark(7, 0, save2);
                        rebark(save, commandPos);   //Update the mod of our jump command to go to the next instruction after the body of the loop.
                        break;
        case readsym  : consume(readsym);       //read <ident>
                        bark(9, 0, 1);          //Bark out a read from user input command
                        storeIdent(tok.ident);  //Store the value read in to the ident token we were given.
                        consume(identsym);
                        break;
        case writesym : consume(writesym);      //write <ident>
                        getIdent(tok.ident);    //Retrieve the value of the ident token we were given
                        bark(9, 0, 0);          //Bark the command to write out the value on the top of the stack to the screen
                        consume(identsym);
                        break;
        default       : break;
    }
}

void condition()
{
    if (tok.idNum == oddsym)
    {
        consume(oddsym);
        expression();
        bark(2, 0, 6);                      //Bark out the oddsym
    } else
    {
        expression();
        int op = tok.idNum;
        switch(tok.idNum)
        {
            case eqlsym : consume(eqlsym);
                          break;
            case neqsym : consume(neqsym);
                          break;
            case lessym : consume(lessym);
                          break;
            case leqsym : consume(leqsym);
                          break;
            case gtrsym : consume(gtrsym);
                          break;
            case geqsym : consume(geqsym);
                          break;
            default     : consume(neqsym);  // If it's not one of these we need an error of some kind.
        }
        expression();
        switch(op)
        {
            case eqlsym : bark(2, 0, 8);
                          break;
            case neqsym : bark(2, 0, 9);
                          break;
            case lessym : bark(2, 0, 10);
                          break;
            case leqsym : bark(2, 0, 11);
                          break;
            case gtrsym : bark(2, 0, 12);
                          break;
            case geqsym : bark(2, 0, 13);
                          break;
        }
    }
}

void expression()
{
    int isNeg=0;
    if (tok.idNum == plussym)
    {
        consume(plussym);
    }
    else if (tok.idNum == minussym)
    {
        consume(minussym);
        isNeg = 1;
    }
    term();
    if (isNeg)
        bark (2, 0, 1);
    while (tok.idNum == plussym || tok.idNum == minussym)
    {
        isNeg=0;
        if (tok.idNum == plussym)
            consume(plussym);
        else
        {
            consume(minussym);
            isNeg=1;
        }
        term();
        if (isNeg)
            bark(2, 0, 3);
        else
            bark(2, 0, 2);
    }
}

void term()
{
    int isMult;
    factor();
    while (tok.idNum == multsym || tok.idNum == slashsym)
    {
        isMult=0;
        if (tok.idNum == multsym)
        {
            consume(multsym);
            isMult=1;
        }
        else
            consume(slashsym);
        factor();
        if (isMult)
            bark(2, 0, 4);
        else
            bark(2, 0, 5);
    }
}

void factor()
{
    if (tok.idNum == identsym)
    {
        getIdent(tok.ident);
        consume(identsym);
    }else if (tok.idNum == numbersym)
    {
        bark(1, 0, tok.value);
        consume(numbersym);
    } else
    {
        consume(lparentsym);
        expression();
        consume(rparentsym);
    }
}

void consume(int last)
{
    int *nToken = malloc(sizeof(int));
    char tName[13];
    int success;

    if (tok.idNum == last)
    {
        success = getNextToken(inFile, nToken, tName);
        if (success)
        {
            fclose(inFile);
            printf("Lexer failed to parse token #%d\n", tokenNum+1);
            exit(0);
        }
        tok.idNum = *nToken;
        if (tok.idNum == numbersym)
            tok.value = atoi(tName);
        strcpy(tok.ident, tName);
    } else
    {
        printf("Wrong token at token #%d\n", tokenNum);
        switch (last)
        {
        case nulsym:
            printf("Expected nulsym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case identsym:
            printf("Expected identsym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case numbersym:
            printf("Expected numbersym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case plussym:
            printf("Expected plussym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case minussym:
            printf("Expected minussym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case multsym:
            printf("Expected multsym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case slashsym:
            printf("Expected slashsym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case oddsym:
            printf("Expected oddsym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case eqlsym:
            printf("Expected eqlsym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case neqsym:
            printf("Expected neqsym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case lessym:
            printf("Expected lessym, but found %s instead.\n", symbolName[tok.idNum]);
            fclose(inFile);
            exit(0);
        case leqsym:
            printf("Expected leqsym, but found %s instead.\n", symbolName[tok.idNum]);
            fclose(inFile);
            exit(0);
        case gtrsym:
            printf("Expected gtrsym, but found %s instead.\n", symbolName[tok.idNum]);
            fclose(inFile);
            exit(0);
        case geqsym:
            printf("Expected geqsym, but found %s instead.\n", symbolName[tok.idNum]);
            fclose(inFile);
            exit(0);
        case lparentsym:
            printf("Expected lparentsym, but found %s instead.\n", symbolName[tok.idNum]);
            fclose(inFile);
            exit(0);
        case rparentsym:
            printf("Expected rparentsym, but found %s instead.\n", symbolName[tok.idNum]);
            fclose(inFile);
            exit(0);
        case commasym:
            printf("Expected commasym, but found %s instead.\n", symbolName[tok.idNum]);
            fclose(inFile);
            exit(0);
        case semicolonsym:
            printf("Expected semicolonsym, but found %s instead.\n", symbolName[tok.idNum]);
            fclose(inFile);
            exit(0);
        case periodsym:
            printf("Expected periodsym, but found %s instead.\n", symbolName[tok.idNum]);
            fclose(inFile);
            exit(0);
        case becomessym:
            printf("Expected becomessym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case beginsym:
            printf("Expected beginsym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case endsym:
            printf("Expected endsym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case ifsym:
            printf("Expected ifsym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case thensym:
            printf("Expected thensym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case whilesym:
            printf("Expected whilesym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case dosym:
            printf("Expected dosym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case callsym:
            printf("Expected callsym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case constsym:
            printf("Expected constsym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case varsym:
            printf("Expected varsym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case procsym:
            printf("Expected procsym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case writesym:
            printf("Expected writesym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case readsym:
            printf("Expected readsym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        case elsesym:
            printf("Expected elsesym, but found %s: %s instead.\n", symbolName[tok.idNum], tok.ident);
            fclose(inFile);
            exit(0);
        default:
            printf("WHAT? This shouldn't happen! Token expected was %s\n", symbolName[last]);
            fclose(inFile);
            exit(1);
        }
    }
    free(nToken);
    tokenNum++;
}

void bark(int op, int l, int m)
{
    outputProgram[commandPos].op = op;
    outputProgram[commandPos].lex = l;
    outputProgram[commandPos].mod = m;
    commandPos++;
}

void rebark(int addr, int m)
{
    outputProgram[addr].mod = m;
}

void emitBark()
{
    int i;
    for (i=0; i<commandPos; i++)
    {
        fprintf(outFile, "%d %d %d\n", outputProgram[i].op, outputProgram[i].lex, outputProgram[i].mod);
    }
}

void ident(int kind)
{
    int i;
    for (i=0; i<pos; i++)
    {
        if (symbolTable[i].kind == 3 && kind == 3)              //Can't have 2 accessible procedures with the same name
        {
            if (strcmp(tok.ident, symbolTable[i].name) == 0)
            {
                printf("Error, procedure with the name %s already exists.\n", tok.ident);
                printf("At lex level %d\n", lexLev);
                exit (0);
            }
        } else if (symbolTable[i].kind != 3 && kind != 3)       //if they are both non-procedures with the same name
        {
            if (strcmp(tok.ident, symbolTable[i].name) == 0 && symbolTable[i].level == lexLev)      //at the same lex level
            {
                printf("Error, duplicate identifier\n");    //Can't have two identifiers in the list at the same level with the same name
                exit(0);
            }
        }
    }


    if (pos == MSTS)
    {
        printf("Too many symbols in the symbol table\n");
        exit(0);
    }else if (kind == 1)                                //If our ident is a constant
    {
        symbolTable[pos].kind = 1;                      //Mark the ident as a constant
        strcpy(symbolTable[pos].name, tok.ident);       //Copy the name of the constant into the table
        consume(identsym);                              //Next symbol
        consume(eqlsym);                                //Next symbol
        symbolTable[pos].val = tok.value;               //Save the value of the constant into the table
        consume(numbersym);                             //Next symbol
        symbolTable[pos].level = lexLev;                //Set the lex level of our constant
    }else if (kind == 2)                                //If our ident is a variable
    {
        symbolTable[pos].kind = 2;                      //Mark it as a variable
        strcpy(symbolTable[pos].name, tok.ident);       //Copy the name into the table
        symbolTable[pos].level = lexLev;                //Set the proper lex level
        symbolTable[pos].addr = frameSize;              //Save the memory position of the variable into the table
        frameSize++;                                    //Increase the frame size
        consume(identsym);                              //Next symbol
    }else if (kind == 3)
    {
        symbolTable[pos].kind = 3;                      //Mark the identifier as a procedure
        strcpy(symbolTable[pos].name, tok.ident);       //Copy the name into the table
        symbolTable[pos].level = lexLev;                //Set the proper lex level
        symbolTable[pos].addr = commandPos;             //Set the address of the procedure here
        frameSize = 4;                                  //reset the framesize since we're going to have a new stack frame
        consume(identsym);                              //Next symbol
    }
    pos++;                                              //Next position in the symbol table
}

void getIdent(char * name)
{
    int i, loc = -1;
    for (i=0; i<pos; i++)                               //Search for the identifier in the table
    {
        if (strcmp(name, symbolTable[i].name) == 0 && symbolTable[i].kind != 3)
        {
            loc = i;
        }
    }
    if (loc == -1)
    {
        printf("Identifier not declared in symbol table\n");
        exit(0);
    }
    if (symbolTable[loc].kind == 1)                     //If it's a constant
    {
        bark(1, 0, symbolTable[loc].val);              //Put the value on the stack
    }else                                               //Otherwise it's a variable
    {
        bark(3, lexLev - symbolTable[loc].level, symbolTable[loc].addr);    //Load it's value from memory, and put it on the top of the stack
    }                                                   //The frame we need to go to will be our current lex level - the lex level of the symbol
}                                                       //example the variable belongs to main so it's level is 0, we are in a child of main, so our current is 1. 1-0 = 1 or one frame back

void storeIdent(char * name)
{
    int i, loc = -1;
    for (i=0; i<pos; i++)
    {
        if (strcmp(name, symbolTable[i].name) == 0 && symbolTable[i].kind != 3)
        {
            loc = i;
        }
    }
    if (loc == -1)
    {
        printf("Identifier not declared in symbol table\n");
        exit(0);
    }
    if (symbolTable[loc].kind == 1)                     //If it's a constant
    {
        printf("Cannot change the value of a constant\n");  //Can't change a constant
        exit(0);
    } else                                              //Otherwise it's a variable and we can store it
    {
        bark(4, lexLev - symbolTable[loc].level, symbolTable[loc].addr);
    }
}

void callIdent()
{
    int i, loc = -1;
    for (i=0; i<pos; i++)
    {
        if (strcmp(tok.ident, symbolTable[i].name) == 0 && symbolTable[i].kind == 3)
        {
            loc = i;
            break;
        }
    }
    if (loc == -1)
    {
        printf("Undeclared procedure\n");
        exit(0);
    }
    consume(identsym);
    bark(5, lexLev + 1 - symbolTable[loc].level, symbolTable[loc].addr);
}
