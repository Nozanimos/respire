// json_editor_window.c
#include "json_editor_window.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <cjson/cJSON.h>
#include <SDL2/SDL2_gfxPrimitives.h>

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
    // CHARGEMENT DU FICHIER
    // ─────────────────────────────────────────────────────────────────────────
    if (!charger_fichier_json(editor)) {
        debug_printf("⚠️ Fichier JSON non trouvé, buffer vide\n");
    }

    // Activer la saisie texte pour cette fenêtre
    SDL_StartTextInput();

    debug_printf("✅ Éditeur JSON créé\n");
    return editor;
}

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
    editor->modified = false;
    editor->nb_lignes = compter_lignes(editor->buffer);

    editor->json_valide = valider_json(editor);

    // ✅ AJOUTER CE LOG
    debug_printf("✅ Fichier chargé: %zu octets, %d lignes\n", bytes_read, editor->nb_lignes);
    debug_printf("🎯 Position initiale du curseur: %d\n", editor->curseur_position);
    debug_printf("🎯 Premier caractère du buffer: '%c'\n", editor->buffer[0]);
    debug_printf("🎯 Caractère à position 730: '%c'\n", editor->buffer[730]);

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

// ════════════════════════════════════════════════════════════════════════════
//  INSERTION DE CARACTÈRE
// ════════════════════════════════════════════════════════════════════════════
void inserer_caractere(JsonEditor* editor, char c) {
    if (!editor) return;

    int len = strlen(editor->buffer);
    if (len >= JSON_BUFFER_SIZE - 1) return;

    // ✅ AJOUTER CE LOG
    debug_printf("🔍 INSERT: curseur_pos=%d, char_insere='%c'\n",
                 editor->curseur_position, c);

    // Décaler tous les caractères après le curseur
    memmove(&editor->buffer[editor->curseur_position + 1],
            &editor->buffer[editor->curseur_position],
            len - editor->curseur_position + 1);

    // Insérer le caractère
    editor->buffer[editor->curseur_position] = c;
    editor->curseur_position++;
    editor->modified = true;
    editor->nb_lignes = compter_lignes(editor->buffer);

    // ✅ AJOUTER CE LOG
    debug_printf("✅ Après insertion: nouveau_curseur_pos=%d\n", editor->curseur_position);
}

// ════════════════════════════════════════════════════════════════════════════
//  SUPPRESSION DE CARACTÈRE (BACKSPACE)
// ════════════════════════════════════════════════════════════════════════════
void supprimer_caractere(JsonEditor* editor) {
    if (!editor || editor->curseur_position <= 0) return;

    int len = strlen(editor->buffer);

    // ✅ AJOUTER CE LOG
    debug_printf("🔍 BACKSPACE: curseur_pos=%d, char_avant='%c', char_apres='%c'\n",
                 editor->curseur_position,
                 editor->buffer[editor->curseur_position - 1],
                 editor->buffer[editor->curseur_position]);

    // Décaler tous les caractères après le curseur vers la gauche
    memmove(&editor->buffer[editor->curseur_position - 1],
            &editor->buffer[editor->curseur_position],
            len - editor->curseur_position + 1);

    editor->curseur_position--;
    editor->modified = true;
    editor->nb_lignes = compter_lignes(editor->buffer);

    // ✅ AJOUTER CE LOG AUSSI
    debug_printf("✅ Après suppression: nouveau_curseur_pos=%d\n", editor->curseur_position);
}

// ════════════════════════════════════════════════════════════════════════════
//  INSERTION NOUVELLE LIGNE
// ════════════════════════════════════════════════════════════════════════════
void inserer_nouvelle_ligne(JsonEditor* editor) {
    inserer_caractere(editor, '\n');
}

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

    extern void render_text(SDL_Renderer* renderer, TTF_Font* font,
                            const char* text, int x, int y, Uint32 color);

    for (int i = editor->scroll_offset; i < editor->scroll_offset + nb_lignes_visibles && i < editor->nb_lignes; i++) {
        obtenir_ligne(editor->buffer, i, ligne, sizeof(ligne));

        if (!editor->font_mono) continue;

        // Numéro de ligne (gris moyen)
        char num_str[16];
        snprintf(num_str, sizeof(num_str), "%3d", i + 1);
        render_text(editor->renderer, editor->font_mono, num_str, 5, y, 0xFF888888);

        // Contenu de la ligne (blanc) - seulement si non vide
        // ✅ FIX: ligne est un tableau, donc toujours non-null, on teste juste le 1er char
        if (ligne[0] != '\0') {
            render_text(editor->renderer, editor->font_mono, ligne, LEFT_MARGIN, y, 0xFFEEEEEE);
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
    strncpy(texte_avant_curseur, ligne_actuelle, len);
    texte_avant_curseur[len] = '\0';

    // Mesurer la largeur réelle avec TTF_SizeText
    int largeur_texte = 0;
    if (editor->font_mono && len > 0) {
        TTF_SizeText(editor->font_mono, texte_avant_curseur, &largeur_texte, NULL);
    }

    int curseur_x = LEFT_MARGIN + largeur_texte;  // ✅ Position PRÉCISE !
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

// ════════════════════════════════════════════════════════════════════════════
//  DÉPLACEMENT VERTICAL DU CURSEUR
// ════════════════════════════════════════════════════════════════════════════
static void deplacer_curseur_vertical(JsonEditor* editor, int direction) {
    if (!editor) return;

    // Calculer ligne et colonne actuelles
    int ligne_actuelle = 0;
    int colonne_actuelle = 0;

    for (int i = 0; i < editor->curseur_position; i++) {
        if (editor->buffer[i] == '\n') {
            ligne_actuelle++;
            colonne_actuelle = 0;
        } else {
            colonne_actuelle++;
        }
    }

    // Ligne cible
    int ligne_cible = ligne_actuelle + direction;
    if (ligne_cible < 0) {
        ligne_cible = 0;
        colonne_actuelle = 0; // Si on remonte en haut, on va au début
    }
    if (ligne_cible >= editor->nb_lignes) {
        ligne_cible = editor->nb_lignes - 1;
    }

    // ✅ FIX : Trouver le début de la ligne cible CORRECTEMENT
    int pos = 0;
    int ligne_courante = 0;

    // Parcourir jusqu'à la ligne cible
    while (editor->buffer[pos] && ligne_courante < ligne_cible) {
        if (editor->buffer[pos] == '\n') {
            ligne_courante++;
        }
        pos++;
    }

    // Maintenant pos est au DÉBUT de la ligne cible
    // Avancer sur la ligne jusqu'à la colonne souhaitée (ou fin de ligne)
    int col = 0;
    while (col < colonne_actuelle &&
        editor->buffer[pos] &&
        editor->buffer[pos] != '\n') {
        pos++;
    col++;
        }

        editor->curseur_position = pos;

        // ✅ LOG pour vérifier
        debug_printf("🔧 VERTICAL: ligne_actuelle=%d, ligne_cible=%d, col=%d, nouvelle_pos=%d\n",
                     ligne_actuelle, ligne_cible, colonne_actuelle, pos);
}

// ════════════════════════════════════════════════════════════════════════════
//  GESTION DES ÉVÉNEMENTS
// ════════════════════════════════════════════════════════════════════════════

bool gerer_evenements_json_editor(JsonEditor* editor, SDL_Event* event) {
    if (!editor || !event) return false;

    Uint32 window_id = SDL_GetWindowID(editor->window);

    switch (event->type) {
        case SDL_WINDOWEVENT:
            if (event->window.windowID == window_id) {
                if (event->window.event == SDL_WINDOWEVENT_CLOSE) {
                    editor->est_ouvert = false;
                }
                return true;
            }
            break;

        case SDL_KEYDOWN:
            if (event->key.windowID == window_id) {
                switch (event->key.keysym.sym) {
                    case SDLK_BACKSPACE:
                        supprimer_caractere(editor);
                        return true;
                    case SDLK_RETURN:
                        inserer_nouvelle_ligne(editor);
                        return true;
                    case SDLK_LEFT:
                        if (editor->curseur_position > 0) {
                            editor->curseur_position--;
                        }
                        return true;
                    case SDLK_RIGHT:
                        if (editor->curseur_position < (int)strlen(editor->buffer)) {
                            editor->curseur_position++;
                        }
                        return true;
                    case SDLK_UP:
                        deplacer_curseur_vertical(editor, -1);
                        return true;
                    case SDLK_DOWN:
                        deplacer_curseur_vertical(editor, 1);
                        return true;
                    case SDLK_HOME:
                        while (editor->curseur_position > 0 &&
                               editor->buffer[editor->curseur_position - 1] != '\n') {
                            editor->curseur_position--;
                        }
                        return true;
                    case SDLK_END:
                        while (editor->buffer[editor->curseur_position] != '\0' &&
                               editor->buffer[editor->curseur_position] != '\n') {
                            editor->curseur_position++;
                        }
                        return true;
                }
            }
            break;

        case SDL_TEXTINPUT:
            if (event->text.windowID == window_id) {
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

                editor->bouton_recharger_survole =
                    (x >= editor->bouton_recharger.x && 
                     x <= editor->bouton_recharger.x + editor->bouton_recharger.w &&
                     y >= editor->bouton_recharger.y && 
                     y <= editor->bouton_recharger.y + editor->bouton_recharger.h);

                editor->bouton_sauvegarder_survole =
                    (x >= editor->bouton_sauvegarder.x && 
                     x <= editor->bouton_sauvegarder.x + editor->bouton_sauvegarder.w &&
                     y >= editor->bouton_sauvegarder.y && 
                     y <= editor->bouton_sauvegarder.y + editor->bouton_sauvegarder.h);

                return true;
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (event->button.windowID == window_id) {
                int x = event->button.x;
                int y = event->button.y;

                if (x >= editor->bouton_recharger.x &&
                    x <= editor->bouton_recharger.x + editor->bouton_recharger.w &&
                    y >= editor->bouton_recharger.y &&
                    y <= editor->bouton_recharger.y + editor->bouton_recharger.h) {
                    charger_fichier_json(editor);
                    debug_printf("🔄 JSON rechargé\n");
                    return true;
                }

                if (x >= editor->bouton_sauvegarder.x &&
                    x <= editor->bouton_sauvegarder.x + editor->bouton_sauvegarder.w &&
                    y >= editor->bouton_sauvegarder.y &&
                    y <= editor->bouton_sauvegarder.y + editor->bouton_sauvegarder.h) {
                    if (sauvegarder_fichier_json(editor)) {
                        debug_printf("💾 JSON sauvegardé\n");
                    }
                    return true;
                }

                // Clic dans la zone de texte pour positionner le curseur
                if (y >= 40 && y < EDITOR_HEIGHT - 60 && x >= LEFT_MARGIN) {
                    int ligne_cliquee = editor->scroll_offset + ((y - 40) / LINE_HEIGHT);
                    if (ligne_cliquee >= editor->nb_lignes) {
                        ligne_cliquee = editor->nb_lignes - 1;
                    }

                    // ✅ Récupérer la ligne complète
                    char ligne_texte[256];
                    obtenir_ligne(editor->buffer, ligne_cliquee, ligne_texte, sizeof(ligne_texte));

                    // ✅ Trouver la colonne en mesurant la largeur caractère par caractère
                    int colonne_trouvee = 0;
                    //int largeur_cumulee = 0;
                    int distance_min = 999999;
                    int x_relatif = x - LEFT_MARGIN;  // Position du clic relative au début du texte

                    // Parcourir chaque caractère et mesurer
                    for (int col = 0; ligne_texte[col] != '\0'; col++) {
                        // Mesurer la largeur jusqu'à cette colonne
                        char substring[256];
                        strncpy(substring, ligne_texte, col);
                        substring[col] = '\0';

                        int largeur = 0;
                        if (editor->font_mono && col > 0) {
                            TTF_SizeText(editor->font_mono, substring, &largeur, NULL);
                        }

                        // Distance entre le clic et cette position
                        int distance = abs(x_relatif - largeur);
                        if (distance < distance_min) {
                            distance_min = distance;
                            colonne_trouvee = col;
                        } else {
                            // On s'éloigne, on a trouvé la meilleure position
                            break;
                        }
                    }

                    // Si on a cliqué après la fin de la ligne, aller à la fin
                    int largeur_ligne_complete = 0;
                    if (editor->font_mono && ligne_texte[0] != '\0') {
                        TTF_SizeText(editor->font_mono, ligne_texte, &largeur_ligne_complete, NULL);
                    }
                    if (x_relatif > largeur_ligne_complete) {
                        colonne_trouvee = strlen(ligne_texte);
                    }

                    // ✅ Trouver la position dans le buffer
                    int pos = 0;
                    int ligne_courante = 0;

                    // Aller à la ligne
                    while (editor->buffer[pos] && ligne_courante < ligne_cliquee) {
                        if (editor->buffer[pos] == '\n') ligne_courante++;
                        pos++;
                    }

                    // Avancer jusqu'à la colonne
                    int col = 0;
                    while (col < colonne_trouvee && editor->buffer[pos] && editor->buffer[pos] != '\n') {
                        pos++;
                        col++;
                    }

                    editor->curseur_position = pos;

                    debug_printf("🖱️ CLIC: ligne=%d, col=%d, pos=%d\n",
                                 ligne_cliquee, colonne_trouvee, pos);

                    return true;
                }
            }
            break;
    }

    return false;
}

// ════════════════════════════════════════════════════════════════════════════
//  UTILITAIRES
// ════════════════════════════════════════════════════════════════════════════
int compter_lignes(const char* buffer) {
    if (!buffer) return 0;
    int count = 1;
    for (const char* p = buffer; *p; p++) {
        if (*p == '\n') count++;
    }
    return count;
}

const char* obtenir_ligne(const char* buffer, int index_ligne, char* dest, int max_len) {
    if (!buffer || !dest || index_ligne < 0) {
        dest[0] = '\0';
        return dest;
    }

    const char* start = buffer;
    int current_line = 0;

    // Trouver le début de la ligne
    while (*start && current_line < index_ligne) {
        if (*start == '\n') current_line++;
        start++;
    }

    // Copier jusqu'au \n ou fin
    int i = 0;
    while (*start && *start != '\n' && i < max_len - 1) {
        dest[i++] = *start++;
    }
    dest[i] = '\0';

    return dest;
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
