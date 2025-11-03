// SPDX-License-Identifier: GPL-3.0-or-later
#include "json_editor.h"
#include "json_syntax.h"
#include <SDL2/SDL2_gfxPrimitives.h>


// Déclaration externe pour render_text
extern void render_text(SDL_Renderer* renderer, TTF_Font* font,
                        const char* text, int x, int y, Uint32 color);

// ════════════════════════════════════════════════════════════════════════════
//  RENDU DE L'ÉDITEUR
// ════════════════════════════════════════════════════════════════════════════
void rendre_json_editor(JsonEditor* editor) {
    if (!editor || !editor->renderer) return;

    // ─────────────────────────────────────────────────────────────────────────
    // FOND THÈME SOMBRE
    // ─────────────────────────────────────────────────────────────────────────
    SDL_Color bg_color = editor->json_valide ?
    (SDL_Color){30, 30, 35, 255} :      // Gris foncé si valide
    (SDL_Color){50, 25, 25, 255};       // Rouge foncé si invalide

    SDL_SetRenderDrawColor(editor->renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
    SDL_RenderClear(editor->renderer);

    // ─────────────────────────────────────────────────────────────────────────
    // TITRE
    // ─────────────────────────────────────────────────────────────────────────
    if (editor->font_ui) {
        char titre[300];
        snprintf(titre, sizeof(titre), "JSON Helper - %s%s",
                 editor->filepath,
                 editor->modified ? " *" : "");

        extern void render_text(SDL_Renderer* renderer, TTF_Font* font,
                                const char* text, int x, int y, Uint32 color);
        render_text(editor->renderer, editor->font_ui, titre, 10, 10, 0xFFCCCCCC);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // ZONE DE TEXTE (ligne par ligne)
    // ─────────────────────────────────────────────────────────────────────────
    int y = 40;
    char ligne[256];
    int nb_lignes_visibles = (EDITOR_HEIGHT - 100) / LINE_HEIGHT;


    // Calculer les positions de sélection si active
    int sel_min = -1, sel_max = -1;
    if (editor->selection_active) {
        sel_min = min_int(editor->selection_start, editor->selection_end);
        sel_max = max_int(editor->selection_start, editor->selection_end);
    }

    for (int i = editor->scroll_offset; i < editor->scroll_offset + nb_lignes_visibles && i < editor->nb_lignes; i++) {
        obtenir_ligne(editor->buffer, i, ligne, sizeof(ligne));

        if (!editor->font_mono) continue;

        // Numéro de ligne (gris moyen)
        char num_str[16];
        snprintf(num_str, sizeof(num_str), "%3d", i + 1);
        render_text(editor->renderer, editor->font_mono, num_str, 5, y, 0xFF888888);

        // DESSINER LA SURBRILLANCE DE SÉLECTION (si elle touche cette ligne)
        if (editor->selection_active && sel_min != sel_max) {
            // Calculer position de début de cette ligne dans le buffer
            int pos_debut_ligne = 0;
            int ligne_count = 0;
            while (editor->buffer[pos_debut_ligne] && ligne_count < i) {
                if (editor->buffer[pos_debut_ligne] == '\n') ligne_count++;
                pos_debut_ligne++;
            }

            int pos_fin_ligne = pos_debut_ligne;
            while (editor->buffer[pos_fin_ligne] && editor->buffer[pos_fin_ligne] != '\n') {
                pos_fin_ligne++;
            }

            // Cette ligne est-elle dans la sélection ?
            if (pos_fin_ligne >= sel_min && pos_debut_ligne <= sel_max) {
                // Calculer quelle partie de la ligne est sélectionnée
                int sel_debut_sur_ligne = (sel_min > pos_debut_ligne) ? sel_min - pos_debut_ligne : 0;
                int sel_fin_sur_ligne = (sel_max < pos_fin_ligne) ? sel_max - pos_debut_ligne : pos_fin_ligne - pos_debut_ligne;

                // Mesurer les largeurs
                char avant_sel[256], dans_sel[256];
                strncpy(avant_sel, ligne, sel_debut_sur_ligne);
                avant_sel[sel_debut_sur_ligne] = '\0';

                int largeur_avant = 0;
                if (sel_debut_sur_ligne > 0 && editor->font_mono) {
                    TTF_SizeUTF8(editor->font_mono, avant_sel, &largeur_avant, NULL);
                }

                int len_selection = sel_fin_sur_ligne - sel_debut_sur_ligne;
                strncpy(dans_sel, ligne + sel_debut_sur_ligne, len_selection);
                dans_sel[len_selection] = '\0';

                int largeur_selection = 0;
                if (len_selection > 0 && editor->font_mono) {
                    TTF_SizeUTF8(editor->font_mono, dans_sel, &largeur_selection, NULL);
                }

                // Dessiner le rectangle de surbrillance
                SDL_Rect highlight_rect = {
                    LEFT_MARGIN + largeur_avant - editor->scroll_offset_x,
                    y,
                    largeur_selection,
                    LINE_HEIGHT
                };
                SDL_SetRenderDrawColor(editor->renderer, 70, 130, 180, 255);  // Bleu clair
                SDL_RenderFillRect(editor->renderer, &highlight_rect);
            }
        }

        // ═════════════════════════════════════════════════════════════════════
        // RENDU DU TEXTE AVEC COLORATION SYNTAXIQUE
        // ═════════════════════════════════════════════════════════════════════
        if (ligne[0] != '\0') {
            // Parser la ligne pour obtenir les segments colorés
            LigneColoree* ligne_coloree = parser_ligne_json(ligne);

            if (ligne_coloree && ligne_coloree->nb_segments > 0) {
                // Rendre chaque segment avec sa couleur
                int x_offset = LEFT_MARGIN - editor->scroll_offset_x;

                for (int seg_idx = 0; seg_idx < ligne_coloree->nb_segments; seg_idx++) {
                    SegmentColore* seg = &ligne_coloree->segments[seg_idx];

                    // Extraire le texte du segment
                    char segment_text[256];
                    int len = (seg->longueur < 255) ? seg->longueur : 255;
                    strncpy(segment_text, ligne + seg->debut, len);
                    segment_text[len] = '\0';

                    // Mesurer la largeur du texte AVANT ce segment (pour le positionner)
                    int x_avant = 0;
                    if (seg->debut > 0) {
                        char texte_avant[256];
                        int len_avant = (seg->debut < 255) ? seg->debut : 255;
                        strncpy(texte_avant, ligne, len_avant);
                        texte_avant[len_avant] = '\0';
                        TTF_SizeUTF8(editor->font_mono, texte_avant, &x_avant, NULL);
                    }

                    // Obtenir la couleur du segment
                    SDL_Color couleur = obtenir_couleur_token(seg->type);
                    Uint32 color_hex = (couleur.a << 24) | (couleur.r << 16) |
                    (couleur.g << 8) | couleur.b;

                    // Rendre le segment
                    render_text(editor->renderer, editor->font_mono, segment_text,
                                x_offset + x_avant, y, color_hex);
                }

                liberer_ligne_coloree(ligne_coloree);
            } else {
                // Fallback : si le parsing échoue, rendre en blanc
                render_text(editor->renderer, editor->font_mono, ligne,
                            LEFT_MARGIN - editor->scroll_offset_x, y, 0xFFEEEEEE);
            }
        }

        y += LINE_HEIGHT;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // CURSEUR (ligne verticale clignotante)
    // ─────────────────────────────────────────────────────────────────────────
    int curseur_ligne = 0;
    int curseur_colonne = 0;
    for (int i = 0; i < editor->curseur_position && editor->buffer[i]; i++) {
        if (editor->buffer[i] == '\n') {
            curseur_ligne++;
            curseur_colonne = 0;
        } else {
            curseur_colonne++;
        }
    }

    // Clignotement (toutes les 500ms)
    static Uint32 last_blink = 0;
    static bool curseur_visible = true;
    Uint32 now = SDL_GetTicks();
    if (now - last_blink > 500) {
        curseur_visible = !curseur_visible;
        last_blink = now;
    }

    // Dessiner le curseur si visible et dans la zone visible
    if (curseur_visible &&
        curseur_ligne >= editor->scroll_offset &&
        curseur_ligne < editor->scroll_offset + nb_lignes_visibles) {

        // ✅ CALCUL PRÉCIS : mesurer la largeur réelle du texte jusqu'au curseur
        char ligne_actuelle[256];
    obtenir_ligne(editor->buffer, curseur_ligne, ligne_actuelle, sizeof(ligne_actuelle));

    // Extraire la partie de la ligne jusqu'au curseur
    char texte_avant_curseur[256];
    int len = (curseur_colonne < 255) ? curseur_colonne : 255;
    utf8_strncpy(texte_avant_curseur, ligne_actuelle, len, sizeof(texte_avant_curseur));    texte_avant_curseur[len] = '\0';

    // Mesurer la largeur réelle avec TTF_SizeUTF8
    int largeur_texte = 0;
    if (editor->font_mono && len > 0) {
        TTF_SizeUTF8(editor->font_mono, texte_avant_curseur, &largeur_texte, NULL);
    }

    int curseur_x = LEFT_MARGIN + largeur_texte - editor->scroll_offset_x;  // ✅ Avec scroll horizontal !
    int curseur_y = 40 + ((curseur_ligne - editor->scroll_offset) * LINE_HEIGHT);

    SDL_SetRenderDrawColor(editor->renderer, 255, 255, 255, 255);
    SDL_Rect curseur_rect = {curseur_x, curseur_y, 2, LINE_HEIGHT - 2};
    SDL_RenderFillRect(editor->renderer, &curseur_rect);
        }

        // ─────────────────────────────────────────────────────────────────────────
        // BOUTONS
        // ─────────────────────────────────────────────────────────────────────────
        rendre_boutons(editor);

        // ─────────────────────────────────────────────────────────────────────────
        // PRÉSENTATION
        // ─────────────────────────────────────────────────────────────────────────
        SDL_RenderPresent(editor->renderer);
}

// ════════════════════════════════════════════════════════════════════════════
//  RENDU DES BOUTONS
// ════════════════════════════════════════════════════════════════════════════
void rendre_boutons(JsonEditor* editor) {
    if (!editor) return;

    // Bouton RECHARGER
    Uint32 color_recharger = editor->bouton_recharger_survole ? 0xFF4080FF : 0xFF2060E0;
    boxColor(editor->renderer,
             editor->bouton_recharger.x, editor->bouton_recharger.y,
             editor->bouton_recharger.x + editor->bouton_recharger.w,
             editor->bouton_recharger.y + editor->bouton_recharger.h,
             color_recharger);

    // Bouton SAUVEGARDER
    Uint32 color_sauvegarder = editor->bouton_sauvegarder_survole ? 0xFF40C060 : 0xFF30A050;
    boxColor(editor->renderer,
             editor->bouton_sauvegarder.x, editor->bouton_sauvegarder.y,
             editor->bouton_sauvegarder.x + editor->bouton_sauvegarder.w,
             editor->bouton_sauvegarder.y + editor->bouton_sauvegarder.h,
             color_sauvegarder);

    // Textes des boutons
    if (editor->font_ui) {
        extern void render_text(SDL_Renderer* renderer, TTF_Font* font,
                                const char* text, int x, int y, Uint32 color);

        render_text(editor->renderer, editor->font_ui, "Recharger",
                    editor->bouton_recharger.x + 25, editor->bouton_recharger.y + 8, 0xFFFFFFFF);

        render_text(editor->renderer, editor->font_ui, "Sauvegarder",
                    editor->bouton_sauvegarder.x + 20, editor->bouton_sauvegarder.y + 8, 0xFFFFFFFF);
    }
}
