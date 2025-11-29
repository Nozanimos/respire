// SPDX-License-Identifier: GPL-3.0-or-later
#include "debug.h"


FILE *debug_file = NULL;
int verbose_mode = 0;  // 0 = normal, 1 = verbose

void debug_printf(const char *format, ...) {
    if (debug_file) {
        va_list args;
        va_start(args, format);
        vfprintf(debug_file, format, args);
        va_end(args);
        fflush(debug_file);  // ✅ Forcer l'écriture
    }
    /*// ✅ OPTIONNEL : Afficher aussi dans la console
    va_list args2;
    va_start(args2, format);
    vprintf(format, args2);
    va_end(args2);*/
}

void debug_verbose(const char *format, ...) {
    if (debug_file && verbose_mode) {
        va_list args;
        va_start(args, format);
        vfprintf(debug_file, format, args);
        va_end(args);
        fflush(debug_file);
    }
}

//  FONCTIONS D'AMÉLIORATION VISUELLE DU DEBUG

void debug_blank_line() {
    if (debug_file) {
        fprintf(debug_file, "\n");
        fflush(debug_file);
    }
}

void debug_separator() {
    if (debug_file) {
        fprintf(debug_file, "────────────────────────────────────────────────────────────────────────────────\n");
        fflush(debug_file);
    }
}

void debug_section(const char* title) {
    if (debug_file) {
        fprintf(debug_file, "\n");
        fprintf(debug_file, "════════════════════════════════════════════════════════════════════════════════\n");
        fprintf(debug_file, "  %s\n", title);
        fprintf(debug_file, "════════════════════════════════════════════════════════════════════════════════\n");
        fflush(debug_file);
    }
}

void debug_subsection(const char* title) {
    if (debug_file) {
        fprintf(debug_file, "\n");
        fprintf(debug_file, "─── %s ───\n", title);
        fflush(debug_file);
    }
}
