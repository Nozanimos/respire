// SPDX-License-Identifier: GPL-3.0-or-later
#include "json_editor.h"
#include "core/debug.h"
#include <stdlib.h>
#include <string.h>



//  SAUVEGARDE UN SNAPSHOT DANS L'HISTORIQUE UNDO
void sauvegarder_etat_undo(JsonEditor* editor) {
    if (!editor) return;

    // Créer un nouveau nœud
    UndoNode* nouveau = malloc(sizeof(UndoNode));
    if (!nouveau) {
        debug_printf("❌ UNDO: Erreur allocation mémoire\n");
        return;
    }

    // Copier le buffer actuel
    nouveau->buffer_snapshot = malloc(strlen(editor->buffer) + 1);
    if (!nouveau->buffer_snapshot) {
        free(nouveau);
        debug_printf("❌ UNDO: Erreur allocation buffer\n");
        return;
    }
    strcpy(nouveau->buffer_snapshot, editor->buffer);

    // Copier l'état
    nouveau->curseur_position = editor->curseur_position;
    nouveau->scroll_offset = editor->scroll_offset;
    nouveau->scroll_offset_x = editor->scroll_offset_x;

    // Insérer dans la liste chaînée
    nouveau->prev = editor->current_undo;
    nouveau->next = NULL;

    // Si on avait fait undo puis on modifie : on supprime tout le "futur"
    if (editor->current_undo && editor->current_undo->next) {
        UndoNode* temp = editor->current_undo->next;
        while (temp) {
            UndoNode* suivant = temp->next;
            free(temp->buffer_snapshot);
            free(temp);
            temp = suivant;
            editor->undo_count--;
        }
    }

    if (editor->current_undo) {
        editor->current_undo->next = nouveau;
    }

    editor->current_undo = nouveau;
    editor->undo_count++;

    // Limiter l'historique (supprimer le plus ancien si on dépasse)
    if (editor->undo_count > editor->max_undo_count) {
        // Trouver le premier nœud
        UndoNode* premier = editor->current_undo;
        while (premier->prev) {
            premier = premier->prev;
        }

        // Supprimer le premier
        if (premier->next) {
            premier->next->prev = NULL;
            free(premier->buffer_snapshot);
            free(premier);
            editor->undo_count--;
        }
    }

    debug_printf("💾 UNDO: État sauvegardé (total: %d)\n", editor->undo_count);
}

//  UNDO : Revenir à l'état précédent
void faire_undo(JsonEditor* editor) {
    if (!editor || !editor->current_undo || !editor->current_undo->prev) {
        debug_printf("⏪ UNDO: Pas d'état précédent\n");
        return;
    }

    // Revenir à l'état précédent
    editor->current_undo = editor->current_undo->prev;

    // Restaurer l'état
    strncpy(editor->buffer, editor->current_undo->buffer_snapshot, JSON_BUFFER_SIZE - 1);
    editor->buffer[JSON_BUFFER_SIZE - 1] = '\0';

    editor->curseur_position = editor->current_undo->curseur_position;
    editor->scroll_offset = editor->current_undo->scroll_offset;
    editor->scroll_offset_x = editor->current_undo->scroll_offset_x;
    editor->nb_lignes = compter_lignes(editor->buffer);
    editor->modified = true;

    debug_printf("⏪ UNDO: Retour à l'état précédent (curseur=%d)\n", editor->curseur_position);
}

//  REDO : Avancer vers l'état suivant
void faire_redo(JsonEditor* editor) {
    if (!editor || !editor->current_undo || !editor->current_undo->next) {
        debug_printf("⏩ REDO: Pas d'état suivant\n");
        return;
    }

    // Avancer vers l'état suivant
    editor->current_undo = editor->current_undo->next;

    // Restaurer l'état
    strncpy(editor->buffer, editor->current_undo->buffer_snapshot, JSON_BUFFER_SIZE - 1);
    editor->buffer[JSON_BUFFER_SIZE - 1] = '\0';

    editor->curseur_position = editor->current_undo->curseur_position;
    editor->scroll_offset = editor->current_undo->scroll_offset;
    editor->scroll_offset_x = editor->current_undo->scroll_offset_x;
    editor->nb_lignes = compter_lignes(editor->buffer);
    editor->modified = true;

    debug_printf("⏩ REDO: Avance à l'état suivant (curseur=%d)\n", editor->curseur_position);
}
