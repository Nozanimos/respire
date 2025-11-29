// SPDX-License-Identifier: GPL-3.0-or-later
#include "json_editor.h"
#include "../core/debug.h"
#include <string.h>



// ═════════════════════════════════════════════════════════════════════════
//  COPIER LA SÉLECTION DANS LE PRESSE-PAPIER
// ═════════════════════════════════════════════════════════════════════════
void copier_selection(JsonEditor* editor) {
    if (!editor || !editor->selection_active) {
        debug_printf("⚠️ COPIER: Aucune sélection active\n");
        return;
    }

    int sel_min = min_int(editor->selection_start, editor->selection_end);
    int sel_max = max_int(editor->selection_start, editor->selection_end);

    if (sel_min == sel_max) {
        debug_printf("⚠️ COPIER: Sélection vide\n");
        return;
    }

    int len = sel_max - sel_min;
    if (len >= JSON_BUFFER_SIZE) {
        debug_printf("⚠️ COPIER: Sélection trop grande\n");
        return;
    }

    // Copier dans le presse-papier interne
    strncpy(editor->clipboard, editor->buffer + sel_min, len);
    editor->clipboard[len] = '\0';

    // ✅ OPTIONNEL : Copier aussi dans le presse-papier système SDL
    SDL_SetClipboardText(editor->clipboard);

    debug_printf("✅ COPIER: %d caractères copiés\n", len);
}

// ═════════════════════════════════════════════════════════════════════════
//  COUPER LA SÉLECTION
// ═════════════════════════════════════════════════════════════════════════
void couper_selection(JsonEditor* editor) {
    if (!editor || !editor->selection_active) {
        debug_printf("⚠️ COUPER: Aucune sélection active\n");
        return;
    }

    // Sauvegarder l'état pour le undo
    sauvegarder_etat_undo(editor);

    // D'abord copier
    copier_selection(editor);

    // Puis supprimer
    int sel_min = min_int(editor->selection_start, editor->selection_end);
    int sel_max = max_int(editor->selection_start, editor->selection_end);

    if (sel_min != sel_max) {
        int len = strlen(editor->buffer);
        memmove(&editor->buffer[sel_min],
                &editor->buffer[sel_max],
                len - sel_max + 1);

        editor->curseur_position = sel_min;
        editor->modified = true;
        editor->nb_lignes = compter_lignes(editor->buffer);

        deselectionner(editor);
        debug_printf("✅ COUPER: Sélection coupée\n");
    }
}

// ═════════════════════════════════════════════════════════════════════════
//  COLLER DU TEXTE
// ═════════════════════════════════════════════════════════════════════════
void coller_texte(JsonEditor* editor) {
    if (!editor) return;

    // Sauvegarder l'état pour le undo
    sauvegarder_etat_undo(editor);

    // ✅ Essayer d'abord le presse-papier système
    char* texte_systeme = SDL_GetClipboardText();
    const char* texte_a_coller = NULL;

    if (texte_systeme && texte_systeme[0] != '\0') {
        texte_a_coller = texte_systeme;
    } else {
        // Sinon utiliser le presse-papier interne
        texte_a_coller = editor->clipboard;
    }

    if (!texte_a_coller || texte_a_coller[0] == '\0') {
        debug_printf("⚠️ COLLER: Presse-papier vide\n");
        if (texte_systeme) SDL_free(texte_systeme);
        return;
    }

    // Si une sélection est active, la supprimer d'abord
    if (editor->selection_active) {
        int sel_min = min_int(editor->selection_start, editor->selection_end);
        int sel_max = max_int(editor->selection_start, editor->selection_end);

        if (sel_min != sel_max) {
            int len = strlen(editor->buffer);
            memmove(&editor->buffer[sel_min],
                    &editor->buffer[sel_max],
                    len - sel_max + 1);
            editor->curseur_position = sel_min;
        }
        deselectionner(editor);
    }

    // Insérer chaque caractère du texte à coller
    for (int i = 0; texte_a_coller[i] != '\0'; i++) {
        int len = strlen(editor->buffer);
        if (len >= JSON_BUFFER_SIZE - 1) {
            debug_printf("⚠️ COLLER: Buffer plein\n");
            break;
        }

        // Décaler et insérer
        memmove(&editor->buffer[editor->curseur_position + 1],
                &editor->buffer[editor->curseur_position],
                len - editor->curseur_position + 1);

        editor->buffer[editor->curseur_position] = texte_a_coller[i];
        editor->curseur_position++;
    }

    editor->modified = true;
    editor->nb_lignes = compter_lignes(editor->buffer);

    debug_printf("✅ COLLER: Texte collé\n");

    if (texte_systeme) SDL_free(texte_systeme);
}
