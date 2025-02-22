#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define MAX_SYMBOL_TABLE_SIZE 100
#define MAX_LEXEME_LENGTH 50

typedef struct {
    int row, col;
    char type[20];   // e.g. "keyword", "id", "variable", "string", "number", "operator", etc.
    char lexeme[50];
} Token;

typedef struct {
    int hash;
    char name[MAX_LEXEME_LENGTH];
    char type[20];   // For Perl, we use "function" for subs.
    char size[20];   // Not applicable for Perl; left blank.
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

Token getNextToken(FILE *fp) {
    Token token;
    token.row = row;
    token.col = col;
    char c;
    
    while ((c = fgetc(fp)) != EOF) {
        col++;
        
        // Skip whitespace; update row and col.
        if (isspace(c)) {
            if (c == '\n') {
                row++;
                col = 1;
            }
            continue;
        }
        
        // Perl single-line comments start with '#'
        if (c == '#') {
            while ((c = fgetc(fp)) != '\n' && c != EOF);
            row++;
            col = 1;
            continue;
        }
        
        // String literal handling: single or double quotes.
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
        
        // Identifiers, keywords, and variables.
        // In Perl, identifiers may begin with a letter or underscore;
        // variables begin with a sigil ($, @, %).
        if (isalpha(c) || c == '_' || c == '$' || c == '@' || c == '%') {
            int i = 0;
            token.lexeme[i++] = c;
            while ((c = fgetc(fp)) != EOF && (isalnum(c) || c == '_' || c == '$' || c == '@' || c == '%')) {
                token.lexeme[i++] = c;
                col++;
            }
            token.lexeme[i] = '\0';
            ungetc(c, fp);
            
            // For Perl, we only treat "sub" as a keyword that marks a function definition.
            if (strcmp(token.lexeme, "sub") == 0)
                strcpy(token.type, "keyword");
            else {
                // If the first character is one of the sigils, mark it as variable.
                if (token.lexeme[0] == '$' || token.lexeme[0] == '@' || token.lexeme[0] == '%')
                    strcpy(token.type, "variable");
                else
                    strcpy(token.type, "id");
            }
            return token;
        }
        
        // Operators and punctuation (a simple set)
        if (strchr("+-*/=%;:,(){}[].<>!", c) != NULL) {
            int i = 0;
            token.lexeme[i++] = c;
            // Check for two-character operators like '==', '!=' etc.
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
        
        // Any other character: return as unknown.
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
        
        // Look for function definitions: keyword "sub" followed by an identifier.
        if (strcmp(token.type, "keyword") == 0 && strcmp(token.lexeme, "sub") == 0) {
            Token nextToken = getNextToken(fp);
            if (strcmp(nextToken.type, "id") == 0) {
                addToSymbolTable(nextToken.lexeme, "function");
            }
            continue;
        }
        
        // Optionally, you might want to add variables based on their sigils.
        // For example, add a variable if token.type is "variable".
        // Uncomment the following if desired:
        
        if (strcmp(token.type, "variable") == 0) {
            addToSymbolTable(token.lexeme, "variable");
        }
        
    }
}

void printSymbolTable() {
    printf("Perl Symbol Table:\n");
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
    FILE *input_fp = fopen("perl.pl", "r");
    if (!input_fp) {
        printf("Cannot open source.pl\n");
        return 1;
    }
    
    // Generate the symbol table from the Perl source.
    generateSymbolTable(input_fp);
    // Print the resulting symbol table.
    printSymbolTable();
    
    fclose(input_fp);
    return 0;
}
