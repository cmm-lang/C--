#!/bin/bash
# ============================================================
#  CLANG MAX WARNINGS — strict but practical
#  No -Weverything (too much noise)
#  No -Wpadded, no -Wsign-conversion (pure noise for this code)
# ============================================================

FILE="$1"
OUT="${2:-program}"

if [ -z "$FILE" ]; then
    echo "Usage: $0 <file.c> [output]"
    exit 1
fi

clang \
    -std=c11 \
    -O2 \
    \
    `# === BASELINE ===` \
    -Wall \
    -Wextra \
    -Wpedantic \
    \
    `# === UNUSED / DEAD CODE ===` \
    -Wunused-variable \
    -Wunused-parameter \
    -Wunused-function \
    -Wunused-value \
    -Wunused-label \
    -Wunused-local-typedef \
    -Wunused-but-set-variable \
    -Wunreachable-code \
    \
    `# === SHADOWING ===` \
    -Wshadow \
    -Wshadow-field \
    \
    `# === INITIALIZATION ===` \
    -Wuninitialized \
    -Wconditional-uninitialized \
    -Wsometimes-uninitialized \
    \
    `# === TYPES ===` \
    -Wconversion \
    -Wimplicit-int-conversion \
    -Wimplicit-float-conversion \
    -Wimplicit-const-int-float-conversion \
    -Wshorten-64-to-32 \
    -Wconstant-conversion \
    -Wint-conversion \
    -Wpointer-sign \
    -Wpointer-to-int-cast \
    -Wint-to-pointer-cast \
    -Wint-to-void-pointer-cast \
    -Wenum-conversion \
    -Wbool-conversion \
    -Wbool-operation \
    \
    `# === POINTERS / MEMORY ===` \
    -Wpointer-arith \
    -Wnull-dereference \
    -Wnull-pointer-arithmetic \
    -Wcast-align \
    -Wcast-qual \
    -Wstrict-aliasing \
    -Wsizeof-array-div \
    -Wsizeof-pointer-div \
    -Wsizeof-pointer-memaccess \
    -Warray-bounds \
    \
    `# === FORMAT STRINGS ===` \
    -Wformat \
    -Wformat-security \
    -Wformat-nonliteral \
    -Wformat-overflow \
    -Wformat-truncation \
    \
    `# === FUNCTIONS ===` \
    -Wmissing-prototypes \
    -Wmissing-variable-declarations \
    -Wmissing-noreturn \
    -Wmissing-braces \
    -Wreturn-type \
    -Wreturn-stack-address \
    -Wstrict-prototypes \
    -Wold-style-definition \
    -Wmain \
    \
    `# === STRUCTURES ===` \
    -Wpacked \
    -Wmissing-field-initializers \
    \
    `# === STRINGS ===` \
    -Wwritable-strings \
    -Wstring-concatenation \
    -Wstring-compare \
    -Wstring-plus-int \
    \
    `# === LOGIC / COMPARISONS ===` \
    -Wparentheses \
    -Wmisleading-indentation \
    -Wlogical-op-parentheses \
    -Wlogical-not-parentheses \
    -Wtautological-compare \
    -Wtautological-constant-out-of-range-compare \
    -Wtautological-pointer-compare \
    -Wshift-op-parentheses \
    -Wbitwise-op-parentheses \
    -Wshift-count-overflow \
    -Wdangling-else \
    -Wempty-body \
    -Wimplicit-fallthrough \
    -Wcomma \
    \
    `# === SWITCH ===` \
    -Wswitch \
    -Wswitch-default \
    -Wswitch-bool \
    -Wduplicate-enum \
    \
    `# === LOOPS ===` \
    -Wfor-loop-analysis \
    -Wloop-analysis \
    \
    `# === PREPROCESSOR ===` \
    -Wundef \
    -Wmacro-redefined \
    -Wexpansion-to-defined \
    -Wheader-guard \
    -Wpragma-once-outside-header \
    \
    `# === MISC ===` \
    -Wvla \
    -Wdeprecated \
    -Wdeprecated-implementations \
    \
    `# === PROMOTE TO ERRORS (critical) ===` \
    -Werror=return-type \
    -Werror=implicit-function-declaration \
    -Werror=incompatible-pointer-types \
    -Werror=implicit-int \
    -Werror=int-conversion \
    -Werror=uninitialized \
    -Werror=sizeof-pointer-memaccess \
    \
    "$FILE" -o "$OUT"

RC=$?
if [ $RC -eq 0 ]; then
    echo "✅ Compiled: $OUT"
else
    echo "❌ Compilation failed"
    exit $RC
fi
