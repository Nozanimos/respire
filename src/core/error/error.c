// SPDX-License-Identifier: GPL-3.0-or-later
#include "error.h"
#include "core/debug.h"
#include <string.h>

void error_set(Error* err, ErrorCode code, const char* message,
               const char* file, int line, const char* function) {
    if (!err) return;

    err->code = code;
    err->message = message;
    err->file = file;
    err->line = line;
    err->function = function;
}

void error_print(const Error* err) {
    if (!err || err->code == ERR_NONE) return;

    debug_printf("❌ ERREUR [%s]:\n", error_code_to_string(err->code));
    if (err->message) {
        debug_printf("   Message  : %s\n", err->message);
    }
    if (err->function) {
        debug_printf("   Fonction : %s()\n", err->function);
    }
    if (err->file) {
        debug_printf("   Fichier  : %s:%d\n", err->file, err->line);
    }
}

const char* error_code_to_string(ErrorCode code) {
    switch (code) {
        case ERR_NONE:           return "NONE";
        case ERR_ALLOC:          return "ALLOCATION_FAILED";
        case ERR_FILE_NOT_FOUND: return "FILE_NOT_FOUND";
        case ERR_FILE_READ:      return "FILE_READ_ERROR";
        case ERR_FILE_WRITE:     return "FILE_WRITE_ERROR";
        case ERR_INVALID_PARAM:  return "INVALID_PARAMETER";
        case ERR_NULL_POINTER:   return "NULL_POINTER";
        case ERR_SDL:            return "SDL_ERROR";
        case ERR_TTF:            return "TTF_ERROR";
        case ERR_IMG:            return "IMAGE_ERROR";
        case ERR_JSON_PARSE:     return "JSON_PARSE_ERROR";
        case ERR_INIT:           return "INITIALIZATION_ERROR";
        case ERR_UNKNOWN:        return "UNKNOWN_ERROR";
        default:                 return "UNDEFINED_ERROR";
    }
}
