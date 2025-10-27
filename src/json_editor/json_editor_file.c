#include "json_editor.h"
#include "../debug.h"
#include <stdio.h>
#include <string.h>
#include <cjson/cJSON.h>

// ════════════════════════════════════════════════════════════════════════════
//  CHARGEMENT DU FICHIER
// ════════════════════════════════════════════════════════════════════════════
bool charger_fichier_json(JsonEditor* editor) {
    if (!editor) return false;

    FILE* file = fopen(editor->filepath, "r");
    if (!file) {
        debug_printf("❌ Impossible d'ouvrir: %s\n", editor->filepath);
        return false;
    }

    size_t bytes_read = fread(editor->buffer, 1, JSON_BUFFER_SIZE - 1, file);
    editor->buffer[bytes_read] = '\0';
    fclose(file);

    editor->curseur_position = 0;  // ← Devrait être 0
    editor->scroll_offset = 0;
    editor->scroll_offset_x = 0;  // ← Scroll horizontal à 0
    editor->modified = false;
    editor->nb_lignes = compter_lignes(editor->buffer);

    editor->json_valide = valider_json(editor);

    // LOG
    debug_printf("✅ Fichier chargé: %zu octets, %d lignes\n", bytes_read, editor->nb_lignes);
    debug_printf("🎯 Position initiale du curseur: %d\n", editor->curseur_position);
    debug_printf("🎯 Premier caractère du buffer: '%c'\n", editor->buffer[0]);
    debug_printf("🎯 Caractère à position 730: '%c'\n", editor->buffer[730]);

    // ETAT INITIAL POUR UNDO :
    sauvegarder_etat_undo(editor);
    debug_printf("💾 État initial sauvegardé pour le undo\n");

    return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  SAUVEGARDE DU FICHIER
// ════════════════════════════════════════════════════════════════════════════
bool sauvegarder_fichier_json(JsonEditor* editor) {
    if (!editor) return false;

    // Valider avant de sauvegarder
    if (!valider_json(editor)) {
        debug_printf("❌ JSON invalide, sauvegarde annulée\n");
        return false;
    }

    FILE* file = fopen(editor->filepath, "w");
    if (!file) {
        debug_printf("❌ Impossible d'écrire: %s\n", editor->filepath);
        return false;
    }

    fprintf(file, "%s", editor->buffer);
    fclose(file);

    editor->modified = false;
    debug_printf("✅ Fichier sauvegardé: %s\n", editor->filepath);
    return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  VALIDATION JSON
// ════════════════════════════════════════════════════════════════════════════
bool valider_json(JsonEditor* editor) {
    if (!editor) return false;

    cJSON* root = cJSON_Parse(editor->buffer);
    if (!root) {
        const char* error = cJSON_GetErrorPtr();
        if (error) {
            debug_printf("❌ JSON invalide: %s\n", error);
        }
        return false;
    }

    cJSON_Delete(root);
    return true;
}
