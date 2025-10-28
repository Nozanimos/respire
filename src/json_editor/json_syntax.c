#include "json_syntax.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ════════════════════════════════════════════════════════════════════════════
//  COULEURS POUR CHAQUE TYPE DE TOKEN
// ════════════════════════════════════════════════════════════════════════════
SDL_Color obtenir_couleur_token(TypeToken type) {
    switch (type) {
        case TOKEN_CLE:
            return (SDL_Color){100, 180, 255, 255};  // Bleu clair
        case TOKEN_STRING:
            return (SDL_Color){100, 200, 100, 255};  // Vert
        case TOKEN_NOMBRE:
            return (SDL_Color){230, 126, 34, 255};  // Orange
        case TOKEN_MOT_CLE:
            return (SDL_Color){200, 130, 255, 255};  // Violet
        case TOKEN_DELIMITEUR:
            return (SDL_Color){150, 150, 150, 255};  // Gris clair
        case TOKEN_ERREUR:
            return (SDL_Color){255, 100, 100, 255};  // Rouge
        case TOKEN_NORMAL:
        default:
            return (SDL_Color){220, 220, 220, 255};  // Blanc cassé
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  HELPERS
// ════════════════════════════════════════════════════════════════════════════

// Vérifie si un caractère est un délimiteur JSON
static bool est_delimiteur(char c) {
    return (c == '{' || c == '}' || c == '[' || c == ']' ||
    c == ':' || c == ',');
}

// Vérifie si une chaîne commence par un mot-clé JSON
static bool est_mot_cle(const char* str, int* longueur) {
    if (strncmp(str, "true", 4) == 0) {
        *longueur = 4;
        return true;
    }
    if (strncmp(str, "false", 5) == 0) {
        *longueur = 5;
        return true;
    }
    if (strncmp(str, "null", 4) == 0) {
        *longueur = 4;
        return true;
    }
    return false;
}

// ════════════════════════════════════════════════════════════════════════════
//  PARSING DE LIGNE JSON
// ════════════════════════════════════════════════════════════════════════════
LigneColoree* parser_ligne_json(const char* ligne) {
    if (!ligne) return NULL;

    LigneColoree* resultat = malloc(sizeof(LigneColoree));
    if (!resultat) return NULL;

    resultat->nb_segments = 0;

    int len = strlen(ligne);
    int i = 0;

    while (i < len && resultat->nb_segments < MAX_SEGMENTS) {
        char c = ligne[i];

        // ═════════════════════════════════════════════════════════════════════
        // ESPACES ET TABULATIONS → on ignore
        // ═════════════════════════════════════════════════════════════════════
        if (isspace(c)) {
            i++;
            continue;
        }

        // ═════════════════════════════════════════════════════════════════════
        // DÉLIMITEURS : { } [ ] : ,
        // ═════════════════════════════════════════════════════════════════════
        if (est_delimiteur(c)) {
            SegmentColore seg = {
                .debut = i,
                .longueur = 1,
                .type = TOKEN_DELIMITEUR
            };
            resultat->segments[resultat->nb_segments++] = seg;
            i++;
            continue;
        }

        // ═════════════════════════════════════════════════════════════════════
        // STRINGS : "..."
        // ═════════════════════════════════════════════════════════════════════
        if (c == '"') {
            int debut = i;
            i++; // Passer le " initial

            // Parcourir jusqu'au " de fermeture
            bool string_fermee = false;
            while (i < len) {
                if (ligne[i] == '\\') {
                    // Échappement : on saute le prochain caractère
                    i += 2;
                    continue;
                }
                if (ligne[i] == '"') {
                    i++; // Inclure le " final
                    string_fermee = true;
                    break;
                }
                i++;
            }

            // Si la string n'est pas fermée, c'est une erreur
            TypeToken type_token;
            if (!string_fermee) {
                type_token = TOKEN_ERREUR;
            } else {
                // Déterminer si c'est une CLÉ ou une STRING
                bool est_cle = false;
                int j = i;
                while (j < len && isspace(ligne[j])) j++;
                if (j < len && ligne[j] == ':') {
                    est_cle = true;
                }
                type_token = est_cle ? TOKEN_CLE : TOKEN_STRING;
            }

            SegmentColore seg = {
                .debut = debut,
                .longueur = i - debut,
                .type = type_token
            };

            resultat->segments[resultat->nb_segments++] = seg;
            continue;
        }

        // ═════════════════════════════════════════════════════════════════════
        // NOMBRES : 42, -3.14, 1e10
        // ═════════════════════════════════════════════════════════════════════
        if (isdigit(c) || c == '-') {
            int debut = i;

            // Signe optionnel
            if (c == '-') i++;

            // Chiffres
            while (i < len && isdigit(ligne[i])) i++;

            // Partie décimale optionnelle
            if (i < len && ligne[i] == '.') {
                i++;
                while (i < len && isdigit(ligne[i])) i++;
            }

            // Exposant optionnel (e ou E)
            if (i < len && (ligne[i] == 'e' || ligne[i] == 'E')) {
                i++;
                if (i < len && (ligne[i] == '+' || ligne[i] == '-')) i++;
                while (i < len && isdigit(ligne[i])) i++;
            }

            SegmentColore seg = {
                .debut = debut,
                .longueur = i - debut,
                .type = TOKEN_NOMBRE
            };
            resultat->segments[resultat->nb_segments++] = seg;
            continue;
        }

        // ═════════════════════════════════════════════════════════════════════
        // MOTS-CLÉS : true, false, null
        // ═════════════════════════════════════════════════════════════════════
        int longueur_mot_cle = 0;
        if (est_mot_cle(ligne + i, &longueur_mot_cle)) {
            SegmentColore seg = {
                .debut = i,
                .longueur = longueur_mot_cle,
                .type = TOKEN_MOT_CLE
            };
            resultat->segments[resultat->nb_segments++] = seg;
            i += longueur_mot_cle;
            continue;
        }

        // ═════════════════════════════════════════════════════════════════════
        // CARACTÈRE INCONNU → ERREUR
        // ═════════════════════════════════════════════════════════════════════
        SegmentColore seg = {
            .debut = i,
            .longueur = 1,
            .type = TOKEN_ERREUR
        };
        resultat->segments[resultat->nb_segments++] = seg;
        i++;
    }

    return resultat;
}

// ════════════════════════════════════════════════════════════════════════════
//  LIBÉRATION MÉMOIRE
// ════════════════════════════════════════════════════════════════════════════
void liberer_ligne_coloree(LigneColoree* ligne) {
    if (ligne) {
        free(ligne);
    }
}
