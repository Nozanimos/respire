// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef ERROR_H
#define ERROR_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Codes d'erreur standardisés pour l'application
 */
typedef enum {
    ERR_NONE = 0,              // Pas d'erreur
    ERR_ALLOC,                 // Échec allocation mémoire
    ERR_FILE_NOT_FOUND,        // Fichier introuvable
    ERR_FILE_READ,             // Erreur lecture fichier
    ERR_FILE_WRITE,            // Erreur écriture fichier
    ERR_INVALID_PARAM,         // Paramètre invalide
    ERR_NULL_POINTER,          // Pointeur NULL inattendu
    ERR_SDL,                   // Erreur SDL
    ERR_TTF,                   // Erreur SDL_TTF
    ERR_IMG,                   // Erreur SDL_Image
    ERR_JSON_PARSE,            // Erreur parsing JSON
    ERR_INIT,                  // Erreur initialisation
    ERR_UNKNOWN                // Erreur inconnue
} ErrorCode;

/**
 * @brief Structure contenant les détails d'une erreur
 */
typedef struct {
    ErrorCode code;            // Code d'erreur
    const char* message;       // Message descriptif
    const char* file;          // Fichier source où l'erreur s'est produite
    int line;                  // Ligne source
    const char* function;      // Fonction où l'erreur s'est produite
} Error;

/**
 * @brief Initialise une structure Error à ERR_NONE
 *
 * @param err Pointeur vers la structure à initialiser
 */
static inline void error_init(Error* err) {
    if (err) {
        err->code = ERR_NONE;
        err->message = NULL;
        err->file = NULL;
        err->line = 0;
        err->function = NULL;
    }
}

/**
 * @brief Vérifie si une erreur s'est produite
 *
 * @param err Pointeur vers la structure Error
 * @return true si une erreur est présente
 */
static inline bool error_occurred(const Error* err) {
    return (err && err->code != ERR_NONE);
}

/**
 * @brief Définit une erreur avec tous les détails
 *
 * @param err Pointeur vers la structure Error
 * @param code Code d'erreur
 * @param message Message descriptif
 * @param file Fichier source (utiliser __FILE__)
 * @param line Ligne source (utiliser __LINE__)
 * @param function Fonction (utiliser __func__)
 */
void error_set(Error* err, ErrorCode code, const char* message,
               const char* file, int line, const char* function);

/**
 * @brief Affiche l'erreur dans les logs de debug
 *
 * @param err Pointeur vers la structure Error
 */
void error_print(const Error* err);

/**
 * @brief Retourne une description textuelle du code d'erreur
 *
 * @param code Code d'erreur
 * @return Chaîne de caractères constante décrivant l'erreur
 */
const char* error_code_to_string(ErrorCode code);

// MACROS UTILITAIRES

/**
 * @brief Définit une erreur avec localisation automatique
 *
 * Usage: SET_ERROR(err, ERR_ALLOC, "Échec allocation widget");
 */
#define SET_ERROR(err, error_code, msg) \
    error_set((err), (error_code), (msg), __FILE__, __LINE__, __func__)

/**
 * @brief Vérifie une condition et définit une erreur si fausse
 *
 * Usage: CHECK(ptr != NULL, err, ERR_NULL_POINTER, "Widget est NULL");
 */
#define CHECK(condition, err, error_code, msg) \
    do { \
        if (!(condition)) { \
            SET_ERROR((err), (error_code), (msg)); \
            goto cleanup; \
        } \
    } while(0)

/**
 * @brief Vérifie allocation mémoire et définit erreur si échec
 *
 * Usage: CHECK_ALLOC(ptr, err, "Échec allocation widget");
 */
#define CHECK_ALLOC(ptr, err, msg) \
    CHECK((ptr) != NULL, (err), ERR_ALLOC, (msg))

/**
 * @brief Vérifie pointeur non NULL et définit erreur si NULL
 *
 * Usage: CHECK_PTR(widget, err, "Widget est NULL");
 */
#define CHECK_PTR(ptr, err, msg) \
    CHECK((ptr) != NULL, (err), ERR_NULL_POINTER, (msg))

/**
 * @brief Vérifie paramètre et définit erreur si invalide
 *
 * Usage: CHECK_PARAM(size > 0, err, "Taille invalide");
 */
#define CHECK_PARAM(condition, err, msg) \
    CHECK((condition), (err), ERR_INVALID_PARAM, (msg))

/**
 * @brief Retourne une valeur si erreur déjà présente
 *
 * Usage: RETURN_IF_ERROR(err, NULL);
 */
#define RETURN_IF_ERROR(err, retval) \
    do { \
        if (error_occurred(err)) { \
            return (retval); \
        } \
    } while(0)

/**
 * @brief Propage une erreur d'un appel de fonction
 *
 * Usage:
 *   Widget* w = create_widget(&err);
 *   PROPAGATE_ERROR(err, NULL);
 */
#define PROPAGATE_ERROR(err, retval) \
    do { \
        if (error_occurred(err)) { \
            error_print(err); \
            goto cleanup; \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif // ERROR_H
