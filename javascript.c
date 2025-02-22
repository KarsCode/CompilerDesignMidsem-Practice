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
    char type[20];  // For variables, this will be "var", "let", "const" or "function"
    char size[20];  // For JS variables, size is not applicable so we leave it blank
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
    // Avoid duplicate entries.
    for (int i = 0; i < symbolTableIndex; i++) {
        if (strcmp(symbolTable[i].name, name) == 0) {
            return;
        }
    }
    if (symbolTableIndex < MAX_SYMBOL_TABLE_SIZE) {
        SymbolTableEntry entry;
        strcpy(entry.name, name);
        strcpy(entry.type, declType);
        strcpy(entry.size, "");  // For JavaScript, size is not applicable.
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

        // Skip whitespace; update row and column.
        if (isspace(c)) {
            if (c == '\n') {
                row++;
                col = 1;
            }
            continue;
        }

        // Single-line comment (//)
        if (c == '/' && (c = fgetc(fp)) == '/') {
            while ((c = fgetc(fp)) != '\n' && c != EOF);
            row++;
            col = 1;
            continue;
        }

        // String literal handling (double or single quotes)
        if (c == '"' || c == '\'') {
            char quote = c;
            int i = 0;
            token.lexeme[i++] = c;
            while ((c = fgetc(fp)) != EOF && c != quote) {
                token.lexeme[i++] = c;
                col++;
            }
            if (c == quote)
                token.lexeme[i++] = c;
            token.lexeme[i] = '\0';
            strcpy(token.type, "string");
            return token;
        }

        // Numeric literal handling (only integers for simplicity)
        if (isdigit(c)) {
            int i = 0;
            token.lexeme[i++] = c;
            while (isdigit(c = fgetc(fp))) {
                token.lexeme[i++] = c;
                col++;
            }
            token.lexeme[i] = '\0';
            ungetc(c, fp);
            strcpy(token.type, "number");
            return token;
        }

        // Identifier and keyword handling.
        if (isalpha(c) || c == '_' || c == '$') {
            int i = 0;
            token.lexeme[i++] = c;
            while ((c = fgetc(fp)) != EOF && (isalnum(c) || c == '_' || c == '$')) {
                token.lexeme[i++] = c;
                col++;
            }
            token.lexeme[i] = '\0';
            ungetc(c, fp);
            
            // List of JavaScript keywords.
            const char *keywords[] = {
                "break", "case", "catch", "class", "const", "continue", "debugger",
                "default", "delete", "do", "else", "export", "extends", "finally",
                "for", "function", "if", "import", "in", "instanceof", "new", "return",
                "super", "switch", "this", "throw", "try", "typeof", "var", "void",
                "while", "with", "yield", "let"
            };
            int numKeywords = sizeof(keywords) / sizeof(keywords[0]);
            int isKeyword = 0;
            for (int j = 0; j < numKeywords; j++) {
                if (strcmp(token.lexeme, keywords[j]) == 0) {
                    isKeyword = 1;
                    break;
                }
            }
            
            if (isKeyword)
                strcpy(token.type, "keyword");
            else
                strcpy(token.type, "id");
            
            return token;
        }

        // Operators and punctuation: simple handling.
        if (strchr("+-*/=%;:,(){}[].<>!", c) != NULL) {
            int i = 0;
            token.lexeme[i++] = c;
            // Check for two-character operators like '==', '!=', '<=', '>='.
            if ((c == '=' || c == '!' || c == '<' || c == '>')) {
                char next = fgetc(fp);
                if (next == '=') {
                    token.lexeme[i++] = next;
                    col++;
                } else {
                    ungetc(next, fp);
                }
            }
            token.lexeme[i] = '\0';
            strcpy(token.type, "operator");
            return token;
        }

        // Unknown token.
        token.lexeme[0] = c;
        token.lexeme[1] = '\0';
        strcpy(token.type, "unknown");
        return token;
    }

    strcpy(token.type, "EOF");
    strcpy(token.lexeme, "EOF");
    return token;
}

void generateSymbolTable(FILE *fp) {
    Token token;
    rewind(fp);
    row = 1;
    col = 1;
    
    while (1) {
        token = getNextToken(fp);
        if (strcmp(token.type, "EOF") == 0)
            break;

        // Handle variable declarations.
        // If we see a variable declaration keyword (var, let, or const),
        // then the next token should be the variable name.
        if (strcmp(token.type, "keyword") == 0 &&
            (strcmp(token.lexeme, "var") == 0 ||
             strcmp(token.lexeme, "let") == 0 ||
             strcmp(token.lexeme, "const") == 0)) {
            // Save the declaration type for later use.
            char declType[20];
            strcpy(declType, token.lexeme);
            
            // Get the next token (which should be an identifier).
            Token nextToken = getNextToken(fp);
            if (strcmp(nextToken.type, "id") == 0) {
                addToSymbolTable(nextToken.lexeme, declType);
            }
            continue;
        }
        
        // Handle function declarations.
        if (strcmp(token.type, "keyword") == 0 && strcmp(token.lexeme, "function") == 0) {
            // Next token should be the function name.
            Token nextToken = getNextToken(fp);
            if (strcmp(nextToken.type, "id") == 0) {
                addToSymbolTable(nextToken.lexeme, "function");
            }
            continue;
        }
    }
}

void printSymbolTable() {
    printf("Local Symbol Table:\n");
    printf("---------------------------------------------------\n");
    printf("Hash\tName\t\tType\t\tSize\n");
    printf("---------------------------------------------------\n");
    
    for (int i = 0; i < symbolTableIndex; i++) {
        printf("%d\t%-12s\t%-12s\t%s\n", 
               symbolTable[i].hash, 
               symbolTable[i].name, 
               symbolTable[i].type, 
               symbolTable[i].size);
    }
}

int main() {
    FILE *input_fp = fopen("script.js", "r");
    if (input_fp == NULL) {
        printf("Cannot open input file\n");
        return 1;
    }

    // First, generate the symbol table from the input.
    generateSymbolTable(input_fp);
    
    // Then, print the symbol table.
    printSymbolTable();

    fclose(input_fp);
    return 0;
}
