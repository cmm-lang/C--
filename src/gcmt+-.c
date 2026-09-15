#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// ============================================================
//                        CONSTANTS
// ============================================================
#define MAX_TOKEN 5000
#define MAX_SOURCE 100000
#define MAX_ERRORS 100
#define MAX_TEMP_VARS 256

// ============================================================
//                        TOKENS
// ============================================================
typedef enum {
    // key words
    TOKEN_CONST, TOKEN_VAR, TOKEN_IF, TOKEN_ELSE, TOKEN_ELSE_IF,
    TOKEN_WHILE, TOKEN_FOR, TOKEN_FN, TOKEN_RETURN,
    TOKEN_BREAK, TOKEN_CONTINUE, TOKEN_ARROW,
    
    // input/output
    TOKEN_SAY, TOKEN_SAY_EXCLAM, TOKEN_INPUT, TOKEN_CLS, TOKEN_PRESS, TOKEN_FATAL,
    
    // time
    TOKEN_SSLEEP, TOKEN_MSLEEP,
    
    // files
    TOKEN_CREATE_FILE, TOKEN_APPEND_FILE, TOKEN_WRITE_FILE, TOKEN_READ_FILE,
    TOKEN_FSAY, TOKEN_FSAY_EXCLAM,
    
    // string functions
    TOKEN_STRLEN, TOKEN_STRUPPER, TOKEN_STRLOWER, TOKEN_STRTRIM,
    
    // C insert
    TOKEN_C,
    
    // literals
    TOKEN_IDENTIFIER, TOKEN_NUMBER, TOKEN_FLOAT, TOKEN_STRING,
    TOKEN_BOOLEAN, TOKEN_CHAR, TOKEN_NULL,
    
    // types
    TOKEN_WORD_NUMBER, TOKEN_WORD_STRING, TOKEN_WORD_BOOLEAN,
    TOKEN_WORD_CHAR, TOKEN_WORD_FLOAT, TOKEN_WORD_VOID,
    
    // enum / struct
    TOKEN_ENUM,

    // operators
    TOKEN_EQ, TOKEN_EQ_EQ, TOKEN_NEQ, TOKEN_LT, TOKEN_GT,
    TOKEN_LTE, TOKEN_GTE, TOKEN_PLUS, TOKEN_MINUS,
    TOKEN_STAR, TOKEN_SLASH, TOKEN_PERCENT, TOKEN_INC, TOKEN_DEC,
    TOKEN_AND, TOKEN_OR, TOKEN_NOT, TOKEN_DOT,
    
    // symbols
    TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_LPAREN, TOKEN_RPAREN,
    TOKEN_LBRACKET, TOKEN_RBRACKET, TOKEN_COMMA, TOKEN_SEMICOLON,
    TOKEN_COLON,
    
    // comments
    TOKEN_COMMENT,          // //
    
    // special
    TOKEN_EOF, TOKEN_UNKNOWN, TOKEN_FUTURE_FEATURE
} TokenType;

// ============================================================
//                        STRUCTS
// ============================================================

// ---- TOKEN ----
typedef struct {
    TokenType type;
    char value[MAX_TOKEN];
    int line;
    int column;
} Token;

// ---- LEXER ----
typedef struct {
    char *source;
    char *pos;
    int line;
    int column;
} Lexer;

// ---- ERROR SYSTEM ----
typedef struct {
    int line;
    int column;
    char message[512];
    char code[MAX_TOKEN];
    int is_warning;      // ← 0 = error, 1 = warning
} ErrorEntry;

typedef struct {
    ErrorEntry entries[MAX_ERRORS];
    int count;
    int error_count;
    int warning_count;
} ErrorContext;

// ---- FUTURE FEATURE ----
typedef struct {
    char *name;
    char *version;
} FutureFeature;

// ---- AST (ABSTRACT SYNTAX TREE) ----
typedef enum {
    // Basic
    NODE_PROGRAM, 
    NODE_CONST, 
    NODE_VAR, 
    NODE_SAY, 
    NODE_SAY_EXCLAM,
    NODE_IF, 
    NODE_WHILE, 
    NODE_FOR,
    NODE_BINARY_OP, 
    NODE_IDENTIFIER, 
    NODE_NUMBER, 
    NODE_STRING,
    NODE_BOOLEAN, 
    NODE_CHAR, 
    NODE_NULL,
    NODE_BLOCK, 
    NODE_C, 
    NODE_ARRAY,
    NODE_INDEX,
    NODE_STR_INDEX,
    NODE_ARRLEN,
    NODE_EOF,
    
    // Control
    NODE_ELSE,
    NODE_COMPARE,
    NODE_NOT,
    NODE_LOGICAL_OP,
    
    // Input
    NODE_INPUT,
    
    // String functions
    NODE_STRLEN,
    NODE_STRUPPER,
    NODE_STRLOWER,
    NODE_STRTRIM,
    
    // Files
    NODE_CREATE_FILE,
    NODE_APPEND_FILE,
    NODE_WRITE_FILE,
    NODE_READ_FILE,
    NODE_FSAY,
    NODE_FSAY_EXCLAM,
    
    // Errors
    NODE_FATAL,
    
    // System
    NODE_CLS,
    NODE_PRESS,
    NODE_SSLEEP,
    NODE_MSLEEP,
    
    // Enum / struct
    NODE_ENUM,
    NODE_ENUM_ITEM,
    NODE_ENUM_ACCESS,

    // Additional
    NODE_RETURN,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_FN,
    NODE_CALL,
    NODE_PARAM
} NodeType;

// --- symbol ---
struct Symbol;
typedef struct Node {
    NodeType type;
    char data_type[20];
    struct Node *left;
    struct Node *right;
    struct Node *body;
    struct Node *next;
    struct Node *init;
    struct Node *cond;
    struct Node *inc;
    char value[MAX_TOKEN];
    int line;
    int column;
    int buffer_size;
    int is_dynamic;
    int is_declaration;
    
    struct Symbol* resolved_symbol;
} Node;

// ---- PARSER ----
#define PARSER_STACK_SIZE 50
typedef struct {
    char *pos;
    int line;
    int column;
    int brace_depth;
} ParserState;

typedef struct Parser {
    Lexer *lexer;
    Token current;
    ErrorContext *error;
    char *source;
    int brace_depth;

    // stack for parser_unadv
    ParserState stack[PARSER_STACK_SIZE];
    int stack_top;
} Parser;
// ============================================================
//                      FUNCTION PROTOTYPES
// ============================================================

// ---- ERROR FUNCTIONS ----
ErrorContext* error_init(void);
void error_free(ErrorContext* e);
void error_report(ErrorContext* e, char* source, int line, int column, char* msg);
void error_syntax(ErrorContext* e, char* source, Token t, char* expected);
void error_unknown(ErrorContext* e, char* source, Token t);
void error_undefined(ErrorContext* e, char* source, Token t);
void error_unknown_type(ErrorContext* e, char* source, Token t);
void error_unknown_feature(ErrorContext* e, char* source, Token t);
void error_future_feature(ErrorContext* e, char* source, Token t, char* version);
void error_unexpected(ErrorContext* e, char* source, Token t);
void error_expected_variable(ErrorContext* e, char* source, Token t);
void error_expected_expression(ErrorContext* e, char* source, Token t);
void error_expected_number(ErrorContext* e, char* source, Token t);
void error_expected_name(ErrorContext* e, char* source, Token t);
void error_expected_filename(ErrorContext* e, char* source, Token t);

// ---- UTILITY FUNCTIONS ----
int detect_type(char* value);
void skip_to_sync(Parser* p);

// ---- AST FUNCTIONS ----
Node* create_node(NodeType type, char* value, int line, int column);
char* get_c_type(Node* node);

// ---- LEXER FUNCTIONS ----
Lexer* lexer_init(char *source);
void lexer_free(Lexer *l);
Token lexer_next(Lexer *l);

// ---- PARSER FUNCTIONS ----
Parser* parser_init(Lexer *l, char* source);
void parser_free(Parser *p);
void parser_advance(Parser *p);
Node* parse_statement(Parser* p);
Node* parse_block(Parser* p, const char* scope_name);
Node* parse_condition(Parser* p);
Node* parse_expression(Parser* p);
Node* parse_function(Parser* p);
Node* parse_program(Parser* p);

// ---- TRANSLATOR FUNCTIONS ----
void translate_expression(Node* node, FILE* out);
void translate_condition(Node* cond, FILE* out);
void translate_statement(Node* node, FILE* out, int depth);
void translate_block(Node* block, FILE* out, int depth);
void translate_function(Node* fn, FILE* out);
void translate_program(Node* ast, char* filename);
static void indent(FILE* out, int depth);
// For-loop helpers (no indent, no semicolon)
void translate_for_init(Node* node, FILE* out);
void translate_for_inc(Node* node, FILE* out);
// ===============================
// ======== SYMBOL TABLE =========
// ===============================

typedef struct Symbol {
    char name[200];
    char type[20];      // "int", "void*", "string", ...
    int is_const;
    int use;
    int line;
    int column;
    struct Symbol* next;
} Symbol;

Symbol* symtab = NULL;

typedef struct FuncInfo {
    char name[200];
    char return_type[20];      // "int", "string", "void", ...
    Node* params;
    int param_count;
    int use;
    int line;
    int column;
    struct FuncInfo* next;
} FuncInfo;

FuncInfo* functab = NULL;

typedef struct Scope {
    Symbol* symbols;
    struct Scope* parent;
    struct Scope* next_all;
    
    char name[128];
    int  level;
    int  id;
} Scope;

Scope* current_scope = NULL;
Scope* all_scopes = NULL;
int global_scope_counter = 0;

void scope_enter(const char* name) {
    Scope* s = malloc(sizeof(Scope));
    s->symbols = NULL;
    s->parent = current_scope;
    s->next_all = all_scopes;
    
    strncpy(s->name, name ? name : "?", sizeof(s->name) - 1);
    s->name[sizeof(s->name) - 1] = '\0';
    s->id = global_scope_counter++;
    s->level = current_scope ? current_scope->level + 1 : 0;
    
    all_scopes = s;
    current_scope = s;
}

void scope_exit(void) {
    if (!current_scope) return;
    current_scope = current_scope->parent;
}

Symbol* find_symbol(const char* name) {
    Scope* s = current_scope;
    while (s) {
        Symbol* sym = s->symbols;
        while (sym) {
            if (strcmp(sym->name, name) == 0) {
                sym->use = 1;
                return sym;
            }
            sym = sym->next;
        }
        s = s->parent;
    }
    return NULL;
}

void add_symbol(const char* name, const char* type, int is_const,
                int line, int column) {
    Symbol* s = current_scope->symbols;
    while (s) {
        if (strcmp(s->name, name) == 0) {
            fprintf(stderr, "redeclaration of '%s'\n", name);
            return;
        }
        s = s->next;
    }
    
    Symbol* sym = malloc(sizeof(Symbol));
    strcpy(sym->name, name);
    strcpy(sym->type, type);
    sym->is_const = is_const;
    sym->use = 0;
    sym->line = line;
    sym->column = column;
    sym->next = current_scope->symbols;
    current_scope->symbols = sym;
}

FuncInfo* find_func(const char* name) {
    FuncInfo* f = functab;
    while (f) {
        if (strcmp(f->name, name) == 0) {
        f->use = 1;
        return f;
 }
        f = f->next;
    }
    return NULL;
}

void add_func(const char* name, const char* return_type, 
              Node* params, int param_count,
              int line, int column) {
    FuncInfo* existing = find_func(name);
    if (existing) {
        fprintf(stderr, "conflict name: '%s vs %s'\n", name, existing->name);
        return;
    }
    
    FuncInfo* f = malloc(sizeof(FuncInfo));
    strcpy(f->name, name);
    strcpy(f->return_type, return_type);
    f->params = params;
    f->param_count = param_count;
    f->use = 0;
    f->line = line;
    f->column = column;
    f->next = functab;
    functab = f;
}

// ============================================================
//                 GET TYPE EXPR (for parser)
// ============================================================
// Infers the C-- type of a partially or fully built AST node.
// Used during parsing (Option 3: "infer at node creation").
//
// Works with Node*, uses Symbol Table for identifiers.
// Returns C-- type as a string:
//   "int", "float", "string", "char", "boolean", "null",
//   "void*", "[int]", "[string]", ...
//
// NOTE: some cases return pointers into a static buffer.
//       Caller must copy immediately (strcpy) if needed later.
// ============================================================

const char* get_type_expr(Node* node) {
    if (!node) return "unknown";

    switch (node->type) {

        // ---- Literals ----

        case NODE_NUMBER: {
            // Float if contains '.', else int
            for (int i = 0; node->value[i]; i++) {
                if (node->value[i] == '.') return "float";
            }
            return "int";
        }

        case NODE_STRING:  return "string";
        case NODE_CHAR:    return "char";
        case NODE_BOOLEAN: return "bool";
        case NODE_NULL:    return "null";

        // ---- Identifier: lookup in Symbol Table ----

   case NODE_IDENTIFIER: {
    if (node->resolved_symbol) {
        node->resolved_symbol->use = 1;
        return node->resolved_symbol->type;
    }
    
    Symbol* s = find_symbol(node->value);
    if (s) {
        node->resolved_symbol = s;
        return s->type;
    }
    return "unknown";
}

        // ---- Binary operations ----

case NODE_BINARY_OP: {
    const char* lt = get_type_expr(node->left);
    const char* rt = get_type_expr(node->right);
    
    // Helper: is numeric type?
    int l_num = (strcmp(lt, "int") == 0 ||
                 strcmp(lt, "float") == 0 ||
                 strcmp(lt, "char") == 0 ||
                 strcmp(lt, "bool") == 0);
    int r_num = (strcmp(rt, "int") == 0 ||
                 strcmp(rt, "float") == 0 ||
                 strcmp(rt, "char") == 0 ||
                 strcmp(rt, "bool") == 0);
    
    // float + anything numeric → float
    if ((strcmp(lt, "float") == 0 && r_num) ||
        (strcmp(rt, "float") == 0 && l_num)) {
        return "float";
    }
    
    // char + char → int
    // char + int  → int
    // int + char  → int
    // int + int   → int
    // bool + int  → int
    if (l_num && r_num) {
        return "int";
    }
    
    // string + string → error
    if (strcmp(lt, "string") == 0 && strcmp(rt, "string") == 0) {
        return "UB";
    }
    
    return "unknown";
}

        // ---- Comparisons and logic -> boolean ----

        case NODE_COMPARE: return "boolean";
        case NODE_NOT:     return "boolean";

        // ---- Array literal ----

        case NODE_ARRAY: {
            // Type = [element_type]
            static char buf[64];
            if (node->body) {
                const char* elem = get_type_expr(node->body);
                snprintf(buf, sizeof(buf), "[%s]", elem);
            } else {
                strcpy(buf, "[]");
            }
            return buf;
        }

        // ---- Indexing ----
case NODE_STR_INDEX: {
    // Check if base is an identifier with known type
    if (node->left && node->left->type == NODE_IDENTIFIER) {
        Symbol* s = find_symbol(node->left->value);
        if (s) {
            // If it's a fixed-size string (char array) → char
            if (strcmp(s->type, "string") == 0) {
                return "char";
            }
            // If it's an array [type] → element type
            if (s->type[0] == '[') {
                static char buf[64];
                size_t len = strlen(s->type);
                if (len >= 2) {
                    strncpy(buf, s->type + 1, len - 2);
                    buf[len - 2] = '\0';
                    return buf;
                }
            }
            // If it's "auto" and variable has buffer_size → assume char
            // (fallback for `var text[256]` without explicit type)
            return "char";  // ← fallback
        }
    }
    
    // Fallback: check via get_type_expr
    const char* base = get_type_expr(node->left);
    
    if (base[0] == '[') {
        static char buf[64];
        size_t len = strlen(base);
        if (len >= 2) {
            strncpy(buf, base + 1, len - 2);
            buf[len - 2] = '\0';
            return buf;
        }
    }
    
    if (strcmp(base, "string") == 0) return "char";
    
    return "unknown";
}

        // ---- Built-in string functions ----

        case NODE_STRLEN:   return "int";
        case NODE_STRUPPER: return "string";
        case NODE_STRLOWER: return "string";
        case NODE_STRTRIM:  return "string";

        // ---- arrlen ----

        case NODE_ARRLEN:   return "int";

        // ---- User function calls ----

        case NODE_CALL: {
    FuncInfo* f = find_func(node->value);
    if (f) return f->return_type;
    return "unknown";
}

        // ---- Input ----

        case NODE_INPUT: {
            // Type stored in data_type during parse_statement
            if (node->data_type[0] != '\0' &&
                strcmp(node->data_type, "auto") != 0) {
                return node->data_type;
            }
            return "unknown";
        }

        // ---- File read ----

        case NODE_READ_FILE: return "string read_file";

        // ---- Default ----

        default:
            return "unknown";
    }
}
// ============================================================
//                      ERROR SYSTEM
// ============================================================
ErrorContext* error_init(void) {
    ErrorContext* e = (ErrorContext*)calloc(1, sizeof(ErrorContext));
    e->error_count = 0;
    return e;
}

void error_free(ErrorContext* e) {
    if (e) free(e);
}
// ============================================================
//                      FORMATTED ERROR
// ============================================================
void error_report(ErrorContext* e, char* source, int line, int column, char* msg) {
    if (e->error_count >= MAX_ERRORS) {
        fprintf(stderr, "⚠️ Too many errors, stopping\n");
        return;
    }
    
    ErrorEntry* err = &e->entries[e->error_count];
    err->line = line;
    err->column = column;
    strcpy(err->message, msg);
    e->error_count++;
    
    fprintf(stderr, "\n");
    fprintf(stderr, "line: %d, symbol: %d ERROR:\n", 
            line, column);
    
    char* line_start = source;
    int current_line = 1;
    
    while (*line_start && current_line < line) {
        if (*line_start == '\n') current_line++;
        line_start++;
    }
    
    char* line_end = line_start;
    while (*line_end && *line_end != '\n') line_end++;
    
    int len = (int)(line_end - line_start);
    for (int i = 0; i < len; i++) {
        fputc(line_start[i], stderr);
    }
    fprintf(stderr, "\n");
    
    for (int i = 0; i < column - 1; i++) {
        fputc(' ', stderr);
    }
    fprintf(stderr, "^^^\n");
    
    fprintf(stderr, "REASON: %s\n", msg);
}
void warning_report(ErrorContext* e, char* source, int line, int column, char* msg) {
    if (e->count >= MAX_ERRORS) return;
    
    ErrorEntry* w = &e->entries[e->count];
    w->line = line;
    w->column = column;
    strcpy(w->message, msg);
    w->is_warning = 1;
    e->count++;
    e->warning_count++;
    
    fprintf(stderr, "\n");
    fprintf(stderr, "line: %d, symbol: %d WARNING:\n", line, column);
    
    // copying error_report
    char* line_start = source;
    int current_line = 1;
    
    while (*line_start && current_line < line) {
        if (*line_start == '\n') current_line++;
        line_start++;
    }
    
    char* line_end = line_start;
    while (*line_end && *line_end != '\n') line_end++;
    
    int len = (int)(line_end - line_start);
    for (int i = 0; i < len; i++) {
        fputc(line_start[i], stderr);
    }
    fprintf(stderr, "\n");
    
    for (int i = 0; i < column - 1; i++) {
        fputc(' ', stderr);
    }
    fprintf(stderr, "^^^\n");
    
    fprintf(stderr, "REASON: %s\n", msg);
}

// ============================================================
//                      FUTURE FEATURES
// ============================================================
// SOON
// ============================================================
//                      ERROR FUNCTIONS
// ============================================================
void error_syntax(ErrorContext* e, char* source, Token t, char* expected) {
    char msg[512];
    sprintf(msg, "Expected '%s', got '%s'", expected, t.value);
    error_report(e, source, t.line, t.column, msg);
}

void error_unknown(ErrorContext* e, char* source, Token t) {
    char msg[512];
    sprintf(msg, "Unknown symbol '%s'", t.value);
    error_report(e, source, t.line, t.column, msg);
}

void error_undefined(ErrorContext* e, char* source, Token t) {
    char msg[512];
    sprintf(msg, "Undefined variable '%s'", t.value);
    error_report(e, source, t.line, t.column, msg);
}

void error_unknown_type(ErrorContext* e, char* source, Token t) {
    char msg[512];
    sprintf(msg, "Unknown type '%s' need \"string, char, int, float, boolean\"", t.value);
    error_report(e, source, t.line, t.column, msg);
}

void error_unknown_feature(ErrorContext* e, char* source, Token t) {
    char msg[512];
    sprintf(msg, "Unknown feature '%s'", t.value);
    error_report(e, source, t.line, t.column, msg);
}

void error_future_feature(ErrorContext* e, char* source, Token t, char* version) {
    char msg[512];
    sprintf(msg, "Feature '%s' is not Available in C-- 1.0", t.value);
    error_report(e, source, t.line, t.column, msg);
    fprintf(stderr, "AVAILABLE: C-- %s\n", version);
}

void error_unexpected(ErrorContext* e, char* source, Token t) {
    char msg[512];
    sprintf(msg, "Unexpected '%s'", t.value);
    error_report(e, source, t.line, t.column, msg);
}

void error_expected_variable(ErrorContext* e, char* source, Token t) {
    char msg[512];
    sprintf(msg, "Expected variable name, got '%s'", t.value);
    error_report(e, source, t.line, t.column, msg);
}

void error_expected_expression(ErrorContext* e, char* source, Token t) {
    char msg[512];
    sprintf(msg, "Expected expression, got '%s'", t.value);
    error_report(e, source, t.line, t.column, msg);
}

void error_expected_number(ErrorContext* e, char* source, Token t) {
    char msg[512];
    sprintf(msg, "Expected number, got '%s'", t.value);
    error_report(e, source, t.line, t.column, msg);
}

void error_expected_name(ErrorContext* e, char* source, Token t) {
    char msg[512];
    sprintf(msg, "Expected function name, got '%s'", t.value);
    error_report(e, source, t.line, t.column, msg);
}

void error_expected_filename(ErrorContext* e, char* source, Token t) {
    char msg[512];
    sprintf(msg, "Expected filename, got '%s'", t.value);
    error_report(e, source, t.line, t.column, msg);
}

void warning_unused_variable(ErrorContext* e, char* source,
                              const char* name, int line, int column) {
    char msg[512];
    sprintf(msg, "Unused variable '%s'", name);
    warning_report(e, source, line, column, msg);
}

void warning_unused_function(ErrorContext* e, char* source,
                              const char* name, int line, int column) {
    char msg[512];
    sprintf(msg, "Unused function '%s'", name);
    warning_report(e, source, line, column, msg);
}

void warning_uninitialized(ErrorContext* e, char* source,
                            const char* name, int line, int column) {
    char msg[512];
    sprintf(msg, "Variable '%s' is never initialized", name);
    warning_report(e, source, line, column, msg);
}

void warning_no_return(ErrorContext* e, char* source,
                        const char* name, int line, int column) {
    char msg[512];
    sprintf(msg, "Function '%s' may not return a value", name);
    warning_report(e, source, line, column, msg);
}
// ============================================================
//                        LEXER
// ============================================================
Lexer* lexer_init(char *source) {
    Lexer *l = (Lexer*)malloc(sizeof(Lexer));
    l->source = source;
    l->pos = source;
    l->line = 1;
    l->column = 1;
    return l;
}

void lexer_free(Lexer *l) {
    if (l) free(l);
}

Token lexer_next(Lexer *l) {
    Token t;
    t.line = l->line;
    t.column = l->column;
    t.value[0] = '\0';
    
    while (*l->pos == ' ' || *l->pos == '\t' || *l->pos == '\n' || *l->pos == '\r') {
        if (*l->pos == '\n') {
            l->line++;
            l->column = 1;
        } else {
            l->column++;
        }
        l->pos++;
    }
    
    t.line = l->line;
    t.column = l->column;
    
    if (*l->pos == '\0') {
        t.type = TOKEN_EOF;
        return t;
    }
    
    if (*l->pos >= '0' && *l->pos <= '9') {
        t.type = TOKEN_NUMBER;
        int i = 0;
        int has_dot = 0;
        while ((*l->pos >= '0' && *l->pos <= '9') || (*l->pos == '.' && !has_dot)) {
            if (*l->pos == '.') has_dot = 1;
            if (i < MAX_TOKEN - 1) t.value[i++] = *l->pos;
            l->pos++;
            l->column++;
        }
        t.value[i] = '\0';
        if (has_dot) t.type = TOKEN_FLOAT;
        return t;
    }
    
    if (*l->pos == '"') {
        t.type = TOKEN_STRING;
        l->pos++;
        l->column++;
        int i = 0;
        while (*l->pos != '"' && *l->pos != '\0') {
            if (*l->pos == '\\') {
                l->pos++;
                l->column++;
                switch (*l->pos) {
                    case 'n': t.value[i++] = '\n'; break;
                    case 't': t.value[i++] = '\t'; break;
                    case '\\': t.value[i++] = '\\'; break;
                    case '"': t.value[i++] = '"'; break;
                    default: t.value[i++] = *l->pos; break;
                }
                l->pos++;
                l->column++;
            } else {
                if (i < MAX_TOKEN - 1) t.value[i++] = *l->pos;
                l->pos++;
                l->column++;
            }
        }
        t.value[i] = '\0';
        if (*l->pos == '"') {
            l->pos++;
            l->column++;
        }
        return t;
    }
    
    if (*l->pos == '\'') {
        t.type = TOKEN_CHAR;
        l->pos++;
        l->column++;
        int i = 0;
        while (*l->pos != '\'' && *l->pos != '\0') {
            if (*l->pos == '\\') {
                l->pos++;
                l->column++;
                switch (*l->pos) {
                    case 'n': t.value[i++] = '\n'; break;
                    case 't': t.value[i++] = '\t'; break;
                    case '\\': t.value[i++] = '\\'; break;
                    case '\'': t.value[i++] = '\''; break;
                    default: t.value[i++] = *l->pos; break;
                }
                l->pos++;
                l->column++;
            } else {
                if (i < MAX_TOKEN - 1) t.value[i++] = *l->pos;
                l->pos++;
                l->column++;
            }
        }
        t.value[i] = '\0';
        if (*l->pos == '\'') {
            l->pos++;
            l->column++;
        }
        return t;
    }
    
    if ((*l->pos >= 'a' && *l->pos <= 'z') || 
        (*l->pos >= 'A' && *l->pos <= 'Z') || 
        *l->pos == '_') {
        int i = 0;
        while ((*l->pos >= 'a' && *l->pos <= 'z') || 
               (*l->pos >= 'A' && *l->pos <= 'Z') || 
               (*l->pos >= '0' && *l->pos <= '9') ||
               *l->pos == '_' || *l->pos == '!') {
            if (i < MAX_TOKEN - 1) t.value[i++] = *l->pos;
            l->pos++;
            l->column++;
        }
        t.value[i] = '\0';
        
        if (strcmp(t.value, "const") == 0) t.type = TOKEN_CONST;
        else if (strcmp(t.value, "var") == 0) t.type = TOKEN_VAR;
        else if (strcmp(t.value, "say") == 0) t.type = TOKEN_SAY;
        else if (strcmp(t.value, "say!") == 0) t.type = TOKEN_SAY_EXCLAM;
        else if (strcmp(t.value, "if") == 0) t.type = TOKEN_IF;
        else if (strcmp(t.value, "else") == 0) t.type = TOKEN_ELSE;
        else if (strcmp(t.value, "while") == 0) t.type = TOKEN_WHILE;
        else if (strcmp(t.value, "for") == 0) t.type = TOKEN_FOR;
        else if (strcmp(t.value, "fn") == 0) t.type = TOKEN_FN;
        else if (strcmp(t.value, "return") == 0) t.type = TOKEN_RETURN;
        else if (strcmp(t.value, "break") == 0) t.type = TOKEN_BREAK;
        else if (strcmp(t.value, "continue") == 0) t.type = TOKEN_CONTINUE;
        else if (strcmp(t.value, "input") == 0) t.type = TOKEN_INPUT;
        else if (strcmp(t.value, "cls") == 0) t.type = TOKEN_CLS;
        else if (strcmp(t.value, "press") == 0) t.type = TOKEN_PRESS;
        else if (strcmp(t.value, "ssleep") == 0) t.type = TOKEN_SSLEEP;
        else if (strcmp(t.value, "msleep") == 0) t.type = TOKEN_MSLEEP;
        else if (strcmp(t.value, "create_file") == 0) t.type = TOKEN_CREATE_FILE;
        else if (strcmp(t.value, "append_file") == 0) t.type = TOKEN_APPEND_FILE;
        else if (strcmp(t.value, "write_file") == 0) t.type = TOKEN_WRITE_FILE;
        else if (strcmp(t.value, "read_file") == 0) t.type = TOKEN_READ_FILE;
        else if (strcmp(t.value, "fsay") == 0) t.type = TOKEN_FSAY;
        else if (strcmp(t.value, "fsay!") == 0) t.type = TOKEN_FSAY_EXCLAM;
        else if (strcmp(t.value, "strlen") == 0) t.type = TOKEN_STRLEN;
        else if (strcmp(t.value, "strupper") == 0) t.type = TOKEN_STRUPPER;
        else if (strcmp(t.value, "strlower") == 0) t.type = TOKEN_STRLOWER;
        else if (strcmp(t.value, "strtrim") == 0) t.type = TOKEN_STRTRIM;
        else if (strcmp(t.value, "fatal") == 0) t.type = TOKEN_FATAL;
        else if (strcmp(t.value, "C") == 0) t.type = TOKEN_C;
        else if (strcmp(t.value, "enum") == 0) t.type = TOKEN_ENUM;
        else if (strcmp(t.value, "true") == 0 || strcmp(t.value, "false") == 0) {
            t.type = TOKEN_BOOLEAN;
        }
        else if (strcmp(t.value, "int") == 0) t.type = TOKEN_WORD_NUMBER;
        else if (strcmp(t.value, "float") == 0) t.type = TOKEN_WORD_FLOAT;
        else if (strcmp(t.value, "string") == 0) t.type = TOKEN_WORD_STRING;
        else if (strcmp(t.value, "bool") == 0) t.type = TOKEN_WORD_BOOLEAN;
        else if (strcmp(t.value, "char") == 0) t.type = TOKEN_WORD_CHAR;
        else if (strcmp(t.value, "void") == 0) t.type = TOKEN_WORD_VOID;
        else if (strcmp(t.value, "null") == 0) {
            t.type = TOKEN_NULL;
        }
        else {
                t.type = TOKEN_IDENTIFIER;
                }
                
                return t;
      }
      
    switch (*l->pos) {
        case '=':
            l->pos++; l->column++;
            if (*l->pos == '=') {
                t.type = TOKEN_EQ_EQ;
                strcpy(t.value, "==");
                l->pos++; l->column++;
            } else {
                t.type = TOKEN_EQ;
                strcpy(t.value, "=");
            }
            return t;
            
        case '!':
            l->pos++; l->column++;
            if (*l->pos == '=') {
                t.type = TOKEN_NEQ;
                strcpy(t.value, "!=");
                l->pos++; l->column++;
            } else {
                t.type = TOKEN_NOT;
                strcpy(t.value, "!");
            }
            return t;
            
        case '&':
            l->pos++; l->column++;
            if (*l->pos == '&') {
                t.type = TOKEN_AND;
                strcpy(t.value, "&&");
                l->pos++; l->column++;
            } else {
                t.type = TOKEN_UNKNOWN;
                strcpy(t.value, "&");
            }
            return t;
            
        case '|':
            l->pos++; l->column++;
            if (*l->pos == '|') {
                t.type = TOKEN_OR;
                strcpy(t.value, "||");
                l->pos++; l->column++;
            } else {
                t.type = TOKEN_UNKNOWN;
                strcpy(t.value, "|");
            }
            return t;
            
        case '<':
            t.type = TOKEN_LT;
            strcpy(t.value, "<");
            l->pos++; l->column++;
            if (*l->pos == '=') {
                t.type = TOKEN_LTE;
                strcpy(t.value, "<=");
                l->pos++; l->column++;
            }
            return t;
            
        case '>':
            t.type = TOKEN_GT;
            strcpy(t.value, ">");
            l->pos++; l->column++;
            if (*l->pos == '=') {
                t.type = TOKEN_GTE;
                strcpy(t.value, ">=");
                l->pos++; l->column++;
            }
            return t;
            
        case '+':
            t.type = TOKEN_PLUS;
            strcpy(t.value, "+");
            l->pos++; l->column++;
            if (*l->pos == '+') {
                t.type = TOKEN_INC;
                strcpy(t.value, "++");
                l->pos++; l->column++;
            }
            return t;
            
        case '-':
            t.type = TOKEN_MINUS;
            strcpy(t.value, "-");
            l->pos++; l->column++;
             
            if (*l->pos == '-') {
                t.type = TOKEN_DEC;
                strcpy(t.value, "--");
                l->pos++; l->column++;
            }
            else if (*l->pos == '>') {
                t.type = TOKEN_ARROW;
                strcpy(t.value, "->");
                l->pos++; l->column++;
            }
            return t;
            
            
        case '*':
            t.type = TOKEN_STAR;
            strcpy(t.value, "*");
            l->pos++; l->column++;
            return t;
            
        case '/':
            t.type = TOKEN_SLASH;
            strcpy(t.value, "/");
            l->pos++; l->column++;
            if (*l->pos == '/') {
                t.type = TOKEN_COMMENT;
                strcpy(t.value, "//");
                l->pos++; l->column++;
                while (*l->pos != '\n' && *l->pos != '\0') {
                    l->pos++;
                    l->column++;
                }
            }
            return t;
            
            case '%':
    t.type = TOKEN_PERCENT;
    strcpy(t.value, "%");
    l->pos++; l->column++;
    return t;
            
        case '{': t.type = TOKEN_LBRACE; strcpy(t.value, "{"); l->pos++; l->column++; return t;
        case '}': t.type = TOKEN_RBRACE; strcpy(t.value, "}"); l->pos++; l->column++; return t;
        case '(': t.type = TOKEN_LPAREN; strcpy(t.value, "("); l->pos++; l->column++; return t;
        case ')': t.type = TOKEN_RPAREN; strcpy(t.value, ")"); l->pos++; l->column++; return t;
        case '[': t.type = TOKEN_LBRACKET; strcpy(t.value, "["); l->pos++; l->column++; return t;
        case ']': t.type = TOKEN_RBRACKET; strcpy(t.value, "]"); l->pos++; l->column++; return t;
        case ',': t.type = TOKEN_COMMA; strcpy(t.value, ","); l->pos++; l->column++; return t;
        case '.': t.type = TOKEN_DOT; strcpy(t.value, "."); l->pos++; l->column++; return t;
        case ';': t.type = TOKEN_SEMICOLON; strcpy(t.value, ";"); l->pos++; l->column++; return t;
        case ':': t.type = TOKEN_COLON; strcpy(t.value, ":"); l->pos++; l->column++; return t;
        
        default:
            t.type = TOKEN_UNKNOWN;
            t.value[0] = *l->pos;
            t.value[1] = '\0';
            l->pos++; l->column++;
            return t;
    }
}
// ============================================================
//                        AST FUNCTIONS
// ============================================================
Node* create_node(NodeType type, char* value, int line, int column) {
    Node *node = (Node*)calloc(1, sizeof(Node));
    node->type = type;
    if (value) strcpy(node->value, value);
    node->line = line;
    node->column = column;
    return node;
}

// ============================================================
//                      TYPES
// ============================================================
#define TYPE_INT     0
#define TYPE_FLOAT   1
#define TYPE_STRING  2
#define TYPE_BOOLEAN 3
#define TYPE_CHAR    4
#define TYPE_AUTO    5
// ============================================================
//                 TYPE CHECKING
// ============================================================
// type compare
// returning:
//   1 — OK
//   0 — error
// ============================================================

int check_binary_op(const char* op, const char* lt, const char* rt) {
    // int, float, boolean, char — numbers
    int l_numeric = (strcmp(lt, "int") == 0 || 
                     strcmp(lt, "float") == 0 ||
                     strcmp(lt, "bool") == 0 ||
                     strcmp(lt, "char") == 0);
    int r_numeric = (strcmp(rt, "int") == 0 || 
                     strcmp(rt, "float") == 0 ||
                     strcmp(rt, "bool") == 0 ||
                     strcmp(rt, "char") == 0);
    
    // ---- arithmetic ----
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
    strcmp(op, "*") == 0 || strcmp(op, "/") == 0 ||
    strcmp(op, "%") == 0) {
        
        // numbers -> ok
        if (l_numeric && r_numeric) return 1;
        
        // strings -> error
        if (strcmp(lt, "string") == 0 && strcmp(rt, "string") == 0) {
            return 0;
        }
        
        // number and string error
        if (l_numeric && strcmp(rt, "string") == 0) return 0;
        if (strcmp(lt, "string") == 0 && r_numeric) return 0;
        
        // else -> error
        return 0;
    }
    
    // ---- comparison ----
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) {
        // numbers -> ok
        if (l_numeric && r_numeric) return 1;
        
        //  strings -> ok
        if (strcmp(lt, "string") == 0 && strcmp(rt, "string") == 0) return 1;
        
        // else -> error
        return 0;
    }
    
    if (strcmp(op, ">") == 0 || strcmp(op, "<") == 0 ||
        strcmp(op, ">=") == 0 || strcmp(op, "<=") == 0) {
        
        // only numbers
        if (l_numeric && r_numeric) return 1;
        return 0;
    }
    
    return 0;  // else -> error
}
int detect_type(char* value) {
    if (value[0] == '\0') return TYPE_AUTO;
    if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0) return TYPE_BOOLEAN;
    if (strlen(value) == 1 && isprint(value[0])) return TYPE_CHAR;
    
    int is_int = 1, is_float = 0, has_dot = 0;
    for (int i = 0; value[i]; i++) {
        if (value[i] == '.') {
            if (has_dot) { is_float = 0; break; }
            has_dot = 1; is_float = 1;
        } else if (!isdigit(value[i]) && value[i] != '-') {
            is_int = 0; is_float = 0; break;
        }
    }
    if (is_int && !has_dot) return TYPE_INT;
    if (is_float) return TYPE_FLOAT;
    return TYPE_STRING;
}

char* get_c_type(Node* node) {
    if (node->left) {
    const char* t = get_type_expr(node->left);
    
    if (strcmp(t, "int") == 0)     return "int";
    if (strcmp(t, "float") == 0)   return "double";
    if (strcmp(t, "bool") == 0) return "int";
    if (strcmp(t, "char") == 0)    return "char";
    if (strcmp(t, "string read_file") == 0)  return "char*";
    
        switch (node->left->type) {
            case NODE_NULL:
                return "void*";
            case NODE_ARRAY: {
                Node* first = node->left->body;
                if (first) {
                    if (first->type == NODE_NUMBER) {
                        char* val = first->value;
                        for (int i = 0; val[i]; i++) {
                            if (val[i] == '.') return "double";
                        }
                        return "int";
                    }
                    else if (first->type == NODE_STRING) return "char*";
                    else if (first->type == NODE_BOOLEAN) return "int";
                    else if (first->type == NODE_CHAR) return "char";
                }
                
                return "int";
            }
            case NODE_CALL: {
    // call function
    const char* ret_type = get_type_expr(node->left);
    
    if (strcmp(ret_type, "int") == 0) return "int";
    if (strcmp(ret_type, "float") == 0) return "double";
    if (strcmp(ret_type, "string") == 0) return "char*";
    if (strcmp(ret_type, "bool") == 0) return "int";
    if (strcmp(ret_type, "char") == 0) return "char";
    return "int";
}

            default:
                break;
        }
    }

    if (node->buffer_size > 0) {
        return "char";
    }
    
    if (node->is_dynamic) {
        return "char*";
    }
    
    return "int";
}

// ============================================================
//                        PARSER
// ============================================================
Parser* parser_init(Lexer *l, char* source) {
    Parser *p = (Parser*)malloc(sizeof(Parser));
    p->lexer = l;
    p->current = lexer_next(l);
    p->error = error_init();
    p->source = source;
    p->brace_depth = 0;
    p->stack_top = 0;
    return p;
}

void parser_free(Parser *p) {
    if (p) {
        error_free(p->error);
        free(p);
    }
}

// save
void parser_save(Parser *p) {
    if (p->stack_top < PARSER_STACK_SIZE) {
        p->stack[p->stack_top].pos = p->lexer->pos;
        p->stack[p->stack_top].line = p->lexer->line;
        p->stack[p->stack_top].column = p->lexer->column;
        p->stack[p->stack_top].brace_depth = p->brace_depth;
        p->stack_top++;
    }
}

// advance
void parser_advance(Parser *p) {
    if (p->current.type == TOKEN_LBRACE) {
        p->brace_depth++;
    } else if (p->current.type == TOKEN_RBRACE) {
        p->brace_depth--;
    }
    
    p->current = lexer_next(p->lexer);
}

void parser_unadv(Parser *p) {
    if (p->stack_top == 0) {
        fprintf(stderr, "parser_unadv: stack is empty!\n");
        return;
    }

    p->stack_top--;
    ParserState s = p->stack[p->stack_top];

    // restore position
    p->lexer->pos = s.pos;
    p->lexer->line = s.line;
    p->lexer->column = s.column;
    p->brace_depth = s.brace_depth;

    // re-read token
    p->current = lexer_next(p->lexer);
}

// ============================================================
//                      ERROR RECOVERY
// ============================================================
void skip_to_sync(Parser* p) {
    int start_depth = p->brace_depth;
    int max_steps = 1000;
    int steps = 0;
    
    while (p->current.type != TOKEN_EOF && steps < max_steps) {
        steps++;
        
        if (p->current.type == TOKEN_RBRACE) {
            if (p->brace_depth - 1 < start_depth) {
                return;
            }
            parser_advance(p);
            continue;
        }
        
        if (p->current.type == TOKEN_CONST || 
            p->current.type == TOKEN_VAR ||
            p->current.type == TOKEN_IF ||
            p->current.type == TOKEN_WHILE ||
            p->current.type == TOKEN_FOR ||
            p->current.type == TOKEN_FN ||
            p->current.type == TOKEN_RETURN ||
            p->current.type == TOKEN_SAY ||
            p->current.type == TOKEN_SAY_EXCLAM ||
            p->current.type == TOKEN_IDENTIFIER) {
            return;
        }
        
        parser_advance(p);
    }
}
// ============================================================
//              PARSE BLOCK
// ============================================================
Node* parse_block(Parser* p, const char* scope_name) {
    int own_scope = (scope_name != NULL);
    if (own_scope) {
        scope_enter(scope_name);
    }
    
    if (p->current.type != TOKEN_LBRACE) {
        error_syntax(p->error, p->source, p->current, "{");
        skip_to_sync(p);
        if (own_scope) scope_exit();
        return NULL;
    }
    parser_advance(p);
    
    Node* block = create_node(NODE_BLOCK, "", 0, 0);
    Node* first = NULL;
    Node* last = NULL;
    
    while (p->current.type != TOKEN_RBRACE && p->current.type != TOKEN_EOF) {
        if (p->current.type == TOKEN_COMMENT) {
            parser_advance(p);
            continue;
        }
        
        Node* stmt = parse_statement(p);
        if (stmt) {
            if (!first) {
                first = stmt;
                last = stmt;
            } else {
                last->next = stmt;
                last = stmt;
            }
        }
    }
    
    if (p->current.type != TOKEN_RBRACE) {
        error_syntax(p->error, p->source, p->current, "}");
        skip_to_sync(p);
        if (own_scope) scope_exit();
        return NULL;
    }
    parser_advance(p);
    
    block->body = first;
    
    if (own_scope) scope_exit();
    
    return block;
}
// ============================================================
//              PARSE FUNCTION
// ============================================================
Node* parse_function(Parser* p) {
    parser_advance(p);
    
    Token name = p->current;
    if (name.type != TOKEN_IDENTIFIER) {
        error_expected_name(p->error, p->source, name);
        skip_to_sync(p);
        return NULL;
    }
    parser_advance(p);
    
    if (p->current.type != TOKEN_LPAREN) {
        error_syntax(p->error, p->source, p->current, "(");
        skip_to_sync(p);
        return NULL;
    }
    parser_advance(p);
    
    char fn_scope_name[128];
    snprintf(fn_scope_name, sizeof(fn_scope_name), "FN %s", name.value);
    scope_enter(fn_scope_name);
    
    Node* param_first = NULL;
    Node* param_last = NULL;
    
    while (p->current.type != TOKEN_RPAREN) {
        Token pname = p->current;
        if (pname.type != TOKEN_IDENTIFIER) {
            error_expected_variable(p->error, p->source, pname);
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        if (p->current.type != TOKEN_COLON) {
            error_syntax(p->error, p->source, p->current, ":");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        // ---- parse type ----
        char ptype[64] = "auto";
        
        if (p->current.type == TOKEN_FN) {
            // ---- fn type: fn(int, int) -> int ----
            parser_advance(p);
            
            if (p->current.type != TOKEN_LPAREN) {
                error_syntax(p->error, p->source, p->current, "(");
                skip_to_sync(p);
                return NULL;
            }
            parser_advance(p);
            
            char fn_args[256] = "";
            int fn_arg_count = 0;
            
            while (p->current.type != TOKEN_RPAREN && p->current.type != TOKEN_EOF) {
                const char* atype = "int";
                if (p->current.type == TOKEN_WORD_NUMBER) atype = "int";
                else if (p->current.type == TOKEN_WORD_FLOAT) atype = "float";
                else if (p->current.type == TOKEN_WORD_STRING) atype = "string";
                else if (p->current.type == TOKEN_WORD_BOOLEAN) atype = "bool";
                else if (p->current.type == TOKEN_WORD_CHAR) atype = "char";
                else if (p->current.type == TOKEN_WORD_VOID) atype = "void";
                else {
                    error_unknown_type(p->error, p->source, p->current);
                    skip_to_sync(p);
                    return NULL;
                }
                parser_advance(p);
                
                if (fn_arg_count > 0) strcat(fn_args, ",");
                strcat(fn_args, atype);
                fn_arg_count++;
                
                if (p->current.type == TOKEN_COMMA) parser_advance(p);
            }
            
            if (p->current.type != TOKEN_RPAREN) {
                error_syntax(p->error, p->source, p->current, ")");
                skip_to_sync(p);
                return NULL;
            }
            parser_advance(p);
            
            const char* fn_ret = "void";
            if (p->current.type == TOKEN_ARROW) {
                parser_advance(p);
                if (p->current.type == TOKEN_WORD_NUMBER) fn_ret = "int";
                else if (p->current.type == TOKEN_WORD_FLOAT) fn_ret = "float";
                else if (p->current.type == TOKEN_WORD_STRING) fn_ret = "string";
                else if (p->current.type == TOKEN_WORD_BOOLEAN) fn_ret = "bool";
                else if (p->current.type == TOKEN_WORD_CHAR) fn_ret = "char";
                else if (p->current.type == TOKEN_WORD_VOID) fn_ret = "void";
                else {
                    error_unknown_type(p->error, p->source, p->current);
                    skip_to_sync(p);
                    return NULL;
                }
                parser_advance(p);
            }
            
            snprintf(ptype, sizeof(ptype), "fn(%s)->%s", fn_args, fn_ret);
        }
        else if (p->current.type == TOKEN_WORD_NUMBER) { strcpy(ptype, "int"); parser_advance(p); }
        else if (p->current.type == TOKEN_WORD_FLOAT) { strcpy(ptype, "float"); parser_advance(p); }
        else if (p->current.type == TOKEN_WORD_STRING) { strcpy(ptype, "string"); parser_advance(p); }
        else if (p->current.type == TOKEN_WORD_BOOLEAN) { strcpy(ptype, "bool"); parser_advance(p); }
        else if (p->current.type == TOKEN_WORD_CHAR) { strcpy(ptype, "char"); parser_advance(p); }
        else if (p->current.type == TOKEN_WORD_VOID) { strcpy(ptype, "void"); parser_advance(p); }
        else {
            error_unknown_type(p->error, p->source, p->current);
            skip_to_sync(p);
            return NULL;
        }
        
        Node* param = create_node(NODE_PARAM, pname.value, pname.line, pname.column);
        strcpy(param->data_type, ptype);
        
        add_symbol(pname.value, ptype, 1, pname.line, pname.column);
        
        if (!param_first) {
            param_first = param;
            param_last = param;
        } else {
            param_last->next = param;
            param_last = param;
        }
        
        if (p->current.type == TOKEN_COMMA) {
            parser_advance(p);
        }
    }
    
    if (p->current.type != TOKEN_RPAREN) {
        error_syntax(p->error, p->source, p->current, ")");
        skip_to_sync(p);
        return NULL;
    }
    parser_advance(p);
    
    // ---- return type ----
    char return_type[20] = "void";
    
    if (p->current.type == TOKEN_ARROW) {
        parser_advance(p);
        
        if (p->current.type == TOKEN_WORD_NUMBER) strcpy(return_type, "int");
        else if (p->current.type == TOKEN_WORD_FLOAT) strcpy(return_type, "float");
        else if (p->current.type == TOKEN_WORD_STRING) strcpy(return_type, "string");
        else if (p->current.type == TOKEN_WORD_BOOLEAN) strcpy(return_type, "bool");
        else if (p->current.type == TOKEN_WORD_CHAR) strcpy(return_type, "char");
        else if (p->current.type == TOKEN_WORD_VOID) strcpy(return_type, "void");
        else if (p->current.type == TOKEN_IDENTIFIER) {
            strcpy(return_type, p->current.value);
        } else {
            error_unknown_type(p->error, p->source, p->current);
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
    }
    
    Node* body = parse_block(p, NULL);
    if (!body) return NULL;
    
    Node* fn = create_node(NODE_FN, name.value, name.line, name.column);
    strcpy(fn->data_type, return_type);
    fn->left = param_first;
    fn->body = body;
    
    int pcount = 0;
    Node* pnode = param_first;
    while (pnode) {
        pcount++;
        pnode = pnode->next;
    }
    
    scope_exit();
    
    add_func(name.value, return_type, param_first, pcount, name.line, name.column);
    
    
    return fn;
}

// ============================================================
//              PARSE CONDITION (with && and ||)
// ============================================================
// Precedence (lowest to highest):
//   || (lowest)
//   &&
//   == != < > <= >=
//   ! (highest)

Node* parse_cond_or(Parser* p);
Node* parse_cond_and(Parser* p);
Node* parse_cond_cmp(Parser* p);
Node* parse_cond_primary(Parser* p);

Node* parse_cond_or(Parser* p) {
    Node* left = parse_cond_and(p);
    if (!left) return NULL;
    
    while (p->current.type == TOKEN_OR) {
        Token op = p->current;
        parser_advance(p);
        
        Node* right = parse_cond_and(p);
        if (!right) return NULL;
        
        Node* node = create_node(NODE_LOGICAL_OP, "||", op.line, op.column);
        node->left = left;
        node->right = right;
        left = node;
    }
    
    return left;
}

Node* parse_cond_and(Parser* p) {
    Node* left = parse_cond_cmp(p);
    if (!left) return NULL;
    
    while (p->current.type == TOKEN_AND) {
        Token op = p->current;
        parser_advance(p);
        
        Node* right = parse_cond_cmp(p);
        if (!right) return NULL;
        
        Node* node = create_node(NODE_LOGICAL_OP, "&&", op.line, op.column);
        node->left = left;
        node->right = right;
        left = node;
    }
    
    return left;
}

Node* parse_cond_cmp(Parser* p) {
    Node* left = parse_cond_primary(p);
    if (!left) return NULL;
    
    if (p->current.type == TOKEN_EQ_EQ || p->current.type == TOKEN_NEQ ||
        p->current.type == TOKEN_LT || p->current.type == TOKEN_GT ||
        p->current.type == TOKEN_LTE || p->current.type == TOKEN_GTE) {
        
        Token op = p->current;
        parser_advance(p);
        
        Node* right = parse_cond_primary(p);
        if (!right) return NULL;
        
        const char* lt = get_type_expr(left);
        const char* rt = get_type_expr(right);
        
        if (!check_binary_op(op.value, lt, rt)) {
            char msg[512];
            sprintf(msg, "can't compare %s and %s", lt, rt);
            error_report(p->error, p->source, op.line, op.column, msg);
            skip_to_sync(p);
            return NULL;
        }
        
        Node* cmp = create_node(NODE_COMPARE, op.value, op.line, op.column);
        cmp->left = left;
        cmp->right = right;
        return cmp;
    }
    
    return left;
}

Node* parse_cond_primary(Parser* p) {
    // NOT
    if (p->current.type == TOKEN_NOT) {
        Token not_t = p->current;
        parser_advance(p);
        
        Node* inner = parse_cond_primary(p);
        if (!inner) return NULL;
        
        Node* node = create_node(NODE_NOT, "", not_t.line, not_t.column);
        node->left = inner;
        return node;
    }
    
     // ---- ( ... ) ----
if (p->current.type == TOKEN_LPAREN) {
    parser_advance(p);
    
    Node* inner = parse_cond_or(p);
    if (!inner) return NULL;
    
    if (p->current.type != TOKEN_RPAREN) {
        error_syntax(p->error, p->source, p->current, ")");
        skip_to_sync(p);
        return NULL;
    }
    parser_advance(p);
    return inner;
}

// ---- IDENTIFIER ----
if (p->current.type == TOKEN_IDENTIFIER) {
    Token var_t = p->current;
    parser_advance(p);
    
    // ---- call function ----
    if (p->current.type == TOKEN_LPAREN) {
        parser_advance(p);
        
        Node* call = create_node(NODE_CALL, var_t.value, var_t.line, var_t.column);
        Node* first_arg = NULL;
        Node* last_arg = NULL;
        
        while (p->current.type != TOKEN_RPAREN && p->current.type != TOKEN_EOF) {
            Node* arg = parse_expression(p);
            if (!arg) return NULL;
            
            if (!first_arg) {
                first_arg = arg;
                last_arg = arg;
            } else {
                last_arg->next = arg;
                last_arg = arg;
            }
            
            if (p->current.type == TOKEN_COMMA) {
                parser_advance(p);
            } else {
                break;
            }
        }
        
        if (p->current.type != TOKEN_RPAREN) {
            error_syntax(p->error, p->source, p->current, ")");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        call->left = first_arg;
        return call;
    }
    
    // ---- arr[i] ----
    Node* base = create_node(NODE_IDENTIFIER, var_t.value, var_t.line, var_t.column);
    
    Symbol* sym = find_symbol(var_t.value);
    if (sym) {
        base->resolved_symbol = sym;
    }
    
    if (p->current.type == TOKEN_LBRACKET) {
        parser_advance(p);
        
        Node* index = parse_expression(p);
        if (!index) return NULL;
        
        if (p->current.type != TOKEN_RBRACKET) {
            error_syntax(p->error, p->source, p->current, "]");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        Node* idx = create_node(NODE_STR_INDEX, "", var_t.line, var_t.column);
        idx->left = base;
        idx->right = index;
        return idx;
    }
    
    return base;
}
    
    // BOOLEAN
    if (p->current.type == TOKEN_BOOLEAN) {
        Node* node = create_node(NODE_BOOLEAN, p->current.value, 
                                  p->current.line, p->current.column);
        parser_advance(p);
        return node;
    }
    
    // NUMBER / FLOAT
    if (p->current.type == TOKEN_NUMBER || p->current.type == TOKEN_FLOAT) {
        Node* node = create_node(NODE_NUMBER, p->current.value,
                                  p->current.line, p->current.column);
        parser_advance(p);
        return node;
    }
    
    // STRING
    if (p->current.type == TOKEN_STRING) {
        Node* node = create_node(NODE_STRING, p->current.value,
                                  p->current.line, p->current.column);
        parser_advance(p);
        return node;
    }
    
    // CHAR
    if (p->current.type == TOKEN_CHAR) {
        Node* node = create_node(NODE_CHAR, p->current.value,
                                  p->current.line, p->current.column);
        parser_advance(p);
        return node;
    }
    
    error_expected_expression(p->error, p->source, p->current);
    skip_to_sync(p);
    return NULL;
}

Node* parse_condition(Parser* p) {
    return parse_cond_or(p);
}

// ============================================================
//              PARSE EXPRESSION
// ============================================================
Node* parse_expression(Parser* p) {
    Token t = p->current;
    Node* expr = NULL;
    int line = t.line;
    int col = t.column;

// ---- (expr) ----
if (t.type == TOKEN_LPAREN) {
    parser_advance(p);
    
    Node* inner = parse_expression(p);
    if (!inner) return NULL;
    
    if (p->current.type != TOKEN_RPAREN) {
        error_syntax(p->error, p->source, p->current, ")");
        skip_to_sync(p);
        return NULL;
    }
    parser_advance(p);
    expr = inner;
}
else if (t.type == TOKEN_NUMBER || t.type == TOKEN_FLOAT) {
        expr = create_node(NODE_NUMBER, t.value, line, col);
        parser_advance(p);
    }
    else if (t.type == TOKEN_STRING) {
        expr = create_node(NODE_STRING, t.value, line, col);
        parser_advance(p);
    }
    else if (t.type == TOKEN_BOOLEAN) {
        expr = create_node(NODE_BOOLEAN, t.value, line, col);
        parser_advance(p);
    }
    else if (t.type == TOKEN_CHAR) {
        expr = create_node(NODE_CHAR, t.value, line, col);
        parser_advance(p);
    }
    
    else if (t.type == TOKEN_READ_FILE) {
    parser_advance(p);
    
    if (p->current.type != TOKEN_LPAREN) {
        error_syntax(p->error, p->source, p->current, "(");
        skip_to_sync(p);
        return NULL;
    }
    parser_advance(p);
    
    if (p->current.type != TOKEN_STRING) {
        error_expected_filename(p->error, p->source, p->current);
        skip_to_sync(p);
        return NULL;
    }
    
    char filename[200];
    strcpy(filename, p->current.value);
    parser_advance(p);
    
    if (p->current.type != TOKEN_RPAREN) {
        error_syntax(p->error, p->source, p->current, ")");
        skip_to_sync(p);
        return NULL;
    }
    parser_advance(p);
    
    expr = create_node(NODE_READ_FILE, filename, t.line, t.column);
}

    else if (t.type == TOKEN_IDENTIFIER) {
        expr = create_node(NODE_IDENTIFIER, t.value, line, col);
        
        Symbol* sym = find_symbol(t.value);
    if (sym) {
        expr->resolved_symbol = sym;
    }
        
        parser_advance(p);
        
        if (p->current.type == TOKEN_LPAREN) {
            parser_advance(p);
            
            Node* call = create_node(NODE_CALL, t.value, line, col);
            Node* first_arg = NULL;
            Node* last_arg = NULL;
            
            while (p->current.type != TOKEN_RPAREN && p->current.type != TOKEN_EOF) {
                Node* arg = parse_expression(p);
                if (!arg) return NULL;
                
                if (!first_arg) {
                    first_arg = arg;
                    last_arg = arg;
                } else {
                    last_arg->next = arg;
                    last_arg = arg;
                }
                
                if (p->current.type == TOKEN_COMMA) {
                    parser_advance(p);
                }
            }
            
            if (p->current.type != TOKEN_RPAREN) {
                error_syntax(p->error, p->source, p->current, ")");
                skip_to_sync(p);
                return NULL;
            }
            parser_advance(p);
            
            call->left = first_arg;
            expr = call;
        }
        else if (p->current.type == TOKEN_LBRACKET) {
            parser_advance(p);
            
            Node* index = parse_expression(p);
            if (!index) return NULL;
            
            if (p->current.type != TOKEN_RBRACKET) {
                error_syntax(p->error, p->source, p->current, "]");
                skip_to_sync(p);
                return NULL;
            }
            parser_advance(p);
            
            Node* index_node = create_node(NODE_STR_INDEX, "", line, col);
            index_node->left = expr;
            index_node->right = index;
            
            expr = index_node;
            
            while (p->current.type == TOKEN_LBRACKET) {
                parser_advance(p);
                
                Node* next_index = parse_expression(p);
                if (!next_index) return NULL;
                
                if (p->current.type != TOKEN_RBRACKET) {
                    error_syntax(p->error, p->source, p->current, "]");
                    skip_to_sync(p);
                    return NULL;
                }
                parser_advance(p);
                
                Node* new_index_node = create_node(NODE_STR_INDEX, "", line, col);
                new_index_node->left = expr;
                new_index_node->right = next_index;
                expr = new_index_node;
            }
        }
        else if (p->current.type == TOKEN_DOT) {
            parser_advance(p);
            
            if (p->current.type == TOKEN_IDENTIFIER) {
                Token field = p->current;
                parser_advance(p);
                
                Node* access = create_node(NODE_ENUM_ACCESS, field.value, field.line, field.column);
                access->left = expr;
                expr = access;
            } else {
                error_expected_expression(p->error, p->source, p->current);
                skip_to_sync(p);
                return NULL;
            }
        }
    }
    else if (t.type == TOKEN_LBRACKET) {
        parser_advance(p);
        
        Node* first = NULL;
        Node* last = NULL;
        
        while (p->current.type != TOKEN_RBRACKET && p->current.type != TOKEN_EOF) {
            Node* elem = parse_expression(p);
            if (!elem) return NULL;
            
            if (!first) {
                first = elem;
                last = elem;
            } else {
                last->next = elem;
                last = elem;
            }
            
            if (p->current.type == TOKEN_COMMA) {
                parser_advance(p);
            }
        }
        
        if (p->current.type != TOKEN_RBRACKET) {
            error_syntax(p->error, p->source, p->current, "]");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        expr = create_node(NODE_ARRAY, "", t.line, t.column);
        expr->body = first;
    }
    else {
        error_expected_expression(p->error, p->source, t);
        skip_to_sync(p);
        return NULL;
    }

    // ---- binary operators ----
while (p->current.type == TOKEN_PLUS || p->current.type == TOKEN_MINUS ||
       p->current.type == TOKEN_STAR || p->current.type == TOKEN_SLASH ||
       p->current.type == TOKEN_PERCENT) {
    Token op = p->current;
    parser_advance(p);
    Node* right = parse_expression(p);
    if (!right) return NULL;
    
    // ---- checking type ----
    const char* lt = get_type_expr(expr);
    const char* rt = get_type_expr(right);
    
    if (!check_binary_op(op.value, lt, rt)) {
        char msg[512];
        sprintf(msg, "can't %s %s and %s", 
        op.value[0] == '+' ? "add" :
        op.value[0] == '-' ? "subtract" :
        op.value[0] == '*' ? "multiply" :
        op.value[0] == '%' ? "modulo" : "divide",
        lt, rt);
        error_report(p->error, p->source, op.line, op.column, msg);
        skip_to_sync(p);
        return NULL;
    }
    
    Node* bin = create_node(NODE_BINARY_OP, op.value, op.line, op.column);
    bin->left = expr;
    bin->right = right;
    expr = bin;
}
// ---- compare ----
    if (p->current.type == TOKEN_EQ_EQ || p->current.type == TOKEN_NEQ ||
        p->current.type == TOKEN_LT || p->current.type == TOKEN_GT ||
        p->current.type == TOKEN_LTE || p->current.type == TOKEN_GTE) {
        
        Token op = p->current;
        parser_advance(p);
        
        Node* right = parse_expression(p);
        if (!right) return NULL;
        
        const char* lt = get_type_expr(expr);
        const char* rt = get_type_expr(right);
        
        if (!check_binary_op(op.value, lt, rt)) {
            char msg[512];
            sprintf(msg, "can't compare %s and %s", lt, rt);
            error_report(p->error, p->source, op.line, op.column, msg);
            skip_to_sync(p);
            return NULL;
        }
        
        Node* cmp = create_node(NODE_COMPARE, op.value, op.line, op.column);
        cmp->left = expr;
        cmp->right = right;
        expr = cmp;
    }
    
    // ---- && and || ----
    while (p->current.type == TOKEN_AND || p->current.type == TOKEN_OR) {
        Token op = p->current;
        parser_advance(p);
        
        Node* right = parse_expression(p);
        if (!right) return NULL;
        
        Node* logical = create_node(NODE_LOGICAL_OP, op.value, op.line, op.column);
        logical->left = expr;
        logical->right = right;
        expr = logical;
    }
    
    return expr;
}
// ============================================================
//              PARSE STATEMENT
// ============================================================
Node* parse_statement(Parser* p) {
    while (p->current.type == TOKEN_COMMENT) {
        parser_advance(p);
    }
    
    Token t = p->current;
    Node* stmt = NULL;
    
    // ---- function declaration inside block ----
if (t.type == TOKEN_FN) {
    return parse_function(p);
}
   else if (t.type == TOKEN_LBRACE) {
        return parse_block(p, "BLOCK");
    }
    
    else if (t.type == TOKEN_RETURN) {
    parser_advance(p);
    
    Node* expr = NULL;
    if (p->current.type != TOKEN_RBRACE && p->current.type != TOKEN_SEMICOLON) {
        expr = parse_condition(p);
    }
    
    stmt = create_node(NODE_RETURN, "", t.line, t.column);
    stmt->left = expr;
    return stmt;
}
    
    else if (t.type == TOKEN_CONST || t.type == TOKEN_VAR) {
        parser_advance(p);
        
        Token name = p->current;
        if (name.type != TOKEN_IDENTIFIER) {
            error_expected_variable(p->error, p->source, name);
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        int buffer_size = 0;
        int is_dynamic = 0;
        
        if (p->current.type == TOKEN_LBRACKET) {
            parser_advance(p);
            
            if (p->current.type == TOKEN_NUMBER) {
                buffer_size = atoi(p->current.value);
                parser_advance(p);
            } else if (p->current.type == TOKEN_RBRACKET) {
                is_dynamic = 1;
            } else {
                error_expected_number(p->error, p->source, p->current);
                skip_to_sync(p);
                return NULL;
            }
            
            if (p->current.type != TOKEN_RBRACKET) {
                error_syntax(p->error, p->source, p->current, "]");
                skip_to_sync(p);
                return NULL;
            }
            parser_advance(p);
        }
        
        char data_type[20] = "auto";
        if (p->current.type == TOKEN_COLON) {
            parser_advance(p);
            
            if (p->current.type == TOKEN_WORD_NUMBER) strcpy(data_type, "int");
            else if (p->current.type == TOKEN_WORD_FLOAT) strcpy(data_type, "float");
            else if (p->current.type == TOKEN_WORD_STRING) strcpy(data_type, "string");
            else if (p->current.type == TOKEN_WORD_BOOLEAN) strcpy(data_type, "bool");
            else if (p->current.type == TOKEN_WORD_CHAR) strcpy(data_type, "char");
            else {
                error_unknown_type(p->error, p->source, p->current);
                skip_to_sync(p);
                return NULL;
            }
            parser_advance(p);
        }
        
NodeType node_type = (t.type == TOKEN_CONST) ? NODE_CONST : NODE_VAR;
stmt = create_node(node_type, name.value, name.line, name.column);
stmt->buffer_size = buffer_size;
stmt->is_dynamic = is_dynamic;
stmt->is_declaration = 1;
strcpy(stmt->data_type, data_type);

if (p->current.type == TOKEN_EQ) {
    parser_advance(p);
    Node* expr = parse_expression(p);
    if (!expr) return NULL;
    stmt->left = expr;
    
    // auto
    if (strcmp(data_type, "auto") == 0) {
        strcpy(data_type, get_type_expr(expr));
        strcpy(stmt->data_type, data_type);
    }
    
    // ★ checking: buffer_size for ARRAY
if (expr->type == NODE_ARRAY) {
    if (buffer_size == 0 && !is_dynamic) {
        error_report(p->error, p->source, 
                     name.line, name.column,
                     "Array must specify size: name[N] or name[]");
        skip_to_sync(p);
        return NULL;
    }
    
    // if [], calculate
    if (is_dynamic) {
        int count = 0;
        Node* elem = expr->body;
        while (elem) { count++; elem = elem->next; }
        buffer_size = count;
        stmt->buffer_size = count;
    }
}

// ★ checking: buffer_size for STRING
if (expr->type == NODE_STRING) {
    if (buffer_size == 0 && !is_dynamic) {
        error_report(p->error, p->source, 
                     name.line, name.column,
                     "string must specify size: name[N] or name[]");
        skip_to_sync(p);
        return NULL;
    }
    
    // if [], calculate length from string
    if (is_dynamic) {
        buffer_size = strlen(expr->value) + 1;  // +1 for \0
        stmt->buffer_size = buffer_size;
        }
    }
}
 

// write to symtab
add_symbol(name.value, data_type, t.type == TOKEN_CONST, name.line, name.column);

return stmt;
    }
    
    else if (strcmp(t.value, "arrlen") == 0) {
        parser_advance(p);
        
        if (p->current.type != TOKEN_LPAREN) {
            error_syntax(p->error, p->source, p->current, "(");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        Node* arg = parse_expression(p);
        if (!arg) return NULL;
        
        if (p->current.type == TOKEN_COMMA) {
            error_report(p->error, p->source, p->current.line, p->current.column, "too many arguments");
            skip_to_sync(p);
            return NULL;
        }
        
        if (p->current.type != TOKEN_RPAREN) {
            error_syntax(p->error, p->source, p->current, ")");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        Node* node = create_node(NODE_ARRLEN, "", t.line, t.column);
        node->left = arg;
        return node;
    }
    
// ============================================================
//                          FATAL
// ============================================================
else if (t.type == TOKEN_FATAL) {
    parser_advance(p);
    
    if (p->current.type != TOKEN_LPAREN) {
        error_syntax(p->error, p->source, p->current, "(");
        skip_to_sync(p);
        return NULL;
    }
    parser_advance(p);
    
    // ── many args ──
    Node* first_arg = NULL;
    Node* last_arg = NULL;
    
    while (p->current.type != TOKEN_RPAREN && 
           p->current.type != TOKEN_EOF) {
        
        Node* arg = parse_expression(p);
        if (!arg) return NULL;
        
        if (!first_arg) {
            first_arg = arg;
            last_arg = arg;
        } else {
            last_arg->next = arg;
            last_arg = arg;
        }
        
        if (p->current.type == TOKEN_COMMA) {
            parser_advance(p);
        } else {
            break;
        }
    }
    
    if (p->current.type != TOKEN_RPAREN) {
        error_syntax(p->error, p->source, p->current, ")");
        skip_to_sync(p);
        return NULL;
    }
    parser_advance(p);
    
    stmt = create_node(NODE_FATAL, "", t.line, t.column);
    stmt->left = first_arg;
    return stmt;
}
    
    else if (t.type == TOKEN_ENUM) {
        parser_advance(p);
        
        Token name = p->current;
        if (name.type != TOKEN_IDENTIFIER) {
            error_expected_name(p->error, p->source, name);
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        if (p->current.type != TOKEN_LBRACE) {
            error_syntax(p->error, p->source, p->current, "{");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        Node* first = NULL;
        Node* last = NULL;
        int value = 0;
        
        while (p->current.type != TOKEN_RBRACE) {
            Token item = p->current;
            if (item.type != TOKEN_IDENTIFIER) {
                error_expected_name(p->error, p->source, item);
                skip_to_sync(p);
                return NULL;
            }
            parser_advance(p);
            
            if (p->current.type == TOKEN_EQ) {
                parser_advance(p);
                if (p->current.type == TOKEN_NUMBER) {
                    value = atoi(p->current.value);
                    parser_advance(p);
                } else {
                    error_expected_number(p->error, p->source, p->current);
                    skip_to_sync(p);
                    return NULL;
                }
            }
            
            Node* enum_item = create_node(NODE_ENUM_ITEM, item.value, item.line, item.column);
            char val[20];
            sprintf(val, "%d", value);
            strcpy(enum_item->data_type, val);
            
            if (!first) {
                first = enum_item;
                last = enum_item;
            } else {
                last->next = enum_item;
                last = enum_item;
            }
            
            value++;
            
            if (p->current.type == TOKEN_COMMA) {
                parser_advance(p);
            }
        }
        
        if (p->current.type != TOKEN_RBRACE) {
            error_syntax(p->error, p->source, p->current, "}");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        Node* enum_node = create_node(NODE_ENUM, name.value, name.line, name.column);
        enum_node->body = first;
        return enum_node;
    }
    
    else if (t.type == TOKEN_INPUT) {
        parser_advance(p);
        
        if (p->current.type != TOKEN_LPAREN) {
            error_syntax(p->error, p->source, p->current, "(");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        if (p->current.type != TOKEN_IDENTIFIER) {
            error_expected_variable(p->error, p->source, p->current);
            skip_to_sync(p);
            return NULL;
        }
        
        char var_name[200];
        strcpy(var_name, p->current.value);
        parser_advance(p);
        
        if (p->current.type != TOKEN_COMMA) {
            error_syntax(p->error, p->source, p->current, ",");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        char input_type[20];
        if (p->current.type == TOKEN_WORD_NUMBER) strcpy(input_type, "int");
        else if (p->current.type == TOKEN_WORD_FLOAT) strcpy(input_type, "float");
        else if (p->current.type == TOKEN_WORD_STRING) strcpy(input_type, "string");
        else if (p->current.type == TOKEN_WORD_BOOLEAN) strcpy(input_type, "bool");
        else if (p->current.type == TOKEN_WORD_CHAR) strcpy(input_type, "char");
        else {
            error_unknown_type(p->error, p->source, p->current);
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        if (p->current.type == TOKEN_COMMA) {
            error_report(p->error, p->source, p->current.line, p->current.column, "too many arguments");
            skip_to_sync(p);
            return NULL;
        }
        
        if (p->current.type != TOKEN_RPAREN) {
            error_syntax(p->error, p->source, p->current, ")");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        stmt = create_node(NODE_INPUT, var_name, t.line, t.column);
        strcpy(stmt->data_type, input_type);
        return stmt;
    }
    
    else if (t.type == TOKEN_STRLEN || t.type == TOKEN_STRUPPER || 
             t.type == TOKEN_STRLOWER || t.type == TOKEN_STRTRIM) {
        parser_advance(p);
        
        if (p->current.type != TOKEN_LPAREN) {
            error_syntax(p->error, p->source, p->current, "(");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        Node* arg = parse_expression(p);
        if (!arg) return NULL;
        
        if (p->current.type == TOKEN_COMMA) {
            error_report(p->error, p->source, p->current.line, p->current.column, "too many arguments");
            skip_to_sync(p);
            return NULL;
        }
        
        if (p->current.type != TOKEN_RPAREN) {
            error_syntax(p->error, p->source, p->current, ")");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        NodeType type = 0;
        if (t.type == TOKEN_STRLEN) type = NODE_STRLEN;
        else if (t.type == TOKEN_STRUPPER) type = NODE_STRUPPER;
        else if (t.type == TOKEN_STRLOWER) type = NODE_STRLOWER;
        else if (t.type == TOKEN_STRTRIM) type = NODE_STRTRIM;
        
        stmt = create_node(type, "", t.line, t.column);
        stmt->left = arg;
        return stmt;
    }
    
    else if (t.type == TOKEN_CREATE_FILE || t.type == TOKEN_APPEND_FILE || 
             t.type == TOKEN_WRITE_FILE) {
        parser_advance(p);
        
        if (p->current.type != TOKEN_LPAREN) {
            error_syntax(p->error, p->source, p->current, "(");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        if (p->current.type != TOKEN_STRING) {
            error_expected_filename(p->error, p->source, p->current);
            skip_to_sync(p);
            return NULL;
        }
        
        char filename[200];
        strcpy(filename, p->current.value);
        parser_advance(p);
        
        if (p->current.type == TOKEN_COMMA) {
            error_report(p->error, p->source, p->current.line, p->current.column, "too many arguments");
            skip_to_sync(p);
            return NULL;
        }
        
        if (p->current.type != TOKEN_RPAREN) {
            error_syntax(p->error, p->source, p->current, ")");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        Node* body = parse_block(p, "FILE BODY");
        if (!body) return NULL;
        
        NodeType file_type = 0;
        if (t.type == TOKEN_CREATE_FILE) file_type = NODE_CREATE_FILE;
        else if (t.type == TOKEN_APPEND_FILE) file_type = NODE_APPEND_FILE;
        else if (t.type == TOKEN_WRITE_FILE) file_type = NODE_WRITE_FILE;
        
        stmt = create_node(file_type, filename, t.line, t.column);
        stmt->body = body->body;
        return stmt;
    }
    
    else if (t.type == TOKEN_CLS || t.type == TOKEN_PRESS) {
        parser_advance(p);
        
        if (p->current.type != TOKEN_LPAREN) {
            error_syntax(p->error, p->source, p->current, "(");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        if (p->current.type != TOKEN_RPAREN) {
            error_syntax(p->error, p->source, p->current, ")");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        NodeType type = (t.type == TOKEN_CLS) ? NODE_CLS : NODE_PRESS;
        stmt = create_node(type, "", t.line, t.column);
        return stmt;
    }
    
    // ============================================================
//                      FSAY / FSAY!
// ============================================================
else if (t.type == TOKEN_FSAY || t.type == TOKEN_FSAY_EXCLAM) {
    parser_advance(p);
    
    if (p->current.type != TOKEN_LPAREN) {
        error_syntax(p->error, p->source, p->current, "(");
        skip_to_sync(p);
        return NULL;
    }
    parser_advance(p);
    
    // ── Много аргументов ──
    Node* first_arg = NULL;
    Node* last_arg = NULL;
    
    while (p->current.type != TOKEN_RPAREN && 
           p->current.type != TOKEN_EOF) {
        
        Node* arg = parse_expression(p);
        if (!arg) return NULL;
        
        if (!first_arg) {
            first_arg = arg;
            last_arg = arg;
        } else {
            last_arg->next = arg;
            last_arg = arg;
        }
        
        if (p->current.type == TOKEN_COMMA) {
            parser_advance(p);
        } else {
            break;
        }
    }
    
    if (p->current.type != TOKEN_RPAREN) {
        error_syntax(p->error, p->source, p->current, ")");
        skip_to_sync(p);
        return NULL;
    }
    parser_advance(p);
    
    NodeType type = (t.type == TOKEN_FSAY) ? NODE_FSAY : NODE_FSAY_EXCLAM;
    stmt = create_node(type, "", t.line, t.column);
    stmt->left = first_arg;
    return stmt;
}
    
    else if (t.type == TOKEN_SSLEEP || t.type == TOKEN_MSLEEP) {
        parser_advance(p);
        
        if (p->current.type != TOKEN_LPAREN) {
            error_syntax(p->error, p->source, p->current, "(");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        if (p->current.type != TOKEN_NUMBER) {
            error_expected_number(p->error, p->source, p->current);
            skip_to_sync(p);
            return NULL;
        }
        
        char num[20];
        strcpy(num, p->current.value);
        parser_advance(p);
        
        if (p->current.type == TOKEN_COMMA) {
            error_report(p->error, p->source, p->current.line, p->current.column, "too many arguments");
            skip_to_sync(p);
            return NULL;
        }
        
        if (p->current.type != TOKEN_RPAREN) {
            error_syntax(p->error, p->source, p->current, ")");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        NodeType type = (t.type == TOKEN_SSLEEP) ? NODE_SSLEEP : NODE_MSLEEP;
        stmt = create_node(type, num, t.line, t.column);
        return stmt;
    }
    
    else if (t.type == TOKEN_IF) {
        parser_advance(p);
        
        if (p->current.type != TOKEN_LPAREN) {
            error_syntax(p->error, p->source, p->current, "(");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        Node* cond = parse_condition(p);
        if (!cond) return NULL;
        
        if (p->current.type == TOKEN_COMMA) {
            error_report(p->error, p->source, p->current.line, p->current.column, "too many arguments");
            skip_to_sync(p);
            return NULL;
        }
        
        if (p->current.type != TOKEN_RPAREN) {
            error_syntax(p->error, p->source, p->current, ")");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        Node* body = parse_block(p, "SCOPE IF");
        if (!body) return NULL;
        
        stmt = create_node(NODE_IF, "", t.line, t.column);
        stmt->cond = cond;
        stmt->body = body;
        
        if (p->current.type == TOKEN_ELSE) {
    parser_advance(p);
    
    Node* else_body = NULL;
    
    // checking else if
    if (p->current.type == TOKEN_IF) {
        Node* else_if = parse_statement(p);
        if (else_if) {
            else_body = create_node(NODE_BLOCK, "", 0, 0);
            else_body->body = else_if;
        }
    } else {
        else_body = parse_block(p, "SCOPE ELSE");
    }
    
    stmt->right = else_body;
}
        
        return stmt;
    }
    
    else if (t.type == TOKEN_WHILE) {
        parser_advance(p);
        
        if (p->current.type != TOKEN_LPAREN) {
            error_syntax(p->error, p->source, p->current, "(");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        Node* cond = parse_condition(p);
        if (!cond) return NULL;
        
        if (p->current.type == TOKEN_COMMA) {
            error_report(p->error, p->source, p->current.line, p->current.column, "too many arguments");
            skip_to_sync(p);
            return NULL;
        }
        
        if (p->current.type != TOKEN_RPAREN) {
            error_syntax(p->error, p->source, p->current, ")");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        Node* body = parse_block(p, "SCOPE WHILE");
        if (!body) return NULL;
        
        stmt = create_node(NODE_WHILE, "", t.line, t.column);
        stmt->cond = cond;
        stmt->body = body;
        return stmt;
    }
    
    else if (t.type == TOKEN_FOR) {
        parser_advance(p);
        
        if (p->current.type != TOKEN_LPAREN) {
            error_syntax(p->error, p->source, p->current, "(");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        if (p->current.type != TOKEN_VAR && p->current.type != TOKEN_IDENTIFIER) {
            error_syntax(p->error, p->source, p->current, "var or identifier");
            skip_to_sync(p);
            return NULL;
        }
        
        int is_var = (p->current.type == TOKEN_VAR);
        if (is_var) {
            parser_advance(p);
        }
        
        if (p->current.type != TOKEN_IDENTIFIER) {
            error_expected_variable(p->error, p->source, p->current);
            skip_to_sync(p);
            return NULL;
        }
        
        char init_var[200];
        strcpy(init_var, p->current.value);
        parser_advance(p);
        
        if (p->current.type != TOKEN_EQ) {
            error_syntax(p->error, p->source, p->current, "=");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        if (p->current.type != TOKEN_NUMBER) {
            error_expected_number(p->error, p->source, p->current);
            skip_to_sync(p);
            return NULL;
        }
        
        Node* init_node;
        if (is_var) {
            init_node = create_node(NODE_VAR, init_var, t.line, t.column);
            init_node->is_declaration = 1;
        } else {
            init_node = create_node(NODE_VAR, init_var, t.line, t.column);
            init_node->is_declaration = 0;
        }
        init_node->left = create_node(NODE_NUMBER, p->current.value, p->current.line, p->current.column);
        parser_advance(p);
        
        if (p->current.type != TOKEN_COMMA) {
            error_syntax(p->error, p->source, p->current, ",");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        Node* cond = parse_condition(p);
        if (!cond) return NULL;
        
        if (p->current.type != TOKEN_COMMA) {
            error_syntax(p->error, p->source, p->current, ",");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        if (p->current.type != TOKEN_IDENTIFIER) {
            error_expected_variable(p->error, p->source, p->current);
            skip_to_sync(p);
            return NULL;
        }
        
        char inc_var[200];
        strcpy(inc_var, p->current.value);
        parser_advance(p);
        
        if (p->current.type != TOKEN_INC && p->current.type != TOKEN_DEC) {
            error_syntax(p->error, p->source, p->current, "++ or --");
            skip_to_sync(p);
            return NULL;
        }
        
        Node* inc_node = create_node(NODE_VAR, inc_var, t.line, t.column);
        strcpy(inc_node->data_type, (p->current.type == TOKEN_INC) ? "inc" : "dec");
        parser_advance(p);
        
        if (p->current.type == TOKEN_COMMA) {
            error_report(p->error, p->source, p->current.line, p->current.column, "too many arguments");
            skip_to_sync(p);
            return NULL;
        }
        
        if (p->current.type != TOKEN_RPAREN) {
            error_syntax(p->error, p->source, p->current, ")");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        Node* body = parse_block(p, "SCOPE FOR");
        if (!body) return NULL;
        
        stmt = create_node(NODE_FOR, "", t.line, t.column);
        stmt->init = init_node;
        stmt->cond = cond;
        stmt->inc = inc_node;
        stmt->body = body;
        return stmt;
    }
    
    else if (t.type == TOKEN_SAY || t.type == TOKEN_SAY_EXCLAM) {
    parser_advance(p);
    
    if (p->current.type != TOKEN_LPAREN) {
        error_syntax(p->error, p->source, p->current, "(");
        skip_to_sync(p);
        return NULL;
    }
    parser_advance(p);
    
    // ── parsing args ──
    Node* first_arg = NULL;
    Node* last_arg = NULL;
    
    while (p->current.type != TOKEN_RPAREN && 
           p->current.type != TOKEN_EOF) {
        
        Node* arg = parse_expression(p);
        if (!arg) return NULL;
        
        if (!first_arg) {
            first_arg = arg;
            last_arg = arg;
        } else {
            last_arg->next = arg;
            last_arg = arg;
        }
        
        // if (comma) continue else break
        if (p->current.type == TOKEN_COMMA) {
            parser_advance(p);
        } else {
            break;
        }
    }
    
    if (p->current.type != TOKEN_RPAREN) {
        error_syntax(p->error, p->source, p->current, ")");
        skip_to_sync(p);
        return NULL;
    }
    parser_advance(p);
    
    NodeType type = (t.type == TOKEN_SAY) ? NODE_SAY : NODE_SAY_EXCLAM;
    stmt = create_node(type, "", t.line, t.column);
    stmt->left = first_arg;   // ← list args
    return stmt;
}
    
    else if (t.type == TOKEN_IDENTIFIER) {
        Node* expr = parse_expression(p);
        if (!expr) return NULL;
        
        if (p->current.type == TOKEN_EQ) {
            parser_advance(p);
            Node* right_expr = parse_expression(p);
            if (!right_expr) return NULL;
            
            stmt = create_node(NODE_VAR, "", t.line, t.column);
            stmt->is_declaration = 0;
            stmt->left = expr;
            stmt->right = right_expr;
            return stmt;
        }
        
        if (p->current.type == TOKEN_INC) {
            parser_advance(p);
            
            stmt = create_node(NODE_VAR, t.value, t.line, t.column);
            strcpy(stmt->data_type, "inc");
            return stmt;
        }
        
        if (p->current.type == TOKEN_DEC) {
            parser_advance(p);
            
            stmt = create_node(NODE_VAR, t.value, t.line, t.column);
            strcpy(stmt->data_type, "dec");
            return stmt;
        }
        else {
        	error_unknown_feature(p->error, p->source, t);
            skip_to_sync(p);
            return NULL;
        }
    }
    
    else if (t.type == TOKEN_C) {
        parser_advance(p);
        
        if (p->current.type != TOKEN_LBRACE) {
            error_syntax(p->error, p->source, p->current, "{");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        char c_code[20000] = {0};
        int brace_count = 1;
        
        while (brace_count > 0 && p->current.type != TOKEN_EOF) {
            if (p->current.type == TOKEN_LBRACE) {
                brace_count++;
                strcat(c_code, "{ ");
            } else if (p->current.type == TOKEN_RBRACE) {
                brace_count--;
                if (brace_count == 0) {
                    parser_advance(p);
                    break;
                }
                strcat(c_code, "} ");
            } else {
                strcat(c_code, p->current.value);
                strcat(c_code, " ");
            }
            parser_advance(p);
        }
        
        if (brace_count != 0) {
            error_syntax(p->error, p->source, p->current, "}");
            skip_to_sync(p);
            return NULL;
        }
        
        stmt = create_node(NODE_C, c_code, t.line, t.column);
        return stmt;
    }
    
    else if (t.type == TOKEN_BREAK) {
        parser_advance(p);
        stmt = create_node(NODE_BREAK, "", t.line, t.column);
        return stmt;
    }
    
    else if (t.type == TOKEN_CONTINUE) {
        parser_advance(p);
        stmt = create_node(NODE_CONTINUE, "", t.line, t.column);
        return stmt;
    }
    
    if (t.type != TOKEN_EOF && t.type != TOKEN_RBRACE) {
        error_unknown_feature(p->error, p->source, t);
        skip_to_sync(p);
    }
    
    return NULL;
}
// ============================================================
//              PARSE PROGRAM
// ============================================================
Node* parse_program(Parser* p) {
	scope_enter("GLOBAL");
	
    Node* program = create_node(NODE_PROGRAM, "", 0, 0);
    Node* first = NULL;
    Node* last = NULL;
    
    while (p->current.type != TOKEN_EOF) {
        if (p->current.type == TOKEN_COMMENT) {
            parser_advance(p);
            continue;
        }
        
        if (p->current.type == TOKEN_RBRACE) {
            parser_advance(p);
            continue;
        }
        
        Node* stmt = NULL;
        
        stmt = parse_statement(p);
        
        if (stmt) {
            if (!first) {
                first = stmt;
                last = stmt;
            } else {
                last->next = stmt;
                last = stmt;
            }
        }
    }
    
    scope_exit();
    
    program->body = first;
    return program;
}
// ============================================================
//                      TRANSLATOR
// ============================================================
void translate_expression(Node* node, FILE* out) {
    if (!node) return;
    switch (node->type) {
        case NODE_NUMBER:
            fprintf(out, "%s", node->value);
            break;
        case NODE_STRING: {
        	fprintf(out, "\"");
                for (char* p = node->value; *p; p++) {
    if (*p == '\n') fprintf(out, "\\n");
    else if (*p == '\0') fprintf(out, "\\0");
    else if (*p == '\t') fprintf(out, "\\t");
    else if (*p == '"') fprintf(out, "\\\"");
    else if (*p == '\\') fprintf(out, "\\\\");
    else fputc(*p, out);
}
fprintf(out, "\"");
            break;
        }
        case NODE_BOOLEAN:
    fprintf(out, "%s", strcmp(node->value, "true") == 0 ? "1" : "0");
    break;
        case NODE_CHAR: {
    // Handle special characters
    if (node->value[0] == '\0') {
        fprintf(out, "'\\0'");
    }
    else if (node->value[0] == '\n') {
        fprintf(out, "'\\n'");
    }
    else if (node->value[0] == '\t') {
        fprintf(out, "'\\t'");
    }
    else if (node->value[0] == '\r') {
        fprintf(out, "'\\r'");
    }
    else if (node->value[0] == '\'') {
        fprintf(out, "'\\''");
    }
    else if (node->value[0] == '\\') {
        fprintf(out, "'\\\\'");
    }
    else {
        fprintf(out, "'%s'", node->value);
    }
    break;
}
        case NODE_READ_FILE: {
    fprintf(out, "read_file(\"%s\")", node->value);
    break;
}

        case NODE_IDENTIFIER:
            fprintf(out, "%s", node->value);
            break;
        case NODE_NULL:
            fprintf(out, "NULL");
            break;
        case NODE_ENUM_ACCESS: {
            fprintf(out, "%s_%s", node->left->value, node->value);
            break;
        }
        case NODE_STR_INDEX: {
            translate_expression(node->left, out);
            fprintf(out, "[");
            translate_expression(node->right, out);
            fprintf(out, "]");
            break;
        }
        case NODE_CALL: {
            fprintf(out, "%s(", node->value);
            Node* arg = node->left;
            int first = 1;
            while (arg) {
                if (!first) fprintf(out, ", ");
                first = 0;
                translate_expression(arg, out);
                arg = arg->next;
            }
            fprintf(out, ")");
            break;
        }
        case NODE_ARRAY: {
            fprintf(out, "{");
            Node* elem = node->body;
            int first = 1;
            while (elem) {
                if (!first) fprintf(out, ", ");
                first = 0;
                translate_expression(elem, out);
                elem = elem->next;
            }
            fprintf(out, "}");
            break;
        }
        
            case NODE_ARRLEN:
    fprintf(out, "(sizeof(");
    translate_expression(node->left, out);
    fprintf(out, ") / sizeof(");
    translate_expression(node->left, out);
    fprintf(out, "[0]))");
    break;
    
        case NODE_COMPARE:
    translate_condition(node, out);
    break;

case NODE_LOGICAL_OP:
    translate_condition(node, out);
    break;
    
        case NODE_STRLEN:
            fprintf(out, "strlen(");
            translate_expression(node->left, out);
            fprintf(out, ")");
            break;
        case NODE_STRUPPER:
            fprintf(out, "strupper(");
            translate_expression(node->left, out);
            fprintf(out, ")");
            break;
        case NODE_STRLOWER:
            fprintf(out, "strlower(");
            translate_expression(node->left, out);
            fprintf(out, ")");
            break;
        case NODE_STRTRIM:
            fprintf(out, "strtrim(");
            translate_expression(node->left, out);
            fprintf(out, ")");
            break;
        case NODE_BINARY_OP: {
            fprintf(out, "(");
            translate_expression(node->left, out);
            fprintf(out, " %s ", node->value);
            translate_expression(node->right, out);
            fprintf(out, ")");
            break;
        }
        default:
            fprintf(out, "0");
            break;
    }
}

void translate_condition(Node* cond, FILE* out) {
    if (!cond) return;
    
    switch (cond->type) {
        case NODE_NOT:
            fprintf(out, "!(");
            translate_condition(cond->left, out);
            fprintf(out, ")");
            break;
        
        // LOGICAL OP (&& and ||)
        case NODE_LOGICAL_OP: {
            fprintf(out, "(");
            translate_condition(cond->left, out);
            fprintf(out, " %s ", cond->value);
            translate_condition(cond->right, out);
            fprintf(out, ")");
            break;
        }
        
        case NODE_BOOLEAN:
            fprintf(out, "%s", 
                    strcmp(cond->value, "true") == 0 ? "1" : "0");
            break;
        
        case NODE_IDENTIFIER:
            fprintf(out, "%s", cond->value);
            break;
        
        // INDEX: arr[i]
        case NODE_STR_INDEX:
            translate_expression(cond->left, out);
            fprintf(out, "[");
            translate_expression(cond->right, out);
            fprintf(out, "]");
            break;
        
        case NODE_NUMBER:
            fprintf(out, "%s", cond->value);
            break;
        
        case NODE_STRING:
            fprintf(out, "\"%s\"", cond->value);
            break;
        
        case NODE_CHAR:
            fprintf(out, "'%s'", cond->value);
            break;
        
        case NODE_NULL:
            fprintf(out, "NULL");
            break;
        
        case NODE_COMPARE: {
            int use_strcmp = 0;
            
            if (cond->left && cond->left->type == NODE_IDENTIFIER) {
                if (strcmp(cond->left->data_type, "string") == 0) {
                    use_strcmp = 1;
                }
            }
            
            if (cond->right) {
                if (cond->right->type == NODE_STRING) {
                    use_strcmp = 1;
                }
                if (cond->right->type == NODE_IDENTIFIER && 
                    strcmp(cond->right->data_type, "string") == 0) {
                    use_strcmp = 1;
                }
            }
            
            if (cond->left && cond->left->type == NODE_NUMBER && 
                cond->right && cond->right->type == NODE_NUMBER) {
                use_strcmp = 0;
            }
            
            if (use_strcmp && 
                (strcmp(cond->value, "==") == 0 || 
                 strcmp(cond->value, "!=") == 0)) {
                
                if (strcmp(cond->value, "==") == 0) {
                    fprintf(out, "strcmp(");
                    translate_expression(cond->left, out);
                    fprintf(out, ", ");
                    translate_expression(cond->right, out);
                    fprintf(out, ") == 0");
                } else {
                    fprintf(out, "strcmp(");
                    translate_expression(cond->left, out);
                    fprintf(out, ", ");
                    translate_expression(cond->right, out);
                    fprintf(out, ") != 0");
                }
            } else {
                translate_expression(cond->left, out);
                fprintf(out, " %s ", cond->value);
                translate_expression(cond->right, out);
            }
            break;
        }
        
        default:
            translate_expression(cond, out);
            break;
    }
}

// ============================================================
//                      TRANSLATE BLOCK
// ============================================================
void translate_block(Node* block, FILE* out, int depth) {
    if (!block) {
        indent(out, depth);
        fprintf(out, "{\n");
        indent(out, depth);
        fprintf(out, "}\n");
        return;
    }
    
    indent(out, depth);
    fprintf(out, "{\n");
    
    Node* stmt = block->body;
    while (stmt) {
        translate_statement(stmt, out, depth + 1);
        stmt = stmt->next;
    }
    
    indent(out, depth);
    fprintf(out, "}\n");
}

// ============================================================
//                      TRANSLATE FUNCTION
// ============================================================
void translate_function(Node* fn, FILE* out) {
    if (!fn || fn->type != NODE_FN) return;
    
    // ---- return type ----
    char* ret_type = "void";
    
    if (strcmp(fn->value, "main") == 0) {
        ret_type = "int";
    }
    else if (strcmp(fn->data_type, "int") == 0) ret_type = "int";
    else if (strcmp(fn->data_type, "float") == 0) ret_type = "double";
    else if (strcmp(fn->data_type, "string") == 0) ret_type = "char*";
    else if (strcmp(fn->data_type, "bool") == 0) ret_type = "int";
    else if (strcmp(fn->data_type, "char") == 0) ret_type = "char";
    else if (strcmp(fn->data_type, "void") == 0) ret_type = "void";
    else ret_type = fn->data_type;
    
    fprintf(out, "%s %s(", ret_type, fn->value);
    
    // ---- parameters ----
    Node* param = fn->left;
    int first = 1;
    while (param) {
        if (!first) fprintf(out, ", ");
        first = 0;
        
        // ---- fn type: fn(int, int) -> int ----
        if (strncmp(param->data_type, "fn(", 3) == 0) {
            char arg_part[128] = "";
            char ret_part[32] = "void";
            
            char* p_open = strchr(param->data_type, '(');
            char* p_close = strchr(param->data_type, ')');
            char* p_arrow = strstr(param->data_type, "->");
            
            if (p_open && p_close && p_arrow) {
                int arg_len = (int)(p_close - p_open - 1);
                if (arg_len > 0 && arg_len < (int)sizeof(arg_part)) {
                    strncpy(arg_part, p_open + 1, arg_len);
                    arg_part[arg_len] = '\0';
                }
                strcpy(ret_part, p_arrow + 2);
            }
            
            // build c args
            char c_args[256] = "";
            char arg_copy[128];
            strcpy(arg_copy, arg_part);
            char* tok = strtok(arg_copy, ",");
            int fa = 1;
            while (tok) {
                if (!fa) strcat(c_args, ", ");
                fa = 0;
                
                if (strcmp(tok, "int") == 0) strcat(c_args, "int");
                else if (strcmp(tok, "float") == 0) strcat(c_args, "double");
                else if (strcmp(tok, "string") == 0) strcat(c_args, "char*");
                else if (strcmp(tok, "bool") == 0) strcat(c_args, "int");
                else if (strcmp(tok, "char") == 0) strcat(c_args, "char");
                else if (strcmp(tok, "void") == 0) strcat(c_args, "void");
                else strcat(c_args, "int");
                
                tok = strtok(NULL, ",");
            }
            
            // c return type
            char* c_ret = "int";
            if (strcmp(ret_part, "void") == 0) c_ret = "void";
            else if (strcmp(ret_part, "int") == 0) c_ret = "int";
            else if (strcmp(ret_part, "float") == 0) c_ret = "double";
            else if (strcmp(ret_part, "string") == 0) c_ret = "char*";
            else if (strcmp(ret_part, "bool") == 0) c_ret = "int";
            else if (strcmp(ret_part, "char") == 0) c_ret = "char";
            
            // print: int (*name)(int, int)
            fprintf(out, "%s (*%s)(%s)", c_ret, param->value, c_args);
        }
        else {
            // ---- regular types ----
            char* ptype = "int";
            if (strcmp(param->data_type, "int") == 0) ptype = "int";
            else if (strcmp(param->data_type, "float") == 0) ptype = "double";
            else if (strcmp(param->data_type, "string") == 0) ptype = "char*";
            else if (strcmp(param->data_type, "bool") == 0) ptype = "int";
            else if (strcmp(param->data_type, "char") == 0) ptype = "char";
            else if (strcmp(param->data_type, "void") == 0) ptype = "void";
            else ptype = param->data_type;
            
            fprintf(out, "%s %s", ptype, param->value);
        }
        
        param = param->next;
    }
    
    fprintf(out, ") {\n");
    
    // ---- body ----
    Node* stmt = fn->body ? fn->body->body : NULL;
    while (stmt) {
        translate_statement(stmt, out, 1);
        stmt = stmt->next;
    }
    
    fprintf(out, "}\n");
}
// ============================================================
//              TRANSLATE FOR-INIT (no indent, no semicolon)
// ============================================================
void translate_for_init(Node* node, FILE* out) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_VAR: {
            // var i = 0
            char* ctype = get_c_type(node);
            fprintf(out, "%s %s", ctype, node->value);
            if (node->left) {
                fprintf(out, " = ");
                translate_expression(node->left, out);
            }
            // No semicolon!
            break;
        }
        default:
            // fallback: just print value
            fprintf(out, "%s", node->value);
            break;
    }
}

// ============================================================
//              TRANSLATE FOR-INC (no indent, no semicolon)
// ============================================================
void translate_for_inc(Node* node, FILE* out) {
    if (!node) return;
    
    if (strcmp(node->data_type, "inc") == 0) {
        fprintf(out, "%s++", node->value);
    } else if (strcmp(node->data_type, "dec") == 0) {
        fprintf(out, "%s--", node->value);
    } else {
        fprintf(out, "%s", node->value);
    }
}

// ============================================================
//                      INDENT HELPER
// ============================================================
// Prints `depth * 4` spaces. Used by all translate_* functions.
static void indent(FILE* out, int depth) {
    for (int i = 0; i < depth; i++) {
        fprintf(out, "    ");
    }
}

void translate_statement(Node* node, FILE* out, int depth) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_FN:
            translate_function(node, out);
            break;
            
        case NODE_BLOCK:
            translate_block(node, out, depth);
            break;
 
        case NODE_RETURN: {
            indent(out, depth);
            fprintf(out, "return");
            if (node->left) {
                fprintf(out, " ");
                translate_expression(node->left, out);
            }
            fprintf(out, ";\n");
            break;
        }
        
        case NODE_CONST: {
            char* ctype = get_c_type(node);
            indent(out, depth);
            
            if (node->left && node->left->type == NODE_ARRAY) {
                fprintf(out, "const %s %s[%d] = ", ctype, node->value, node->buffer_size);
                translate_expression(node->left, out);
                fprintf(out, ";\n");
                
                indent(out, depth);
                fprintf(out, "const int %s_len = %d;\n", node->value, node->buffer_size);
            }
            else if (node->buffer_size > 0) {
                fprintf(out, "const %s %s[%d]", ctype, node->value, node->buffer_size);
                if (node->left) {
                    fprintf(out, " = ");
                    translate_expression(node->left, out);
                }
                fprintf(out, ";\n");
            }
            else {
                fprintf(out, "const %s %s", ctype, node->value);
                if (node->left) {
                    fprintf(out, " = ");
                    translate_expression(node->left, out);
                }
                fprintf(out, ";\n");
            }
            break;
        }

        case NODE_VAR: {
    char* ctype = get_c_type(node);
    indent(out, depth);
    
    if (strcmp(node->data_type, "inc") == 0) {
        fprintf(out, "%s++;\n", node->value);
    }
    else if (strcmp(node->data_type, "dec") == 0) {
        fprintf(out, "%s--;\n", node->value);
    }
    else if (node->left && node->right && !node->is_declaration) {
        translate_expression(node->left, out);
        fprintf(out, " = ");
        translate_expression(node->right, out);
        fprintf(out, ";\n");
    }
    else if (node->left && !node->is_declaration) {
        fprintf(out, "%s = ", node->value);
        translate_expression(node->left, out);
        fprintf(out, ";\n");
    }
    else if (node->buffer_size > 0) {
                fprintf(out, "%s %s[%d]", ctype, node->value, node->buffer_size);
                if (node->left) {
                    fprintf(out, " = ");
                    translate_expression(node->left, out);
                }
                fprintf(out, ";\n");
            }
            
    else if (node->left && node->left->type == NODE_ARRAY) {
        fprintf(out, "%s %s[] = ", ctype, node->value);
        translate_expression(node->left, out);
        fprintf(out, ";\n");
        
        Node* elem = node->left->body;
        int count = 0;
        while (elem) {
            count++;
            elem = elem->next;
        }
        indent(out, depth);
        fprintf(out, "int %s_len = %d;\n", node->value, count);
    }
    else {
        fprintf(out, "%s %s", ctype, node->value);
        if (node->left) {
            fprintf(out, " = ");
            translate_expression(node->left, out);
        }
        fprintf(out, ";\n");
    }
    break;
}

        case NODE_STR_INDEX: {
            indent(out, depth);
            translate_expression(node->left, out);
            fprintf(out, "[");
            translate_expression(node->right, out);
            fprintf(out, "]");
            fprintf(out, " = ");
            translate_expression(node->left, out);
            fprintf(out, ";\n");
            break;
        }
        
        case NODE_STRLEN:
        case NODE_STRUPPER:
        case NODE_STRLOWER:
        case NODE_STRTRIM: {
            indent(out, depth);
            const char* fn = "strlen";
            if (node->type == NODE_STRUPPER) fn = "strupper";
            else if (node->type == NODE_STRLOWER) fn = "strlower";
            else if (node->type == NODE_STRTRIM) fn = "strtrim";
            fprintf(out, "%s(", fn);
            translate_expression(node->left, out);
            fprintf(out, ");\n");
            break;
        }
        
        case NODE_CREATE_FILE: {
            indent(out, depth);
            fprintf(out, "FILE* fp = fopen(\"%s\", \"wx\");\n", node->value);
            indent(out, depth);
            fprintf(out, "if (fp) {\n");
            
            Node* stmt = node->body;
            while (stmt) {
                translate_statement(stmt, out, depth + 1);
                stmt = stmt->next;
            }
            
            indent(out, depth);
            fprintf(out, "    fclose(fp);\n");
            indent(out, depth);
            fprintf(out, "} else {\n");
            indent(out, depth);
            fprintf(out, "    perror(\"file\");\n");
            indent(out, depth);
            fprintf(out, "}\n");
            break;
        }

        case NODE_APPEND_FILE: {
            indent(out, depth);
            fprintf(out, "FILE* fp = fopen(\"%s\", \"a\");\n", node->value);
            indent(out, depth);
            fprintf(out, "if (fp) {\n");
            
            Node* stmt = node->body;
            while (stmt) {
                translate_statement(stmt, out, depth + 1);
                stmt = stmt->next;
            }
            
            indent(out, depth);
            fprintf(out, "    fclose(fp);\n");
            indent(out, depth);
            fprintf(out, "} else {\n");
            indent(out, depth);
            fprintf(out, "    perror(\"file\");\n");
            indent(out, depth);
            fprintf(out, "}\n");
            break;
        }

        case NODE_WRITE_FILE: {
            indent(out, depth);
            fprintf(out, "FILE* fp = fopen(\"%s\", \"w\");\n", node->value);
            indent(out, depth);
            fprintf(out, "if (fp) {\n");
            
            Node* stmt = node->body;
            while (stmt) {
                translate_statement(stmt, out, depth + 1);
                stmt = stmt->next;
            }
            
            indent(out, depth);
            fprintf(out, "    fclose(fp);\n");
            indent(out, depth);
            fprintf(out, "} else {\n");
            indent(out, depth);
            fprintf(out, "    perror(\"file\");\n");
            indent(out, depth);
            fprintf(out, "}\n");
            break;
        }


        case NODE_FSAY:
case NODE_FSAY_EXCLAM: {
    indent(out, depth);
    
    // ── format ──
    char format[512] = "";
    int fmt_len = 0;
    
    Node* arg = node->left;
    while (arg) {
        const char* t = get_type_expr(arg);
        
        const char* spec = "%s";
        if (strcmp(t, "int") == 0)           spec = "%d";
        else if (strcmp(t, "float") == 0)    spec = "%f";
        else if (strcmp(t, "string") == 0)   spec = "%s";
        else if (strcmp(t, "char") == 0)     spec = "%c";
        else if (strcmp(t, "bool") == 0)  spec = "%d";
        else if (t[0] == '[')                spec = "%p";
        
        size_t slen = strlen(spec);
        if (fmt_len + slen < (int)sizeof(format) - 1) {
            strcpy(format + fmt_len, spec);
            fmt_len += slen;
        }
        arg = arg->next;
    }
    
    if (node->type == NODE_FSAY_EXCLAM) {
        if (fmt_len + 1 < (int)sizeof(format) - 1) {
            format[fmt_len++] = '\\';
            format[fmt_len++] = 'n';
            format[fmt_len] = '\0';
        }
    }
    
    // ── fprintf ──
    fprintf(out, "fprintf(fp, \"%s\"", format);
    
    // ── args ──
    arg = node->left;
    while (arg) {
        fprintf(out, ", ");
        translate_expression(arg, out);
        arg = arg->next;
    }
    
    fprintf(out, ");\n");
    break;
}

        case NODE_SAY:
case NODE_SAY_EXCLAM: {
    indent(out, depth);
    
    // ── format ──
    char format[512] = "";
    int fmt_len = 0;
    
    Node* arg = node->left;
    
    while (arg) {
        const char* t = get_type_expr(arg);
        
        const char* spec = "%s";   // fallback
        if (strcmp(t, "int") == 0)           spec = "%d";
        else if (strcmp(t, "float") == 0)    spec = "%f";
        else if (strcmp(t, "string") == 0)   spec = "%s";
        else if (strcmp(t, "char") == 0)     spec = "%c";
        else if (strcmp(t, "bool") == 0)  spec = "%d";
        else if (t[0] == '[') {
        	// for array print ptr
            spec = "%p";
        }
        
        size_t slen = strlen(spec);
        if (fmt_len + slen < (int)sizeof(format) - 1) {
            strcpy(format + fmt_len, spec);
            fmt_len += slen;
        }
        
        arg = arg->next;
    }
    
    if (node->type == NODE_SAY_EXCLAM) {
        if (fmt_len + 1 < (int)sizeof(format) - 1) {
            format[fmt_len++] = '\\';
            format[fmt_len++] = 'n';
            format[fmt_len] = '\0';
        }
    }
    
    // ── printf ──
    fprintf(out, "printf(\"%s\"", format);
    
    // ── print args ──
    arg = node->left;
    while (arg) {
        fprintf(out, ", ");
        
        const char* t = get_type_expr(arg);
        if (t[0] == '[') {
            // array -> print name
            translate_expression(arg, out);
        } else {
            translate_expression(arg, out);
        }
        
        arg = arg->next;
    }
    
    fprintf(out, ");\n");
    break;
}

        case NODE_FATAL: {
    indent(out, depth);
    
    // ── format ──
    char format[512] = "";
    int fmt_len = 0;
    
    Node* arg = node->left;
    while (arg) {
        const char* t = get_type_expr(arg);
        
        const char* spec = "%s";
        if (strcmp(t, "int") == 0)           spec = "%d";
        else if (strcmp(t, "float") == 0)    spec = "%f";
        else if (strcmp(t, "string") == 0)   spec = "%s";
        else if (strcmp(t, "char") == 0)     spec = "%c";
        else if (strcmp(t, "bool") == 0)  spec = "%d";
        else if (t[0] == '[')                spec = "%p";
        
        size_t slen = strlen(spec);
        if (fmt_len + slen < (int)sizeof(format) - 1) {
            strcpy(format + fmt_len, spec);
            fmt_len += slen;
        }
        arg = arg->next;
    }
    
    // always \n
    if (fmt_len + 1 < (int)sizeof(format) - 1) {
        format[fmt_len++] = '\\';
        format[fmt_len++] = 'n';
        format[fmt_len] = '\0';
    }
    
    // ── printf ──
    fprintf(out, "printf(\"%s\"", format);
    
    // ── args ──
    arg = node->left;
    while (arg) {
        fprintf(out, ", ");
        translate_expression(arg, out);
        arg = arg->next;
    }
    
    fprintf(out, ");\n");
    indent(out, depth);
    fprintf(out, "exit(1);\n");
    break;
}
        
        case NODE_ENUM: {
            Node* item = node->body;
            while (item) {
                fprintf(out, "#define %s_%s %s\n", node->value, item->value, item->data_type);
                item = item->next;
            }
            fprintf(out, "\n");
            break;
        }

        case NODE_IF: {
            indent(out, depth);
            fprintf(out, "if (");
            translate_condition(node->cond, out);
            fprintf(out, ") {\n");
            
            Node* stmt = node->body ? node->body->body : NULL;
            while (stmt) {
                translate_statement(stmt, out, depth + 1);
                stmt = stmt->next;
            }
            
            indent(out, depth);
            fprintf(out, "}");
            
            if (node->right) {
                // Check if else-branch is a single `if` (else if)
                Node* else_body = node->right;
                int is_else_if = (else_body->body && 
                                  else_body->body->type == NODE_IF &&
                                  else_body->body->next == NULL);
                
                if (is_else_if) {
                    fprintf(out, " else ");
                    // Print nested if without indent prefix
                    translate_statement(else_body->body, out, depth);
                } else {
                    fprintf(out, " else {\n");
                    Node* s = else_body ? else_body->body : NULL;
                    while (s) {
                        translate_statement(s, out, depth + 1);
                        s = s->next;
                    }
                    indent(out, depth);
                    fprintf(out, "}");
                }
            }
            fprintf(out, "\n");
            break;
        }
        
        case NODE_WHILE: {
            indent(out, depth);
            fprintf(out, "while (");
            translate_condition(node->cond, out);
            fprintf(out, ") {\n");
            
            Node* stmt = node->body ? node->body->body : NULL;
            while (stmt) {
                translate_statement(stmt, out, depth + 1);
                stmt = stmt->next;
            }
            
            indent(out, depth);
            fprintf(out, "}\n");
            break;
        }
        
        case NODE_FOR: {
            indent(out, depth);
            fprintf(out, "for (");
            
            // Init — no indent, no semicolon
            if (node->init) {
                translate_for_init(node->init, out);
            }
            fprintf(out, "; ");
            
            // Condition
            if (node->cond) {
                translate_condition(node->cond, out);
            }
            fprintf(out, "; ");
            
            // Increment — no indent, no semicolon
            if (node->inc) {
                translate_for_inc(node->inc, out);
            }
            
            fprintf(out, ") {\n");
            
            Node* stmt = node->body ? node->body->body : NULL;
            while (stmt) {
                translate_statement(stmt, out, depth + 1);
                stmt = stmt->next;
            }
            
            indent(out, depth);
            fprintf(out, "}\n");
            break;
        }
        
        case NODE_CLS: {
            indent(out, depth);
            #ifdef _WIN32
                fprintf(out, "system(\"cls\");\n");
            #else
                fprintf(out, "system(\"clear\");\n");
            #endif
            break;
        }

        case NODE_PRESS: {
            indent(out, depth);
            fprintf(out, "while ((getchar()) != '\\n');\n");
            break;
        }
        
        case NODE_INPUT: {
    indent(out, depth);
    if (strcmp(node->data_type, "int") == 0) {
        fprintf(out, "scanf(\"%%d\", &%s);\n", node->value);
    } else if (strcmp(node->data_type, "float") == 0) {
        fprintf(out, "scanf(\"%%lf\", &%s);\n", node->value);
    } else if (strcmp(node->data_type, "char") == 0) {
        fprintf(out, "scanf(\" %%c\", &%s);\n", node->value);
    } else if (strcmp(node->data_type, "string") == 0) {
        fprintf(out, "scanf(\"%%s\", %s);\n", node->value);
    } else {
        fprintf(out, "scanf(\"%%d\", &%s);\n", node->value);
    }
    break;
}
        
        case NODE_BREAK:
            indent(out, depth);
            fprintf(out, "break;\n");
            break;
            
        case NODE_CONTINUE:
            indent(out, depth);
            fprintf(out, "continue;\n");
            break;
            
        case NODE_SSLEEP: {
            indent(out, depth);
            fprintf(out, "ssleep(%s);\n", node->value);
            break;
        }

        case NODE_MSLEEP: {
            indent(out, depth);
            fprintf(out, "msleep(%s);\n", node->value);
            break;
        }

        case NODE_C:
            fprintf(out, "%s\n", node->value);
            break;
        
        default:
            indent(out, depth);
            fprintf(out, "// TODO: node type %d\n", node->type);
            break;
    }
}

// ============================================================
//                      TRANSLATE PROGRAM
// ============================================================
void translate_program(Node* ast, char* filename) {
    FILE* out = fopen(filename, "w");
    
    fprintf(out, "#ifdef _WIN32\n");
    fprintf(out, "#include <windows.h>\n");
    fprintf(out, "#define ssleep(x) Sleep(x * 1000)\n");
    fprintf(out, "#define msleep(x) Sleep(x)\n");
    fprintf(out, "#else\n");
    fprintf(out, "#include <unistd.h>\n");
    fprintf(out, "#define ssleep(x) sleep(x)\n");
    fprintf(out, "#define msleep(x) usleep(x * 1000)\n");
    fprintf(out, "#endif\n\n");

    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <string.h>\n");
    fprintf(out, "#include <stdlib.h>\n");
    fprintf(out, "#include <stdbool.h>\n");
    fprintf(out, "#include <ctype.h>\n\n");
    
fprintf(out, "char* read_file(const char* filename) {\n");
fprintf(out, "    FILE* fp = fopen(filename, \"r\");\n");
fprintf(out, "    if (!fp) return NULL;\n");
fprintf(out, "    fseek(fp, 0, SEEK_END);\n");
fprintf(out, "    long size = ftell(fp);\n");
fprintf(out, "    rewind(fp);\n");
fprintf(out, "    char* buf = (char*)malloc(size + 1);\n");
fprintf(out, "    size_t n = fread(buf, 1, size, fp);\n");
fprintf(out, "    buf[n] = '\\0';\n");
fprintf(out, "    fclose(fp);\n");
fprintf(out, "    return buf;\n");
fprintf(out, "}\n\n");
  
    // ---- Built-in helper functions ----
    fprintf(out, "char* strupper(char* s) {\n");
    fprintf(out, "    char* result = strdup(s);\n");
    fprintf(out, "    for (int i = 0; result[i]; i++) {\n");
    fprintf(out, "        result[i] = toupper(result[i]);\n");
    fprintf(out, "    }\n");
    fprintf(out, "    return result;\n");
    fprintf(out, "}\n\n");
    
    fprintf(out, "char* strlower(char* s) {\n");
    fprintf(out, "    char* result = strdup(s);\n");
    fprintf(out, "    for (int i = 0; result[i]; i++) {\n");
    fprintf(out, "        result[i] = tolower(result[i]);\n");
    fprintf(out, "    }\n");
    fprintf(out, "    return result;\n");
    fprintf(out, "}\n\n");
    
    fprintf(out, "char* strtrim(char* s) {\n");
    fprintf(out, "    while (*s == ' ' || *s == '\\t' || *s == '\\n') s++;\n");
    fprintf(out, "    char* end = s + strlen(s) - 1;\n");
    fprintf(out, "    while (end > s && (*end == ' ' || *end == '\\t' || *end == '\\n')) end--;\n");
    fprintf(out, "    char* result = malloc(end - s + 2);\n");
    fprintf(out, "    strncpy(result, s, end - s + 1);\n");
    fprintf(out, "    result[end - s + 1] = '\\0';\n");
    fprintf(out, "    return result;\n");
    fprintf(out, "}\n\n");
    
    // ============================================================
    // ---- Function prototypes ----
    // ============================================================
    Node* stmt = ast->body;
    while (stmt) {
        if (stmt->type == NODE_FN) {
            // return type
            char* ret_type = "void";
            if (strcmp(stmt->value, "main") == 0) ret_type = "int";
            else if (strcmp(stmt->data_type, "int") == 0) ret_type = "int";
            else if (strcmp(stmt->data_type, "float") == 0) ret_type = "double";
            else if (strcmp(stmt->data_type, "string") == 0) ret_type = "char*";
            else if (strcmp(stmt->data_type, "bool") == 0) ret_type = "int";
            else if (strcmp(stmt->data_type, "char") == 0) ret_type = "char";
            
            fprintf(out, "%s %s(", ret_type, stmt->value);
            
            Node* param = stmt->left;
            int first = 1;
            while (param) {
                if (!first) fprintf(out, ", ");
                first = 0;
                
                // ---- fn type ----
                if (strncmp(param->data_type, "fn(", 3) == 0) {
                    char arg_part[128] = "";
                    char ret_part[32] = "void";
                    
                    char* p_open = strchr(param->data_type, '(');
                    char* p_close = strchr(param->data_type, ')');
                    char* p_arrow = strstr(param->data_type, "->");
                    
                    if (p_open && p_close && p_arrow) {
                        int arg_len = (int)(p_close - p_open - 1);
                        if (arg_len > 0 && arg_len < (int)sizeof(arg_part)) {
                            strncpy(arg_part, p_open + 1, arg_len);
                            arg_part[arg_len] = '\0';
                        }
                        strcpy(ret_part, p_arrow + 2);
                    }
                    
                    char c_args[256] = "";
                    char arg_copy[128];
                    strcpy(arg_copy, arg_part);
                    char* tok = strtok(arg_copy, ",");
                    int fa = 1;
                    while (tok) {
                        if (!fa) strcat(c_args, ", ");
                        fa = 0;
                        
                        if (strcmp(tok, "int") == 0) strcat(c_args, "int");
                        else if (strcmp(tok, "float") == 0) strcat(c_args, "double");
                        else if (strcmp(tok, "string") == 0) strcat(c_args, "char*");
                        else if (strcmp(tok, "bool") == 0) strcat(c_args, "int");
                        else if (strcmp(tok, "char") == 0) strcat(c_args, "char");
                        else strcat(c_args, "int");
                        
                        tok = strtok(NULL, ",");
                    }
                    
                    char* c_ret = "int";
                    if (strcmp(ret_part, "void") == 0) c_ret = "void";
                    else if (strcmp(ret_part, "int") == 0) c_ret = "int";
                    else if (strcmp(ret_part, "float") == 0) c_ret = "double";
                    else if (strcmp(ret_part, "string") == 0) c_ret = "char*";
                    else if (strcmp(ret_part, "bool") == 0) c_ret = "int";
                    else if (strcmp(ret_part, "char") == 0) c_ret = "char";
                    
                    fprintf(out, "%s (*)(%s)", c_ret, c_args);
                }
                else {
                    char* ptype = "int";
                    if (strcmp(param->data_type, "int") == 0) ptype = "int";
                    else if (strcmp(param->data_type, "float") == 0) ptype = "double";
                    else if (strcmp(param->data_type, "string") == 0) ptype = "char*";
                    else if (strcmp(param->data_type, "bool") == 0) ptype = "int";
                    else if (strcmp(param->data_type, "char") == 0) ptype = "char";
                    else if (strcmp(param->data_type, "void") == 0) ptype = "void";
                    else ptype = "int";
                    
                    fprintf(out, "%s", ptype);
                }
                
                param = param->next;
            }
            
            fprintf(out, ");\n");
        }
        stmt = stmt->next;
    }
    fprintf(out, "\n");
    
    // ============================================================
    // ---- checking main ----
    // ============================================================
    int has_main = 0;
    stmt = ast->body;
    while (stmt) {
        if (stmt->type == NODE_FN && strcmp(stmt->value, "main") == 0) {
            has_main = 1;
            break;
        }
        stmt = stmt->next;
    }
    
    if (!has_main) {
        fprintf(stderr, "\nERROR: no 'fn main()' found\n");
        fprintf(stderr, "C-- requires fn main() as entry point\n");
        fclose(out);
        return;
    }
    
    // ============================================================
    // ---- Function definitions ----
    // ============================================================
    stmt = ast->body;
    while (stmt) {
        translate_statement(stmt, out, 0);
        stmt = stmt->next;
    }
    
    fclose(out);
}

// ============================================================
//                      PRINT AST
// ============================================================
// Pretty-prints the AST in a tree-like format:
//   PROGRAM
//   ├── FN: main
//   │   └── BLOCK
//   │       ├── CONST: x
//   │       │   └── NUMBER: 10
//   │       └── SAY_EXCLAM
//   │           └── STRING: "Hello"
//
// Usage:
//   print_ast(ast, 0);
// ============================================================

const char* node_type_name(NodeType type) {
    switch (type) {
        case NODE_PROGRAM:      return "PROGRAM";
        case NODE_CONST:        return "CONST";
        case NODE_VAR:          return "VAR";
        case NODE_SAY:          return "SAY";
        case NODE_SAY_EXCLAM:   return "SAY_EXCLAM";
        case NODE_IF:           return "IF";
        case NODE_WHILE:        return "WHILE";
        case NODE_FOR:          return "FOR";
        case NODE_BINARY_OP:    return "BINARY_OP";
        case NODE_IDENTIFIER:   return "IDENTIFIER";
        case NODE_NUMBER:       return "NUMBER";
        case NODE_STRING:       return "STRING";
        case NODE_BOOLEAN:      return "BOOLEAN";
        case NODE_CHAR:         return "CHAR";
        case NODE_NULL:         return "NULL";
        case NODE_BLOCK:        return "BLOCK";
        case NODE_C:            return "C_INSERT";
        case NODE_ARRAY:        return "ARRAY";
        case NODE_INDEX:        return "INDEX";
        case NODE_STR_INDEX:    return "STR_INDEX";
        case NODE_ARRLEN:       return "ARRLEN";
        case NODE_EOF:          return "EOF";
        case NODE_ELSE:         return "ELSE";
        case NODE_LOGICAL_OP:   return "LOGICAL_OP";
        case NODE_COMPARE:      return "COMPARE";
        case NODE_NOT:          return "NOT";
        case NODE_INPUT:        return "INPUT";
        case NODE_STRLEN:       return "STRLEN";
        case NODE_STRUPPER:     return "STRUPPER";
        case NODE_STRLOWER:     return "STRLOWER";
        case NODE_STRTRIM:      return "STRTRIM";
        case NODE_CREATE_FILE:  return "CREATE_FILE";
        case NODE_APPEND_FILE:  return "APPEND_FILE";
        case NODE_WRITE_FILE:   return "WRITE_FILE";
        case NODE_READ_FILE:    return "READ_FILE";
        case NODE_FSAY:         return "FSAY";
        case NODE_FSAY_EXCLAM:  return "FSAY_EXCLAM";
        case NODE_FATAL:        return "FATAL";
        case NODE_CLS:          return "CLS";
        case NODE_PRESS:        return "PRESS";
        case NODE_SSLEEP:       return "SSLEEP";
        case NODE_MSLEEP:       return "MSLEEP";
        case NODE_ENUM:         return "ENUM";
        case NODE_ENUM_ITEM:    return "ENUM_ITEM";
        case NODE_ENUM_ACCESS:  return "ENUM_ACCESS";
        case NODE_RETURN:       return "RETURN";
        case NODE_BREAK:        return "BREAK";
        case NODE_CONTINUE:     return "CONTINUE";
        case NODE_FN:           return "FN";
        case NODE_CALL:         return "CALL";
        case NODE_PARAM:        return "PARAM";
        default:                return "UNKNOWN";
    }
}

// Short description of a node's data
// (printed after "NODE_XXX:")
static void print_node_info(Node* node) {
    if (!node) return;

    // Value — printed if non-empty
    if (node->value[0] != '\0') {
        printf(" \"%s\"", node->value);
    }

    // Data type — printed if not "auto" / "inc" / "dec"
    if (node->data_type[0] != '\0' &&
        strcmp(node->data_type, "auto") != 0 &&
        strcmp(node->data_type, "inc") != 0 &&
        strcmp(node->data_type, "dec") != 0) {
        printf(" :%s", node->data_type);
    }
    
    if (node->type == NODE_CONST || node->type == NODE_VAR) {
        char* ctype = get_c_type(node);
        printf(" => c_type=%s", ctype);
    }

    // Buffer size (for fixed-size strings)
    if (node->buffer_size > 0) {
        printf(" [buffer=%d]", node->buffer_size);
    }

    // Dynamic array flag
    if (node->is_dynamic) {
        printf(" [dynamic]");
    }

    // Declaration flag (for const/var)
    if (node->is_declaration) {
        printf(" [decl]");
    }
}

// Recursive tree printer
void print_ast_rec(Node* node, int depth, int is_last) {
    if (!node) return;

    // Indentation
    for (int i = 0; i < depth - 1; i++) {
        printf("|   ");
    }
    if (depth > 0) {
        printf(is_last ? "\\-- " : "+-- ");
    }

    // Node type
    printf("%s", node_type_name(node->type));
    print_node_info(node);
    printf("\n");

    // Collect children into an array to know who is last
    Node* children[8] = {0};
    int child_count = 0;

    // Order matters: left, init, cond, inc, body, right
    if (node->left)      children[child_count++] = node->left;
    if (node->init)      children[child_count++] = node->init;
    if (node->cond)      children[child_count++] = node->cond;
    if (node->inc)       children[child_count++] = node->inc;
    if (node->body)      children[child_count++] = node->body;
    if (node->right)     children[child_count++] = node->right;

    // Print children recursively
    for (int i = 0; i < child_count; i++) {
        print_ast_rec(children[i], depth + 1, i == child_count - 1);
    }

    // Siblings (next) — for statement lists, params, enum items
    // next is a sibling, not a child; print at the same level.
    if (node->next && node->type != NODE_FN) {
        print_ast_rec(node->next, depth, is_last);
    }
}

// Public wrapper
void print_ast(Node* ast, int depth) {
    if (!ast) {
        printf("(null AST)\n");
        return;
    }
    printf("\n");
    printf("========================================\n");
    printf("              AST TREE\n");
    printf("========================================\n");
    print_ast_rec(ast, depth, 1);
    printf("========================================\n\n");
}

// ============================================================
//                      PRINT SYMBOL TABLE
// ============================================================
void print_symtab(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║                   SYMBOL TABLE                       ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    
    if (!all_scopes) {
        printf("  (empty)\n\n");
        return;
    }
    
    Scope* scopes[256];
    int scope_count = 0;
    Scope* sc = all_scopes;
    while (sc && scope_count < 256) {
        scopes[scope_count++] = sc;
        sc = sc->next_all;
    }
    
    int total_symbols = 0;
    
    for (int i = scope_count - 1; i >= 0; i--) {
        Scope* s = scopes[i];
        
        int indent = s->level * 2;
        
        printf("\n");
        for (int k = 0; k < indent; k++) printf(" ");
        
        if (strcmp(s->name, "GLOBAL") == 0) {
            printf("🌍 [%s]\n", s->name);
        } else if (strncmp(s->name, "FN ", 3) == 0) {
            printf("🔷 [%s]\n", s->name);
        } else if (strcmp(s->name, "BODY") == 0) {
            printf("   └─ body\n");
        } else {
            printf("🔸 [%s]\n", s->name);
        }
        
        if (!s->symbols) {
            for (int k = 0; k < indent + 2; k++) printf(" ");
            printf("(empty)\n");
            continue;
        }
        
        for (int k = 0; k < indent + 2; k++) printf(" ");
        printf("%-4s %-15s %-15s %-6s\n", "#", "NAME", "TYPE", "CONST");
        for (int k = 0; k < indent + 2; k++) printf(" ");
        printf("────────────────────────────────────────────\n");
        
        Symbol* sym = s->symbols;
        int local = 0;
        while (sym) {
            for (int k = 0; k < indent + 2; k++) printf(" ");
            printf("%-4d %-15s %-15s %-6s\n",
                   local, sym->name, sym->type,
                   sym->is_const ? "yes" : "no");
            local++;
            total_symbols++;
            sym = sym->next;
        }
    }
    
    printf("\n");
    printf("══════════════════════════════════════════════════════\n");
    printf("  Total: %d symbol%s in %d scope%s\n",
           total_symbols, total_symbols == 1 ? "" : "s",
           scope_count, scope_count == 1 ? "" : "s");
    printf("══════════════════════════════════════════════════════\n\n");
}
// ============================================================
//                      PRINT WARNINGS
// ============================================================
void print_warnings(ErrorContext* e, char* source) {
    Scope* sc = all_scopes;
    while (sc) {
        Symbol* s = sc->symbols;
        while (s) {
            if (s->use == 0) {
                warning_unused_variable(e, source, s->name, s->line, s->column);
            }
            s = s->next;
        }
        sc = sc->next_all;
    }
    
    FuncInfo* f = functab;
    while (f) {
        if (f->use == 0 && strcmp(f->name, "main") != 0) {
            warning_unused_function(e, source, f->name, f->line, f->column);
        }
        f = f->next;
    }
}

// ============================================================
//                        MAIN
// ============================================================
int main(void) {
    char source[MAX_SOURCE] = {0};
    
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), stdin)) {
        if (strlen(source) + strlen(buffer) < MAX_SOURCE) {
            strcat(source, buffer);
        } else {
            fprintf(stderr, "ERROR: Source too large (max %d bytes)\n", MAX_SOURCE);
            return 1;
        }
    }
    
    if (strlen(source) == 0) {
        fprintf(stderr, "ERROR: No input provided\n");
        return 1;
    }
    
    printf("========================================\n");
    printf("              SOURCE CODE\n");
    printf("========================================\n");
    printf("%s", source);
    printf("========================================\n\n");
    Lexer *l = lexer_init(source);
    Parser* p = parser_init(l, source);
    Node* ast = parse_program(p);
    
    print_ast(ast, 0);
    print_symtab();
    print_warnings(p->error, source);
    
    if (p->error->error_count == 0) {
        printf("\n✅ PARSING SUCCESSFUL\n");
        printf("AST generated\n\n");
        printf("========================================\n");
        printf("         GENERATED C CODE\n");
        printf("========================================\n");
        
        translate_program(ast, "output.c");
        printf("✅ C code written to output.c\n\n");
        
        FILE* f = fopen("output.c", "r");
        if (f) {
            printf("========================================\n");
            printf("         OUTPUT.C CONTENT\n");
            printf("========================================\n");
            char line[1024];
            int line_num = 1;
            while (fgets(line, sizeof(line), f)) {
                printf("%4d | %s", line_num, line);
                line_num++;
            }
            fclose(f);
            printf("========================================\n");
        }
        printf("\naccomplishment: \n");
        fflush(stdout);
        int compile_result = system("gcc output.c -o program 2>&1");
        if (compile_result == 0) {
            system("./program");
        } else {
            printf("❌ Compilation failed!\n");
        }
        
    } else {
        printf("\n❌ PARSING FAILED\n");
        printf("Total errors: %d\n", p->error->error_count);
    }
    
    parser_free(p);
    lexer_free(l);
    
    return 0;
}
