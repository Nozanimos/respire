// json_editor_window.c
#include "json_editor_window.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <cjson/cJSON.h>
#include <SDL2/SDL2_gfxPrimitives.h>

// ════════════════════════════════════════════════════════════════════════════
//  PROTOTYPES DES FONCTIONS INTERNES
// ════════════════════════════════════════════════════════════════════════════
static void sauvegarder_etat_undo(JsonEditor* editor);
static void faire_undo(JsonEditor* editor);
static void faire_redo(JsonEditor* editor);
static void auto_scroll_curseur(JsonEditor* editor);
static void deselectionner(JsonEditor* editor);

// ════════════════════════════════════════════════════════════════════════════
//  UTILITAIRES UTF8 POUR LA SÉLECTION
// ════════════════════════════════════════════════════════════════════════════
static inline int min_int(int a, int b) { return a < b ? a : b; }
static inline int max_int(int a, int b) { return a > b ? a : b; }

// Retourne le nombre d'octets d'un caractère UTF-8
static int utf8_char_len(const char* str) {
    unsigned char c = (unsigned char)*str;
    if (c < 0x80) return 1;        // ASCII: 0xxxxxxx
    if ((c & 0xE0) == 0xC0) return 2;  // 110xxxxx
    if ((c & 0xF0) == 0xE0) return 3;  // 1110xxxx
    if ((c & 0xF8) == 0xF0) return 4;  // 11110xxx
    return 1;  // Invalid UTF-8, traiter comme 1 octet
}

// Compte le nombre de caractères UTF-8 dans une chaîne
static int utf8_strlen(const char* str) {
    int count = 0;
    while (*str) {
        str += utf8_char_len(str);
        count++;
    }
    return count;
}

// Avance de n caractères UTF-8 dans une chaîne, retourne le nombre d'octets avancés
static int utf8_advance(const char* str, int n_chars) {
    int bytes = 0;
    for (int i = 0; i < n_chars && str[bytes]; i++) {
        bytes += utf8_char_len(str + bytes);
    }
    return bytes;
}

// Copie n caractères UTF-8 (pas n octets!) dans dest
static void utf8_strncpy(char* dest, const char* src, int n_chars, int max_bytes) {
    int bytes_copied = 0;
    int chars_copied = 0;

    while (chars_copied < n_chars && src[bytes_copied] && bytes_copied < max_bytes - 1) {
        int char_len = utf8_char_len(src + bytes_copied);
        if (bytes_copied + char_len >= max_bytes) break;

        for (int i = 0; i < char_len; i++) {
            dest[bytes_copied] = src[bytes_copied];
            bytes_copied++;
        }
        chars_copied++;
    }
    dest[bytes_copied] = '\0';
}

// ════════════════════════════════════════════════════════════════════════════
//  DÉSÉLECTIONNER LE TEXTE
// ════════════════════════════════════════════════════════════════════════════
static void deselectionner(JsonEditor* editor) {
    if (!editor) return;
    editor->selection_active = false;
    editor->selection_start = -1;
    editor->selection_end = -1;
}
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

    // Initialisation de la sélection
    editor->selection_start = -1;
    editor->selection_end = -1;
    editor->selection_active = false;

    // Activer la saisie texte pour cette fenêtre
    SDL_StartTextInput();

    debug_printf("✅ Éditeur JSON créé\n");

    // ─────────────────────────────────────────────────────────────────────────
    // INITIALISATION DU SYSTÈME UNDO
    // ─────────────────────────────────────────────────────────────────────────
    editor->current_undo = NULL;
    editor->undo_count = 0;
    editor->max_undo_count = 100;  // Maximum 100 états dans l'historique

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
    editor->scroll_offset_x = 0;  // ← Scroll horizontal à 0
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

    // Annuler la sélection si on tape du texte
    if (editor->selection_active) {
        deselectionner(editor);
    }

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
    if (!editor) return;

    // Si une sélection est active, supprimer toute la sélection
    if (editor->selection_active) {
        int sel_min = min_int(editor->selection_start, editor->selection_end);
        int sel_max = max_int(editor->selection_start, editor->selection_end);

        if (sel_min != sel_max) {
            // Supprimer tout le texte entre sel_min et sel_max
            int len = strlen(editor->buffer);
            memmove(&editor->buffer[sel_min],
                    &editor->buffer[sel_max],
                    len - sel_max + 1);

            editor->curseur_position = sel_min;
            editor->modified = true;
            editor->nb_lignes = compter_lignes(editor->buffer);

            deselectionner(editor);
            debug_printf("✅ Sélection supprimée (de %d à %d)\n", sel_min, sel_max);
            return;
        }
    }

    // ═════════════════════════════════════════════════════════════════════════
    // SUPPRESSION NORMALE D'UN SEUL CARACTÈRE (UTF-8 AWARE)
    // ═════════════════════════════════════════════════════════════════════════

    debug_printf("🔍 [BACKSPACE] Début - curseur_pos=%d\n", editor->curseur_position);

    if (editor->curseur_position <= 0) {
        debug_printf("⚠️ [BACKSPACE] Curseur au début du buffer, rien à supprimer\n");
        return;
    }

    int pos = editor->curseur_position;

    // Afficher le contexte avant suppression
    if (pos > 0) {
        debug_printf("🔍 [BACKSPACE] Caractère avant curseur: 0x%02X '%c'\n",
                     (unsigned char)editor->buffer[pos-1],
                     (editor->buffer[pos-1] >= 32 && editor->buffer[pos-1] < 127) ? editor->buffer[pos-1] : '?');
    }

    // ─────────────────────────────────────────────────────────────────────────
    // TROUVER LE DÉBUT DU CARACTÈRE UTF-8 À SUPPRIMER
    // ─────────────────────────────────────────────────────────────────────────
    int char_start = pos - 1;

    // Si on est sur un octet de continuation (10xxxxxx), reculer jusqu'au début
    while (char_start > 0 && (editor->buffer[char_start] & 0xC0) == 0x80) {
        char_start--;
        debug_printf("🔍 [BACKSPACE] Recul UTF-8: char_start maintenant à %d\n", char_start);
    }

    // Calculer le nombre d'octets du caractère à supprimer
    int char_len = pos - char_start;

    debug_printf("🔍 [BACKSPACE] Caractère UTF-8 détecté:\n");
    debug_printf("              - Position début: %d\n", char_start);
    debug_printf("              - Position fin: %d\n", pos);
    debug_printf("              - Longueur: %d octets\n", char_len);

    // Afficher les octets du caractère
    debug_printf("              - Octets: ");
    for (int i = char_start; i < pos; i++) {
        debug_printf("0x%02X ", (unsigned char)editor->buffer[i]);
    }
    debug_printf("\n");

    // ─────────────────────────────────────────────────────────────────────────
    // SUPPRIMER LE CARACTÈRE
    // ─────────────────────────────────────────────────────────────────────────
    int buffer_len = strlen(editor->buffer);

    debug_printf("🔍 [BACKSPACE] memmove: dest=%d, src=%d, len=%d\n",
                 char_start, pos, buffer_len - pos + 1);

    memmove(editor->buffer + char_start,
            editor->buffer + pos,
            buffer_len - pos + 1);  // +1 pour le '\0'

            // Mettre à jour le curseur
            editor->curseur_position = char_start;
            editor->modified = true;
            editor->nb_lignes = compter_lignes(editor->buffer);

            debug_printf("✅ [BACKSPACE] Caractère supprimé! Nouveau curseur_pos=%d\n",
                         editor->curseur_position);
            debug_printf("✅ [BACKSPACE] Buffer après suppression: longueur=%d\n",
                         (int)strlen(editor->buffer));
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

        // Maintenant le texte par-dessus
        if (ligne[0] != '\0') {
            render_text(editor->renderer, editor->font_mono, ligne,
                        LEFT_MARGIN - editor->scroll_offset_x, y, 0xFFEEEEEE);
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
//  AUTO-SCROLL POUR SUIVRE LE CURSEUR (VERTICAL ET HORIZONTAL)
// ════════════════════════════════════════════════════════════════════════════
static void auto_scroll_curseur(JsonEditor* editor) {
    if (!editor) return;

    // ─────────────────────────────────────────────────────────────────────────
    // SCROLL VERTICAL
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

    int nb_lignes_visibles = (EDITOR_HEIGHT - 100) / LINE_HEIGHT;

    // Si le curseur est au-dessus de la zone visible, scroller vers le haut
    if (curseur_ligne < editor->scroll_offset) {
        editor->scroll_offset = curseur_ligne;
        debug_printf("⬆️ AUTO-SCROLL HAUT: ligne=%d\n", curseur_ligne);
    }

    // Si le curseur est en-dessous de la zone visible, scroller vers le bas
    if (curseur_ligne >= editor->scroll_offset + nb_lignes_visibles) {
        editor->scroll_offset = curseur_ligne - nb_lignes_visibles + 1;
        debug_printf("⬇️ AUTO-SCROLL BAS: ligne=%d\n", curseur_ligne);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // SCROLL HORIZONTAL
    // ─────────────────────────────────────────────────────────────────────────
    // Seulement si on a une police chargée
    if (!editor->font_mono) return;

    // Calculer la largeur du texte avant le curseur (= position du curseur)
    char ligne_actuelle[256];
    obtenir_ligne(editor->buffer, curseur_ligne, ligne_actuelle, sizeof(ligne_actuelle));

    char texte_avant_curseur[256];
    int len = (curseur_colonne < 255) ? curseur_colonne : 255;
    strncpy(texte_avant_curseur, ligne_actuelle, len);
    texte_avant_curseur[len] = '\0';

    int largeur_texte = 0;
    if (len > 0) {
        TTF_SizeUTF8(editor->font_mono, texte_avant_curseur, &largeur_texte, NULL);
    }

    // Position ABSOLUE du curseur (sans tenir compte du scroll)
    int curseur_x_absolu = largeur_texte;

    // Position du curseur à l'ÉCRAN (avec le scroll actuel)
    int curseur_x_ecran = LEFT_MARGIN + curseur_x_absolu - editor->scroll_offset_x;

    // Largeur de la zone de texte visible
    int largeur_zone_texte = EDITOR_WIDTH - LEFT_MARGIN - 20;  // 20px de marge droite

    // Marge de confort : on laisse 80 pixels avant de scroller
    const int MARGE_CONFORT = 80;

    // ═════════════════════════════════════════════════════════════════════════
    // CAS 1 : Le curseur sort à DROITE → on scroll vers la droite
    // ═════════════════════════════════════════════════════════════════════════
    if (curseur_x_ecran > EDITOR_WIDTH - MARGE_CONFORT) {
        // Décaler le scroll pour ramener le curseur à une position confortable
        editor->scroll_offset_x = curseur_x_absolu - largeur_zone_texte + MARGE_CONFORT;
        debug_printf("➡️ AUTO-SCROLL DROITE: curseur_abs=%d, offset_x=%d\n",
                     curseur_x_absolu, editor->scroll_offset_x);
    }

    // ═════════════════════════════════════════════════════════════════════════
    // CAS 2 : Le curseur sort à GAUCHE → on scroll vers la gauche
    // ═════════════════════════════════════════════════════════════════════════
    if (curseur_x_ecran < LEFT_MARGIN + MARGE_CONFORT) {
        // Décaler le scroll pour ramener le curseur à une position confortable
        editor->scroll_offset_x = curseur_x_absolu - MARGE_CONFORT;
        if (editor->scroll_offset_x < 0) editor->scroll_offset_x = 0;
        debug_printf("⬅️ AUTO-SCROLL GAUCHE: curseur_abs=%d, offset_x=%d\n",
                     curseur_x_absolu, editor->scroll_offset_x);
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  SAUVEGARDE UN SNAPSHOT DANS L'HISTORIQUE UNDO
// ════════════════════════════════════════════════════════════════════════════
static void sauvegarder_etat_undo(JsonEditor* editor) {
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

// ════════════════════════════════════════════════════════════════════════════
//  UNDO : Revenir à l'état précédent
// ════════════════════════════════════════════════════════════════════════════
static void faire_undo(JsonEditor* editor) {
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

// ════════════════════════════════════════════════════════════════════════════
//  REDO : Avancer vers l'état suivant
// ════════════════════════════════════════════════════════════════════════════
static void faire_redo(JsonEditor* editor) {
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
                // ═════════════════════════════════════════════════════════════════
                // GESTION DES RACCOURCIS CLAVIER (Ctrl+...)
                // ═════════════════════════════════════════════════════════════════
                SDL_Keymod mod = SDL_GetModState();

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
                // Ctrl+Shift+Z → REDO (pour plus tard)
                // ─────────────────────────────────────────────────────────────────
                if ((mod & KMOD_CTRL) && (mod & KMOD_SHIFT) && event->key.keysym.sym == SDLK_z) {
                    faire_redo(editor);
                    return true;
                }

                // ─────────────────────────────────────────────────────────────────
                // Ctrl+Y → REDO alternatif (certains éditeurs utilisent ça)
                // ─────────────────────────────────────────────────────────────────
                if ((mod & KMOD_CTRL) && event->key.keysym.sym == SDLK_y) {
                    faire_redo(editor);
                    return true;
                }

                // Si aucun raccourci, on continue avec les touches normales
                switch (event->key.keysym.sym) {
                    case SDLK_BACKSPACE:
                        sauvegarder_etat_undo(editor);
                        supprimer_caractere(editor);
                        auto_scroll_curseur(editor);  // ← Scroll auto
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

                        if (editor->curseur_position > 0) {
                            editor->curseur_position--;
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

                        if (editor->curseur_position < (int)strlen(editor->buffer)) {
                            editor->curseur_position++;
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

                            // Si bouton gauche enfoncé, on sélectionne en glissant
                            if (event->motion.state & SDL_BUTTON_LMASK) {
                                // Clic-glissé dans la zone de texte
                                if (y >= 40 && y < EDITOR_HEIGHT - 60 && x >= LEFT_MARGIN) {
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
                                    int col = 0;
                                    while (col < colonne_survol && editor->buffer[pos] && editor->buffer[pos] != '\n') {
                                        pos++;
                                        col++;
                                    }

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
                                // event->wheel.y > 0 = scroll vers le haut (reculer)
                                // event->wheel.y < 0 = scroll vers le bas (avancer)
                                int scroll_amount = -event->wheel.y * 3;  // 3 lignes par cran de molette

                                editor->scroll_offset += scroll_amount;

                                // ─────────────────────────────────────────────────────────────
                                // LIMITES DE SCROLL
                                // ─────────────────────────────────────────────────────────────
                                // Ne pas scroller avant le début
                                if (editor->scroll_offset < 0) {
                                    editor->scroll_offset = 0;
                                }

                                // Ne pas scroller au-delà de la fin
                                int nb_lignes_visibles = (EDITOR_HEIGHT - 100) / LINE_HEIGHT;
                                int max_scroll = editor->nb_lignes - nb_lignes_visibles;
                                if (max_scroll < 0) max_scroll = 0;  // Si le document tient dans l'écran

                                if (editor->scroll_offset > max_scroll) {
                                    editor->scroll_offset = max_scroll;
                                }

                                debug_printf("📜 SCROLL: offset=%d, nb_lignes=%d, max_scroll=%d\n",
                                             editor->scroll_offset, editor->nb_lignes, max_scroll);

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

                                            // Démarrer une sélection si on clique
                                            editor->selection_start = pos;
                                            editor->selection_end = pos;
                                            editor->selection_active = false;  // Pas encore actif tant qu'on ne bouge pas

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
