#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_SYMBOL_TABLE_SIZE 100
#define MAX_LEXEME_LENGTH 50

typedef struct {
    int row, col;
    char type[20];      // e.g., "keyword", "id", "number", "operator", etc.
    char lexeme[50];
} Token;

typedef struct {
    int hash;
    char name[MAX_LEXEME_LENGTH];
    char type[20];      // For C: for variables this is the data type; for functions, "function"
    char size[20];      // For primitives, size might be specified; for functions, left blank.
} SymbolTableEntry;

int row = 1, col = 1;
SymbolTableEntry symbolTable[MAX_SYMBOL_TABLE_SIZE];
int symbolTableIndex = 0;

// Hash function for symbol names.
int calculateHash(const char* str) {
    int hash = 0;
    while (*str) {
        hash = hash * 31 + *str++;
    }
    return abs(hash % MAX_SYMBOL_TABLE_SIZE);
}

void addToSymbolTable(const char* name, const char* declType) {
    // Avoid duplicates.
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

// Helper function to peek the next non-whitespace character.
char peekNextChar(FILE *fp) {
    int ch;
    while ((ch = fgetc(fp)) != EOF) {
        if (!isspace(ch)) {
            ungetc(ch, fp);
            return ch;
        }
    }
    return EOF;
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
        // Single-line comment handling (C uses // and /* ... */)
        if (c == '/' && (c = fgetc(fp)) == '/') {
            while ((c = fgetc(fp)) != '\n' && c != EOF);
            row++; col = 1;
            continue;
        }
        if (c == '/' && (c = fgetc(fp)) == '*') {
            while ((c = fgetc(fp)) != EOF) {
                if (c == '*' && (c = fgetc(fp)) == '/') break;
            }
            col++;
            continue;
        }
        // String literal handling.
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
        // Numeric literal handling.
        if (isdigit(c)) {
            int i = 0;
            token.lexeme[i++] = c;
            while (isdigit(c = fgetc(fp))) {
                token.lexeme[i++] = c; col++;
            }
            token.lexeme[i] = '\0';
            ungetc(c, fp);
            strcpy(token.type, "number");
            return token;
        }
        // Identifier and keyword handling.
        if (isalpha(c) || c == '_') {
            int i = 0;
            token.lexeme[i++] = c;
            while ((c = fgetc(fp)) != EOF && (isalnum(c) || c == '_')) {
                token.lexeme[i++] = c; col++;
            }
            token.lexeme[i] = '\0';
            ungetc(c, fp);
            // List of C keywords (a subset)
            const char *keywords[] = {
                "auto", "break", "case", "char", "const", "continue", "default", "do",
                "double", "else", "enum", "extern", "float", "for", "goto", "if",
                "inline", "int", "long", "register", "restrict", "return", "short",
                "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
                "unsigned", "void", "volatile", "while"
            };
            int numKeywords = sizeof(keywords)/sizeof(keywords[0]);
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
        // Operators and punctuation.
        if (strchr("+-*/=%;:,(){}[].<>!", c) != NULL) {
            int i = 0;
            token.lexeme[i++] = c;
            if ((c=='=' || c=='!' || c=='<' || c=='>')) {
                char next = fgetc(fp);
                if (next == '=') { token.lexeme[i++] = next; col++; }
                else { ungetc(next, fp); }
            }
            token.lexeme[i] = '\0';
            strcpy(token.type, "operator");
            return token;
        }
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
    rewind(fp); row = 1; col = 1;
    while (1) {
        token = getNextToken(fp);
        if (strcmp(token.type, "EOF") == 0)
            break;
        
        // For function detection in C:
        // When a token is a valid C type (e.g., int, float, char, double, void), we treat it as a return type candidate.
        if (strcmp(token.type, "keyword") == 0 &&
            (strcmp(token.lexeme, "int") == 0 || strcmp(token.lexeme, "float") == 0 ||
             strcmp(token.lexeme, "char") == 0 || strcmp(token.lexeme, "double") == 0 ||
             strcmp(token.lexeme, "void") == 0)) {
            
            // Save the return type.
            char returnType[20];
            strcpy(returnType, token.lexeme);
            
            // Next token should be an identifier.
            Token nextToken = getNextToken(fp);
            if (strcmp(nextToken.type, "id") == 0) {
                // Peek the next non-whitespace character to check for '('.
                char nextChar = peekNextChar(fp);
                if (nextChar == '(') {
                    // This is a function declaration.
                    addToSymbolTable(nextToken.lexeme, "function");
                    continue;
                } else {
                    // Otherwise, it's a variable declaration.
                    addToSymbolTable(nextToken.lexeme, returnType);
                    continue;
                }
            }
        }
    }
}

void printSymbolTable() {
    printf("C Symbol Table:\n");
    printf("---------------------------------------------------\n");
    printf("Index\tName\t\tType\t\tSize\n");
    printf("---------------------------------------------------\n");
    for (int i = 0; i < symbolTableIndex; i++) {
        // Here we print the index instead of the hash.
        printf("%d\t%-12s\t%-12s\t%s\n",
               i,
               symbolTable[i].name,
               symbolTable[i].type,
               symbolTable[i].size);
    }
}

int main() {
    FILE *input_fp = fopen("source.c", "r");
    if (!input_fp) { printf("Cannot open source.c\n"); return 1; }
    generateSymbolTable(input_fp);
    printSymbolTable();
    fclose(input_fp);
    return 0;
}
