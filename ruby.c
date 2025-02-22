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
    char type[20];  // For Ruby, this will be "function" for defs.
    char size[20];  // Not applicable for Ruby; will be left empty.
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
            if (c == '\n') { row++; col = 1; }
            continue;
        }
        // Comments in Ruby start with #
        if (c == '#') {
            while ((c = fgetc(fp)) != '\n' && c != EOF);
            row++; col = 1;
            continue;
        }
        // String literals
        if (c == '"' || c == '\'') {
            char quote = c;
            int i = 0;
            token.lexeme[i++] = c;
            while ((c = fgetc(fp)) != EOF && c != quote) {
                token.lexeme[i++] = c; col++;
            }
            if (c == quote) token.lexeme[i++] = c;
            token.lexeme[i] = '\0';
            strcpy(token.type, "string");
            return token;
        }
        // Numbers
        if (isdigit(c)) {
            int i = 0;
            token.lexeme[i++] = c;
            while (isdigit(c = fgetc(fp))) { token.lexeme[i++] = c; col++; }
            token.lexeme[i] = '\0';
            ungetc(c, fp);
            strcpy(token.type, "number");
            return token;
        }
        // Identifiers and keywords
        if (isalpha(c) || c == '_' || c == '@' || c == '$') {
            int i = 0;
            token.lexeme[i++] = c;
            while ((c = fgetc(fp)) != EOF && (isalnum(c) || c == '_' || c == '@' || c == '$')) {
                token.lexeme[i++] = c; col++;
            }
            token.lexeme[i] = '\0';
            ungetc(c, fp);
            // For Ruby, we only treat "def" as a keyword for function definitions.
            if (strcmp(token.lexeme, "def") == 0)
                strcpy(token.type, "keyword");
            else
                strcpy(token.type, "id");
            return token;
        }
        // Operators and punctuation
        if (strchr("+-*/=%;:,(){}[].<>!", c)) {
            int i = 0;
            token.lexeme[i++] = c;
            token.lexeme[i] = '\0';
            strcpy(token.type, "operator");
            return token;
        }
        // Unknown token
        token.lexeme[0] = c; token.lexeme[1] = '\0';
        strcpy(token.type, "unknown");
        return token;
    }
    
    strcpy(token.type, "EOF");
    strcpy(token.lexeme, "EOF");
    return token;
}

void generateSymbolTable(FILE *fp) {
    Token token;
    rewind(fp); row = 1; col = 1;
    
    while (1) {
        token = getNextToken(fp);
        if (strcmp(token.type, "EOF") == 0)
            break;
        // Look for function definitions: keyword "def" followed by an identifier.
        if (strcmp(token.type, "keyword") == 0 && strcmp(token.lexeme, "def") == 0) {
            Token nextToken = getNextToken(fp);
            if (strcmp(nextToken.type, "id") == 0) {
                addToSymbolTable(nextToken.lexeme, "function");
            }
        }

        if(strcmp(token.type,"id")==0)
        addToSymbolTable(token.lexeme, "variable");
    }
}

void printSymbolTable() {
    printf("Ruby Symbol Table:\n");
    printf("---------------------------------------------------\n");
    printf("Hash\tName\t\tType\t\tSize\n");
    printf("---------------------------------------------------\n");
    for (int i = 0; i < symbolTableIndex; i++) {
        printf("%d\t%-12s\t%-12s\t%s\n", 
               symbolTable[i].hash, symbolTable[i].name, symbolTable[i].type, symbolTable[i].size);
    }
}

int main() {
    FILE *input_fp = fopen("source.rb", "r");
    if (!input_fp) { printf("Cannot open source.rb\n"); return 1; }
    generateSymbolTable(input_fp);
    printSymbolTable();
    fclose(input_fp);
    return 0;
}
