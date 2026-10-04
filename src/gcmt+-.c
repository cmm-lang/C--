#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================
//                        CONSTANTS
// ============================================================
#define MAX_TOKEN            5000
#define MAX_SOURCE           100000
#define MAX_ERRORS           1000
// ============================================================
//                        TOKENS
// ============================================================
typedef enum {
    // key words
    TOKEN_CONST, TOKEN_VAR, TOKEN_IF, TOKEN_ELSE,
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
    TOKEN_ENUM, TOKEN_STRUCT,

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
} ErrorEntry;

typedef struct {
    ErrorEntry entries[MAX_ERRORS];
    int error_count;
    int warning_count;
} ErrorContext;

// ---- FUTURE FEATURE ----
// soon

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
    NODE_STR_INDEX,
    NODE_ARRLEN,
    
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
    NODE_STRUCT,         // struct User { ... }
NODE_STRUCT_ITEM,    // const name[500]: string
NODE_STRUCT_INIT,    // User{"Andrey", 10}
NODE_STRUCT_ACCESS,  // user.name

    // Additional
    NODE_RETURN,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_FN,
    NODE_CALL,
    NODE_PARAM
} NodeType;


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

// --- symbols and functions ---
typedef struct Symbol {
    char name[200];
    char type[20];      // "int", "void*", "string", ...
    int is_const;
    int is_nullable;
    int use;
    int line;
    int column;
    struct Symbol* next;
} Symbol;


typedef struct FuncInfo {
    char name[200];
    char return_type[64];
    char fn_type[256];      // ← "fn(int,int)->int"
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
} Scope;

// ============================================================
// ENUM TABLE
// ============================================================
typedef struct EnumInfo {
    char name[200];          // "STATE"
    char** items;            // ["GOOD", "BAD", "FATAL"]
    int* values;             // [0, 1, 2]
    int item_count;
    int line;
    int column;
    struct EnumInfo* next;
} EnumInfo;

EnumInfo* enumtab = NULL;

typedef struct StructField {
    char name[200];
    char type[64];
    int buffer_size;
    int is_const;
} StructField;

typedef struct StructInfo {
    char name[200];
    StructField* fields;
    int field_count;
    int line;
    int column;
    struct StructInfo* next;
} StructInfo;

StructInfo* structtab = NULL;

// ---- PARSER ----
typedef struct Parser {
    Lexer *lexer;
    Token current;
    ErrorContext *error;
    char *source;
    int brace_depth;
} Parser;
// ============================================================
//                      FUNCTION PROTOTYPES
// ============================================================

// ---- ERROR FUNCTIONS ----
ErrorContext* error_init(void);
void          error_free(ErrorContext* e);
void          error_report  (ErrorContext* e, char* source, int line, int column, const char* msg);
void error_summary(ErrorContext* e);
void          warning_report(ErrorContext* e, char* source, int line, int column, const char* msg);

// specific errors
void error_syntax           (ErrorContext* e, char* source, Token t, const char* expected);
void error_unknown          (ErrorContext* e, char* source, Token t);
void error_undefined        (ErrorContext* e, char* source, Token t);
void error_undefined_func   (ErrorContext* e, char* source, Token t);
void error_const_assign     (ErrorContext* e, char* source, Token t);
void error_arg_count        (ErrorContext* e, char* source, Token t, int expected, int got);
void error_arg_type         (ErrorContext* e, char* source, Token t, int arg_num, const char* expected, const char* got);
void error_unknown_type     (ErrorContext* e, char* source, Token t);
void error_unknown_feature  (ErrorContext* e, char* source, Token t);
void error_unexpected       (ErrorContext* e, char* source, Token t);
void error_expected_variable(ErrorContext* e, char* source, Token t);
void error_expected_expression(ErrorContext* e, char* source, Token t);
void error_expected_number  (ErrorContext* e, char* source, Token t);
void error_expected_name    (ErrorContext* e, char* source, Token t);
void error_expected_filename(ErrorContext* e, char* source, Token t);
void error_duplicate_variable(ErrorContext* e, char* source, Token t);
void error_duplicate_function(ErrorContext* e, char* source, Token t);

// specific warnings
void warning_unused_variable(ErrorContext* e, char* source, const char* name, int line, int column);
void warning_unused_function(ErrorContext* e, char* source, const char* name, int line, int column);

// ---- SYMBOL TABLE ----
void      scope_enter(const char* name);
void      scope_exit(void);
Symbol*   find_symbol(const char* name);
void      add_symbol(Parser* p, const char* name, const char* type, int is_const, int line, int column);
void      mark_symbol_nullable(const char* name);

// ---- FUNCTION TABLE ----
FuncInfo* find_func(const char* name);
void      add_func(Parser* p, const char* name, const char* return_type, Node* params, int param_count, int line, int column);

// ---- ENUM TABLE ----
EnumInfo* find_enum(const char* name);
void      add_enum(const char* name, Node* items, int item_count, int line, int column);
int       find_enum_value(const char* enum_name, const char* item_name);

// ---- STRUCT TABLE ----
StructInfo*  find_struct(const char* name);
void         add_struct(const char* name, Node* fields, int field_count, int line, int column);
StructField* find_struct_field(const char* struct_name, const char* field_name);

// ---- TYPE CHECKING ----
const char* get_type_expr(Node* node);
int         check_binary_op(const char* op, const char* lt, const char* rt);
const char* get_c_type(Node* node);

// ---- LEXER ----
Lexer* lexer_init(char* source);
void   lexer_free(Lexer* l);
Token  lexer_next(Lexer* l);

// ---- PARSER ----
Parser* parser_init(Lexer* l, char* source);
void    parser_free(Parser* p);
void    parser_advance(Parser* p);
void    skip_to_sync(Parser* p);
void    skip_to_rparen(Parser* p);

Node* parse_statement(Parser* p);
Node* parse_block(Parser* p, const char* scope_name);
Node* parse_struct(Parser* p);
Node* parse_function(Parser* p);
Node* parse_cond_or(Parser* p);
Node* parse_cond_and(Parser* p);
Node* parse_cond_cmp(Parser* p);
Node* parse_cond_primary(Parser* p);
Node* parse_condition(Parser* p);
Node* parse_expression(Parser* p);
Node* parse_program(Parser* p);

// ---- AST ----
Node* create_node(NodeType type, const char* value, int line, int column);

// ---- TRANSLATOR ----
void translate_expression(Node* node, FILE* out);
void translate_condition(Node* cond, FILE* out);
void translate_statement(Node* node, FILE* out, int depth);
void translate_block(Node* block, FILE* out, int depth);
void translate_function(Node* fn, FILE* out);
static void indent(FILE* out, int depth);
void translate_program(Node* ast, const char* filename);
void translate_for_init(Node* node, FILE* out);
void translate_for_inc(Node* node, FILE* out);

// ---- DEBUG / PRINT ----
const char* node_type_name(NodeType type);
void print_ast(Node* ast, int depth);
void print_symtab(void);
void print_warnings(ErrorContext* e, char* source);

// ---- MEMORY CLEANUP ----
void free_functab(void);
void free_enumtab(void);
void free_structtab(void);
void free_scopes(void);
void free_ast(Node* node);
void free_all(Node* ast);
// ===============================
// ======== SYMBOL TABLE =========
// ===============================
Scope* current_scope = NULL;
Scope* all_scopes = NULL;
static int has_main = 0;
// ============================================================
//  CONTEXT for 'null' parsing
//  When parsing rhs of a typed variable, bare 'null' is OK.
// ============================================================
static int g_allow_bare_null = 0;
static const char* g_expected_null_type = NULL;

void scope_enter(const char* name) {
    Scope* s = malloc(sizeof(Scope));
    s->symbols = NULL;
    s->parent = current_scope;
    s->next_all = all_scopes;
    
    strncpy(s->name, name ? name : "?", sizeof(s->name) - 1);
    s->name[sizeof(s->name) - 1] = '\0';
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

// ============================================================
//  MARK SYMBOL NULLABLE 
// ============================================================
void mark_symbol_nullable(const char* name) {
    Symbol* s = find_symbol(name);
    if (s) s->is_nullable = 1;
}

void add_symbol(Parser* p, const char* name, const char* type, 
                int is_const, int line, int column) {
    Symbol* s = current_scope->symbols;
    while (s) {
        if (strcmp(s->name, name) == 0) {
            Token t;
        strcpy(t.value, name);
        t.line = line;
        t.column = column;
        
            error_duplicate_variable(p->error, p->source, t);
            return;
        }
        s = s->next;
    }
    
    Symbol* sym = malloc(sizeof(Symbol));
    strcpy(sym->name, name);
    strcpy(sym->type, type);
    sym->is_const = is_const;
    sym->is_nullable = 0;
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

void add_func(Parser* p, const char* name, const char* return_type,
              Node* params, int param_count,
              int line, int column) {
    FuncInfo* existing = find_func(name);
    if (existing) {
        Token t;
        strcpy(t.value, name);
        t.line = line;
        t.column = column;
        error_duplicate_function(p->error, p->source, t);
        return;
    }

    FuncInfo* f = malloc(sizeof(FuncInfo));
    strcpy(f->name, name);
    strcpy(f->return_type, return_type);

    // ---- build fn_type: "fn(T1,T2)->R" ----
    char args[128] = "";
    Node* pnode = params;
    int first = 1;
    while (pnode) {
        if (!first) strcat(args, ",");
        first = 0;
        strcat(args, pnode->data_type);
        pnode = pnode->next;
    }
    snprintf(f->fn_type, sizeof(f->fn_type),
             "fn(%s)->%s", args, return_type);

    f->params = params;
    f->param_count = param_count;
    f->use = 0;
    f->line = line;
    f->column = column;
    f->next = functab;
    functab = f;
}

EnumInfo* find_enum(const char* name) {
    EnumInfo* e = enumtab;
    while (e) {
        if (strcmp(e->name, name) == 0) return e;
        e = e->next;
    }
    return NULL;
}

void add_enum(const char* name, Node* items, int item_count,
              int line, int column) {
    if (find_enum(name)) {
        fprintf(stderr, "duplicate enum: '%s'\n", name);
        return;
    }
    
    EnumInfo* e = malloc(sizeof(EnumInfo));
    strcpy(e->name, name);
    e->item_count = item_count;
    e->items = malloc((size_t)item_count * sizeof(char*));
    e->values = malloc((size_t)item_count * sizeof(int));
    e->line = line;
    e->column = column;
    e->next = enumtab;
    enumtab = e;
    
    Node* item = items;
    int i = 0;
    while (item) {
        e->items[i] = strdup(item->value);
        e->values[i] = atoi(item->data_type);
        item = item->next;
        i++;
    }
}

int find_enum_value(const char* enum_name, const char* item_name) {
    EnumInfo* e = find_enum(enum_name);
    if (!e) return -1;
    
    for (int i = 0; i < e->item_count; i++) {
        if (strcmp(e->items[i], item_name) == 0) {
            return e->values[i];
        }
    }
    return -1;
}
// ============================================================
// STRUCT TABLE
// ============================================================

StructInfo* find_struct(const char* name) {
    StructInfo* s = structtab;
    while (s) {
        if (strcmp(s->name, name) == 0) return s;
        s = s->next;
    }
    return NULL;
}

void add_struct(const char* name, Node* fields, int field_count,
                int line, int column) {
    if (find_struct(name)) {
        fprintf(stderr, "duplicate struct: '%s'\n", name);
        return;
    }
    
    StructInfo* s = malloc(sizeof(StructInfo));
    strcpy(s->name, name);
    s->field_count = field_count;
    s->fields = malloc((size_t)field_count * sizeof(StructField));
    s->line = line;
    s->column = column;
    s->next = structtab;
    structtab = s;
    
    Node* f = fields;
    int i = 0;
    while (f) {
        strcpy(s->fields[i].name, f->value);
        strcpy(s->fields[i].type, f->data_type);
        s->fields[i].buffer_size = f->buffer_size;
        s->fields[i].is_const = f->is_declaration;
        f = f->next;
        i++;
    }
}

StructField* find_struct_field(const char* struct_name, const char* field_name) {
    StructInfo* s = find_struct(struct_name);
    if (!s) return NULL;
    
    for (int i = 0; i < s->field_count; i++) {
        if (strcmp(s->fields[i].name, field_name) == 0) {
            return &s->fields[i];
        }
    }
    return NULL;
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
        case NODE_NULL: {
    if (node->data_type[0] != '\0' &&
        strcmp(node->data_type, "auto") != 0) {
        return node->data_type;
    }
    return "null";
}

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

    // ---- reference to a real function (used without parens) ----
    FuncInfo* f = find_func(node->value);
    if (f) {
        f->use = 1;
        return f->fn_type;      // "fn(int,int)->int"
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

        // ---- Comparisons and logic -> Boolean ----

        case NODE_COMPARE:     return "bool";
case NODE_NOT:         return "bool";
case NODE_LOGICAL_OP:  return "bool";

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
    // ---- real function call ----
    FuncInfo* f = find_func(node->value);
    if (f) {
        f->use = 1;
        return f->return_type;      // "int"
    }

    // ---- fn-typed variable call ----
    Symbol* s = find_symbol(node->value);
    if (s && strncmp(s->type, "fn(", 3) == 0) {
        char* arrow = strstr(s->type, "->");
        if (arrow) {
            static char ret_buf[64];
            strncpy(ret_buf, arrow + 2, sizeof(ret_buf) - 1);
            ret_buf[sizeof(ret_buf) - 1] = '\0';
            return ret_buf;
        }
    }

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

// ---- Struct init ----

case NODE_STRUCT_INIT: {
    // node->value = "User"
    return node->value;
}

case NODE_STRUCT_ACCESS: {
    if (node->data_type[0] != '\0') {
        return node->data_type;
    }
    return "unknown";
}

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
void error_report(ErrorContext* e, char* source, int line, int column, const char* msg) {
    if (e->error_count >= MAX_ERRORS) {
        fprintf(stderr, "Too many errors: MAX: 1000\n");
        return;
    }
    
    ErrorEntry* err = &e->entries[e->error_count];
    err->line = line;
    err->column = column;
    strcpy(err->message, msg);
    e->error_count++;
    
    fprintf(stderr, "\n");
    fprintf(stderr, "line: %d, column: %d ERROR:\n", 
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
void warning_report(ErrorContext* e, char* source, int line, int column, const char* msg) {
    if (e->warning_count >= MAX_ERRORS) return;
    
    ErrorEntry* w = &e->entries[e->error_count + e->warning_count];
    w->line = line;
    w->column = column;
    strcpy(w->message, msg);
    e->warning_count++;
    
    fprintf(stderr, "\n");
    fprintf(stderr, "line: %d, column: %d WARNING:\n", line, column);
    
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
void error_syntax(ErrorContext* e, char* source, Token t, const char* expected) {
    char msg[512];
    sprintf(msg, "Expected '%s', got '%s'", expected, t.value);
    error_report(e, source, t.line, t.column, msg);
}

void error_unknown(ErrorContext* e, char* source, Token t) {
    char msg[512];
    sprintf(msg, "Unknown symbol '%s'", t.value);
    error_report(e, source, t.line, t.column, msg);
}

void error_undefined_func(ErrorContext* e, char* source, Token t) {
    char msg[512];
    sprintf(msg, "Undefined function '%s'", t.value);
    error_report(e, source, t.line, t.column, msg);
}

void error_arg_count(ErrorContext* e, char* source, Token t, int expected, int got) {
    char msg[512];
    sprintf(msg, "Function '%s' expects %d argument(s), got %d", 
            t.value, expected, got);
    error_report(e, source, t.line, t.column, msg);
}

void error_arg_type(ErrorContext* e, char* source, Token t, 
                    int arg_num, const char* expected, const char* got) {
    char msg[512];
    sprintf(msg, "Argument %d of '%s': expected %s, got %s", 
            arg_num, t.value, expected, got);
    error_report(e, source, t.line, t.column, msg);
}

void error_undefined(ErrorContext* e, char* source, Token t) {
    char msg[512];
    sprintf(msg, "Undefined variable '%s'", t.value);
    error_report(e, source, t.line, t.column, msg);
}

void error_const_assign(ErrorContext* e, char* source, Token t) {
    char msg[512];
    sprintf(msg, "Cannot assign to const variable '%s'", t.value);
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

void error_unexpected(ErrorContext* e, char* source, Token t) {
    char msg[512];
    sprintf(msg, "Unexpected '%s'", t.value);
    error_report(e, source, t.line, t.column, msg);
    }
    
    void error_duplicate_variable(ErrorContext* e, char* source, Token t) {
    char msg[512];
    sprintf(msg, "Duplicate variable '%s'", t.value);
    error_report(e, source, t.line, t.column, msg);
}

void error_duplicate_function(ErrorContext* e, char* source, Token t) {
    char msg[512];
    sprintf(msg, "Duplicate function '%s'", t.value);
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
                    case 'r': t.value[i++] = '\r'; break;
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
                    case 'r': t.value[i++] = '\r'; break;
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
        else if (strcmp(t.value, "struct") == 0) t.type = TOKEN_STRUCT;
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
Node* create_node(NodeType type, const char* value, int line, int column) {
    Node *node = (Node*)calloc(1, sizeof(Node));
    node->type = type;
    if (value) strcpy(node->value, value);
    node->line = line;
    node->column = column;
    return node;
}

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

const char* get_c_type(Node* node) {
    if (node->left) {
        const char* t = get_type_expr(node->left);
        
        // ---- [type] → pointer ----
        if (t[0] == '[') {
            char elem[64];
            size_t len = strlen(t);
            if (len >= 3) {
                strncpy(elem, t + 1, len - 2);
                elem[len - 2] = '\0';
                
                if      (strcmp(elem, "int") == 0)    return "int*";
                else if (strcmp(elem, "float") == 0)  return "double*";
                else if (strcmp(elem, "string") == 0) return "char**";
                else if (strcmp(elem, "bool") == 0)   return "int*";
                else if (strcmp(elem, "char") == 0)   return "char*";
            }
            return "int*";
        }
        
        if (strcmp(t, "int") == 0)     return "int";
        if (strcmp(t, "float") == 0)   return "double";
        if (strcmp(t, "bool") == 0)    return "int";
        if (strcmp(t, "char") == 0)    return "char";
        if (strcmp(t, "string read_file") == 0) return "char*";
        
        switch (node->left->type) {
            case NODE_NULL: {
        const char* nt = node->left->data_type;
        if (strcmp(nt, "int") == 0)    return "int";
        if (strcmp(nt, "float") == 0)  return "double";
        if (strcmp(nt, "bool") == 0)   return "int";
        if (strcmp(nt, "char") == 0)   return "char";
        if (strcmp(nt, "string") == 0) return "char*";
        if (strcmp(nt, "void") == 0)   return "void*";
        return nt;
    }
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
    const char* ret_type = get_type_expr(node->left);
    
    // ---- [type] → pointer ----
    if (ret_type[0] == '[') {
        char elem[64];
        size_t len = strlen(ret_type);
        if (len >= 3) {
            strncpy(elem, ret_type + 1, len - 2);
            elem[len - 2] = '\0';
            
            if      (strcmp(elem, "int") == 0)    return "int*";
            else if (strcmp(elem, "float") == 0)  return "double*";
            else if (strcmp(elem, "string") == 0) return "char**";
            else if (strcmp(elem, "bool") == 0)   return "int*";
            else if (strcmp(elem, "char") == 0)   return "char*";
        }
        return "int*";
    }
    
    // ---- types ----
    if (strcmp(ret_type, "int") == 0) return "int";
    if (strcmp(ret_type, "float") == 0) return "double";
    if (strcmp(ret_type, "string") == 0) return "char*";
    if (strcmp(ret_type, "bool") == 0) return "int";
    if (strcmp(ret_type, "char") == 0) return "char";
    else {
        return ret_type;
    }
    
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
    
    // ---- user-defined struct ----
    if (node->data_type[0] != '\0' &&
        strcmp(node->data_type, "auto") != 0 &&
        strcmp(node->data_type, "int") != 0 &&
        strcmp(node->data_type, "float") != 0 &&
        strcmp(node->data_type, "string") != 0 &&
        strcmp(node->data_type, "bool") != 0 &&
        strcmp(node->data_type, "char") != 0 &&
        strcmp(node->data_type, "void") != 0) {
        return node->data_type;   // ← "User"
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
    return p;
}

// ============================================================
//           PARSE FN-TYPE INTO PARAM LIST
// ============================================================
// Given a type string "fn(T1,T2)->R", build a linked list of
// NODE_PARAM nodes (name empty, data_type = Ti).
// Returns head of list, or NULL on malformed input.
// Caller must free the nodes (or accept the leak).
// ============================================================
static Node* fn_type_to_params(const char* fn_type) {
    if (!fn_type || strncmp(fn_type, "fn(", 3) != 0) return NULL;

    const char* p_open  = strchr(fn_type, '(');
    const char* p_close = strchr(fn_type, ')');
    if (!p_open || !p_close || p_close <= p_open) return NULL;

    // copy arg part
    char args[256];
    int arg_len = (int)(p_close - p_open - 1);
    if (arg_len < 0 || arg_len >= (int)sizeof(args)) return NULL;
    strncpy(args, p_open + 1, (size_t)arg_len);
    args[arg_len] = '\0';

    Node* first = NULL;
    Node* last  = NULL;

    char* saveptr = NULL;
    char* tok = strtok_r(args, ",", &saveptr);
    while (tok) {
        // trim leading spaces
        while (*tok == ' ') tok++;

        Node* param = create_node(NODE_PARAM, "", 0, 0);
        strncpy(param->data_type, tok, sizeof(param->data_type) - 1);
        param->data_type[sizeof(param->data_type) - 1] = '\0';

        if (!first) {
            first = param;
            last  = param;
        } else {
            last->next = param;
            last = param;
        }

        tok = strtok_r(NULL, ",", &saveptr);
    }

    return first;
}

static void free_param_list(Node* head) {
    while (head) {
        Node* next = head->next;
        free(head);
        head = next;
    }
}

static int check_function_call(Parser* p, Token t, Node* first_arg) {
    Node* params      = NULL;
    int   param_count = 0;
    int   params_owned = 0;   // 1 if we allocated params here

    FuncInfo* fn = find_func(t.value);
    if (fn) {
        params      = fn->params;
        param_count = fn->param_count;
    }
    else {
        Symbol* sym = find_symbol(t.value);
        if (!sym || strncmp(sym->type, "fn(", 3) != 0) {
            return 1;   // not callable; already reported
        }

        params = fn_type_to_params(sym->type);
        params_owned = 1;

        // count
        Node* n = params;
        while (n) {
            param_count++;
            n = n->next;
        }
    }

    // ---- count actual args ----
    int got = 0;
    Node* arg = first_arg;
    while (arg) {
        got++;
        arg = arg->next;
    }

    if (got != param_count) {
        error_arg_count(p->error, p->source, t, param_count, got);
        skip_to_rparen(p);
        if (params_owned) free_param_list(params);
        return 0;
    }

    // ---- type check ----
    Node* param = params;
    arg = first_arg;
    int arg_num = 1;

    while (param && arg) {
        const char* expected = param->data_type;
        const char* got_type = get_type_expr(arg);

        if (strcmp(expected, got_type) != 0) {
            error_arg_type(p->error, p->source, t, arg_num,
                           expected, got_type);
            skip_to_sync(p);
            if (params_owned) free_param_list(params);
            return 0;
        }

        param = param->next;
        arg = arg->next;
        arg_num++;
    }

    if (params_owned) free_param_list(params);
    return 1;
}

// ============================================================
// SKIP TO RPAREN — skips tokens to the closing ')'
// at the current nesting level (without touching the nested brackets).
// Used inside for(...), if(...), while(...), etc.,
// so as not to jump outside the structure.
// ============================================================
void skip_to_rparen(Parser* p) {
    int depth = 0;
    int max_steps = 10000;
    int steps = 0;

    while (p->current.type != TOKEN_EOF && steps < max_steps) {
        steps++;

        if (p->current.type == TOKEN_LPAREN) {
            depth++;
        } else if (p->current.type == TOKEN_RPAREN) {
            if (depth == 0) {
                parser_advance(p);
                return;
            }
            depth--;
        }

        if (p->current.type == TOKEN_RBRACE && depth == 0) {
            return;
        }

        parser_advance(p);
    }
}

static void skip_body(Parser* p) {
    if (p->current.type != TOKEN_LBRACE) return;

    int depth = 0;
    while (p->current.type != TOKEN_EOF) {
        if (p->current.type == TOKEN_LBRACE) depth++;
        else if (p->current.type == TOKEN_RBRACE) {
            depth--;
            parser_advance(p);
            if (depth == 0) return;
            continue;
        }
        parser_advance(p);
    }
}

// ============================================================
//              EXPECT STATEMENT END
// ============================================================
// Checks that the statement ended correctly.
// Called after every statement.
//
// Returns:
//   1 — OK  (next token can start a new statement)
//   0 — ERR (error already printed, parser skipped to sync)
//
// RULES:
//   OK  — token can legally appear after a statement:
//         * start of a new statement (const, var, if, while, ...)
//         * end of block / file (RBRACE, EOF)
//
//   ERR — token cannot appear here:
//         * closing brackets without opener ( ) ]
//         * separators ; , : .
//         * literals (number, float, string, char, boolean)
//         * identifier (missing operator?)
//         * binary operators + - * / % == != < > <= >= && ||
//         * unary operators ! ++ --
//         * opening brackets ( {  [  ← cannot start here
// ============================================================
static int expect_statement_end(Parser* p) {
    switch (p->current.type) {

        // ============================================================
        //  OK — new statement can start here
        // ============================================================

        // ---- variable declarations ----
        case TOKEN_CONST:
        case TOKEN_VAR:

        // ---- control flow ----
        case TOKEN_IF:
        case TOKEN_ELSE:
        case TOKEN_WHILE:
        case TOKEN_FOR:
        case TOKEN_BREAK:
        case TOKEN_CONTINUE:
        case TOKEN_RETURN:

        // ---- functions ----
        case TOKEN_FN:

        // ---- input / output ----
        case TOKEN_SAY:
        case TOKEN_SAY_EXCLAM:
        case TOKEN_INPUT:
        case TOKEN_PRESS:
        case TOKEN_CLS:
        case TOKEN_FATAL:

        // ---- time ----
        case TOKEN_SSLEEP:
        case TOKEN_MSLEEP:

        // ---- files ----
        case TOKEN_CREATE_FILE:
        case TOKEN_APPEND_FILE:
        case TOKEN_WRITE_FILE:
        case TOKEN_READ_FILE:
        case TOKEN_FSAY:
        case TOKEN_FSAY_EXCLAM:

        // ---- string functions ----
        case TOKEN_STRLEN:
        case TOKEN_STRUPPER:
        case TOKEN_STRLOWER:
        case TOKEN_STRTRIM:

        // ---- C insert ----
        case TOKEN_C:

        // ---- enum / struct ----
        case TOKEN_ENUM:
        case TOKEN_STRUCT:

        // ---- end of block / end of file ----
        case TOKEN_RBRACE:
        case TOKEN_EOF:
        case TOKEN_COMMENT:
        case TOKEN_IDENTIFIER:
        // identfier = missing operator or no?
            return 1;

        // ============================================================
        //  ERR — token cannot appear after a statement
        // ============================================================

        // ---- closing brackets without opener ----
        case TOKEN_RPAREN:
        case TOKEN_RBRACKET:

        // ---- separators ----
        case TOKEN_SEMICOLON:
        case TOKEN_COMMA:
        case TOKEN_COLON:
        case TOKEN_DOT:

        // ---- literals ----
        case TOKEN_NUMBER:
        case TOKEN_FLOAT:
        case TOKEN_STRING:
        case TOKEN_CHAR:
        case TOKEN_BOOLEAN:
        case TOKEN_NULL:
        
        // ---- binary operators without left operand ----
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_PERCENT:
        case TOKEN_EQ:
        case TOKEN_EQ_EQ:
        case TOKEN_NEQ:
        case TOKEN_LT:
        case TOKEN_GT:
        case TOKEN_LTE:
        case TOKEN_GTE:
        case TOKEN_AND:
        case TOKEN_OR:
        case TOKEN_NOT:
        case TOKEN_INC:
        case TOKEN_DEC:

        // ---- opening brackets (cannot start here) ----
        case TOKEN_LPAREN:
        case TOKEN_LBRACKET:
        case TOKEN_LBRACE:

        // ---- type keywords (cannot start here) ----
        case TOKEN_WORD_NUMBER:
        case TOKEN_WORD_STRING:
        case TOKEN_WORD_BOOLEAN:
        case TOKEN_WORD_CHAR:
        case TOKEN_WORD_FLOAT:
        case TOKEN_WORD_VOID:

        // ---- arrow (only inside fn type) ----
        case TOKEN_ARROW:

        // ---- special ----
        case TOKEN_UNKNOWN:
        case TOKEN_FUTURE_FEATURE: {
            error_unexpected(p->error, p->source, p->current);
            parser_advance(p);
            return 0;
        }

        // ============================================================
        //  safety net — any new token will land here
        // ============================================================
        default: {
            error_unexpected(p->error, p->source, p->current);
            skip_to_sync(p);
            return 0;
        }
    }
}

void parser_free(Parser *p) {
    if (p) {
        error_free(p->error);
        free(p);
    }
}

void parser_advance(Parser *p) {
    if (p->current.type == TOKEN_LBRACE) {
        p->brace_depth++;
    } else if (p->current.type == TOKEN_RBRACE) {
        p->brace_depth--;
    }
    
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
            p->current.type == TOKEN_SAY_EXCLAM) {
            return;
            // p->current.type == TOKEN_IDENTIFIER
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
    
    if (p->current.type == TOKEN_RPAREN ||
    p->current.type == TOKEN_RBRACKET ||
    p->current.type == TOKEN_SEMICOLON ||
    p->current.type == TOKEN_COMMA ||
    p->current.type == TOKEN_COLON) {
    error_unexpected(p->error, p->source, p->current);
    skip_to_sync(p);
    return NULL;
}
    
    block->body = first;
    
    if (own_scope) scope_exit();
    
    return block;
}
// ============================================================
// PARSE STRUCT
// ============================================================
Node* parse_struct(Parser* p) {
    parser_advance(p);  // struct
    
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
    
    while (p->current.type != TOKEN_RBRACE && p->current.type != TOKEN_EOF) {
        int is_const = 0;
        if (p->current.type == TOKEN_CONST) {
            is_const = 1;
            parser_advance(p);
        } else if (p->current.type == TOKEN_VAR) {
            parser_advance(p);
        } else {
            error_report(p->error, p->source, p->current.line, p->current.column,
                         "field must have 'const' or 'var' qualifier");
            skip_to_sync(p);
            return NULL;
        }
        
        Token fname = p->current;
        if (fname.type != TOKEN_IDENTIFIER) {
            error_expected_variable(p->error, p->source, fname);
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);
        
        int buffer_size = 0;
        if (p->current.type == TOKEN_LBRACKET) {
            parser_advance(p);
            if (p->current.type == TOKEN_NUMBER) {
                buffer_size = atoi(p->current.value);
                parser_advance(p);
            }
            if (p->current.type != TOKEN_RBRACKET) {
                error_syntax(p->error, p->source, p->current, "]");
                skip_to_sync(p);
                return NULL;
            }
            parser_advance(p);
        }
        
        char ftype[64] = "int";
        if (p->current.type == TOKEN_COLON) {
            parser_advance(p);
            
            // [type]
            if (p->current.type == TOKEN_LBRACKET) {
                parser_advance(p);
                const char* elem = NULL;
                if      (p->current.type == TOKEN_WORD_NUMBER)  elem = "int";
                else if (p->current.type == TOKEN_WORD_FLOAT)   elem = "float";
                else if (p->current.type == TOKEN_WORD_STRING)  elem = "string";
                else if (p->current.type == TOKEN_WORD_BOOLEAN) elem = "bool";
                else if (p->current.type == TOKEN_WORD_CHAR)    elem = "char";
                else if (p->current.type == TOKEN_IDENTIFIER) {
    static char elem_buf[64];
    strncpy(elem_buf, p->current.value, sizeof(elem_buf) - 1);
    elem_buf[sizeof(elem_buf) - 1] = '\0';
    elem = elem_buf;
}
                else {
                    error_unknown_type(p->error, p->source, p->current);
                    skip_to_sync(p);
                    return NULL;
                }
                parser_advance(p);
                
                if (p->current.type != TOKEN_RBRACKET) {
                    error_syntax(p->error, p->source, p->current, "]");
                    skip_to_sync(p);
                    return NULL;
                }
                parser_advance(p);
                
                snprintf(ftype, sizeof(ftype), "[%s]", elem);
            }
            // plain
            else if (p->current.type == TOKEN_WORD_NUMBER)  { strcpy(ftype, "int");    parser_advance(p); }
            else if (p->current.type == TOKEN_WORD_FLOAT)   { strcpy(ftype, "float");  parser_advance(p); }
            else if (p->current.type == TOKEN_WORD_STRING)  {
          if (buffer_size == 0) {
          // soon
          }
          strcpy(ftype, "string");
          parser_advance(p);
}
            else if (p->current.type == TOKEN_WORD_BOOLEAN) { strcpy(ftype, "bool");   parser_advance(p); }
            else if (p->current.type == TOKEN_WORD_CHAR)    { strcpy(ftype, "char");   parser_advance(p); }
            else if (p->current.type == TOKEN_IDENTIFIER)    { strcpy(ftype, p->current.value);   parser_advance(p); }
            else {
                error_unknown_type(p->error, p->source, p->current);
                skip_to_sync(p);
                return NULL;
            }
        }
        
        Node* field = create_node(NODE_STRUCT_ITEM, fname.value, fname.line, fname.column);
        strcpy(field->data_type, ftype);
        field->buffer_size = buffer_size;
        field->is_declaration = is_const;
        
        if (!first) {
            first = field;
            last = field;
        } else {
            last->next = field;
            last = field;
        }
    }
    
    if (p->current.type != TOKEN_RBRACE) {
        error_syntax(p->error, p->source, p->current, "}");
        skip_to_sync(p);
        return NULL;
    }
    parser_advance(p);
    
    Node* struct_node = create_node(NODE_STRUCT, name.value, name.line, name.column);
    struct_node->body = first;
    
    int count = 0;
    Node* f = first;
    while (f) { count++; f = f->next; }
    add_struct(name.value, first, count, name.line, name.column);
    
    if (!expect_statement_end(p)) return NULL;
    return struct_node;
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
        int qualifier = 0;  // 0 = none, 1 = const, 2 = var
    
        if (p->current.type == TOKEN_CONST) {
            qualifier = 1;
            parser_advance(p);
        } else if (p->current.type == TOKEN_VAR) {
            qualifier = 2;
            parser_advance(p);
        }
        
        // ---- qualifier is mandatory ----
if (qualifier == 0) {
    error_report(p->error, p->source,
                 p->current.line, p->current.column,
                 "parameter must have 'var' or 'const' qualifier");
    skip_to_sync(p);
    return NULL;
}
        
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
                char atype[64] = "int";
                
                // ---- [type] inside fn args ----
                if (p->current.type == TOKEN_LBRACKET) {
                    parser_advance(p);
                    
                    const char* elem = NULL;
                    if      (p->current.type == TOKEN_WORD_NUMBER)  elem = "int";
                    else if (p->current.type == TOKEN_WORD_FLOAT)   elem = "float";
                    else if (p->current.type == TOKEN_WORD_STRING)  elem = "string";
                    else if (p->current.type == TOKEN_WORD_BOOLEAN) elem = "bool";
                    else if (p->current.type == TOKEN_WORD_CHAR)    elem = "char";
                    else if (p->current.type == TOKEN_IDENTIFIER) {
    static char elem_buf[64];
    strncpy(elem_buf, p->current.value, sizeof(elem_buf) - 1);
    elem_buf[sizeof(elem_buf) - 1] = '\0';
    elem = elem_buf;
}

                    else {
                        error_unknown_type(p->error, p->source, p->current);
                        skip_to_sync(p);
                        return NULL;
                    }
                    parser_advance(p);
                    
                    if (p->current.type != TOKEN_RBRACKET) {
                        error_syntax(p->error, p->source, p->current, "]");
                        skip_to_sync(p);
                        return NULL;
                    }
                    parser_advance(p);
                    
                    snprintf(atype, sizeof(atype), "[%s]", elem);
                }
                // ---- plain type inside fn args ----
                else if (p->current.type == TOKEN_WORD_NUMBER)  { strcpy(atype, "int");    parser_advance(p); }
                else if (p->current.type == TOKEN_WORD_FLOAT)   { strcpy(atype, "float");  parser_advance(p); }
                else if (p->current.type == TOKEN_WORD_STRING)  { strcpy(atype, "string"); parser_advance(p); }
                else if (p->current.type == TOKEN_WORD_BOOLEAN) { strcpy(atype, "bool");   parser_advance(p); }
                else if (p->current.type == TOKEN_WORD_CHAR)    { strcpy(atype, "char");   parser_advance(p); }
                else if (p->current.type == TOKEN_WORD_VOID)    { strcpy(atype, "void");   parser_advance(p); }
                else if (p->current.type == TOKEN_IDENTIFIER) {
    strcpy(atype, p->current.value); parser_advance(p);
}
                else {
                    error_unknown_type(p->error, p->source, p->current);
                    skip_to_sync(p);
                    return NULL;
                }
                
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
                
                // ---- [type] as fn return ----
                if (p->current.type == TOKEN_LBRACKET) {
                    parser_advance(p);
                    
                    const char* elem = NULL;
                    if      (p->current.type == TOKEN_WORD_NUMBER)  elem = "int";
                    else if (p->current.type == TOKEN_WORD_FLOAT)   elem = "float";
                    else if (p->current.type == TOKEN_WORD_STRING)  elem = "string";
                    else if (p->current.type == TOKEN_WORD_BOOLEAN) elem = "bool";
                    else if (p->current.type == TOKEN_WORD_CHAR)    elem = "char";
                    else if (p->current.type == TOKEN_IDENTIFIER) {
    static char elem_buf[64];
    strncpy(elem_buf, p->current.value, sizeof(elem_buf) - 1);
    elem_buf[sizeof(elem_buf) - 1] = '\0';
    elem = elem_buf;
}
                    else {
                        error_unknown_type(p->error, p->source, p->current);
                        skip_to_sync(p);
                        return NULL;
                    }
                    parser_advance(p);
                    
                    if (p->current.type != TOKEN_RBRACKET) {
                        error_syntax(p->error, p->source, p->current, "]");
                        skip_to_sync(p);
                        return NULL;
                    }
                    parser_advance(p);
                    
                    static char ret_buf[64];
                    snprintf(ret_buf, sizeof(ret_buf), "[%s]", elem);
                    fn_ret = ret_buf;
                }
                // ---- plain fn return ----
                else if (p->current.type == TOKEN_WORD_NUMBER)  { fn_ret = "int";    parser_advance(p); }
                else if (p->current.type == TOKEN_WORD_FLOAT)   { fn_ret = "float";  parser_advance(p); }
                else if (p->current.type == TOKEN_WORD_STRING)  { fn_ret = "string"; parser_advance(p); }
                else if (p->current.type == TOKEN_WORD_BOOLEAN) { fn_ret = "bool";   parser_advance(p); }
                else if (p->current.type == TOKEN_WORD_CHAR)    { fn_ret = "char";   parser_advance(p); }
                else if (p->current.type == TOKEN_WORD_VOID)    { fn_ret = "void";   parser_advance(p); }
                else if (p->current.type == TOKEN_IDENTIFIER) {
    static char ret_buf[64];
    strncpy(ret_buf, p->current.value, sizeof(ret_buf) - 1);
    ret_buf[sizeof(ret_buf) - 1] = '\0';
    fn_ret = ret_buf;
    parser_advance(p);
}
                else {
                    error_unknown_type(p->error, p->source, p->current);
                    skip_to_sync(p);
                    return NULL;
                }
            }
            
            snprintf(ptype, sizeof(ptype), "fn(%s)->%s", fn_args, fn_ret);
        }
        // ---- [type] : array parameter ----
        else if (p->current.type == TOKEN_LBRACKET) {
            parser_advance(p);
            
            const char* elem = NULL;
            if      (p->current.type == TOKEN_WORD_NUMBER)  elem = "int";
            else if (p->current.type == TOKEN_WORD_FLOAT)   elem = "float";
            else if (p->current.type == TOKEN_WORD_STRING)  elem = "string";
            else if (p->current.type == TOKEN_WORD_BOOLEAN) elem = "bool";
            else if (p->current.type == TOKEN_WORD_CHAR)    elem = "char";
            else if (p->current.type == TOKEN_IDENTIFIER) {
    static char elem_buf[64];
    strncpy(elem_buf, p->current.value, sizeof(elem_buf) - 1);
    elem_buf[sizeof(elem_buf) - 1] = '\0';
    elem = elem_buf;
}
            else {
                error_unknown_type(p->error, p->source, p->current);
                skip_to_sync(p);
                return NULL;
            }
            parser_advance(p);
            
            if (p->current.type != TOKEN_RBRACKET) {
                error_syntax(p->error, p->source, p->current, "]");
                skip_to_sync(p);
                return NULL;
            }
            parser_advance(p);
            
            snprintf(ptype, sizeof(ptype), "[%s]", elem);
        }
        // ---- plain types ----
        else if (p->current.type == TOKEN_WORD_NUMBER)  { strcpy(ptype, "int");    parser_advance(p); }
        else if (p->current.type == TOKEN_WORD_FLOAT)   { strcpy(ptype, "float");  parser_advance(p); }
        else if (p->current.type == TOKEN_WORD_STRING)  { strcpy(ptype, "string"); parser_advance(p); }
        else if (p->current.type == TOKEN_WORD_BOOLEAN) { strcpy(ptype, "bool");   parser_advance(p); }
        else if (p->current.type == TOKEN_WORD_CHAR)    { strcpy(ptype, "char");   parser_advance(p); }
        else if (p->current.type == TOKEN_WORD_VOID)    { strcpy(ptype, "void");   parser_advance(p); }
        else if (p->current.type == TOKEN_IDENTIFIER)    { strcpy(ptype, p->current.value);   parser_advance(p); }
        else {
            error_unknown_type(p->error, p->source, p->current);
            skip_to_sync(p);
            return NULL;
        }
        
        Node* param = create_node(NODE_PARAM, pname.value, pname.line, pname.column);
        strcpy(param->data_type, ptype);
        param->is_declaration = qualifier;
        
        add_symbol(p, pname.value, ptype, qualifier == 1, pname.line, pname.column);
        
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
    
    if (p->current.type == TOKEN_RPAREN ||
        p->current.type == TOKEN_RBRACKET ||
        p->current.type == TOKEN_SEMICOLON ||
        p->current.type == TOKEN_COMMA ||
        p->current.type == TOKEN_COLON) {
        error_unexpected(p->error, p->source, p->current);
        skip_to_sync(p);
        return NULL;
    }
    
    // ---- return type ----
    char return_type[64] = "void";
    
    if (p->current.type == TOKEN_ARROW) {
        parser_advance(p);
        
        // ---- [type] as return type ----
        if (p->current.type == TOKEN_LBRACKET) {
            parser_advance(p);
            
            const char* elem = NULL;
            if      (p->current.type == TOKEN_WORD_NUMBER)  elem = "int";
            else if (p->current.type == TOKEN_WORD_FLOAT)   elem = "float";
            else if (p->current.type == TOKEN_WORD_STRING)  elem = "string";
            else if (p->current.type == TOKEN_WORD_BOOLEAN) elem = "bool";
            else if (p->current.type == TOKEN_WORD_CHAR)    elem = "char";
            else if (p->current.type == TOKEN_IDENTIFIER)    { elem = p->current.value; }
            else {
                error_unknown_type(p->error, p->source, p->current);
                skip_to_sync(p);
                return NULL;
            }
            parser_advance(p);
            
            if (p->current.type != TOKEN_RBRACKET) {
                error_syntax(p->error, p->source, p->current, "]");
                skip_to_sync(p);
                return NULL;
            }
            parser_advance(p);
            
            snprintf(return_type, sizeof(return_type), "[%s]", elem);
        }
        // ---- plain return types ----
        else if (p->current.type == TOKEN_WORD_NUMBER)  { strcpy(return_type, "int");    parser_advance(p); }
        else if (p->current.type == TOKEN_WORD_FLOAT)   { strcpy(return_type, "float");  parser_advance(p); }
        else if (p->current.type == TOKEN_WORD_STRING)  { strcpy(return_type, "string"); parser_advance(p); }
        else if (p->current.type == TOKEN_WORD_BOOLEAN) { strcpy(return_type, "bool");   parser_advance(p); }
        else if (p->current.type == TOKEN_WORD_CHAR)    { strcpy(return_type, "char");   parser_advance(p); }
        else if (p->current.type == TOKEN_WORD_VOID)    { strcpy(return_type, "void");   parser_advance(p); }
        else if (p->current.type == TOKEN_IDENTIFIER)   { strcpy(return_type, p->current.value); parser_advance(p); }
        else {
            error_unknown_type(p->error, p->source, p->current);
            skip_to_sync(p);
            return NULL;
        }
        // ---- [type] as return type: warn about dangling pointer ----
if (return_type[0] == '[') {
    warning_report(p->error, p->source,
                   name.line, name.column,
                   "returning [type] may produce a dangling pointer");
}

    }
    
    int pcount = 0;
    Node* pnode = param_first;
    while (pnode) {
        pcount++;
        pnode = pnode->next;
    }
    
    add_func(p, name.value, return_type, param_first, pcount, name.line, name.column);
    
    Node* body = parse_block(p, NULL);
    if (!body) return NULL;
    
    Node* fn = create_node(NODE_FN, name.value, name.line, name.column);
    strcpy(fn->data_type, return_type);
    fn->left = param_first;
    fn->body = body;
    
    scope_exit();

if (strcmp(name.value, "main") == 0) {
    has_main = 1;
}
    
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
    
    while (p->current.type == TOKEN_PERCENT ||
           p->current.type == TOKEN_STAR ||
           p->current.type == TOKEN_SLASH ||
           p->current.type == TOKEN_PLUS ||
           p->current.type == TOKEN_MINUS) {
        
        Token op = p->current;
        parser_advance(p);
        
        Node* right = parse_cond_primary(p);
        if (!right) return NULL;
        
        Node* bin = create_node(NODE_BINARY_OP, op.value, op.line, op.column);
        bin->left = left;
        bin->right = right;
        left = bin;
    }
    
    if (p->current.type == TOKEN_EQ_EQ || p->current.type == TOKEN_NEQ ||
        p->current.type == TOKEN_LT || p->current.type == TOKEN_GT ||
        p->current.type == TOKEN_LTE || p->current.type == TOKEN_GTE) {
        
        Token op = p->current;
        parser_advance(p);
        
        Node* right = parse_cond_primary(p);
        if (!right) return NULL;
        
        while (p->current.type == TOKEN_PERCENT ||
               p->current.type == TOKEN_STAR ||
               p->current.type == TOKEN_SLASH ||
               p->current.type == TOKEN_PLUS ||
               p->current.type == TOKEN_MINUS) {
            
            Token op2 = p->current;
            parser_advance(p);
            
            Node* right2 = parse_cond_primary(p);
            if (!right2) return NULL;
            
            Node* bin = create_node(NODE_BINARY_OP, op2.value, op2.line, op2.column);
            bin->left = right;
            bin->right = right2;
            right = bin;
        }
        
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

// ============================================================
//              PARSE CONDITION PRIMARY
// ============================================================
Node* parse_cond_primary(Parser* p) {
	 // ============================================================
     //  NULL LITERAL:  int null, float null, string null, ...
     // ============================================================
    {
        Token t = p->current;
        const char* ntype = NULL;
        if      (t.type == TOKEN_WORD_NUMBER)  ntype = "int";
        else if (t.type == TOKEN_WORD_FLOAT)   ntype = "float";
        else if (t.type == TOKEN_WORD_STRING)  ntype = "string";
        else if (t.type == TOKEN_WORD_BOOLEAN) ntype = "bool";
        else if (t.type == TOKEN_WORD_CHAR)    ntype = "char";
        else if (t.type == TOKEN_WORD_VOID)    ntype = "void";
        else if (p->current.type == TOKEN_IDENTIFIER) {
    // perera
}

        if (ntype != NULL) {
            parser_advance(p);

            if (p->current.type == TOKEN_NULL) {
                Node* node = create_node(NODE_NULL, "", t.line, t.column);
                strcpy(node->data_type, ntype);
                parser_advance(p);
                return node;
            }

            error_report(p->error, p->source, t.line, t.column,
                         "expected 'null' after type name");
            return NULL;
        }
    }
    
    // ============================================================
//  BARE NULL:  null
// ============================================================
if (p->current.type == TOKEN_NULL) {
    Token tn = p->current;
    if (g_allow_bare_null && g_expected_null_type != NULL) {
        Node* node = create_node(NODE_NULL, "", tn.line, tn.column);
        strcpy(node->data_type, g_expected_null_type);
        parser_advance(p);
        return node;
    }
    error_report(p->error, p->source, tn.line, tn.column,
                 "cannot infer type from bare 'null'");
    parser_advance(p);
    return NULL;
}
    
if (p->current.type == TOKEN_MINUS) {
    Token minus_t = p->current;
    parser_advance(p);
    
    Node* operand = parse_cond_primary(p);
    if (!operand) return NULL;
    
    // -x → (0 - x)
    Node* zero = create_node(NODE_NUMBER, "0", minus_t.line, minus_t.column);
    Node* neg = create_node(NODE_BINARY_OP, "-", minus_t.line, minus_t.column);
    neg->left = zero;
    neg->right = operand;
    return neg;
}

else if (p->current.type == TOKEN_PLUS) {
    parser_advance(p);
    return parse_cond_primary(p);  // +x → x
}

    else if (p->current.type == TOKEN_NOT) {
        Token not_t = p->current;
        parser_advance(p);

        Node* inner = parse_cond_primary(p);
        if (!inner) return NULL;

        Node* node = create_node(NODE_NOT, "", not_t.line, not_t.column);
        node->left = inner;
        return node;
    }

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

    // ---- arrlen(arr) in condition ----
if (p->current.type == TOKEN_IDENTIFIER && strcmp(p->current.value, "arrlen") == 0) {
    Token arrlen_t = p->current;
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
        error_report(p->error, p->source,
                     p->current.line, p->current.column,
                     "too many arguments");
        skip_to_sync(p);
        return NULL;
    }

    if (p->current.type != TOKEN_RPAREN) {
        error_syntax(p->error, p->source, p->current, ")");
        skip_to_sync(p);
        return NULL;
    }
    parser_advance(p);

    Node* node = create_node(NODE_ARRLEN, "", arrlen_t.line, arrlen_t.column);
    node->left = arg;
    return node;
}

    if (p->current.type == TOKEN_IDENTIFIER) {
        Token var_t = p->current;
        parser_advance(p);

        // ---- calling function ----
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

            if (!check_function_call(p, var_t, first_arg)) {
    return NULL;
}

            call->left = first_arg;
            return call;
        }

        // ---- var ----
Symbol* sym = find_symbol(var_t.value);
if (!sym) {
    // ---- try enum item ----
    EnumInfo* ei = enumtab;
    int enum_value = -1;
    while (ei) {
        for (int k = 0; k < ei->item_count; k++) {
            if (strcmp(ei->items[k], var_t.value) == 0) {
                enum_value = ei->values[k];
                break;
            }
        }
        if (enum_value >= 0) break;
        ei = ei->next;
    }
    
    if (enum_value >= 0) {
        Node* node = create_node(NODE_NUMBER, "",
                                 var_t.line, var_t.column);
        snprintf(node->value, sizeof(node->value), "%d", enum_value);
        return node;
    }
    
    error_undefined(p->error, p->source, var_t);
    skip_to_sync(p);
    return NULL;
}

    Node* base = create_node(NODE_IDENTIFIER, var_t.value, var_t.line, var_t.column);
    base->resolved_symbol = sym;

    // ---- postfix chain: .field  [index]  .field[index] ... ----
    while (1) {
        // ---- .field ----
        if (p->current.type == TOKEN_DOT) {
            Token dot = p->current;
            parser_advance(p);

            Token field = p->current;
            if (field.type != TOKEN_IDENTIFIER) {
                error_expected_name(p->error, p->source, field);
                skip_to_sync(p);
                return NULL;
            }
            parser_advance(p);

            const char* base_type = NULL;
            if (base->type == NODE_IDENTIFIER && base->resolved_symbol) {
                base_type = base->resolved_symbol->type;
            } else if (base->type == NODE_STRUCT_ACCESS) {
                base_type = base->data_type;
            } else if (base->type == NODE_STR_INDEX) {
                if (base->data_type[0] != '\0') base_type = base->data_type;
            }

            Node* access = create_node(NODE_STRUCT_ACCESS, field.value,
                                       dot.line, dot.column);
            access->left = base;

            if (base_type && base_type[0] != '\0') {
                StructField* sf = find_struct_field(base_type, field.value);
                if (!sf) {
                    char msg[512];
                    sprintf(msg, "Struct '%s' has no field '%s'",
                            base_type, field.value);
                    error_report(p->error, p->source,
                                 field.line, field.column, msg);
                    skip_to_sync(p);
                    return NULL;
                }
                strcpy(access->data_type, sf->type);
                access->buffer_size = sf->buffer_size;
            }

            base = access;
            continue;
        }

        // ---- [index] ----
        if (p->current.type == TOKEN_LBRACKET) {
            Token br = p->current;
            parser_advance(p);

            Node* index = parse_expression(p);
            if (!index) return NULL;

            if (p->current.type != TOKEN_RBRACKET) {
                error_syntax(p->error, p->source, p->current, "]");
                skip_to_sync(p);
                return NULL;
            }
            parser_advance(p);

            Node* idx = create_node(NODE_STR_INDEX, "", br.line, br.column);
            idx->left  = base;
            idx->right = index;

            // determine element type
            const char* base_type = NULL;
            if (base->type == NODE_IDENTIFIER && base->resolved_symbol) {
                base_type = base->resolved_symbol->type;
            } else if (base->type == NODE_STRUCT_ACCESS) {
                base_type = base->data_type;
            } else if (base->type == NODE_STR_INDEX) {
                if (base->data_type[0] != '\0') base_type = base->data_type;
            }

            if (base_type && base_type[0] == '[') {
                char elem[64];
                size_t len = strlen(base_type);
                if (len >= 3) {
                    strncpy(elem, base_type + 1, len - 2);
                    elem[len - 2] = '\0';
                    strcpy(idx->data_type, elem);
                }
            } else if (base_type && strcmp(base_type, "string") == 0) {
                strcpy(idx->data_type, "char");
            }

            base = idx;
            continue;
        }

        break;
    }

    return base;
}

    // ---- values ----
    if (p->current.type == TOKEN_BOOLEAN) {
        Node* node = create_node(NODE_BOOLEAN, p->current.value,
                                  p->current.line, p->current.column);
        parser_advance(p);
        return node;
    }

    if (p->current.type == TOKEN_NUMBER || p->current.type == TOKEN_FLOAT) {
        Node* node = create_node(NODE_NUMBER, p->current.value,
                                  p->current.line, p->current.column);
        parser_advance(p);
        return node;
    }

    if (p->current.type == TOKEN_STRING) {
        Node* node = create_node(NODE_STRING, p->current.value,
                                  p->current.line, p->current.column);
        parser_advance(p);
        return node;
    }

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

    // ============================================================
//  NULL LITERAL:  int null, float null, string null, ...
// ============================================================
{
    const char* ntype = NULL;
    if      (t.type == TOKEN_WORD_NUMBER)  ntype = "int";
    else if (t.type == TOKEN_WORD_FLOAT)   ntype = "float";
    else if (t.type == TOKEN_WORD_STRING)  ntype = "string";
    else if (t.type == TOKEN_WORD_BOOLEAN) ntype = "bool";
    else if (t.type == TOKEN_WORD_CHAR)    ntype = "char";
    else if (t.type == TOKEN_WORD_VOID)    ntype = "void";
    else if (p->current.type == TOKEN_IDENTIFIER) {
    // perera
}

    if (ntype != NULL) {
        parser_advance(p);

        if (p->current.type == TOKEN_NULL) {
            Node* node = create_node(NODE_NULL, "", line, col);
            strcpy(node->data_type, ntype);
            parser_advance(p);
            return node;
        }

        error_report(p->error, p->source, line, col,
                     "expected 'null' after type name");
        return NULL;
    }
}

// ============================================================
//  BARE NULL:  null
// ============================================================
if (t.type == TOKEN_NULL) {
    if (g_allow_bare_null && g_expected_null_type != NULL) {
        Node* node = create_node(NODE_NULL, "", line, col);
        strcpy(node->data_type, g_expected_null_type);
        parser_advance(p);
        return node;
    }
    error_report(p->error, p->source, t.line, t.column,
                 "cannot infer type from bare 'null'");
    parser_advance(p);
    return NULL;
}

if (t.type == TOKEN_MINUS) {
    parser_advance(p);
    Node* operand = parse_expression(p);
    if (!operand) return NULL;
    
    Node* zero = create_node(NODE_NUMBER, "0", line, col);
    Node* neg = create_node(NODE_BINARY_OP, "-", line, col);
    neg->left = zero;
    neg->right = operand;
    expr = neg;
}
else if (t.type == TOKEN_PLUS) {
    parser_advance(p);
    return parse_expression(p);
}

   else if (t.type == TOKEN_LPAREN) {
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
    
    else if (t.type == TOKEN_NOT) {
    parser_advance(p);

    Node* operand = parse_expression(p);
    if (!operand) return NULL;

    expr = create_node(NODE_NOT, "", t.line, t.column);
    expr->left = operand;
}

    // ---- array literal: [e1, e2, e3] ----
else if (t.type == TOKEN_LBRACKET) {
    parser_advance(p);

    Node* arr = create_node(NODE_ARRAY, "", line, col);
    Node* first_elem = NULL;
    Node* last_elem  = NULL;

if (p->current.type == TOKEN_RBRACKET) {
    error_report(p->error, p->source,
                 t.line, t.column,
                 "array brackets must have 1 element");
    skip_to_sync(p);
    return NULL;
}
else {
        while (p->current.type != TOKEN_RBRACKET &&
               p->current.type != TOKEN_EOF) {

            Node* elem = parse_expression(p);
if (!elem) return NULL;

if (elem->type == NODE_ARRAY) {
    error_report(p->error, p->source,
                 elem->line, elem->column,
                 "nested arrays are not supported in C-- 1.0");
    skip_to_sync(p);
    return NULL;
}

            if (!first_elem) {
                first_elem = elem;
                last_elem  = elem;
            } else {
                last_elem->next = elem;
                last_elem = elem;
            }

            if (p->current.type == TOKEN_COMMA) {
                parser_advance(p);
            } else {
                break;
            }
        }

        if (p->current.type != TOKEN_RBRACKET) {
            error_syntax(p->error, p->source, p->current, "]");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);

        arr->body = first_elem;
        expr = arr;
    }
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

        if (p->current.type != TOKEN_RPAREN) {
            error_syntax(p->error, p->source, p->current, ")");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);

        NodeType type = NODE_STRLEN;
        if (t.type == TOKEN_STRUPPER) type = NODE_STRUPPER;
        else if (t.type == TOKEN_STRLOWER) type = NODE_STRLOWER;
        else if (t.type == TOKEN_STRTRIM) type = NODE_STRTRIM;

        expr = create_node(type, "", t.line, t.column);
        expr->left = arg;
    }
    // ---- arrlen(arr) ----
else if (t.type == TOKEN_IDENTIFIER && strcmp(t.value, "arrlen") == 0) {
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
        error_report(p->error, p->source,
                     p->current.line, p->current.column,
                     "too many arguments");
        skip_to_sync(p);
        return NULL;
    }

    if (p->current.type != TOKEN_RPAREN) {
        error_syntax(p->error, p->source, p->current, ")");
        skip_to_sync(p);
        return NULL;
    }
    parser_advance(p);

    expr = create_node(NODE_ARRLEN, "", t.line, t.column);
    expr->left = arg;
}

    else if (t.type == TOKEN_IDENTIFIER) {
    FuncInfo* fn = find_func(t.value);
    Symbol* sym = find_symbol(t.value);
    EnumInfo* e = find_enum(t.value);
    StructInfo* st = find_struct(t.value);

    // ============================================================
    //  ENUM ACCESS:  Color.red
    // ============================================================
    if (e) {
        parser_advance(p);
        if (p->current.type == TOKEN_DOT) {
            parser_advance(p);

            Token item = p->current;
            if (item.type != TOKEN_IDENTIFIER) {
                error_expected_name(p->error, p->source, item);
                skip_to_sync(p);
                return NULL;
            }
            parser_advance(p);

            int value = find_enum_value(t.value, item.value);
            if (value < 0) {
                error_report(p->error, p->source, item.line, item.column,
                             "Undefined enum item");
                skip_to_sync(p);
                return NULL;
            }
            Node* node = create_node(NODE_NUMBER, "", item.line, item.column);
            snprintf(node->value, sizeof(node->value), "%d", value);
            return node;
        }
        error_report(p->error, p->source, t.line, t.column,
                     "expected '.' after enum name");
        skip_to_sync(p);
        return NULL;
    }

    // ============================================================
    //  STRUCT INIT:  User{...}
    // ============================================================
    if (st) {
        parser_advance(p);
        if (p->current.type != TOKEN_LBRACE) {
            error_syntax(p->error, p->source, p->current, "{");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);

        Node* init = create_node(NODE_STRUCT_INIT, t.value, line, col);
        Node* first_arg = NULL;
        Node* last_arg  = NULL;

        while (p->current.type != TOKEN_RBRACE &&
               p->current.type != TOKEN_EOF) {
            Node* arg = parse_expression(p);
            if (!arg) return NULL;

            if (!first_arg) {
                first_arg = arg;
                last_arg  = arg;
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

        if (p->current.type != TOKEN_RBRACE) {
            error_syntax(p->error, p->source, p->current, "}");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);

        init->left = first_arg;
        return init;
    }

    // ============================================================
    //  fn-typed variable?
    // ============================================================
    int sym_is_fn = (sym && strncmp(sym->type, "fn(", 3) == 0);

    // ---- unknown name ----
if (!sym && !fn && !sym_is_fn) {
    // ---- try enum item ----
    EnumInfo* ei = enumtab;
    int enum_value = -1;
    while (ei) {
        for (int k = 0; k < ei->item_count; k++) {
            if (strcmp(ei->items[k], t.value) == 0) {
                enum_value = ei->values[k];
                break;
            }
        }
        if (enum_value >= 0) break;
        ei = ei->next;
    }
    
    if (enum_value >= 0) {
        Node* node = create_node(NODE_NUMBER, "", line, col);
        snprintf(node->value, sizeof(node->value), "%d", enum_value);
        parser_advance(p);
        return node;
    }
    
    error_undefined(p->error, p->source, t);
    skip_to_sync(p);
    return NULL;
}

    // ============================================================
    //  Build base node
    // ============================================================
    Node* base = create_node(NODE_IDENTIFIER, t.value, line, col);
    if (sym) base->resolved_symbol = sym;

    parser_advance(p);

    // ============================================================
    //  CALL:  name(...)
    // ============================================================
    if (p->current.type == TOKEN_LPAREN) {
        if (!fn && !sym_is_fn) {
            error_undefined_func(p->error, p->source, t);
            skip_to_sync(p);
            return NULL;
        }

        parser_advance(p);

        Node* call = create_node(NODE_CALL, t.value, line, col);
        Node* first_arg = NULL;
        Node* last_arg  = NULL;

        while (p->current.type != TOKEN_RPAREN &&
               p->current.type != TOKEN_EOF) {
            Node* arg = parse_expression(p);
            if (!arg) return NULL;

            if (!first_arg) {
                first_arg = arg;
                last_arg  = arg;
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

        if (!check_function_call(p, t, first_arg)) {
            return NULL;
        }

        base = call;
    }

    // ============================================================
    //  POSTFIX:  .field  [index]  in any combination
    // ============================================================
    while (1) {
        // ---- .field ----
        if (p->current.type == TOKEN_DOT) {
            Token dot = p->current;
            parser_advance(p);

            Token field = p->current;
            if (field.type != TOKEN_IDENTIFIER) {
                error_expected_name(p->error, p->source, field);
                skip_to_sync(p);
                return NULL;
            }
            parser_advance(p);

            const char* base_type = NULL;
if (base->type == NODE_IDENTIFIER && base->resolved_symbol) {
    base_type = base->resolved_symbol->type;
} else if (base->type == NODE_STRUCT_ACCESS) {
    base_type = base->data_type;
} else if (base->type == NODE_STR_INDEX) {
    if (base->data_type[0] != '\0') base_type = base->data_type;
} else if (base->type == NODE_CALL) {
    const char* rt = get_type_expr(base);
    if (rt && strcmp(rt, "unknown") != 0) base_type = rt;
}

Node* access = create_node(NODE_STRUCT_ACCESS, field.value,
                           dot.line, dot.column);
access->left = base;

if (base_type && base_type[0] != '\0') {
    StructField* sf = find_struct_field(base_type, field.value);
    if (!sf) {
        char msg[512];
        sprintf(msg, "Struct '%s' has no field '%s'",
                base_type, field.value);
        error_report(p->error, p->source,
                     field.line, field.column, msg);
        skip_to_sync(p);
        return NULL;
    }
    strcpy(access->data_type, sf->type);
    access->buffer_size = sf->buffer_size;
}

base = access;
continue;
}

        // ---- [index] ----
        if (p->current.type == TOKEN_LBRACKET) {
            Token br = p->current;
            parser_advance(p);

            Node* index = parse_expression(p);
            if (!index) return NULL;

            if (p->current.type != TOKEN_RBRACKET) {
                error_syntax(p->error, p->source, p->current, "]");
                skip_to_sync(p);
                return NULL;
            }
            parser_advance(p);

            Node* idx = create_node(NODE_STR_INDEX, "", br.line, br.column);
            idx->left  = base;
            idx->right = index;

            const char* base_type = NULL;
            if (base->type == NODE_IDENTIFIER && base->resolved_symbol) {
                base_type = base->resolved_symbol->type;
            } else if (base->type == NODE_STRUCT_ACCESS) {
                base_type = base->data_type;
            } else if (base->type == NODE_STR_INDEX) {
                if (base->data_type[0] != '\0') base_type = base->data_type;
            }

            if (base_type && base_type[0] == '[') {
                char elem[64];
                size_t len = strlen(base_type);
                if (len >= 3) {
                    strncpy(elem, base_type + 1, len - 2);
                    elem[len - 2] = '\0';
                    strcpy(idx->data_type, elem);
                }
            } else if (base_type && strcmp(base_type, "string") == 0) {
                strcpy(idx->data_type, "char");
            }

            base = idx;
            continue;
        }

        break;
    }

    expr = base;
}

    // ---- + - * / % ----
    while (p->current.type == TOKEN_PLUS || p->current.type == TOKEN_MINUS ||
           p->current.type == TOKEN_STAR || p->current.type == TOKEN_SLASH ||
           p->current.type == TOKEN_PERCENT) {
        Token op = p->current;
        parser_advance(p);
        Node* right = parse_expression(p);
        if (!right) return NULL;

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

    // ---- compares: == != < > <= >= ----
    while (p->current.type == TOKEN_EQ_EQ || p->current.type == TOKEN_NEQ ||
           p->current.type == TOKEN_LT || p->current.type == TOKEN_GT ||
           p->current.type == TOKEN_LTE || p->current.type == TOKEN_GTE) {

        Token op = p->current;
        parser_advance(p);

        Node* right = parse_expression(p);
        if (!right) return NULL;

        Node* cmp = create_node(NODE_COMPARE, op.value, op.line, op.column);
        cmp->left = expr;
        cmp->right = right;
        expr = cmp;
    }

    // ---- logic: && || ----
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

    if (t.type == TOKEN_FN) {
        return parse_function(p);
    }
    else if (t.type == TOKEN_STRUCT) {
    return parse_struct(p);
}

    else if (t.type == TOKEN_LBRACE) {
        return parse_block(p, "BLOCK");
    }

    else if (t.type == TOKEN_RETURN) {
        parser_advance(p);

        Node* expr = NULL;
        if (p->current.type != TOKEN_RBRACE &&
            p->current.type != TOKEN_SEMICOLON &&
            p->current.type != TOKEN_EOF) {
            expr = parse_expression(p);
            if (!expr) return NULL;
        }

        stmt = create_node(NODE_RETURN, "", t.line, t.column);
        stmt->left = expr;

        if (!expect_statement_end(p)) return NULL;
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
                if (strcmp(p->current.value, "0") == 0) {
                    error_report(p->error, p->source, t.line, t.column, "array or string size must be at less 1");
                    skip_to_sync(p);
                    return NULL;
                }

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

    // ---- [type] ----
    if (p->current.type == TOKEN_LBRACKET) {
        parser_advance(p);

        const char* elem = NULL;
        if      (p->current.type == TOKEN_WORD_NUMBER)  elem = "int";
        else if (p->current.type == TOKEN_WORD_FLOAT)   elem = "float";
        else if (p->current.type == TOKEN_WORD_STRING)  elem = "string";
        else if (p->current.type == TOKEN_WORD_BOOLEAN) elem = "bool";
        else if (p->current.type == TOKEN_WORD_CHAR)    elem = "char";
        else if (p->current.type == TOKEN_IDENTIFIER) {
    static char elem_buf[64];
    strncpy(elem_buf, p->current.value, sizeof(elem_buf) - 1);
    elem_buf[sizeof(elem_buf) - 1] = '\0';
    elem = elem_buf;
    parser_advance(p);
}
        else {
            error_unknown_type(p->error, p->source, p->current);
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);

        if (p->current.type != TOKEN_RBRACKET) {
            error_syntax(p->error, p->source, p->current, "]");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);

        snprintf(data_type, sizeof(data_type), "[%s]", elem);
    }
    // ---- type ----
else if (p->current.type == TOKEN_WORD_NUMBER)  { strcpy(data_type, "int");    parser_advance(p); }
else if (p->current.type == TOKEN_WORD_FLOAT)   { strcpy(data_type, "float");  parser_advance(p); }
else if (p->current.type == TOKEN_WORD_STRING)  { strcpy(data_type, "string"); parser_advance(p); }
else if (p->current.type == TOKEN_WORD_BOOLEAN) { strcpy(data_type, "bool");   parser_advance(p); }
else if (p->current.type == TOKEN_WORD_CHAR)    { strcpy(data_type, "char");   parser_advance(p); }
else if (p->current.type == TOKEN_IDENTIFIER) {
    strcpy(data_type, p->current.value);
    parser_advance(p);
}
else {
    error_unknown_type(p->error, p->source, p->current);
    skip_to_sync(p);
    return NULL;
    }
}

        NodeType node_type = (t.type == TOKEN_CONST) ? NODE_CONST : NODE_VAR;
        stmt = create_node(node_type, name.value, name.line, name.column);
        stmt->buffer_size = buffer_size;
        stmt->is_dynamic = is_dynamic;
        stmt->is_declaration = 1;
        strcpy(stmt->data_type, data_type);

        if (p->current.type == TOKEN_EQ) {
    parser_advance(p);

    // ============================================================
    //  Set null-context if type annotation is known
    // ============================================================
    int saved_allow = g_allow_bare_null;
    const char* saved_type = g_expected_null_type;

    if (strcmp(data_type, "auto") != 0) {
        g_allow_bare_null = 1;
        g_expected_null_type = data_type;
    }

    Node* expr = parse_expression(p);

    // restore context
    g_allow_bare_null = saved_allow;
    g_expected_null_type = saved_type;

    if (!expr) return NULL;
    stmt->left = expr;

    if (strcmp(data_type, "auto") == 0) {
        strcpy(data_type, get_type_expr(expr));
        strcpy(stmt->data_type, data_type);
    }
    else if (expr->type == NODE_NULL) {
        strcpy(expr->data_type, data_type);
    }

            if (expr->type == NODE_ARRAY) {
                if (buffer_size == 0 && !is_dynamic) {
                    error_report(p->error, p->source,
                                 name.line, name.column,
                                 "Array must specify size: name[N] or name[]");
                    skip_to_sync(p);
                    return NULL;
                }

                if (is_dynamic) {
                    int count = 0;
                    Node* elem = expr->body;
                    while (elem) { count++; elem = elem->next; }
                    buffer_size = count;
                    stmt->buffer_size = count;
                }
            }

            if (expr->type == NODE_STRING) {
                if (buffer_size == 0 && !is_dynamic) {
                    error_report(p->error, p->source,
                                 name.line, name.column,
                                 "string must specify size: name[N] or name[]");
                    skip_to_sync(p);
                    return NULL;
                }

                if (is_dynamic) {
                    buffer_size = (int)(strlen(expr->value) + 1);
                    stmt->buffer_size = buffer_size;
                }
            }
        }

        add_symbol(p, name.value, data_type, t.type == TOKEN_CONST, name.line, name.column);

// ---- if init — NODE_NULL, symbol = NULL ----
if (stmt->left && stmt->left->type == NODE_NULL) {
    mark_symbol_nullable(name.value);
    if (strcmp(data_type, "string") == 0 && buffer_size == 0 && !is_dynamic) {
                    error_report(p->error, p->source,
                                 name.line, name.column,
                                 "string must specify size: name[N] or name[]");
                    skip_to_sync(p);
                    return NULL;
                }
}

        if (!expect_statement_end(p)) return NULL;
        return stmt;
    }

    else if (t.type == TOKEN_FATAL) {
        parser_advance(p);

        if (p->current.type != TOKEN_LPAREN) {
            error_syntax(p->error, p->source, p->current, "(");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);

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

        if (!expect_statement_end(p)) return NULL;
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
        
        int count = 0;
    Node* item = first;
    while (item) { count++; item = item->next; }
    
    add_enum(name.value, first, count, name.line, name.column);
    
        if (!expect_statement_end(p)) return NULL;
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

        if (!expect_statement_end(p)) return NULL;
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

        if (!expect_statement_end(p)) return NULL;
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

        if (!expect_statement_end(p)) return NULL;
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

        if (!expect_statement_end(p)) return NULL;
        return stmt;
    }

    else if (t.type == TOKEN_FSAY || t.type == TOKEN_FSAY_EXCLAM) {
        parser_advance(p);

        if (p->current.type != TOKEN_LPAREN) {
            error_syntax(p->error, p->source, p->current, "(");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);

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

        if (!expect_statement_end(p)) return NULL;
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

        if (!expect_statement_end(p)) return NULL;
        return stmt;
    }

    else if (t.type == TOKEN_IF) {
        parser_advance(p);

        if (p->current.type != TOKEN_LPAREN) {
            error_syntax(p->error, p->source, p->current, "(");
            skip_to_rparen(p);
            skip_body(p);
            return NULL;
        }
        parser_advance(p);

        Node* cond = parse_condition(p);
        if (!cond) {
        	skip_to_rparen(p);
            skip_body(p);
            return NULL;
        }

        if (p->current.type == TOKEN_COMMA) {
            error_report(p->error, p->source, p->current.line, p->current.column, "too many arguments");
            skip_to_rparen(p);
            skip_body(p);
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

        if (!expect_statement_end(p)) return NULL;
        return stmt;
    }

    else if (t.type == TOKEN_WHILE) {
        parser_advance(p);

        if (p->current.type != TOKEN_LPAREN) {
            error_syntax(p->error, p->source, p->current, "(");
            skip_to_rparen(p);
            skip_body(p);
            return NULL;
        }
        parser_advance(p);

        Node* cond = parse_condition(p);
        if (!cond) {
        	skip_to_rparen(p);
            skip_body(p);
            return NULL;
            }

        if (p->current.type == TOKEN_COMMA) {
            error_report(p->error, p->source, p->current.line, p->current.column, "too many arguments");
            skip_to_rparen(p);
            skip_body(p);
            return NULL;
        }

        if (p->current.type != TOKEN_RPAREN) {
            error_syntax(p->error, p->source, p->current, ")");
            skip_to_rparen(p);
            skip_body(p);
            return NULL;
        }
        parser_advance(p);

        Node* body = parse_block(p, "SCOPE WHILE");
        if (!body) return NULL;

        stmt = create_node(NODE_WHILE, "", t.line, t.column);
        stmt->cond = cond;
        stmt->body = body;

        if (!expect_statement_end(p)) return NULL;
        return stmt;
    }

    else if (t.type == TOKEN_FOR) {
    parser_advance(p);

    if (p->current.type != TOKEN_LPAREN) {
        error_syntax(p->error, p->source, p->current, "(");
        skip_to_rparen(p);
        skip_body(p);
        return NULL;
    }
    parser_advance(p);

    // ============================================================
    //  INIT:  [var] <expression>
    //         var i = 0
    //         i = 0
    //         var n = arrlen(a)
    // ============================================================
    int is_var = 0;
    if (p->current.type == TOKEN_VAR) {
        is_var = 1;
        parser_advance(p);
    }
    // const в for-init prohibited
    if (p->current.type == TOKEN_CONST) {
        error_report(p->error, p->source,
                     p->current.line, p->current.column,
                     "'const' is not allowed in for-init");
        skip_to_rparen(p);
        skip_body(p);
        return NULL;
    }

    Node* init_node = NULL;

    if (is_var) {
        // ---- var name = <expression> ----
        if (p->current.type != TOKEN_IDENTIFIER) {
            error_expected_variable(p->error, p->source, p->current);
            skip_to_sync(p);
            return NULL;
        }

        Token vname = p->current;
        parser_advance(p);

        if (p->current.type != TOKEN_EQ) {
            error_syntax(p->error, p->source, p->current, "=");
            skip_to_sync(p);
            return NULL;
        }
        parser_advance(p);

        Node* init_expr = parse_expression(p);
if (!init_expr) {
    skip_to_rparen(p);
    skip_body(p);
    return NULL;
}

        // ---- infer type ----
        const char* init_type = get_type_expr(init_expr);

        init_node = create_node(NODE_VAR, vname.value, vname.line, vname.column);
        init_node->is_declaration = 1;
        strcpy(init_node->data_type, init_type);
        init_node->left = init_expr;

        add_symbol(p, vname.value, init_type, 0, vname.line, vname.column);
    }
    else {
        // ---- bare expression:  i = 0  |  i = n  ----
        init_node = parse_expression(p);
        if (!init_node) return NULL;
    }

    if (p->current.type != TOKEN_COMMA) {
        error_syntax(p->error, p->source, p->current, ",");
        skip_to_rparen(p);
        skip_body(p);
        return NULL;
    }
    parser_advance(p);

    // ============================================================
    //  COND
    // ============================================================
    Node* cond = parse_condition(p);
if (!cond) {
    skip_to_rparen(p);
    skip_body(p);
    return NULL;
}

    if (p->current.type != TOKEN_COMMA) {
        error_syntax(p->error, p->source, p->current, ",");
        skip_to_sync(p);
        return NULL;
    }
    parser_advance(p);

    // ============================================================
    //  INC:  all expression
    //        i++
    //        i--
    //        i = i * 2
    //        i = i + step
    //        foo(i)
    // ============================================================
    Node* inc_node = NULL;

    if (p->current.type == TOKEN_RPAREN) {
        error_report(p->error, p->source,
                     p->current.line, p->current.column,
                     "for-inc cannot be empty");
        skip_to_sync(p);
        return NULL;
    }

    // ---- special-case: identifier ++ / -- ----
    if (p->current.type == TOKEN_IDENTIFIER) {
        Token inc_t = p->current;
        parser_advance(p);

        if (p->current.type == TOKEN_INC || p->current.type == TOKEN_DEC) {
            Symbol* sym = find_symbol(inc_t.value);
            if (sym && sym->is_const) {
                error_const_assign(p->error, p->source, inc_t);
                skip_to_sync(p);
                return NULL;
            }

            inc_node = create_node(NODE_VAR, inc_t.value,
                                   inc_t.line, inc_t.column);
            strcpy(inc_node->data_type,
                   (p->current.type == TOKEN_INC) ? "inc" : "dec");
            parser_advance(p);
        } else {
            // HACK: The lexer doesn't have a pushback
            Node* base = create_node(NODE_IDENTIFIER, inc_t.value,
                                     inc_t.line, inc_t.column);

            if (p->current.type == TOKEN_EQ) {
                Symbol* sym = find_symbol(inc_t.value);
                if (sym && sym->is_const) {
                    error_const_assign(p->error, p->source, inc_t);
                    skip_to_sync(p);
                    return NULL;
                }

                parser_advance(p);
                Node* rhs = parse_expression(p);
                if (!rhs) {
        	skip_to_rparen(p);
            skip_body(p);
            return NULL;
    }

                inc_node = create_node(NODE_VAR, "", inc_t.line, inc_t.column);
                inc_node->is_declaration = 0;
                inc_node->left = base;
                inc_node->right = rhs;
            }
            else {
                if (p->current.type == TOKEN_LPAREN) {
                    // calling
                    parser_advance(p);
                    Node* call = create_node(NODE_CALL, inc_t.value,
                                             inc_t.line, inc_t.column);
                    Node* first_arg = NULL;
                    Node* last_arg = NULL;

                    while (p->current.type != TOKEN_RPAREN &&
                           p->current.type != TOKEN_EOF) {
                        Node* arg = parse_expression(p);
                        if (!arg) {
        	skip_to_rparen(p);
            skip_body(p);
            return NULL;
    }

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
                    inc_node = call;
                }
                else if (p->current.type == TOKEN_LBRACKET) {
                    // arr[i]++
                    parser_advance(p);
                    Node* index = parse_expression(p);
                    if (!index) {
        	skip_to_rparen(p);
            skip_body(p);
            return NULL;
    }

                    if (p->current.type != TOKEN_RBRACKET) {
                        error_syntax(p->error, p->source, p->current, "]");
                        skip_to_sync(p);
                        return NULL;
                    }
                    parser_advance(p);

                    Node* idx = create_node(NODE_STR_INDEX, "",
                                            inc_t.line, inc_t.column);
                    idx->left = base;
                    idx->right = index;

                    if (p->current.type == TOKEN_INC ||
                        p->current.type == TOKEN_DEC) {
                        inc_node = create_node(NODE_VAR, "",
                                               inc_t.line, inc_t.column);
                        inc_node->is_declaration = 0;
                        inc_node->left = idx;
                        strcpy(inc_node->data_type,
                               (p->current.type == TOKEN_INC) ? "inc" : "dec");
                        parser_advance(p);
                    } else {
                        inc_node = idx;
                    }
                }
                else {
                    inc_node = base;
                }
            }
        }
    }
    else {
        // ---- else: parse_expression ----
        inc_node = parse_expression(p);
        if (!inc_node) {
        	skip_to_rparen(p);
            skip_body(p);
            return NULL;
    }
}

    // ============================================================
    //  errors
    // ============================================================
    if (p->current.type == TOKEN_COMMA) {
        error_report(p->error, p->source,
                     p->current.line, p->current.column,
                     "too many arguments");
        skip_to_sync(p);
        return NULL;
    }

    if (p->current.type != TOKEN_RPAREN) {
        error_syntax(p->error, p->source, p->current, ")");
        skip_to_rparen(p);
        skip_body(p);
        return NULL;
    }
    parser_advance(p);

    // ============================================================
    //  BODY
    // ============================================================
    Node* body = parse_block(p, "SCOPE FOR");
    if (!body) return NULL;

    stmt = create_node(NODE_FOR, "", t.line, t.column);
    stmt->init = init_node;
    stmt->cond = cond;
    stmt->inc = inc_node;
    stmt->body = body;

    if (!expect_statement_end(p)) return NULL;
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

        if (!expect_statement_end(p)) return NULL;

        NodeType type = (t.type == TOKEN_SAY) ? NODE_SAY : NODE_SAY_EXCLAM;
        stmt = create_node(type, "", t.line, t.column);
        stmt->left = first_arg;
        return stmt;
    }

    else if (t.type == TOKEN_IDENTIFIER) {
        Node* expr = parse_expression(p);
        if (!expr) return NULL;

        if (p->current.type == TOKEN_EQ) {
            parser_advance(p);
            Node* right_expr = parse_expression(p);
if (!right_expr) return NULL;

// ---- if assignment == null symbol = null ----
if (right_expr->type == NODE_NULL) {
    mark_symbol_nullable(t.value);
}
            
            Symbol* sym = find_symbol(t.value);
    if (sym && sym->is_const) {
        error_const_assign(p->error, p->source, t);
        skip_to_sync(p);
        return NULL;
    }
            
            stmt = create_node(NODE_VAR, "", t.line, t.column);
            stmt->is_declaration = 0;
            stmt->left = expr;
            stmt->right = right_expr;

            if (!expect_statement_end(p)) return NULL;
            return stmt;
        }

        if (p->current.type == TOKEN_INC) {
            parser_advance(p);

            Symbol* sym = find_symbol(t.value);
    if (sym && sym->is_const) {
        error_const_assign(p->error, p->source, t);
        skip_to_sync(p);
        return NULL;
    }

            stmt = create_node(NODE_VAR, t.value, t.line, t.column);
            strcpy(stmt->data_type, "inc");

            if (!expect_statement_end(p)) return NULL;
            return stmt;
        }

        if (p->current.type == TOKEN_DEC) {
            parser_advance(p);

            Symbol* sym = find_symbol(t.value);
    if (sym && sym->is_const) {
        error_const_assign(p->error, p->source, t);
        skip_to_sync(p);
        return NULL;
    }

            stmt = create_node(NODE_VAR, t.value, t.line, t.column);
            strcpy(stmt->data_type, "dec");

            if (!expect_statement_end(p)) return NULL;
            return stmt;
        }
        else {
            if (!expect_statement_end(p)) return NULL;
            return expr;
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
    } else if (p->current.type == TOKEN_STRING) {
        strcat(c_code, "\"");
        for (char* s = p->current.value; *s; s++) {
            if (*s == '\n') strcat(c_code, "\\n");
            else if (*s == '\t') strcat(c_code, "\\t");
            else if (*s == '\r') strcat(c_code, "\\r");
            else if (*s == '\\') strcat(c_code, "\\\\");
            else if (*s == '"') strcat(c_code, "\\\"");
            else {
                char buf[2] = { *s, 0 };
                strcat(c_code, buf);
            }
        }
        strcat(c_code, "\" ");
    } else if (p->current.type == TOKEN_CHAR) {
        strcat(c_code, "'");
        char c = p->current.value[0];
        if (c == '\n') strcat(c_code, "\\n");
        else if (c == '\t') strcat(c_code, "\\t");
        else if (c == '\r') strcat(c_code, "\\r");
        else if (c == '\\') strcat(c_code, "\\\\");
        else if (c == '\'') strcat(c_code, "\\'");
        else {
            char buf[2] = { c, 0 };
            strcat(c_code, buf);
        }
        strcat(c_code, "' ");
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

        if (!expect_statement_end(p)) return NULL;
        return stmt;
    }

    else if (t.type == TOKEN_BREAK) {
        parser_advance(p);
        stmt = create_node(NODE_BREAK, "", t.line, t.column);

        if (!expect_statement_end(p)) return NULL;
        return stmt;
    }

    else if (t.type == TOKEN_CONTINUE) {
        parser_advance(p);
        stmt = create_node(NODE_CONTINUE, "", t.line, t.column);

        if (!expect_statement_end(p)) return NULL;
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
    
    if (!has_main && p->error->error_count == 0) {
    error_report(p->error, p->source, 0, 0,
                 "no 'fn main()' found\n C-- requires 'fn main()' as entry point");
}
    
    program->body = first;
    return program;
}
// ============================================================
//                      TRANSLATOR
// ============================================================
void translate_expression(Node* node, FILE* out) {
    if (!node) return;
    switch (node->type) {
    	case NODE_NOT: {
    fprintf(out, "(!");
    translate_expression(node->left, out);
    fprintf(out, ")");
    break;
}

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
        case NODE_NULL: {
    const char* t = node->data_type;
    if (strcmp(t, "int") == 0 || strcmp(t, "float") == 0 || 
        strcmp(t, "bool") == 0 || strcmp(t, "char") == 0) {
        fprintf(out, "0");
    } else if (strcmp(t, "string") == 0) {
        fprintf(out, "\"\"");
    } else {
        fprintf(out, "NULL");
    }
    break;
}
        case NODE_ENUM_ACCESS: {
            fprintf(out, "%s_%s", node->left->value, node->value);
            break;
        }
        case NODE_STRUCT_INIT: {
    fprintf(out, "(%s){", node->value);
    Node* arg = node->left;
    int first = 1;
    while (arg) {
        if (!first) fprintf(out, ", ");
        first = 0;
        translate_expression(arg, out);
        arg = arg->next;
    }
    fprintf(out, "}");
    break;
}

case NODE_STRUCT_ACCESS: {
    translate_expression(node->left, out);
    fprintf(out, ".%s", node->value);
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

        fprintf(out, ", ");

        if (arg->type == NODE_NULL) {
            fprintf(out, "1");
        }
        else if (arg->type == NODE_IDENTIFIER) {
            Symbol* sym = arg->resolved_symbol;
            if (sym && sym->is_nullable) {
                fprintf(out, "%s_is_null", arg->value);
            } else {
                fprintf(out, "0");
            }
        }
        else {
            fprintf(out, "0");
        }

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
    // ============================================================
    //  x == int null  →  x_is_null
    //  x != int null  →  !x_is_null
    //  int null == x  →  x_is_null
    //  int null != x  →  !x_is_null
    // ============================================================
    if (cond->left && cond->right) {
        int eq  = (strcmp(cond->value, "==") == 0);
        int neq = (strcmp(cond->value, "!=") == 0);

        if (eq || neq) {
            // : <IDENTIFIER> <op> <NULL>
            if (cond->left->type == NODE_IDENTIFIER &&
                cond->right->type == NODE_NULL) {
                if (neq) fprintf(out, "!");
                fprintf(out, "%s_is_null", cond->left->value);
                break;
            }
            // : <NULL> <op> <IDENTIFIER>
            if (cond->left->type == NODE_NULL &&
                cond->right->type == NODE_IDENTIFIER) {
                if (neq) fprintf(out, "!");
                fprintf(out, "%s_is_null", cond->right->value);
                break;
            }
        }
    }

    // ---- logic ----
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
    char ret_type[64] = "void";
    
    if (strcmp(fn->value, "main") == 0) {
        strcpy(ret_type, "int");
    }
    // ---- [type] return -> pointer ----
    else if (fn->data_type[0] == '[') {
        char elem[64];
        size_t len = strlen(fn->data_type);
        if (len >= 3) {
            strncpy(elem, fn->data_type + 1, len - 2);
            elem[len - 2] = '\0';
            
            const char* c_elem = "int";
            if      (strcmp(elem, "int") == 0)    c_elem = "int";
            else if (strcmp(elem, "float") == 0)  c_elem = "double";
            else if (strcmp(elem, "string") == 0) c_elem = "char*";
            else if (strcmp(elem, "bool") == 0)   c_elem = "int";
            else if (strcmp(elem, "char") == 0)   c_elem = "char";
            
            snprintf(ret_type, sizeof(ret_type), "%s*", c_elem);
        } else {
            strcpy(ret_type, "int*");
        }
    }
    else if (strcmp(fn->data_type, "int") == 0)    strcpy(ret_type, "int");
    else if (strcmp(fn->data_type, "float") == 0)  strcpy(ret_type, "double");
    else if (strcmp(fn->data_type, "string") == 0) strcpy(ret_type, "char*");
    else if (strcmp(fn->data_type, "bool") == 0)   strcpy(ret_type, "int");
    else if (strcmp(fn->data_type, "char") == 0)   strcpy(ret_type, "char");
    else if (strcmp(fn->data_type, "void") == 0)   strcpy(ret_type, "void");
    else strcpy(ret_type, fn->data_type);
    
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
            char ret_part[64] = "void";
            
            char* p_open = strchr(param->data_type, '(');
            char* p_close = strchr(param->data_type, ')');
            char* p_arrow = strstr(param->data_type, "->");
            
            if (p_open && p_close && p_arrow) {
                int arg_len = (int)(p_close - p_open - 1);
                if (arg_len > 0 && arg_len < (int)sizeof(arg_part)) {
                    strncpy(arg_part, p_open + 1, (size_t)arg_len);
                    arg_part[arg_len] = '\0';
                }
                strcpy(ret_part, p_arrow + 2);
            }
            
            // ---- build C args ----
            char c_args[256] = "";
            char arg_copy[128];
            strcpy(arg_copy, arg_part);
            char* tok = strtok(arg_copy, ",");
            int fa = 1;
            while (tok) {
                if (!fa) strcat(c_args, ", ");
                fa = 0;
                
                // ---- [type] inside fn args ----
                if (tok[0] == '[') {
                    char elem[64];
                    size_t tlen = strlen(tok);
                    if (tlen >= 3) {
                        strncpy(elem, tok + 1, tlen - 2);
                        elem[tlen - 2] = '\0';
                        
                        const char* c_elem = "int";
                        if      (strcmp(elem, "int") == 0)    c_elem = "int";
                        else if (strcmp(elem, "float") == 0)  c_elem = "double";
                        else if (strcmp(elem, "string") == 0) c_elem = "char*";
                        else if (strcmp(elem, "bool") == 0)   c_elem = "int";
                        else if (strcmp(elem, "char") == 0)   c_elem = "char";
                        
                        char buf[64];
                        snprintf(buf, sizeof(buf), "%s*", c_elem);
                        strcat(c_args, buf);
                    } else {
                        strcat(c_args, "int*");
                    }
                }
                else if (strcmp(tok, "int") == 0)    strcat(c_args, "int");
                else if (strcmp(tok, "float") == 0)  strcat(c_args, "double");
                else if (strcmp(tok, "string") == 0) strcat(c_args, "char*");
                else if (strcmp(tok, "bool") == 0)   strcat(c_args, "int");
                else if (strcmp(tok, "char") == 0)   strcat(c_args, "char");
                else if (strcmp(tok, "void") == 0)   strcat(c_args, "void");
                else strcat(c_args, "int");
                
                tok = strtok(NULL, ",");
            }
            
            // ---- C return type ----
            char c_ret[64] = "int";
            if (ret_part[0] == '[') {
                char elem[64];
                size_t tlen = strlen(ret_part);
                if (tlen >= 3) {
                    strncpy(elem, ret_part + 1, tlen - 2);
                    elem[tlen - 2] = '\0';
                    
                    const char* c_elem = "int";
                    if      (strcmp(elem, "int") == 0)    c_elem = "int";
                    else if (strcmp(elem, "float") == 0)  c_elem = "double";
                    else if (strcmp(elem, "string") == 0) c_elem = "char*";
                    else if (strcmp(elem, "bool") == 0)   c_elem = "int";
                    else if (strcmp(elem, "char") == 0)   c_elem = "char";
                    
                    snprintf(c_ret, sizeof(c_ret), "%s*", c_elem);
                } else {
                    strcpy(c_ret, "int*");
                }
            }
            else if (strcmp(ret_part, "void") == 0)   strcpy(c_ret, "void");
            else if (strcmp(ret_part, "int") == 0)    strcpy(c_ret, "int");
            else if (strcmp(ret_part, "float") == 0)  strcpy(c_ret, "double");
            else if (strcmp(ret_part, "string") == 0) strcpy(c_ret, "char*");
            else if (strcmp(ret_part, "bool") == 0)   strcpy(c_ret, "int");
            else if (strcmp(ret_part, "char") == 0)   strcpy(c_ret, "char");
            
            // print: int (*name)(int, int)
            fprintf(out, "%s (*%s)(%s)", c_ret, param->value, c_args);
        }
        // ---- [type] : array parameter -> pointer in C ----
        else if (param->data_type[0] == '[') {
            char elem[64];
            size_t len = strlen(param->data_type);
            if (len < 3) {
                fprintf(out, "int*");
            } else {
                strncpy(elem, param->data_type + 1, len - 2);
                elem[len - 2] = '\0';
                
                const char* c_elem = "int";
                if      (strcmp(elem, "int") == 0)    c_elem = "int";
                else if (strcmp(elem, "float") == 0)  c_elem = "double";
                else if (strcmp(elem, "string") == 0) c_elem = "char*";
                else if (strcmp(elem, "bool") == 0)   c_elem = "int";
                else if (strcmp(elem, "char") == 0)   c_elem = "char";
                
                const char* qual = "";
                if (param->is_declaration == 1) qual = "const ";
                
                fprintf(out, "%s%s* %s", qual, c_elem, param->value);
            }
        }
        // ---- regular types ----
        else {
            const char* ptype = "int";
            if      (strcmp(param->data_type, "int") == 0)    ptype = "int";
            else if (strcmp(param->data_type, "float") == 0)  ptype = "double";
            else if (strcmp(param->data_type, "string") == 0) ptype = "char*";
            else if (strcmp(param->data_type, "bool") == 0)   ptype = "int";
            else if (strcmp(param->data_type, "char") == 0)   ptype = "char";
            else if (strcmp(param->data_type, "void") == 0)   ptype = "void";
            else ptype = param->data_type;
            
            const char* qual = "";
            if (param->is_declaration == 1) qual = "const ";
            
            fprintf(out, "%s%s %s", qual, ptype, param->value);
        }
        const char* flag_qual = "";
if (param->is_declaration == 1) flag_qual = "const ";  // const parameter

fprintf(out, ", %sint %s_is_null", flag_qual, param->value);

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
            // ---- var i = 0 ----
            if (node->is_declaration) {
                const char* ctype = get_c_type(node);
                fprintf(out, "%s %s", ctype, node->value);
                if (node->left) {
                    fprintf(out, " = ");
                    translate_expression(node->left, out);
                }
            }
            // ---- i = 0 (assignment) ----
            else if (node->left && node->right) {
                translate_expression(node->left, out);
                fprintf(out, " = ");
                translate_expression(node->right, out);
            }
            // ---- fallback ----
            else if (node->left) {
                translate_expression(node->left, out);
            }
            break;
        }
        default:
            // ---- i = 0  how NODE_IDENTIFIER и т.д. ----
            translate_expression(node, out);
            break;
    }
}

// ============================================================
//              TRANSLATE FOR-INC (no indent, no semicolon)
// ============================================================
void translate_for_inc(Node* node, FILE* out) {
    if (!node) return;

    switch (node->type) {
        case NODE_VAR: {
            // ---- i++ / i-- ----
            if (strcmp(node->data_type, "inc") == 0) {
                if (node->left) {
                    translate_expression(node->left, out);
                } else {
                    fprintf(out, "%s", node->value);
                }
                fprintf(out, "++");
            }
            else if (strcmp(node->data_type, "dec") == 0) {
                if (node->left) {
                    translate_expression(node->left, out);
                } else {
                    fprintf(out, "%s", node->value);
                }
                fprintf(out, "--");
            }
            // ---- i = i * 2 ----
            else if (node->left && node->right) {
                translate_expression(node->left, out);
                fprintf(out, " = ");
                translate_expression(node->right, out);
            }
            // ---- fallback ----
            else {
                translate_expression(node, out);
            }
            break;
        }
        default:
            translate_expression(node, out);
            break;
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
        
        case NODE_CALL: {
    indent(out, depth);
    translate_expression(node, out);
    fprintf(out, ";\n");
    break;
}
        
        case NODE_CONST: {
    const char* ctype = get_c_type(node);
    indent(out, depth);

    // ============================================================
    //  1. ARRAY LITERAL:  const arr[3] = [1, 2, 3]
    // ============================================================
    if (node->left && node->left->type == NODE_ARRAY) {
        const char* elem_type = "int";
        if (node->data_type[0] == '[') {
            char elem[64];
            size_t len = strlen(node->data_type);
            if (len >= 3) {
                strncpy(elem, node->data_type + 1, len - 2);
                elem[len - 2] = '\0';

                if      (strcmp(elem, "int") == 0)    elem_type = "int";
                else if (strcmp(elem, "float") == 0)  elem_type = "double";
                else if (strcmp(elem, "string") == 0) elem_type = "char*";
                else if (strcmp(elem, "bool") == 0)   elem_type = "int";
                else if (strcmp(elem, "char") == 0)   elem_type = "char";
            }
        }

        if (node->is_dynamic) {
        fprintf(out, "const %s %s[] = ", elem_type, node->value);
        translate_expression(node->left, out);
        fprintf(out, ";\n");
        }
        else {
        	fprintf(out, "const %s %s[%d] = ", elem_type, node->value, node->buffer_size);
        translate_expression(node->left, out);
        fprintf(out, ";\n");
        }
    }
    // ============================================================
    //  2. SIZE:  const buf[500]: string
    // ============================================================
    else if (node->buffer_size > 0) {
        const char* elem_t = ctype;
        if (strcmp(ctype, "char*") == 0) elem_t = "char";

        fprintf(out, "const %s %s[%d]",
                elem_t, node->value, node->buffer_size);
        if (node->left && node->left->type == NODE_NULL) {
            fprintf(out, " = {0}");
        } else if (node->left) {
            fprintf(out, " = ");
            translate_expression(node->left, out);
        }
        fprintf(out, ";\n");

        if (node->left && node->left->type == NODE_NULL) {
            indent(out, depth);
            fprintf(out, "int %s_is_null = 1;\n", node->value);
        }
    }
    // ============================================================
//  3. NULL INIT:  const x = int null
//     struct → {0}
//     everything else → 0
//     always → const is_null = 1
// ============================================================
else if (node->left && node->left->type == NODE_NULL) {
    const char* nt = node->left->data_type;

    // ---- struct: compound literal ----
    if (find_struct(nt)) {
        fprintf(out, "const %s %s = {0};\n", ctype, node->value);
    }
    // ---- scalar ----
    else {
        fprintf(out, "const %s %s = 0;\n", ctype, node->value);
    }

    // ---- is_null flag ----
    indent(out, depth);
    fprintf(out, "const int %s_is_null = 1;\n", node->value);
}
    // ============================================================
    //  4. STRUCT INIT:  const user = User{"Andrey", 10}
    // ============================================================
    else if (node->left && node->left->type == NODE_STRUCT_INIT) {
        fprintf(out, "%s %s = ", ctype, node->value);
        translate_expression(node->left, out);
        fprintf(out, ";\n");
    }
    // ============================================================
    //  5. REGULAR:  const x = 10
    // ============================================================
    else {
        fprintf(out, "const %s %s", ctype, node->value);
        if (node->left) {
            fprintf(out, " = ");
            translate_expression(node->left, out);
        }
        fprintf(out, ";\n");
        
        indent(out, depth);
        
        fprintf(out, "const int %s_is_null = 0;\n", node->value);
    }
    break;
}

        case NODE_VAR: {
    const char* ctype = get_c_type(node);
    indent(out, depth);

    // ============================================================
    //  1. INC / DEC
    // ============================================================
    if (strcmp(node->data_type, "inc") == 0) {
        fprintf(out, "%s++;\n", node->value);
    }
    else if (strcmp(node->data_type, "dec") == 0) {
        fprintf(out, "%s--;\n", node->value);
    }
    // ============================================================
    //  2. SIZE:  var buf[500]: string   (в т.ч. с null-init)
    // ============================================================
    else if (node->is_declaration && node->buffer_size > 0 &&
             !node->is_dynamic &&
             !(node->left && node->left->type == NODE_ARRAY)) {
        const char* elem_t = ctype;
        if (strcmp(ctype, "char*") == 0) elem_t = "char";

        fprintf(out, "%s %s[%d]",
                elem_t, node->value, node->buffer_size);
        if (node->left && node->left->type == NODE_NULL) {
            fprintf(out, " = {0}");
        } else if (node->left) {
            fprintf(out, " = ");
            translate_expression(node->left, out);
        }
        fprintf(out, ";\n");

        if (node->left && node->left->type == NODE_NULL) {
            indent(out, depth);
            fprintf(out, "int %s_is_null = 1;\n", node->value);
        }
    }
    // ============================================================
//  3. NULL INIT:  var x = int null
//     struct → {0}
//     everything else → 0
//     always → is_null = 1
// ============================================================
else if (node->left && node->left->type == NODE_NULL) {
    const char* nt = node->left->data_type;

    // ---- struct: use compound literal {0} ----
    if (find_struct(nt)) {
        fprintf(out, "%s %s = {0};\n", ctype, node->value);
    }
    // ---- everything else: scalar init ----
    else {
        fprintf(out, "%s %s = 0;\n", ctype, node->value);
    }

    // ---- is_null flag for all cases ----
    indent(out, depth);
    fprintf(out, "int %s_is_null = 1;\n", node->value);
}
    // ============================================================
    //  4. ASSIGNMENT with left and right:  x = y + 1
    // ============================================================
    else if (node->left && node->right && !node->is_declaration) {
        // ---- check if left is a string variable ----
        int is_str_assign = 0;
        if (node->left->type == NODE_IDENTIFIER &&
            node->left->resolved_symbol &&
            strcmp(node->left->resolved_symbol->type, "string") == 0) {
            is_str_assign = 1;
        }

        if (is_str_assign) {
            // ---- string assignment: strcpy ----
            fprintf(out, "strcpy(");
            translate_expression(node->left, out);
            fprintf(out, ", ");
            translate_expression(node->right, out);
            fprintf(out, ");\n");
        } else {
            // ---- normal assignment ----
            translate_expression(node->left, out);
            fprintf(out, " = ");
            translate_expression(node->right, out);
            fprintf(out, ";\n");
        }

        if (node->left->type == NODE_IDENTIFIER) {
            const char* var_name = node->left->value;
            indent(out, depth);
            if (node->right->type == NODE_NULL) {
                fprintf(out, "%s_is_null = 1;\n", var_name);
            } else {
                fprintf(out, "%s_is_null = 0;\n", var_name);
            }
        }
    }
    // ============================================================
    //  5. ASSIGNMENT single expr:  x = 10
    // ============================================================
    else if (node->left && !node->is_declaration) {
        // ---- check if it's a string variable ----
        int is_str_assign = 0;
        if (node->left->type == NODE_IDENTIFIER &&
            node->left->resolved_symbol &&
            strcmp(node->left->resolved_symbol->type, "string") == 0) {
            is_str_assign = 1;
        }

        if (is_str_assign) {
            fprintf(out, "strcpy(");
            translate_expression(node->left, out);
            fprintf(out, ", ");
            translate_expression(node->right, out);
            fprintf(out, ");\n");
        } else {
            fprintf(out, "%s = ", node->value);
            translate_expression(node->left, out);
            fprintf(out, ";\n");
        }
    }
    // ============================================================
    //  6. ARRAY LITERAL
    // ============================================================
    else if (node->left && node->left->type == NODE_ARRAY) {
        const char* elem_type = "int";
        if (node->data_type[0] == '[') {
            char elem[64];
            size_t len = strlen(node->data_type);
            if (len >= 3) {
    strncpy(elem, node->data_type + 1, len - 2);
    elem[len - 2] = '\0';

    if      (strcmp(elem, "int") == 0)    elem_type = "int";
    else if (strcmp(elem, "float") == 0)  elem_type = "double";
    else if (strcmp(elem, "string") == 0) elem_type = "char*";
    else if (strcmp(elem, "bool") == 0)   elem_type = "int";
    else if (strcmp(elem, "char") == 0)   elem_type = "char";
    else                                             elem_type = elem;
}
        }
       if (node->is_dynamic) {
        fprintf(out, "%s %s[] = ", elem_type, node->value);
        translate_expression(node->left, out);
        fprintf(out, ";\n");
        }
        else {
        	fprintf(out, "%s %s[%d] = ", elem_type, node->value, node->buffer_size);
        translate_expression(node->left, out);
        fprintf(out, ";\n");
        }
    }
    // ============================================================
    //  7. REGULAR DECLARATION:  var x = 10
    // ============================================================
    else {
        fprintf(out, "%s %s", ctype, node->value);
        if (node->left) {
            fprintf(out, " = ");
            translate_expression(node->left, out);
        }
        fprintf(out, ";\n");
        indent(out, depth);
        fprintf(out, "int %s_is_null = 0;\n", node->value);
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
            fprintf(out, "{\n");
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
            indent(out, depth);
            fprintf(out, "}\n");
            break;
        }

        case NODE_APPEND_FILE: {
        	indent(out, depth);
            fprintf(out, "{\n");
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
            indent(out, depth);
            fprintf(out, "}\n");
            break;
        }

        case NODE_WRITE_FILE: {
        	indent(out, depth);
            fprintf(out, "{\n");
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
        if (fmt_len + (int)slen < (int)sizeof(format) - 1) {
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
    Node* arg = node->left;

    while (arg) {
        const char* t = get_type_expr(arg);
        const char* spec = "%s";
        if (strcmp(t, "int") == 0)          spec = "%d";
        else if (strcmp(t, "float") == 0)   spec = "%f";
        else if (strcmp(t, "string") == 0)  spec = "%s";
        else if (strcmp(t, "char") == 0)    spec = "%c";
        else if (strcmp(t, "bool") == 0)    spec = "%d";
        else if (t[0] == '[')               spec = "%p";

        int is_nullable = 0;
        if (arg->type == NODE_IDENTIFIER &&
            arg->resolved_symbol &&
            arg->resolved_symbol->is_nullable) {
            is_nullable = 1;
        }

        indent(out, depth);

        if (is_nullable) {
            fprintf(out, "if (%s_is_null) {\n", arg->value);

            indent(out, depth + 1);
            fprintf(out, "printf(\"<NULL>\");\n");

            indent(out, depth);
            fprintf(out, "} else {\n");

            indent(out, depth + 1);
            fprintf(out, "printf(\"%s\", ", spec);
            translate_expression(arg, out);
            fprintf(out, ");\n");

            indent(out, depth);
            fprintf(out, "}\n");
        } else {
            fprintf(out, "printf(\"%s\", ", spec);
            translate_expression(arg, out);
            fprintf(out, ");\n");
        }

        arg = arg->next;
    }

    // ── if say!: \n ──
    if (node->type == NODE_SAY_EXCLAM) {
        indent(out, depth);
        fprintf(out, "printf(\"\\n\");\n");
    }
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
        if (fmt_len + (int)slen < (int)sizeof(format) - 1) {
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
        
        case NODE_STRUCT: {
    fprintf(out, "typedef struct {\n");
    Node* field = node->body;
    while (field) {
        const char* ctype = "int";

        // ---- [type] array field ----
        if (field->data_type[0] == '[') {
            char elem[64];
            size_t len = strlen(field->data_type);
            if (len >= 3) {
                strncpy(elem, field->data_type + 1, len - 2);
                elem[len - 2] = '\0';

                if      (strcmp(elem, "int") == 0)    ctype = "int";
                else if (strcmp(elem, "float") == 0)  ctype = "double";
                else if (strcmp(elem, "string") == 0) ctype = "char*";
                else if (strcmp(elem, "bool") == 0)   ctype = "int";
                else if (strcmp(elem, "char") == 0)   ctype = "char";
                else                                  ctype = elem;

                // ---- const qualifier ----
                const char* qual = field->is_declaration ? "const " : "";

                if (field->buffer_size > 0) {
                    fprintf(out, "    %s%s %s[%d];\n",
                            qual, ctype, field->value, field->buffer_size);
                } else {
                    fprintf(out, "    %s%s* %s;\n",
                            qual, ctype, field->value);
                }
                field = field->next;
                continue;
            }
        }

        // ---- plain types ----
        if      (strcmp(field->data_type, "int") == 0)    ctype = "int";
        else if (strcmp(field->data_type, "float") == 0)  ctype = "double";
        else if (strcmp(field->data_type, "string") == 0) ctype = "char";
        else if (strcmp(field->data_type, "bool") == 0)   ctype = "int";
        else if (strcmp(field->data_type, "char") == 0)   ctype = "char";
        else                                                             ctype = field->data_type;

        // ---- const qualifier ----
        const char* qual = field->is_declaration ? "const " : "";

        if (field->buffer_size > 0) {
            fprintf(out, "    %s%s %s[%d];\n",
                    qual, ctype, field->value, field->buffer_size);
        } else {
            fprintf(out, "    %s%s %s;\n",
                    qual, ctype, field->value);
        }
        field = field->next;
    }
    fprintf(out, "} %s;\n\n", node->value);
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
void translate_program(Node* ast, const char* filename) {
    FILE* out = fopen(filename, "w");
    if (!out) {
        fprintf(stderr, "ERROR: cannot open '%s' for writing\n", filename);
        return;
    }

    // ---- prelude: platform ----
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

    // ---- built-in: read_file ----
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

    // ---- built-in: strupper ----
    fprintf(out, "char* strupper(char* s) {\n");
    fprintf(out, "    char* result = strdup(s);\n");
    fprintf(out, "    for (int i = 0; result[i]; i++) {\n");
    fprintf(out, "        result[i] = toupper(result[i]);\n");
    fprintf(out, "    }\n");
    fprintf(out, "    return result;\n");
    fprintf(out, "}\n\n");

    // ---- built-in: strlower ----
    fprintf(out, "char* strlower(char* s) {\n");
    fprintf(out, "    char* result = strdup(s);\n");
    fprintf(out, "    for (int i = 0; result[i]; i++) {\n");
    fprintf(out, "        result[i] = tolower(result[i]);\n");
    fprintf(out, "    }\n");
    fprintf(out, "    return result;\n");
    fprintf(out, "}\n\n");

    // ---- built-in: strtrim ----
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
// ---- Struct typedefs (BEFORE prototypes!) ----
// ============================================================
Node* stmt = ast->body;
while (stmt) {
    if (stmt->type == NODE_STRUCT) {
        translate_statement(stmt, out, 0);
    }
    stmt = stmt->next;
}
fprintf(out, "\n");
   // ============================================================
// ---- Function prototypes ----
// ============================================================
stmt = ast->body;
while (stmt) {
    if (stmt->type == NODE_FN) {
        // ---- return type ----
        char ret_type[64] = "void";

        if (strcmp(stmt->value, "main") == 0) {
            strcpy(ret_type, "int");
        }
        // ---- [type] return -> pointer ----
        else if (stmt->data_type[0] == '[') {
            char elem[64];
            size_t len = strlen(stmt->data_type);
            if (len >= 3) {
                strncpy(elem, stmt->data_type + 1, len - 2);
                elem[len - 2] = '\0';

                const char* c_elem = "int";
                if      (strcmp(elem, "int") == 0)    c_elem = "int";
                else if (strcmp(elem, "float") == 0)  c_elem = "double";
                else if (strcmp(elem, "string") == 0) c_elem = "char*";
                else if (strcmp(elem, "bool") == 0)   c_elem = "int";
                else if (strcmp(elem, "char") == 0)   c_elem = "char";

                snprintf(ret_type, sizeof(ret_type), "%s*", c_elem);
            } else {
                strcpy(ret_type, "int*");
            }
        }
        else if (strcmp(stmt->data_type, "int") == 0)    strcpy(ret_type, "int");
        else if (strcmp(stmt->data_type, "float") == 0)  strcpy(ret_type, "double");
        else if (strcmp(stmt->data_type, "string") == 0) strcpy(ret_type, "char*");
        else if (strcmp(stmt->data_type, "bool") == 0)   strcpy(ret_type, "int");
        else if (strcmp(stmt->data_type, "char") == 0)   strcpy(ret_type, "char");
        else if (strcmp(stmt->data_type, "void") == 0)   strcpy(ret_type, "void");
        else strcpy(ret_type, stmt->data_type);

        fprintf(out, "%s %s(", ret_type, stmt->value);

        // ---- parameters ----
        Node* param = stmt->left;
        int first = 1;
        while (param) {
            if (!first) fprintf(out, ", ");
            first = 0;

            // ---- qual ----
            const char* qual = "";
            if (param->is_declaration == 1) qual = "const ";

            // ============================================================
            //  1. fn type: fn(int, int) -> int
            // ============================================================
            if (strncmp(param->data_type, "fn(", 3) == 0) {
                char arg_part[128] = "";
                char ret_part[64] = "void";

                char* p_open  = strchr(param->data_type, '(');
                char* p_close = strchr(param->data_type, ')');
                char* p_arrow = strstr(param->data_type, "->");

                if (p_open && p_close && p_arrow) {
                    int arg_len = (int)(p_close - p_open - 1);
                    if (arg_len > 0 && arg_len < (int)sizeof(arg_part)) {
                        strncpy(arg_part, p_open + 1, (size_t)arg_len);
                        arg_part[arg_len] = '\0';
                    }
                    strcpy(ret_part, p_arrow + 2);
                }

                // ---- build C args ----
                char c_args[256] = "";
                char arg_copy[128];
                strcpy(arg_copy, arg_part);
                char* tok = strtok(arg_copy, ",");
                int fa = 1;
                while (tok) {
                    if (!fa) strcat(c_args, ", ");
                    fa = 0;

                    // ---- [type] inside fn args ----
                    if (tok[0] == '[') {
                        char elem[64];
                        size_t tlen = strlen(tok);
                        if (tlen >= 3) {
                            strncpy(elem, tok + 1, tlen - 2);
                            elem[tlen - 2] = '\0';

                            const char* c_elem = "int";
                            if      (strcmp(elem, "int") == 0)    c_elem = "int";
                            else if (strcmp(elem, "float") == 0)  c_elem = "double";
                            else if (strcmp(elem, "string") == 0) c_elem = "char*";
                            else if (strcmp(elem, "bool") == 0)   c_elem = "int";
                            else if (strcmp(elem, "char") == 0)   c_elem = "char";

                            char buf[64];
                            snprintf(buf, sizeof(buf), "%s*", c_elem);
                            strcat(c_args, buf);
                        } else {
                            strcat(c_args, "int*");
                        }
                    }
                    else if (strcmp(tok, "int") == 0)    strcat(c_args, "int");
                    else if (strcmp(tok, "float") == 0)  strcat(c_args, "double");
                    else if (strcmp(tok, "string") == 0) strcat(c_args, "char*");
                    else if (strcmp(tok, "bool") == 0)   strcat(c_args, "int");
                    else if (strcmp(tok, "char") == 0)   strcat(c_args, "char");
                    else if (strcmp(tok, "void") == 0)   strcat(c_args, "void");
                    else strcat(c_args, "int");

                    tok = strtok(NULL, ",");
                }

                // ---- C return type of the fn ----
                char c_ret[64] = "int";
                if (ret_part[0] == '[') {
                    char elem[64];
                    size_t tlen = strlen(ret_part);
                    if (tlen >= 3) {
                        strncpy(elem, ret_part + 1, tlen - 2);
                        elem[tlen - 2] = '\0';

                        const char* c_elem = "int";
                        if      (strcmp(elem, "int") == 0)    c_elem = "int";
                        else if (strcmp(elem, "float") == 0)  c_elem = "double";
                        else if (strcmp(elem, "string") == 0) c_elem = "char*";
                        else if (strcmp(elem, "bool") == 0)   c_elem = "int";
                        else if (strcmp(elem, "char") == 0)   c_elem = "char";

                        snprintf(c_ret, sizeof(c_ret), "%s*", c_elem);
                    } else {
                        strcpy(c_ret, "int*");
                    }
                }
                else if (strcmp(ret_part, "void") == 0)   strcpy(c_ret, "void");
                else if (strcmp(ret_part, "int") == 0)    strcpy(c_ret, "int");
                else if (strcmp(ret_part, "float") == 0)  strcpy(c_ret, "double");
                else if (strcmp(ret_part, "string") == 0) strcpy(c_ret, "char*");
                else if (strcmp(ret_part, "bool") == 0)   strcpy(c_ret, "int");
                else if (strcmp(ret_part, "char") == 0)   strcpy(c_ret, "char");

                // ---- const / var ----
                fprintf(out, "%s%s (*%s)(%s)",
                        qual, c_ret, param->value, c_args);
            }
            // ============================================================
            //  2. [type] parameter -> pointer
            // ============================================================
            else if (param->data_type[0] == '[') {
                char elem[64];
                size_t len = strlen(param->data_type);
                if (len < 3) {
                    fprintf(out, "%sint* %s", qual, param->value);
                } else {
                    strncpy(elem, param->data_type + 1, len - 2);
                    elem[len - 2] = '\0';

                    const char* c_elem = "int";
                    if      (strcmp(elem, "int") == 0)    c_elem = "int";
                    else if (strcmp(elem, "float") == 0)  c_elem = "double";
                    else if (strcmp(elem, "string") == 0) c_elem = "char*";
                    else if (strcmp(elem, "bool") == 0)   c_elem = "int";
                    else if (strcmp(elem, "char") == 0)   c_elem = "char";

                    fprintf(out, "%s%s* %s", qual, c_elem, param->value);
                }
            }
            // ============================================================
            //  3. regular type
            // ============================================================
            else {
        const char* ptype = "int";
        if      (strcmp(param->data_type, "int") == 0)    ptype = "int";
        else if (strcmp(param->data_type, "float") == 0)  ptype = "double";
        else if (strcmp(param->data_type, "string") == 0) ptype = "char*";
        else if (strcmp(param->data_type, "bool") == 0)   ptype = "int";
        else if (strcmp(param->data_type, "char") == 0)   ptype = "char";
        else if (strcmp(param->data_type, "void") == 0)   ptype = "void";
        else                                                                ptype = param->data_type;

        fprintf(out, "%s%s %s", qual, ptype, param->value);
    }

            // ============================================================
            //  _is_null flag for every parameter
            // ============================================================
            fprintf(out, ", %sint %s_is_null", qual, param->value);

            param = param->next;
        }

        fprintf(out, ");\n");
    }
    stmt = stmt->next;
}
fprintf(out, "\n");

    // ============================================================
    // ---- Function definitions ----
    // ============================================================
    stmt = ast->body;
    while (stmt) {
    	if (stmt->type != NODE_STRUCT) {
        translate_statement(stmt, out, 0);
        }
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
        case NODE_STR_INDEX:    return "STR_INDEX";
        case NODE_ARRLEN:       return "ARRLEN";
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
        case NODE_STRUCT:         return "STRUCT";
case NODE_STRUCT_ITEM:    return "STRUCT_ITEM";
case NODE_STRUCT_INIT:    return "STRUCT_INIT";
case NODE_STRUCT_ACCESS:  return "STRUCT_ACCESS";
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
        const char* ctype = get_c_type(node);
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
static void print_ast_rec(Node* node, int depth, int is_last) {
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
    if (node->next) {
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
printf("%-4s %-15s %-15s %-6s %-9s\n", "#", "NAME", "TYPE", "CONST", "NULLABLE");
for (int k = 0; k < indent + 2; k++) printf(" ");
printf("──────────────────────────────────────────────────────\n");

Symbol* sym = s->symbols;
int local = 0;
while (sym) {
    for (int k = 0; k < indent + 2; k++) printf(" ");
    printf("%-4d %-15s %-15s %-6s %-9s\n",
           local, sym->name, sym->type,
           sym->is_const ? "yes" : "no",
           sym->is_nullable ? "yes" : "no");
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
//              PRINT ERROR/WARNING SUMMARY
// ============================================================
// Prints total count of errors and warnings to stderr.
// Called at the end of parsing, before exit.
// ============================================================
void error_summary(ErrorContext* e) {
    if (e->error_count == 0 && e->warning_count == 0) {
        return;
    }
    
    fprintf(stderr, "\n");
    
    if (e->error_count > 0) {
        fprintf(stderr, "Total errors: %d\n", e->error_count);
    }
    if (e->warning_count > 0) {
        fprintf(stderr, "Total warnings: %d\n", e->warning_count);
    }
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
//              FREE FUNCTIONS (memory cleanup)
// ============================================================

// ---- Free function table ----
void free_functab(void) {
    FuncInfo* f = functab;
    while (f) {
        FuncInfo* next = f->next;
        free(f);
        f = next;
    }
    functab = NULL;
}

// ---- Free enum table ----
void free_enumtab(void) {
    EnumInfo* e = enumtab;
    while (e) {
        EnumInfo* next = e->next;
        
        if (e->items) {
            for (int i = 0; i < e->item_count; i++) {
                if (e->items[i]) free(e->items[i]);
            }
            free(e->items);
        }
        
        if (e->values) free(e->values);
        
        free(e);
        e = next;
    }
    enumtab = NULL;
}

// ---- Free struct table ----
void free_structtab(void) {
    StructInfo* s = structtab;
    while (s) {
        StructInfo* next = s->next;
        
        if (s->fields) free(s->fields);
        
        free(s);
        s = next;
    }
    structtab = NULL;
}

// ---- Free all scopes and their symbols ----
void free_scopes(void) {
    Scope* s = all_scopes;
    while (s) {
        Scope* next = s->next_all;
        
        Symbol* sym = s->symbols;
        while (sym) {
            Symbol* next_sym = sym->next;
            free(sym);
            sym = next_sym;
        }
        
        free(s);
        s = next;
    }
    all_scopes = NULL;
    current_scope = NULL;
}

// ---- Free the entire AST ----
void free_ast(Node* node) {
    if (!node) return;
    
    free_ast(node->left);
    free_ast(node->right);
    free_ast(node->body);
    free_ast(node->init);
    free_ast(node->cond);
    free_ast(node->inc);
    
    free_ast(node->next);
    
    free(node);
}

// ---- Free everything at once ----
void free_all(Node* ast) {
    free_ast(ast);
    free_scopes();
    free_functab();
    free_enumtab();
    free_structtab();
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
            fprintf(stderr, "GCMT+-: Source too large (max %d bytes)\n", MAX_SOURCE);
            return 1;
        }
    }
    
    if (strlen(source) == 0) {
        fprintf(stderr, "GCMT+-: No input provided\n");
        return 1;
    }
    
    printf("========================================\n");
    printf("              SOURCE CODE\n");
    printf("========================================\n");
    printf("%s", source);
    printf("========================================\n\n");
    
    clock_t start = clock();
    
    Lexer *l = lexer_init(source);
    Parser* p = parser_init(l, source);
    Node* ast = parse_program(p);
    
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    
    print_warnings(p->error, source);
    error_summary(p->error);
    
    if (p->error->error_count == 0) { 
    print_ast(ast, 0);
    print_symtab();
    
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
    }
    
    if (elapsed < 0.001) {
    fprintf(stderr, "[LOG] Parsed and ast in %.0f µs\n", elapsed * 1e6);
} else if (elapsed < 1.0) {
    fprintf(stderr, "[LOG] Parsed and ast in %.3f ms\n", elapsed * 1e3);
} else {
    fprintf(stderr, "[LOG] Parsed and ast in %.3f s\n", elapsed);
}
    free_all(ast);
    parser_free(p);
    lexer_free(l);
    
    return 0;
}