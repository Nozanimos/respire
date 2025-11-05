// SPDX-License-Identifier: GPL-3.0-or-later
#include "json_editor.h"
#include "../debug.h"
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>


// ════════════════════════════════════════════════════════════════════════════
//  DÉCLARATIONS DES FONCTIONS INTERNES DES AUTRES MODULES
// ════════════════════════════════════════════════════════════════════════════
// Depuis json_editor_edit.c
extern void inserer_caractere(JsonEditor* editor, char c);
extern void supprimer_caractere(JsonEditor* editor);
extern void supprimer_caractere_apres(JsonEditor* editor);
extern void inserer_nouvelle_ligne(JsonEditor* editor);
extern void dupliquer_ligne_courante(JsonEditor* editor);

// Depuis json_editor_clipboard.c (fonctions static → doivent être rendues non-static)
void copier_selection(JsonEditor* editor);
void couper_selection(JsonEditor* editor);
void coller_texte(JsonEditor* editor);

// Depuis json_editor_undo.c (fonctions static → doivent être rendues non-static)
void faire_undo(JsonEditor* editor);
void faire_redo(JsonEditor* editor);

// Depuis json_editor_navigation.c (fonctions static → doivent être rendues non-static)
void deplacer_curseur_vertical(JsonEditor* editor, int direction);
void auto_scroll_curseur(JsonEditor* editor);

// Depuis json_editor_file.c
extern bool charger_fichier_json(JsonEditor* editor);
extern bool sauvegarder_fichier_json(JsonEditor* editor);

// ════════════════════════════════════════════════════════════════════════════
//  GESTION DES ÉVÉNEMENTS
// ════════════════════════════════════════════════════════════════════════════

bool gerer_evenements_json_editor(JsonEditor* editor, SDL_Event* event) {
    if (!editor || !event) return false;

    Uint32 window_id = SDL_GetWindowID(editor->window);

    switch (event->type) {
        case SDL_WINDOWEVENT:
            if (event->window.windowID == window_id) {
                // ═════════════════════════════════════════════════════════════════
                // FERMETURE DE LA FENÊTRE
                // ═════════════════════════════════════════════════════════════════
                if (event->window.event == SDL_WINDOWEVENT_CLOSE) {
                    editor->est_ouvert = false;
                }

                // ═════════════════════════════════════════════════════════════════
                // REDIMENSIONNEMENT DE LA FENÊTRE
                // ═════════════════════════════════════════════════════════════════
                // Quand l'utilisateur étire ou réduit la fenêtre, SDL envoie cet événement
                // On en profite pour recalculer les positions des éléments
                else if (event->window.event == SDL_WINDOWEVENT_RESIZED) {
                    recalculer_tous_boutons(editor);
                    debug_printf("🔄 Fenêtre redimensionnée : %dx%d\n",
                                 event->window.data1, event->window.data2);
                }

                return true;
            }
            break;

        case SDL_KEYDOWN:
            if (event->key.windowID == window_id) {
                // ═════════════════════════════════════════════════════════════════
                // GESTION DES RACCOURCIS CLAVIER (Ctrl+...)
                // ═════════════════════════════════════════════════════════════════
                SDL_Keymod mod = SDL_GetModState();

                if (editor->context_menu.visible && event->key.keysym.sym == SDLK_ESCAPE) {
                    cacher_menu_contextuel(editor);
                    return true;
                }

                // ─────────────────────────────────────────────────────────────────
                // Ctrl+S → SAUVEGARDER
                // ─────────────────────────────────────────────────────────────────
                if ((mod & KMOD_CTRL) && event->key.keysym.sym == SDLK_s) {
                    if (sauvegarder_fichier_json(editor)) {
                        debug_printf("💾 [Ctrl+S] JSON sauvegardé avec succès\n");
                    } else {
                        debug_printf("❌ [Ctrl+S] Erreur lors de la sauvegarde\n");
                    }
                    return true;
                }

                // ─────────────────────────────────────────────────────────────────
                // Ctrl+R → RECHARGER
                // ─────────────────────────────────────────────────────────────────
                if ((mod & KMOD_CTRL) && event->key.keysym.sym == SDLK_r) {
                    if (charger_fichier_json(editor)) {
                        debug_printf("🔄 [Ctrl+R] JSON rechargé avec succès\n");
                    } else {
                        debug_printf("❌ [Ctrl+R] Erreur lors du rechargement\n");
                    }
                    return true;
                }

                // ─────────────────────────────────────────────────────────────────
                // Ctrl+Z → UNDO
                // ─────────────────────────────────────────────────────────────────
                if ((mod & KMOD_CTRL) && !(mod & KMOD_SHIFT) && event->key.keysym.sym == SDLK_z) {
                    faire_undo(editor);
                    return true;
                }

                // ─────────────────────────────────────────────────────────────────
                // Ctrl+Shift+Z → REDO
                // ─────────────────────────────────────────────────────────────────
                if ((mod & KMOD_CTRL) && (mod & KMOD_SHIFT) && event->key.keysym.sym == SDLK_z) {
                    faire_redo(editor);
                    return true;
                }

                // ─────────────────────────────────────────────────────────────────
                // Ctrl+Y → REDO alternatif
                // ─────────────────────────────────────────────────────────────────
                if ((mod & KMOD_CTRL) && event->key.keysym.sym == SDLK_y) {
                    faire_redo(editor);
                    return true;
                }

                // ─────────────────────────────────────────────────────────────────
                // Ctrl+D → DUPLIQUER LIGNE
                // ─────────────────────────────────────────────────────────────────
                if ((mod & KMOD_CTRL) && event->key.keysym.sym == SDLK_d) {
                    dupliquer_ligne_courante(editor);
                    auto_scroll_curseur(editor);
                    return true;
                }

                // ─────────────────────────────────────────────────────────────────
                // Ctrl+C → COPIER
                // ─────────────────────────────────────────────────────────────────
                if ((mod & KMOD_CTRL) && event->key.keysym.sym == SDLK_c) {
                    copier_selection(editor);
                    return true;
                }

                // ─────────────────────────────────────────────────────────────────
                // Ctrl+X → COUPER
                // ─────────────────────────────────────────────────────────────────
                if ((mod & KMOD_CTRL) && event->key.keysym.sym == SDLK_x) {
                    couper_selection(editor);
                    auto_scroll_curseur(editor);
                    return true;
                }

                // ─────────────────────────────────────────────────────────────────
                // Ctrl+V → COLLER
                // ─────────────────────────────────────────────────────────────────
                if ((mod & KMOD_CTRL) && event->key.keysym.sym == SDLK_v) {
                    coller_texte(editor);
                    auto_scroll_curseur(editor);
                    return true;
                }

                // ─────────────────────────────────────────────────────────────────
                // Ctrl+A → SELECTIONNER TOUT
                // ─────────────────────────────────────────────────────────────────
                if ((mod & KMOD_CTRL) && event->key.keysym.sym == SDLK_a) {
                    selectionner_tout(editor);
                    return true;
                }

                // ─────────────────────────────────────────────────────────────────
                // Ctrl+Shift+F -> RÉINDENTER PROPREMENT LE JSON
                // ─────────────────────────────────────────────────────────────────
                if ((mod & KMOD_CTRL) && (mod & KMOD_SHIFT) && event->key.keysym.sym == SDLK_f) {
                    reindenter_json(editor);
                    return true;
                }

                // Si aucun raccourci, on continue avec les touches normales
                switch (event->key.keysym.sym) {
                    case SDLK_BACKSPACE:
                        supprimer_caractere(editor);  // ✅ Le undo est géré à l'intérieur
                        auto_scroll_curseur(editor);
                        return true;
                    case SDLK_DELETE:
                        supprimer_caractere_apres(editor);  // ✅ Le undo est géré à l'intérieur
                        auto_scroll_curseur(editor);
                        return true;
                    case SDLK_RETURN:
                        sauvegarder_etat_undo(editor);
                        inserer_nouvelle_ligne(editor);
                        auto_scroll_curseur(editor);  // ← Scroll auto
                        return true;
                    case SDLK_LEFT:
                        // Si Shift enfoncé, on sélectionne
                        if (mod & KMOD_SHIFT) {
                            if (!editor->selection_active) {
                                // Démarrer une nouvelle sélection
                                editor->selection_start = editor->curseur_position;
                                editor->selection_active = true;
                            }
                        } else {
                            // Sans Shift, on annule la sélection
                            deselectionner(editor);
                        }

                        // ═══════════════════════════════════════════════════════════════
                        // RECULER D'UN CARACTÈRE UTF-8 COMPLET (pas juste 1 octet)
                        // ═══════════════════════════════════════════════════════════════
                        if (editor->curseur_position > 0) {
                            int pos = editor->curseur_position - 1;

                            // Si on est sur un octet de continuation UTF-8 (10xxxxxx),
                            // reculer jusqu'au début du caractère
                            while (pos > 0 && (editor->buffer[pos] & 0xC0) == 0x80) {
                                pos--;
                            }

                            editor->curseur_position = pos;
                        }

                        if (editor->selection_active) {
                            editor->selection_end = editor->curseur_position;
                        }

                        auto_scroll_curseur(editor);
                        return true;
                    case SDLK_RIGHT:
                        // Si Shift enfoncé, on sélectionne
                        if (mod & KMOD_SHIFT) {
                            if (!editor->selection_active) {
                                editor->selection_start = editor->curseur_position;
                                editor->selection_active = true;
                            }
                        } else {
                            deselectionner(editor);
                        }

                        // ═══════════════════════════════════════════════════════════════
                        // AVANCER D'UN CARACTÈRE UTF-8 COMPLET
                        // ═══════════════════════════════════════════════════════════════
                        int buffer_len = strlen(editor->buffer);
                        if (editor->curseur_position < buffer_len) {
                            // Déterminer la longueur du caractère UTF-8 actuel
                            unsigned char c = (unsigned char)editor->buffer[editor->curseur_position];
                            int char_len = 1;  // Par défaut, 1 octet (ASCII)

                            if (c < 0x80) {
                                char_len = 1;      // ASCII: 0xxxxxxx
                            } else if ((c & 0xE0) == 0xC0) {
                                char_len = 2;      // 110xxxxx
                            } else if ((c & 0xF0) == 0xE0) {
                                char_len = 3;      // 1110xxxx
                            } else if ((c & 0xF8) == 0xF0) {
                                char_len = 4;      // 11110xxx
                            }

                            // Avancer de char_len octets
                            editor->curseur_position += char_len;

                            // Sécurité : ne pas dépasser la fin du buffer
                            if (editor->curseur_position > buffer_len) {
                                editor->curseur_position = buffer_len;
                            }
                        }

                        if (editor->selection_active) {
                            editor->selection_end = editor->curseur_position;
                        }

                        auto_scroll_curseur(editor);
                        return true;
                    case SDLK_UP:
                        deplacer_curseur_vertical(editor, -1);
                        auto_scroll_curseur(editor);  // ← Scroll auto
                        return true;
                    case SDLK_DOWN:
                        deplacer_curseur_vertical(editor, 1);
                        auto_scroll_curseur(editor);  // ← Scroll auto
                        return true;
                    case SDLK_HOME:
                        while (editor->curseur_position > 0 &&
                            editor->buffer[editor->curseur_position - 1] != '\n') {
                            editor->curseur_position--;
                            }
                            auto_scroll_curseur(editor);  // ← Scroll auto
                            return true;
                    case SDLK_END:
                        while (editor->buffer[editor->curseur_position] != '\0' &&
                            editor->buffer[editor->curseur_position] != '\n') {
                            editor->curseur_position++;
                            }
                            auto_scroll_curseur(editor);  // ← Scroll auto
                            return true;
                }
            }
            break;

                    case SDL_TEXTINPUT:
                        if (event->text.windowID == window_id) {
                            // Sauvegarder seulement si dernier snapshot date de plus de 1 seconde
                            static Uint32 dernier_snapshot = 0;
                            Uint32 maintenant = SDL_GetTicks();

                            if (maintenant - dernier_snapshot > 1000) {  // 1 seconde
                                sauvegarder_etat_undo(editor);
                                dernier_snapshot = maintenant;
                            }

                            for (int i = 0; event->text.text[i] != '\0'; i++) {
                                inserer_caractere(editor, event->text.text[i]);
                            }
                            return true;
                        }
                        break;

                    case SDL_MOUSEMOTION:
                        if (event->motion.windowID == window_id) {
                            int x = event->motion.x;
                            int y = event->motion.y;

                            // Gérer le survol des boutons
                            gerer_survol_boutons(editor, x, y);

                            // Gérer le survol du menu contextuel
                            if (editor->context_menu.visible) {
                                gerer_survol_menu_contextuel(editor);
                            }

                            // Si bouton gauche enfoncé, on sélectionne en glissant
                            if (event->motion.state & SDL_BUTTON_LMASK) {
                                // ─────────────────────────────────────────────────────────────────
                                // Récupérer la hauteur actuelle pour la zone de texte
                                // ─────────────────────────────────────────────────────────────────

                                // Clic-glissé dans la zone de texte
                                if (y >= 40 && y < editor->hauteur_fenetre - 60 && x >= LEFT_MARGIN) {
                                    int ligne_survol = editor->scroll_offset + ((y - 40) / LINE_HEIGHT);
                                    if (ligne_survol >= editor->nb_lignes) {
                                        ligne_survol = editor->nb_lignes - 1;
                                    }

                                    // Même calcul que pour le clic
                                    char ligne_texte[256];
                                    obtenir_ligne(editor->buffer, ligne_survol, ligne_texte, sizeof(ligne_texte));

                                    int colonne_survol = 0;
                                    int distance_min = 999999;
                                    int x_relatif = x - LEFT_MARGIN + editor->scroll_offset_x;  // ← Prendre en compte le scroll horizontal !

                                    int nb_chars_utf8 = utf8_strlen(ligne_texte);
                                    for (int char_index = 0; char_index <= nb_chars_utf8; char_index++) {
                                        char substring[256];
                                        utf8_strncpy(substring, ligne_texte, char_index, sizeof(substring));

                                        int largeur_avant = 0;
                                        if (editor->font_mono && char_index > 0) {
                                            TTF_SizeUTF8(editor->font_mono, substring, &largeur_avant, NULL);
                                        }

                                        int distance = abs(x_relatif - largeur_avant);
                                        if (distance < distance_min) {
                                            distance_min = distance;
                                            colonne_survol = char_index;
                                        }
                                    }

                                    // Position dans le buffer
                                    int pos = 0;
                                    int ligne_courante = 0;
                                    while (editor->buffer[pos] && ligne_courante < ligne_survol) {
                                        if (editor->buffer[pos] == '\n') ligne_courante++;
                                        pos++;
                                    }

                                    // Avancer jusqu'à la colonne (en caractères UTF-8!)
                                    int bytes_to_advance = utf8_advance(editor->buffer + pos, colonne_survol);
                                    pos += bytes_to_advance;

                                    // Activer la sélection et mettre à jour la fin
                                    editor->selection_active = true;
                                    editor->selection_end = pos;
                                    editor->curseur_position = pos;

                                    auto_scroll_curseur(editor);
                                }
                            }

                            return true;
                        }
                        break;

                        case SDL_MOUSEWHEEL:
                            if (event->wheel.windowID == window_id) {
                                // ═════════════════════════════════════════════════════════════════
                                // VÉRIFIER SI ON EST SUR UN NOMBRE
                                // ═════════════════════════════════════════════════════════════════
                                int debut_nombre, fin_nombre;
                                bool sur_nombre = trouver_nombre_au_curseur(editor, &debut_nombre, &fin_nombre);

                                if (sur_nombre) {
                                    // ─────────────────────────────────────────────────────────────
                                    // MODE ÉDITION : Molette modifie le nombre
                                    // ─────────────────────────────────────────────────────────────
                                    int delta = event->wheel.y;  // > 0 = up (incrémenter), < 0 = down (décrémenter)
                                    modifier_nombre_au_curseur(editor, delta);
                                    auto_scroll_curseur(editor);

                                    debug_printf("🎡 MOLETTE sur nombre: delta=%d\n", delta);
                                } else {
                                    // ─────────────────────────────────────────────────────────────
                                    // MODE NORMAL : Molette scroll le document
                                    // ─────────────────────────────────────────────────────────────
                                    int scroll_amount = -event->wheel.y * 3;  // 3 lignes par cran

                                    editor->scroll_offset += scroll_amount;

                                    // Ne pas scroller avant le début
                                    if (editor->scroll_offset < 0) {
                                        editor->scroll_offset = 0;
                                    }

                                    // Ne pas scroller au-delà de la fin
                                    int nb_lignes_visibles = obtenir_nb_lignes_visibles(editor);
                                    int max_scroll = editor->nb_lignes - nb_lignes_visibles;
                                    if (max_scroll < 0) max_scroll = 0;

                                    if (editor->scroll_offset > max_scroll) {
                                        editor->scroll_offset = max_scroll;
                                    }

                                    debug_printf("📜 SCROLL: offset=%d\n", editor->scroll_offset);
                                }

                                return true;
                            }
                            break;

                        case SDL_MOUSEBUTTONDOWN:
                            if (event->button.windowID == window_id) {
                                int x = event->button.x;
                                int y = event->button.y;

                                // ═════════════════════════════════════════════════════════════════
                                // CLIC DROIT - MENU CONTEXTUEL
                                // ═════════════════════════════════════════════════════════════════
                                if (event->button.button == SDL_BUTTON_RIGHT) {
                                    // Si le menu est déjà visible, le cacher
                                    if (editor->context_menu.visible) {
                                        cacher_menu_contextuel(editor);
                                    } else {
                                        // Sinon, l'afficher à la position du clic
                                        afficher_menu_contextuel(editor, x, y);
                                    }
                                    return true;
                                }

                                // ═════════════════════════════════════════════════════════════════
                                // CLIC GAUCHE
                                // ═════════════════════════════════════════════════════════════════
                                else if (event->button.button == SDL_BUTTON_LEFT) {
                                    // Si le menu contextuel est visible, gérer le clic dedans
                                    if (editor->context_menu.visible) {
                                        if (gerer_clic_menu_contextuel(editor, x, y)) {
                                            // Un item a été cliqué, le menu se ferme automatiquement
                                            return true;
                                        } else {
                                            // Clic en dehors du menu, le cacher
                                            cacher_menu_contextuel(editor);
                                        }
                                    }

                                // Tester d'abord les boutons
                                if (gerer_clic_boutons(editor, x, y)) {
                                    return true;
                                }

                                        // Clic dans la zone de texte pour positionner le curseur
                                        // Récupérer la hauteur actuelle de la fenêtre pour la zone de texte
                                        if (y >= 40 && y < editor->hauteur_fenetre - 60 && x >= LEFT_MARGIN) {
                                            int ligne_cliquee = editor->scroll_offset + ((y - 40) / LINE_HEIGHT);
                                            if (ligne_cliquee >= editor->nb_lignes) {
                                                ligne_cliquee = editor->nb_lignes - 1;
                                            }

                                            // ✅ Récupérer la ligne complète
                                            char ligne_texte[256];
                                            obtenir_ligne(editor->buffer, ligne_cliquee, ligne_texte, sizeof(ligne_texte));

                                            // ✅ Trouver la colonne (UTF-8 aware)
                                            int colonne_trouvee = 0;
                                            int distance_min = 999999;
                                            int x_relatif = x - LEFT_MARGIN;

                                            // Compter les caractères UTF-8 (pas les octets!)
                                            int nb_chars_utf8 = utf8_strlen(ligne_texte);

                                            // Pour chaque caractère UTF-8
                                            for (int char_index = 0; char_index <= nb_chars_utf8; char_index++) {
                                                // Extraire une sous-chaîne de char_index caractères UTF-8
                                                char substring[256];
                                                utf8_strncpy(substring, ligne_texte, char_index, sizeof(substring));

                                                int largeur_avant = 0;
                                                if (editor->font_mono && char_index > 0) {
                                                    TTF_SizeUTF8(editor->font_mono, substring, &largeur_avant, NULL);
                                                }

                                                // Distance entre le clic et cette position
                                                int distance = abs(x_relatif - largeur_avant);
                                                if (distance < distance_min) {
                                                    distance_min = distance;
                                                    colonne_trouvee = char_index;
                                                }
                                            }


                                            // ✅ Trouver la position dans le buffer (en octets)
                                            int pos = 0;
                                            int ligne_courante = 0;

                                            // Aller à la ligne
                                            while (editor->buffer[pos] && ligne_courante < ligne_cliquee) {
                                                if (editor->buffer[pos] == '\n') ligne_courante++;
                                                pos++;
                                            }

                                            // Avancer jusqu'à la colonne (en caractères UTF-8!)
                                            int bytes_to_advance = utf8_advance(editor->buffer + pos, colonne_trouvee);
                                            pos += bytes_to_advance;

                                            editor->curseur_position = pos;

                                            // ═════════════════════════════════════════════════════════════════
                                            // GESTION DES CLICS MULTIPLES
                                            // ═════════════════════════════════════════════════════════════════
                                            if (event->button.clicks == 1) {
                                                // Simple clic : démarrer une sélection potentielle
                                                editor->selection_start = pos;
                                                editor->selection_end = pos;
                                                editor->selection_active = false;

                                                debug_printf("🖱️ SIMPLE CLIC: ligne=%d, col=%d, pos=%d\n",
                                                             ligne_cliquee, colonne_trouvee, pos);
                                            }
                                            else if (event->button.clicks == 2) {
                                                // Double-clic : sélectionner le mot
                                                selectionner_mot_au_curseur(editor);

                                                debug_printf("🖱️🖱️ DOUBLE-CLIC: mot sélectionné\n");
                                            }
                                            else if (event->button.clicks >= 3) {
                                                // Triple-clic : sélectionner la ligne
                                                selectionner_ligne_courante(editor);

                                                debug_printf("🖱️🖱️🖱️ TRIPLE-CLIC: ligne sélectionnée\n");
                                            }

                                            return true;
                                        }
                                }
                            }
                            break;
    }

    return false;
}
