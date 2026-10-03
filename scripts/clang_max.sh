#!/bin/bash
# ============================================================
#  CLANG MAX WARNINGS — критические + важные
#  Не используем -Weverything (слишком много мусора)
#  Только то, что РЕАЛЬНО важно
# ============================================================

FILE="$1"
OUT="${2:-program}"

if [ -z "$FILE" ]; then
    echo "Использование: $0 file.c [output]"
    exit 1
fi

clang \
    -std=c11 \
    -O2 \
    \
    `# === БАЗОВЫЕ ===` \
    -Wall \
    -Wextra \
    -Wpedantic \
    \
    `# === ПЕРЕМЕННЫЕ ===` \
    -Wuninitialized \
    -Wunused-variable \
    -Wunused-parameter \
    -Wunused-function \
    -Wunused-value \
    -Wunused-macros \
    -Wunused-label \
    -Wunused-local-typedef \
    -Wunused-but-set-variable \
    -Wshadow \
    -Wshadow-field \
    -Wshadow-uncaptured-local \
    -Wunreachable-code \
    \
    `# === ТИПЫ ===` \
    -Wconversion \
    -Wsign-conversion \
    -Wsign-compare \
    -Wimplicit-int-conversion \
    -Wimplicit-float-conversion \
    -Wshorten-64-to-32 \
    -Wconstant-conversion \
    -Wint-conversion \
    -Wpointer-sign \
    -Wpointer-to-int-cast \
    -Wint-to-pointer-cast \
    -Wenum-conversion \
    \
    `# === УКАЗАТЕЛИ И ПАМЯТЬ ===` \
    -Wpointer-arith \
    -Wnull-dereference \
    -Wnull-pointer-arithmetic \
    -Wformat \
    -Wformat-security \
    -Wformat-nonliteral \
    -Wformat-overflow \
    -Wformat-truncation \
    -Wstrlcpy-strlcat-size \
    \
    `# === ФУНКЦИИ ===` \
    -Wmissing-prototypes \
    -Wmissing-variable-declarations \
    -Wmissing-noreturn \
    -Wreturn-type \
    -Wreturn-stack-address \
    -Wmissing-braces \
    -Wstrict-prototypes \
    -Wold-style-definition \
    -Wmain \
    \
    `# === СТРУКТУРЫ ===` \
    -Wpadded \
    -Wpacked \
    -Wmissing-field-initializers \
    \
    `# === СТРОКИ ===` \
    -Wwritable-strings \
    -Wstring-concatenation \
    -Wstring-compare \
    -Wstring-plus-int \
    \
    `# === ЛОГИКА ===` \
    -Wparentheses \
    -Wmisleading-indentation \
    -Wlogical-op-parentheses \
    -Wlogical-not-parentheses \
    -Wtautological-compare \
    -Wtautological-constant-out-of-range-compare \
    -Wtautological-pointer-compare \
    -Wbool-conversion \
    -Wbool-operation \
    -Wshift-op-parentheses \
    -Wbitwise-op-parentheses \
    -Wdangling-else \
    -Wempty-body \
    -Wimplicit-fallthrough \
    -Wswitch \
    -Wswitch-enum \
    -Wswitch-default \
    -Wcovered-switch-default \
    \
    `# === ЦИКЛЫ ===` \
    -Wfor-loop-analysis \
    -Wloop-analysis \
    \
    `# === МНОГОПОТОЧНОСТЬ ===` \
    -Wthread-safety \
    \
    `# === ПРЕПРОЦЕССОР ===` \
    -Wundef \
    -Wmacro-redefined \
    -Wexpansion-to-defined \
    \
    `# === БЕЗОПАСНОСТЬ ===` \
    -Wcast-align \
    -Wcast-qual \
    -Wconditional-uninitialized \
    -Wdeprecated \
    -Wdeprecated-implementations \
    -Wdocumentation \
    -Wstrict-aliasing \
    \
    `# === ОШИБКИ, А НЕ WARNINGS ===` \
    -Werror=return-type \
    -Werror=implicit-function-declaration \
    -Werror=incompatible-pointer-types \
    -Werror=implicit-int \
    -Werror=int-conversion \
    -Werror=uninitialized \
    \
    "$FILE" -o "$OUT"

echo "Скомпилировано: $OUT"
