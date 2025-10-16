#include "debug.h"


FILE *debug_file = NULL;

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
