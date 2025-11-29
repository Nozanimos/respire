#include "json_editor.h"
#include "core/debug.h"
#include <cjson/cJSON.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

// Vérifie si un objet peut être formaté sur une seule ligne (max 4 propriétés, pas de sous-objets)
static bool peut_formater_en_ligne_objet(cJSON* obj) {
    if (!obj || !obj->child) return true; // Objet vide = toujours en ligne
    int count = 0;
    cJSON* enfant = obj->child;
    while (enfant) {
        count++;
        uint8_t type = enfant->type & 0xFF;
        // Interdit si contient un objet/tableau ou trop de propriétés
        if (type == cJSON_Object || type == cJSON_Array || count > 4) {
            return false;
        }
        enfant = enfant->next;
    }
    return true;
}

// Vérifie si un tableau peut être formaté sur une seule ligne (max 4 éléments, pas de sous-structures)
static bool peut_formater_en_ligne_tableau(cJSON* arr) {
    if (!arr || !arr->child) return true; // Tableau vide = toujours en ligne
    int count = 0;
    cJSON* enfant = arr->child;
    while (enfant) {
        count++;
        uint8_t type = enfant->type & 0xFF;
        // Interdit si contient un objet/tableau ou trop d'éléments
        if (type == cJSON_Object || type == cJSON_Array || count > 4) {
            return false;
        }
        enfant = enfant->next;
    }
    return true;
}

// UTILITAIRE : Échapper les chaînes JSON
static void echapper_chaine(char* dest, const char* src, size_t max_len) {
    char* p = dest;
    const char* end = dest + max_len - 1;
    while (*src && p < end) {
        switch (*src) {
            case '"':  *p++ = '\\'; *p++ = '"'; break;
            case '\\': *p++ = '\\'; *p++ = '\\'; break;
            case '\b': *p++ = '\\'; *p++ = 'b'; break;
            case '\f': *p++ = '\\'; *p++ = 'f'; break;
            case '\n': *p++ = '\\'; *p++ = 'n'; break;
            case '\r': *p++ = '\\'; *p++ = 'r'; break;
            case '\t': *p++ = '\\'; *p++ = 't'; break;
            default:   *p++ = *src; break;
        }
        src++;
    }
    *p = '\0';
}

// FORMATEUR RÉCURSIF PERSONNALISÉ
static void formater_element(cJSON* item, int indent_level, char** buffer, size_t* remaining) {
    if (!item || !*buffer || *remaining == 0) return;

    #define APPEND(str) do { \
        if (*remaining == 0) return; \
        size_t len = strlen(str); \
        if (len >= *remaining) len = *remaining - 1; \
        strncpy(*buffer, str, len); \
        *buffer += len; \
        *remaining -= len; \
    } while(0)

    #define INDENT() do { \
    for (int i = 0; i < indent_level; i++) { \
        if (*remaining < 4) return; \
            APPEND("    "); \
    } \
    } while(0)

    switch (item->type & 0xFF) {
        case cJSON_Object: {
            // ▶▶▶ NOUVEAU : Formatage en ligne pour les petits objets ▶▶▶
            if (peut_formater_en_ligne_objet(item)) {
                APPEND("{");
                cJSON* enfant = item->child;
                while (enfant) {
                    APPEND("\"");
                    APPEND(enfant->string);
                    APPEND("\": ");
                    formater_element(enfant, indent_level, buffer, remaining); // Même niveau d'indentation
                    if (enfant->next) APPEND(", ");
                    enfant = enfant->next;
                }
                APPEND("}");
            }
            // ▶▶▶ Formatage multi-lignes classique ▶▶▶
            else {
                APPEND("{");
                if (item->child) {
                    APPEND("\n");
                    cJSON* enfant = item->child;
                    while (enfant) {
                        INDENT();
                        APPEND("    \"");
                        APPEND(enfant->string);
                        APPEND("\": ");
                        formater_element(enfant, indent_level + 1, buffer, remaining);
                        if (enfant->next) APPEND(",");
                        APPEND("\n");
                        enfant = enfant->next;
                    }
                    INDENT();
                }
                APPEND("}");
            }
            break;
        }

        case cJSON_Array: {
            // ▶▶▶ NOUVEAU : Formatage en ligne pour les petits tableaux ▶▶▶
            if (peut_formater_en_ligne_tableau(item)) {
                APPEND("[");
                cJSON* enfant = item->child;
                while (enfant) {
                    formater_element(enfant, indent_level, buffer, remaining); // Même niveau
                    if (enfant->next) APPEND(", ");
                    enfant = enfant->next;
                }
                APPEND("]");
            }
            // ▶▶▶ Formatage multi-lignes classique ▶▶▶
            else {
                APPEND("[");
                if (item->child) {
                    APPEND("\n");
                    cJSON* enfant = item->child;
                    while (enfant) {
                        INDENT();
                        APPEND("    ");
                        formater_element(enfant, indent_level + 1, buffer, remaining);
                        if (enfant->next) APPEND(",");
                        APPEND("\n");
                        enfant = enfant->next;
                    }
                    INDENT();
                }
                APPEND("]");
            }
            break;
        }

        case cJSON_String: {
            char escaped[256];
            echapper_chaine(escaped, item->valuestring, sizeof(escaped));
            APPEND("\"");
            APPEND(escaped);
            APPEND("\"");
            break;
        }

        case cJSON_Number: {
            char num_str[32];
            if (floor(item->valuedouble) == item->valuedouble) {
                snprintf(num_str, sizeof(num_str), "%d", (int)item->valuedouble);
            } else {
                snprintf(num_str, sizeof(num_str), "%g", item->valuedouble);
            }
            APPEND(num_str);
            break;
        }

        case cJSON_True:  APPEND("true"); break;
        case cJSON_False: APPEND("false"); break;
        case cJSON_NULL:  APPEND("null"); break;

        default: break;
    }
    #undef APPEND
    #undef INDENT
}

// FONCTION PUBLIQUE : Réindenter selon le style
void reindenter_json(JsonEditor* editor) {
    debug_printf("🔍 reindenter_json() DÉBUT\n");

    if (!editor) {
        debug_printf("❌ editor est NULL\n");
        return;
    }

    if (!editor->buffer[0]) {
        debug_printf("❌ buffer est vide\n");
        return;
    }

    debug_printf("✅ editor et buffer OK, taille buffer: %zu\n", strlen(editor->buffer));

    // 1. Valider le JSON
    debug_printf("🔍 Validation JSON...\n");
    if (!valider_json(editor)) {
        debug_printf("❌ Validation JSON échouée\n");
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "JSON invalide",
                                 "Corrigez les erreurs avant de réindenter.", editor->window);
        return;
    }
    debug_printf("✅ JSON valide\n");

    // 2. Parser avec cJSON
    debug_printf("🔍 Parsing JSON avec cJSON...\n");
    cJSON* root = cJSON_Parse(editor->buffer);
    if (!root) {
        debug_printf("❌ cJSON_Parse a échoué\n");
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Erreur de parsing",
                                 "Impossible de parser le JSON valide (bug interne).", editor->window);
        return;
    }
    debug_printf("✅ JSON parsé avec succès\n");

    // 3. Formater avec NOTRE style personnalisé
    // Utiliser un buffer 2x plus grand pour l'indentation
    debug_printf("🔍 Formatage du JSON (buffer: %d octets)...\n", JSON_BUFFER_SIZE * 2);
    char formatted[JSON_BUFFER_SIZE * 2];
    char* ptr = formatted;
    size_t remaining = (JSON_BUFFER_SIZE * 2) - 1;

    formater_element(root, 0, &ptr, &remaining);
    debug_printf("✅ Formatage terminé, octets restants: %zu\n", remaining);

    // CRITIQUE : Ne pas écrire '\0' en dehors du buffer !
    if (remaining > 0) {
        *ptr = '\0';
        debug_printf("✅ Terminateur NULL placé normalement\n");
    } else {
        formatted[(JSON_BUFFER_SIZE * 2) - 1] = '\0';
        debug_printf("⚠️  ATTENTION : Buffer de formatage saturé (%zu octets restants)\n", remaining);
    }

    cJSON_Delete(root);

    // 4. Vérifier la taille
    size_t len = strlen(formatted);
    debug_printf("🔍 Longueur JSON formaté: %zu octets (limite: %d)\n", len, JSON_BUFFER_SIZE);
    if (len >= JSON_BUFFER_SIZE) {
        debug_printf("❌ JSON trop grand pour l'éditeur\n");
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Trop grand",
                                 "Le JSON formaté dépasse la limite de 8 Ko.", editor->window);
        return;
    }

    // 5. Appliquer les changements
    debug_printf("🔍 Application des changements dans le buffer de l'éditeur...\n");
    sauvegarder_etat_undo(editor);
    strncpy(editor->buffer, formatted, JSON_BUFFER_SIZE - 1);
    editor->buffer[JSON_BUFFER_SIZE - 1] = '\0';
    editor->curseur_position = 0;
    editor->scroll_offset = 0;
    editor->scroll_offset_x = 0;
    editor->nb_lignes = compter_lignes(editor->buffer);
    editor->selection_active = false;
    marquer_modification(editor);

    debug_printf("✨ JSON réindenté selon le style personnalisé\n");
}
