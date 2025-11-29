// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdarg.h>

// Fonctions de debug
void init_debug_mode(int argc, char **argv);
void cleanup_debug_mode();
void debug_printf(const char *format, ...);
void debug_verbose(const char *format, ...);  // Pour les messages verbose uniquement

extern FILE *debug_file;
extern int verbose_mode;  // 0 = normal, 1 = verbose

// Fonctions d'amélioration visuelle du debug
void debug_separator();              // Ligne de séparation fine
void debug_section(const char* title); // Section avec titre encadré
void debug_blank_line();              // Ligne vide pour aérer
void debug_subsection(const char* title); // Sous-section

// Macros pratiques pour différents niveaux de log
#ifdef DEBUG_MODE
    #define DEBUG_INFO(...) debug_printf("ℹ️ [INFO] " __VA_ARGS__)
    #define DEBUG_WARN(...) debug_printf("⚠️ [WARN] " __VA_ARGS__)
    #define DEBUG_ERROR(...) debug_printf("❌ [ERROR] " __VA_ARGS__)
    #define DEBUG_TRACE(...) debug_printf("🔍 [TRACE] " __VA_ARGS__)
#else
    #define LOG_INFO(...)
    #define LOG_WARN(...)
    #define LOG_ERROR(...)
    #define LOG_TRACE(...)
#endif

#endif
