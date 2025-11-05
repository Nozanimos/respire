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
    // Calculer combien de lignes on peut afficher selon la hauteur actuelle
    int nb_lignes_visibles = obtenir_nb_lignes_visibles(editor);


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
        rendre_tous_boutons(editor);

        // ─────────────────────────────────────────────────────────────────────────
        // MENU CONTEXTUEL
        // ─────────────────────────────────────────────────────────────────────────
        if (editor->context_menu.visible) {
            dessiner_menu_contextuel(editor);
        }

        // ─────────────────────────────────────────────────────────────────────────
        // PRÉSENTATION
        // ─────────────────────────────────────────────────────────────────────────
        SDL_RenderPresent(editor->renderer);
}

// ════════════════════════════════════════════════════════════════════════════
//  RENDU DE TOUS LES BOUTONS
// ════════════════════════════════════════════════════════════════════════════
void rendre_tous_boutons(JsonEditor* editor) {
    if (!editor || !editor->font_ui) return;

    extern void render_text(SDL_Renderer* renderer, TTF_Font* font,
                            const char* text, int x, int y, Uint32 color);

    // Dessiner chaque bouton
    for (int i = 0; i < editor->nb_boutons; i++) {
        EditorButton* btn = &editor->boutons[i];

        // Choisir la couleur selon le survol
        Uint32 couleur = btn->survole ? btn->couleur_survol : btn->couleur_normale;

        // Dessiner le rectangle du bouton
        boxColor(editor->renderer,
                 btn->rect.x, btn->rect.y,
                 btn->rect.x + btn->rect.w,
                 btn->rect.y + btn->rect.h,
                 couleur);

        // Centrer le texte
        int texte_w, texte_h;
        TTF_SizeUTF8(editor->font_ui, btn->label, &texte_w, &texte_h);
        int centre_x = btn->rect.x + (btn->rect.w - texte_w) / 2;
        int centre_y = btn->rect.y + (btn->rect.h - texte_h) / 2;

        render_text(editor->renderer, editor->font_ui, btn->label,
                    centre_x, centre_y, 0xFFFFFFFF);
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  RENDU DU MENU CONTEXTUEL
// ════════════════════════════════════════════════════════════════════════════
void dessiner_menu_contextuel(JsonEditor* editor) {
    ContextMenu* menu = &editor->context_menu;

    if (!menu->visible) return;

    // SAUVEGARDER le mode de blend actuel
    SDL_BlendMode oldBlendMode;
    SDL_GetRenderDrawBlendMode(editor->renderer, &oldBlendMode);

    // ACTIVER le blending pour la transparence
    SDL_SetRenderDrawBlendMode(editor->renderer, SDL_BLENDMODE_BLEND);

    // Fond du menu (opaque)
    SDL_SetRenderDrawColor(editor->renderer,
                           menu->bg_color.r, menu->bg_color.g, menu->bg_color.b, menu->bg_color.a);
    SDL_Rect background = {menu->x, menu->y, menu->width, menu->height};
    SDL_RenderFillRect(editor->renderer, &background);

    // Bordure
    SDL_SetRenderDrawColor(editor->renderer,
                           menu->border_color.r, menu->border_color.g, menu->border_color.b, menu->border_color.a);
    SDL_RenderDrawRect(editor->renderer, &background);

    // Items avec surbrillance au survol (semi-transparente)
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    for (int i = 0; i < menu->item_count; i++) {
        ContextMenuItem* item = &menu->items[i];

        // Position absolue de l'item
        SDL_Rect absolute_rect = {
            menu->x + item->rect.x,
            menu->y + item->rect.y,
            item->rect.w,
            item->rect.h
        };

        if (item->label == NULL) {
            // ═════════════════════════════════════════════════════════════
            // DESSIN DU SÉPARATEUR
            // ═════════════════════════════════════════════════════════════
            SDL_SetRenderDrawColor(editor->renderer,
                                   menu->border_color.r, menu->border_color.g, menu->border_color.b, 100);
            int line_y = absolute_rect.y + (absolute_rect.h / 2);
            SDL_RenderDrawLine(editor->renderer,
                               absolute_rect.x + 5, line_y,
                               absolute_rect.x + absolute_rect.w - 5, line_y);
        } else {
            // ═════════════════════════════════════════════════════════════
            // DESSIN DE L'ITEM NORMAL
            // ═════════════════════════════════════════════════════════════

            // Surbrillance si la souris est sur l'item
            if (mouse_x >= absolute_rect.x && mouse_x <= absolute_rect.x + absolute_rect.w &&
                mouse_y >= absolute_rect.y && mouse_y <= absolute_rect.y + absolute_rect.h &&
                item->enabled) {
                SDL_SetRenderDrawColor(editor->renderer,
                                       menu->hover_color.r, menu->hover_color.g, menu->hover_color.b, menu->hover_color.a);
                SDL_RenderFillRect(editor->renderer, &absolute_rect);
                }

                // Texte
                SDL_Color text_color = item->enabled ? menu->text_enabled_color : menu->text_disabled_color;

            if (editor->font_ui) {
                SDL_Surface* surface = TTF_RenderUTF8_Blended(editor->font_ui, item->label, text_color);
                if (surface) {
                    SDL_Texture* texture = SDL_CreateTextureFromSurface(editor->renderer, surface);

                    // Centrer le texte verticalement et l'aligner à gauche avec marge
                    SDL_Rect text_rect = {
                        absolute_rect.x + 10,
                        absolute_rect.y + (absolute_rect.h - surface->h) / 2,
                        surface->w,
                        surface->h
                    };

                    SDL_RenderCopy(editor->renderer, texture, NULL, &text_rect);
                    SDL_FreeSurface(surface);
                    SDL_DestroyTexture(texture);

                    // ─────────────────────────────────────────────────────────
                    // DESSINER LA FLÈCHE POUR LES ITEMS AVEC SOUS-MENU
                    // ─────────────────────────────────────────────────────────
                    if (item->sous_menu) {
                        // Positionner la flèche à droite avec marge de 3px
                        int arrow_size = 6;  // Taille du triangle
                        int arrow_x = absolute_rect.x + absolute_rect.w - arrow_size - 3;
                        int arrow_y = absolute_rect.y + (absolute_rect.h / 2);

                        // Convertir SDL_Color en Uint32 RGBA pour filledPolygonColor
                        Uint32 arrow_color = (text_color.r << 24) |
                        (text_color.g << 16) |
                        (text_color.b << 8) |
                        text_color.a;

                        // Dessiner un triangle pointant vers la droite
                        Sint16 triangle_x[3] = {
                            arrow_x - arrow_size,     // Point gauche haut
                            arrow_x - arrow_size,     // Point gauche bas
                            arrow_x                   // Pointe droite
                        };
                        Sint16 triangle_y[3] = {
                            arrow_y - arrow_size/2,   // Point gauche haut
                            arrow_y + arrow_size/2,   // Point gauche bas
                            arrow_y                   // Pointe droite (milieu)
                        };

                        filledPolygonColor(editor->renderer,
                                           triangle_x, triangle_y, 3,
                                           arrow_color);
                    }
                }
            }
        }
    }

    // ═════════════════════════════════════════════════════════════════════════
    // DESSIN DES SOUS-MENUS
    // ═════════════════════════════════════════════════════════════════════════
    for (int i = 0; i < menu->item_count; i++) {
        ContextMenu* sous_menu = menu->items[i].sous_menu;

        if (!sous_menu || !sous_menu->visible) continue;

        // Fond du sous-menu
        SDL_SetRenderDrawColor(editor->renderer,
                               sous_menu->bg_color.r, sous_menu->bg_color.g,
                               sous_menu->bg_color.b, sous_menu->bg_color.a);
        SDL_Rect sous_menu_bg = {sous_menu->x, sous_menu->y, sous_menu->width, sous_menu->height};
        SDL_RenderFillRect(editor->renderer, &sous_menu_bg);

        // Bordure du sous-menu
        SDL_SetRenderDrawColor(editor->renderer,
                               sous_menu->border_color.r, sous_menu->border_color.g,
                               sous_menu->border_color.b, sous_menu->border_color.a);
        SDL_RenderDrawRect(editor->renderer, &sous_menu_bg);

        // Items du sous-menu
        for (int j = 0; j < sous_menu->item_count; j++) {
            ContextMenuItem* sub_item = &sous_menu->items[j];

            SDL_Rect absolute_rect = {
                sous_menu->x + sub_item->rect.x,
                sous_menu->y + sub_item->rect.y,
                sub_item->rect.w,
                sub_item->rect.h
            };

            // Surbrillance si survolé
            if (j == sous_menu->hovered_item) {
                SDL_SetRenderDrawColor(editor->renderer,
                                       sous_menu->hover_color.r, sous_menu->hover_color.g,
                                       sous_menu->hover_color.b, sous_menu->hover_color.a);
                SDL_RenderFillRect(editor->renderer, &absolute_rect);
            }

            // Texte
            if (editor->font_ui && sub_item->label) {
                SDL_Color text_color = sub_item->enabled ?
                sous_menu->text_enabled_color : sous_menu->text_disabled_color;

                SDL_Surface* surface = TTF_RenderUTF8_Blended(editor->font_ui, sub_item->label, text_color);
                if (surface) {
                    SDL_Texture* texture = SDL_CreateTextureFromSurface(editor->renderer, surface);

                    SDL_Rect text_rect = {
                        absolute_rect.x + 10,
                        absolute_rect.y + (absolute_rect.h - surface->h) / 2,
                        surface->w,
                        surface->h
                    };

                    SDL_RenderCopy(editor->renderer, texture, NULL, &text_rect);
                    SDL_FreeSurface(surface);
                    SDL_DestroyTexture(texture);
                }
            }
        }
    }

    // RESTAURER l'ancien mode de blend
    SDL_SetRenderDrawBlendMode(editor->renderer, oldBlendMode);
}
