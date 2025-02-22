#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_SYMBOL_TABLE_SIZE 100
#define MAX_LEXEME_LENGTH 50

typedef struct {
    int row, col;
    char type[20];
    char lexeme[50];
} Token;

typedef struct {
    int hash;
    char name[MAX_LEXEME_LENGTH];
    char type[20];  // For Java, e.g., "int", "function", etc.
    char size[20];  // Size for primitives; functions left empty.
} SymbolTableEntry;

int row = 1, col = 1;
SymbolTableEntry symbolTable[MAX_SYMBOL_TABLE_SIZE];
int symbolTableIndex = 0;

int calculateHash(const char* str) {
    int hash = 0;
    while (*str) {
        hash = hash * 31 + *str++;
    }
    return abs(hash % MAX_SYMBOL_TABLE_SIZE);
}

void addToSymbolTable(const char* name, const char* declType) {
    for (int i = 0; i < symbolTableIndex; i++) {
        if (strcmp(symbolTable[i].name, name) == 0)
            return;
    }
    if (symbolTableIndex < MAX_SYMBOL_TABLE_SIZE) {
        SymbolTableEntry entry;
        strcpy(entry.name, name);
        strcpy(entry.type, declType);
        // For primitives, we could assign sizes; here we'll leave it blank.
        strcpy(entry.size, "");
        entry.hash = calculateHash(name);
        symbolTable[symbolTableIndex++] = entry;
    }
}

Token getNextToken(FILE *fp) {
    Token token;
    token.row = row;
    token.col = col;
    char c;
    
    while ((c = fgetc(fp)) != EOF) {
        col++;
        if (isspace(c)) {
            if (c=='\n') { row++; col = 1; }
            continue;
        }
        // Single-line comments (//)
        if (c=='/' && (c=fgetc(fp))=='/') {
            while ((c=fgetc(fp))!='\n' && c!=EOF);
            row++; col = 1;
            continue;
        }
        // String literal handling
        if (c=='"' || c=='\'') {
            char quote = c;
            int i=0;
            token.lexeme[i++]=c;
            while ((c=fgetc(fp))!=EOF && c!=quote) { token.lexeme[i++]=c; col++; }
            if(c==quote) token.lexeme[i++]=c;
            token.lexeme[i]='\0';
            strcpy(token.type, "string");
            return token;
        }
        // Numbers
        if (isdigit(c)) {
            int i=0;
            token.lexeme[i++]=c;
            while(isdigit(c=fgetc(fp))) { token.lexeme[i++]=c; col++; }
            token.lexeme[i]='\0';
            ungetc(c, fp);
            strcpy(token.type, "number");
            return token;
        }
        // Identifiers and keywords (Java identifiers can start with letter or underscore)
        if (isalpha(c) || c=='_') {
            int i=0;
            token.lexeme[i++]=c;
            while ((c=fgetc(fp))!=EOF && (isalnum(c) || c=='_')) { token.lexeme[i++]=c; col++; }
            token.lexeme[i]='\0';
            ungetc(c, fp);
            
            // List of Java keywords (a subset)
            const char *keywords[] = {
                "abstract", "assert", "boolean", "break", "byte", "case", "catch",
                "char", "class", "const", "continue", "default", "do", "double",
                "else", "enum", "extends", "final", "finally", "float", "for",
                "if", "implements", "import", "instanceof", "int", "interface",
                "long", "native", "new", "package", "private", "protected",
                "public", "return", "short", "static", "strictfp", "super", "switch",
                "synchronized", "this", "throw", "throws", "transient", "try",
                "void", "volatile", "while"
            };
            int numKeywords = sizeof(keywords)/sizeof(keywords[0]);
            int isKeyword = 0;
            for (int j=0; j<numKeywords; j++) {
                if(strcmp(token.lexeme, keywords[j])==0) { isKeyword=1; break; }
            }
            if(isKeyword)
                strcpy(token.type, "keyword");
            else
                strcpy(token.type, "id");
            return token;
        }
        // Operators and punctuation
        if(strchr("+-*/=%;:,(){}[].<>!", c)!=NULL) {
            int i=0;
            token.lexeme[i++]=c;
            if((c=='='|| c=='!' || c=='<' || c=='>')) {
                char next=fgetc(fp);
                if(next=='=') { token.lexeme[i++]=next; col++; }
                else { ungetc(next, fp); }
            }
            token.lexeme[i]='\0';
            strcpy(token.type, "operator");
            return token;
        }
        token.lexeme[0]=c; token.lexeme[1]='\0';
        strcpy(token.type, "unknown");
        return token;
    }
    strcpy(token.type, "EOF");
    strcpy(token.lexeme, "EOF");
    return token;
}

void generateSymbolTable(FILE *fp) {
    Token token;
    rewind(fp); row=1; col=1;
    while(1) {
        token = getNextToken(fp);
        if(strcmp(token.type,"EOF")==0) break;
        
        // Variable declarations: check for keywords "int", "float", etc. or "var", "let", "const"
        if(strcmp(token.type,"keyword")==0 &&
           (strcmp(token.lexeme, "int")==0 || strcmp(token.lexeme, "float")==0 ||
            strcmp(token.lexeme, "double")==0 || strcmp(token.lexeme, "char")==0 ||
            strcmp(token.lexeme, "boolean")==0 || strcmp(token.lexeme, "var")==0 ||
            strcmp(token.lexeme, "let")==0 || strcmp(token.lexeme, "const")==0) || strcmp(token.lexeme, "String")==0) {
            char declType[20];
            strcpy(declType, token.lexeme);
            // Next token should be an identifier.
            Token nextToken = getNextToken(fp);
            if(strcmp(nextToken.type,"id")==0) {
                addToSymbolTable(nextToken.lexeme, declType);
            }
            continue;
        }
        
        if(strcmp(token.type,"keyword")==0 &&
           (strcmp(token.lexeme,"void")==0 || strcmp(token.lexeme,"int")==0 ||
            strcmp(token.lexeme,"string")==0 || strcmp(token.lexeme,"bool")==0 ||
            strcmp(token.lexeme,"float")==0 || strcmp(token.lexeme,"double")==0 || strcmp(token.lexeme,"char")==0)) {
            char retType[20]; strcpy(retType, token.lexeme);
            Token nextToken = getNextToken(fp);
            if(strcmp(nextToken.type,"id")==0) {
                // Look ahead for '('
                char nextChar = fgetc(fp);
                while(isspace(nextChar)) { 
                    if(nextChar=='\n'){ row++; col=1; } 
                    nextChar = fgetc(fp);
                }
                ungetc(nextChar, fp);
                if(nextChar=='(')
                    addToSymbolTable(nextToken.lexeme, "function");
                else
                    addToSymbolTable(nextToken.lexeme, retType);
            }
            continue;
        }
    }
}

void printSymbolTable() {
    printf("Java Symbol Table:\n");
    printf("---------------------------------------------------\n");
    printf("Hash\tName\t\tType\t\tSize\n");
    printf("---------------------------------------------------\n");
    for(int i=0; i<symbolTableIndex; i++){
        printf("%d\t%-12s\t%-12s\t%s\n", 
               symbolTable[i].hash, symbolTable[i].name, symbolTable[i].type, symbolTable[i].size);
    }
}

int main() {
    FILE *input_fp = fopen("source.java", "r");
    if(!input_fp){ printf("Cannot open source.java\n"); return 1; }
    generateSymbolTable(input_fp);
    printSymbolTable();
    fclose(input_fp);
    return 0;
}
