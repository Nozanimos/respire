// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef __JSON_SYNTAX_H__
#define __JSON_SYNTAX_H__

#include <SDL2/SDL.h>
#include <stdbool.h>

// ════════════════════════════════════════════════════════════════════════════
//  TYPES DE TOKENS JSON
// ════════════════════════════════════════════════════════════════════════════
typedef enum {
    TOKEN_NORMAL,          // Texte normal (blanc)
    TOKEN_CLE,            // Clé JSON (bleu clair)
    TOKEN_STRING,         // Chaîne de caractères (vert)
    TOKEN_NOMBRE,         // Nombre (orange)
    TOKEN_MOT_CLE,        // true, false, null (violet)
    TOKEN_DELIMITEUR,     // { } [ ] : , (gris clair)
    TOKEN_ERREUR          // Syntaxe invalide (rouge)
} TypeToken;

// ════════════════════════════════════════════════════════════════════════════
//  SEGMENT COLORÉ
// ════════════════════════════════════════════════════════════════════════════
typedef struct {
    int debut;              // Position de début (en octets)
    int longueur;           // Longueur (en octets)
    TypeToken type;         // Type de token
} SegmentColore;

// ════════════════════════════════════════════════════════════════════════════
//  RÉSULTAT DU PARSING D'UNE LIGNE
// ════════════════════════════════════════════════════════════════════════════
#define MAX_SEGMENTS 64     // Maximum de segments par ligne

typedef struct {
    SegmentColore segments[MAX_SEGMENTS];
    int nb_segments;
} LigneColoree;

// ════════════════════════════════════════════════════════════════════════════
//  PROTOTYPES
// ════════════════════════════════════════════════════════════════════════════

// Parse une ligne de JSON et retourne les segments colorés
LigneColoree* parser_ligne_json(const char* ligne);

// Retourne la couleur SDL associée à un type de token
SDL_Color obtenir_couleur_token(TypeToken type);

// Libère la mémoire d'une ligne colorée
void liberer_ligne_coloree(LigneColoree* ligne);

#endif
