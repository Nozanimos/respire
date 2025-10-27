#include "json_editor.h"
#include "../debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


// ════════════════════════════════════════════════════════════════════════════
//  CRÉATION DE L'ÉDITEUR
// ════════════════════════════════════════════════════════════════════════════
JsonEditor* creer_json_editor(const char* filepath, int pos_x, int pos_y) {
    JsonEditor* editor = malloc(sizeof(JsonEditor));
    if (!editor) {
        debug_printf("❌ Erreur allocation JsonEditor\n");
        return NULL;
    }

    // Initialisation
    memset(editor, 0, sizeof(JsonEditor));
    snprintf(editor->filepath, sizeof(editor->filepath), "%s", filepath);
    editor->est_ouvert = true;
    editor->json_valide = true;

    // ─────────────────────────────────────────────────────────────────────────
    // CRÉATION DE LA FENÊTRE
    // ─────────────────────────────────────────────────────────────────────────
    editor->window = SDL_CreateWindow(
        "JSON Helper - Éditeur de configuration",
        pos_x, pos_y,
        EDITOR_WIDTH, EDITOR_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!editor->window) {
        debug_printf("❌ Erreur création fenêtre éditeur: %s\n", SDL_GetError());
        free(editor);
        return NULL;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // CRÉATION DU RENDERER
    // ─────────────────────────────────────────────────────────────────────────
    editor->renderer = SDL_CreateRenderer(
        editor->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!editor->renderer) {
        debug_printf("❌ Erreur création renderer éditeur: %s\n", SDL_GetError());
        SDL_DestroyWindow(editor->window);
        free(editor);
        return NULL;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // CHARGEMENT DES POLICES
    // ─────────────────────────────────────────────────────────────────────────
    editor->font_mono = TTF_OpenFont("../fonts/arial/ARIAL.TTF", 14);
    editor->font_ui = TTF_OpenFont("../fonts/arial/ARIAL.TTF", 16);

    if (!editor->font_mono || !editor->font_ui) {
        debug_printf("⚠️ Police non trouvée, essai fallback\n");
        editor->font_mono = TTF_OpenFont("/usr/share/fonts/gnu-free/FreeMono.otf", 14);
        editor->font_ui = TTF_OpenFont("/usr/share/fonts/gnu-free/FreeSans.otf", 16);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // POSITION DES BOUTONS (en bas de la fenêtre)
    // ─────────────────────────────────────────────────────────────────────────
    editor->bouton_recharger = (SDL_Rect){20, EDITOR_HEIGHT - 50, 150, 35};
    editor->bouton_sauvegarder = (SDL_Rect){190, EDITOR_HEIGHT - 50, 150, 35};

    // ─────────────────────────────────────────────────────────────────────────
    // INITIALISATION DU SYSTÈME UNDO
    // ─────────────────────────────────────────────────────────────────────────
    editor->current_undo = NULL;
    editor->undo_count = 0;
    editor->max_undo_count = 100;  // Maximum 100 états dans l'historique

    // ─────────────────────────────────────────────────────────────────────────
    // CHARGEMENT DU FICHIER
    // ─────────────────────────────────────────────────────────────────────────
    if (!charger_fichier_json(editor)) {
        debug_printf("⚠️ Fichier JSON non trouvé, buffer vide\n");
    }

    // Initialisation de la sélection
    editor->selection_start = -1;
    editor->selection_end = -1;
    editor->selection_active = false;

    // Activer la saisie texte pour cette fenêtre
    SDL_StartTextInput();

    debug_printf("✅ Éditeur JSON créé\n");

    return editor;
}

// ════════════════════════════════════════════════════════════════════════════
//  DESTRUCTION
// ════════════════════════════════════════════════════════════════════════════
void detruire_json_editor(JsonEditor* editor) {
    if (!editor) return;

    if (editor->font_mono) TTF_CloseFont(editor->font_mono);
    if (editor->font_ui) TTF_CloseFont(editor->font_ui);
    if (editor->renderer) SDL_DestroyRenderer(editor->renderer);
    if (editor->window) SDL_DestroyWindow(editor->window);

    free(editor);
    debug_printf("🗑️ Éditeur JSON détruit\n");
}
