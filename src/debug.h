#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdarg.h>

// Fonctions de debug
void init_debug_mode(int argc, char **argv);
void cleanup_debug_mode();
void debug_printf(const char *format, ...);
extern FILE *debug_file;

// Macros pratiques pour différents niveaux de log
#ifdef DEBUG_MODE
    #define LOG_INFO(...) printf("[INFO] " __VA_ARGS__)
    #define LOG_WARN(...) printf("[WARN] " __VA_ARGS__)
    #define LOG_ERROR(...) printf("[ERROR] " __VA_ARGS__)
    #define LOG_DEBUG(...) printf("[DEBUG] " __VA_ARGS__)
#else
    #define LOG_INFO(...)
    #define LOG_WARN(...)
    #define LOG_ERROR(...)
    #define LOG_DEBUG(...)
#endif

#endif
