// SPDX-License-Identifier: GPL-3.0-or-later
#include "json_editor.h"
#include "core/debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "core/widget_base.h"
#include "core/memory/memory.h"

//  CALLBACKS DES BOUTONS
// Ces fonctions sont appelées quand on clique sur les boutons

static void callback_recharger(JsonEditor* editor) {
    if (charger_fichier_json(editor)) {
        debug_printf("🔄 JSON rechargé via bouton\n");
    } else {
        debug_printf("❌ Erreur rechargement\n");
    }
}

static void callback_sauvegarder(JsonEditor* editor) {
    if (sauvegarder_fichier_json(editor)) {
        debug_printf("💾 JSON sauvegardé via bouton\n");
    } else {
        debug_printf("❌ Erreur sauvegarde\n");
    }
}

static void callback_toggle_autosave(JsonEditor* editor) {
    // Toggle l'état de la sauvegarde automatique
    editor->auto_save_enabled = !editor->auto_save_enabled;

    // Trouver le bouton "Auto-Save" et changer sa couleur
    for (int i = 0; i < editor->nb_boutons; i++) {
        if (strcmp(editor->boutons[i].label, "Auto-Save") == 0) {
            if (editor->auto_save_enabled) {
                // Activé: bleu
                editor->boutons[i].couleur_normale = 0xFF2060E0;
                editor->boutons[i].couleur_survol = 0xFF4080FF;
                debug_printf("✅ Sauvegarde automatique ACTIVÉE\n");
            } else {
                // Désactivé: gris
                editor->boutons[i].couleur_normale = 0xFF808080;
                editor->boutons[i].couleur_survol = 0xFFA0A0A0;
                debug_printf("❌ Sauvegarde automatique DÉSACTIVÉE\n");
            }
            break;
        }
    }
}

//  GESTION DES BOUTONS

// CRÉATION D'UN BOUTON
EditorButton creer_bouton(const char* label, int largeur, int hauteur,
                          Uint32 couleur_normale, Uint32 couleur_survol,
                          BoutonCallback callback) {
    EditorButton btn;
    snprintf(btn.label, sizeof(btn.label), "%s", label);

    // Position sera calculée par ajouter_bouton()
    btn.rect = (SDL_Rect){0, 0, largeur, hauteur};

    btn.couleur_normale = couleur_normale;
    btn.couleur_survol = couleur_survol;
    btn.survole = false;
    btn.callback = callback;

    return btn;
}

// AJOUT D'UN BOUTON À L'ÉDITEUR
void ajouter_bouton(JsonEditor* editor, EditorButton bouton) {
    if (!editor) return;

    // Allouer ou agrandir le tableau si nécessaire
    if (editor->nb_boutons >= editor->capacite_boutons) {
        int nouvelle_capacite = editor->capacite_boutons == 0 ? 4 : editor->capacite_boutons * 2;
        EditorButton* nouveau = realloc(editor->boutons, nouvelle_capacite * sizeof(EditorButton));
        if (!nouveau) {
            debug_printf("❌ Erreur allocation boutons\n");
            return;
        }
        editor->boutons = nouveau;
        editor->capacite_boutons = nouvelle_capacite;
    }

    // Ajouter le bouton
    editor->boutons[editor->nb_boutons] = bouton;
    editor->nb_boutons++;

    debug_printf("➕ Bouton ajouté : %s (total: %d)\n", bouton.label, editor->nb_boutons);
}

// RECALCUL DES POSITIONS DE TOUS LES BOUTONS
void recalculer_tous_boutons(JsonEditor* editor) {
    if (!editor || !editor->window) return;

    // Récupérer la hauteur actuelle
    SDL_GetWindowSize(editor->window, NULL, &editor->hauteur_fenetre);

    // Position de départ : 20px de la gauche, 50px du bas
    int x_courant = 20;
    int y_boutons = editor->hauteur_fenetre - 50;
    int espacement = 10;  // Espace entre les boutons

    // Positionner chaque bouton côte à côte
    for (int i = 0; i < editor->nb_boutons; i++) {
        editor->boutons[i].rect.x = x_courant;
        editor->boutons[i].rect.y = y_boutons;

        // Prochain bouton commence après celui-ci + espacement
        x_courant += editor->boutons[i].rect.w + espacement;
    }

    debug_printf("📐 %d boutons repositionnés (y=%d)\n", editor->nb_boutons, y_boutons);
}

// GESTION DES CLICS SUR LES BOUTONS
bool gerer_clic_boutons(JsonEditor* editor, int x, int y) {
    if (!editor) return false;

    // Tester chaque bouton
    for (int i = 0; i < editor->nb_boutons; i++) {
        SDL_Rect* rect = &editor->boutons[i].rect;

        if (x >= rect->x && x <= rect->x + rect->w &&
            y >= rect->y && y <= rect->y + rect->h) {

            // Bouton cliqué ! Appeler son callback
            if (editor->boutons[i].callback) {
                editor->boutons[i].callback(editor);
            }

            debug_printf("🖱️ Clic bouton : %s\n", editor->boutons[i].label);
            return true;
        }
    }

    return false;
}

// GESTION DU SURVOL DES BOUTONS
void gerer_survol_boutons(JsonEditor* editor, int x, int y) {
    if (!editor) return;

    // Tester chaque bouton
    for (int i = 0; i < editor->nb_boutons; i++) {
        SDL_Rect* rect = &editor->boutons[i].rect;

        bool survole = (x >= rect->x && x <= rect->x + rect->w &&
                        y >= rect->y && y <= rect->y + rect->h);

        editor->boutons[i].survole = survole;
    }
}

// DESTRUCTION DES BOUTONS
void detruire_boutons(JsonEditor* editor) {
    if (!editor) return;

    if (editor->boutons) {
        SAFE_FREE(editor->boutons);
        editor->boutons = NULL;
    }

    editor->nb_boutons = 0;
    editor->capacite_boutons = 0;
}

//  CRÉATION DE L'ÉDITEUR
JsonEditor* creer_json_editor(const char* filepath, int pos_x, int pos_y) {
    JsonEditor* editor = SAFE_MALLOC(sizeof(JsonEditor));
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
        SAFE_FREE(editor);
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
        SAFE_FREE(editor);
        return NULL;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // CHARGEMENT DES POLICES
    // ─────────────────────────────────────────────────────────────────────────
    // Obtenir les polices depuis le gestionnaire centralisé
    editor->font_mono = get_font_for_size(14);
    editor->font_ui = get_font_for_size(16);

    if (!editor->font_mono || !editor->font_ui) {
        debug_printf("❌ JSON Editor: impossible d'obtenir les polices\n");
        SAFE_FREE(editor);
        return NULL;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // INITIALISATION DU MENU CONTEXTUEL (APRÈS les polices!)
    // ─────────────────────────────────────────────────────────────────────────
    initialiser_menu_contextuel(editor);

    // ─────────────────────────────────────────────────────────────────────────
    // INITIALISATION DU SYSTÈME DE BOUTONS
    // ─────────────────────────────────────────────────────────────────────────
    editor->boutons = NULL;
    editor->nb_boutons = 0;
    editor->capacite_boutons = 0;

    // Créer et ajouter les boutons
    EditorButton btn_recharger = creer_bouton(
        "Recharger",                    // Label
        130, 30,                        // Largeur, Hauteur
        0xFF2060E0,                     // Couleur normale (bleu)
        0xFF4080FF,                     // Couleur survol (bleu clair)
        callback_recharger              // Fonction à appeler
    );
    ajouter_bouton(editor, btn_recharger);

    EditorButton btn_sauvegarder = creer_bouton(
        "Sauvegarder",                  // Label
        130, 30,                        // Largeur, Hauteur
        0xFF30A050,                     // Couleur normale (vert)
        0xFF40C060,                     // Couleur survol (vert clair)
        callback_sauvegarder            // Fonction à appeler
    );
    ajouter_bouton(editor, btn_sauvegarder);

    EditorButton btn_autosave = creer_bouton(
        "Auto-Save",                    // Label
        130, 30,                        // Largeur, Hauteur
        0xFF808080,                     // Couleur normale (gris - désactivé par défaut)
        0xFFA0A0A0,                     // Couleur survol (gris clair)
        callback_toggle_autosave        // Fonction à appeler
    );
    ajouter_bouton(editor, btn_autosave);

    // Calculer les positions initiales
    recalculer_tous_boutons(editor);

    // ─────────────────────────────────────────────────────────────────────────
    // INITIALISATION DU SYSTÈME UNDO
    // ─────────────────────────────────────────────────────────────────────────
    editor->current_undo = NULL;
    editor->undo_count = 0;
    editor->max_undo_count = 100;  // Maximum 100 états dans l'historique

    // ─────────────────────────────────────────────────────────────────────────
    // INITIALISATION DU SYSTÈME AUTO-SAVE
    // ─────────────────────────────────────────────────────────────────────────
    editor->auto_save_enabled = false;  // Désactivé par défaut (bouton gris)
    editor->last_modification_time = 0;
    editor->auto_save_delay = 0.3f;  // Sauvegarder 0.3s après la dernière modif

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

//  CALCUL DU NOMBRE DE LIGNES VISIBLES
int obtenir_nb_lignes_visibles(JsonEditor* editor) {
    if (!editor || !editor->window) return 0;

    // ─────────────────────────────────────────────────────────────────────────
    // RÉCUPÉRATION DE LA HAUTEUR ACTUELLE
    // ─────────────────────────────────────────────────────────────────────────
    // On récupère la hauteur et on la stocke dans la structure
    // pour éviter de rappeler SDL_GetWindowSize plusieurs fois
    SDL_GetWindowSize(editor->window, NULL, &editor->hauteur_fenetre);

    // ─────────────────────────────────────────────────────────────────────────
    // CALCUL DES LIGNES VISIBLES
    // ─────────────────────────────────────────────────────────────────────────
    // Zone de texte = hauteur totale - header (40px) - footer avec boutons (60px)
    // Donc hauteur_fenetre - 100 pixels
    int zone_texte = editor->hauteur_fenetre - 100;
    int nb_lignes = zone_texte / LINE_HEIGHT;

    return nb_lignes;
}

// MENU CONTEXTUEL
// Actions du menu contextuel
void action_copier_contextuel(JsonEditor* editor) {
    copier_selection(editor);
    debug_printf("📋 [Menu] Copier\n");
}

void action_couper_contextuel(JsonEditor* editor) {
    couper_selection(editor);
    debug_printf("✂️ [Menu] Couper\n");
}

void action_coller_contextuel(JsonEditor* editor) {
    coller_texte(editor);
    debug_printf("📝 [Menu] Coller\n");
}

void action_tout_selectionner_contextuel(JsonEditor* editor) {
    selectionner_tout(editor);
    debug_printf("🔲 [Menu] Tout sélectionner\n");
}

void action_dupliquer_contextuel(JsonEditor* editor) {
    // Utiliser ta fonction existante de duplication
    // (à adapter selon ton implémentation)
    dupliquer_ligne_courante(editor);
    debug_printf("📄 [Menu] Dupliquer\n");
}

void action_reindenter_contextuel(JsonEditor* editor){
    reindenter_json(editor);
    debug_printf("📄 [Menu] réindenter\n");
}

void action_generer_code_c_contextuel(JsonEditor* editor) {
    // Appeler la fonction de génération de code C
    if (generer_code_c_depuis_json(editor)) {
        debug_printf("✨ [Menu] Code C généré avec succès\n");
    } else {
        debug_printf("❌ [Menu] Erreur lors de la génération du code C\n");
    }
}

// Initialisation avec boucle pour éviter la répétition
void initialiser_menu_contextuel(JsonEditor* editor) {
    ContextMenu* menu = &editor->context_menu;

    // Configuration des couleurs (thème sombre)
    menu->bg_color = (SDL_Color){30, 30, 35, 255};
    menu->border_color = (SDL_Color){100, 100, 110, 255};
    menu->text_enabled_color = (SDL_Color){255, 255, 255, 255};
    menu->text_disabled_color = (SDL_Color){128, 128, 128, 255};
    menu->hover_color = (SDL_Color){181, 181, 181, 50};

    // Définition des items avec groupes et séparateurs
    struct {
        char* label;
        void (*action)(JsonEditor*);
        bool default_enabled;
        bool is_separator;  // true pour les séparateurs
        bool has_submenu;   // true pour les items avec sous-menu
    } items_config[] = {
        // Groupe 1: Copier/Couper/Coller
        {"Copier", action_copier_contextuel, false, false, false},
        {"Couper", action_couper_contextuel, false, false, false},
        {"Coller", action_coller_contextuel, true, false, false},

        // Séparateur 1
        {NULL, NULL, false, true, false},

        // Groupe 2: Tout sélectionner
        {"Tout sélectionner", action_tout_selectionner_contextuel, true, false, false},

        // Séparateur 2
        {NULL, NULL, false, true, false},

        // Groupe 3: Dupliquer et Formatage
        {"Dupliquer", action_dupliquer_contextuel, true, false, false},
        {"Réindenter", action_reindenter_contextuel, true, false, false},

        // Séparateur 3
        {NULL, NULL, false, true, false},

        // Groupe 4: Templates (avec sous-menu)
        {"Templates", NULL, true, false, true},

        // Séparateur 4
        {NULL, NULL, false, true, false},

        // Groupe 5: Génération de code (EN DERNIER, sur fond rouge sombre)
        {"Générer Code C", action_generer_code_c_contextuel, true, false, false}
    };

    menu->item_count = sizeof(items_config) / sizeof(items_config[0]);

    // Initialisation des items avec boucle
    for (int i = 0; i < menu->item_count; i++) {
        if (items_config[i].is_separator) {
            // Item séparateur
            menu->items[i] = (ContextMenuItem){
                .label = NULL,  // Pas de label
                .rect = (SDL_Rect){0, 0, 0, 0},
                .enabled = false,
                .action = NULL,   // Pas d'action
                .sous_menu = NULL,
                .template_data = NULL
            };
        } else if (items_config[i].has_submenu) {
            // Item avec sous-menu (Templates)
            menu->items[i] = (ContextMenuItem){
                .label = items_config[i].label,
                .rect = (SDL_Rect){0, 0, 0, 0},
                .enabled = items_config[i].default_enabled,
                .action = NULL,   // Pas d'action directe, c'est un sous-menu
                .sous_menu = creer_sous_menu_templates(editor),  // Créer le sous-menu
                .template_data = NULL
            };
        } else {
            // Item normal
            menu->items[i] = (ContextMenuItem){
                .label = items_config[i].label,
                .rect = (SDL_Rect){0, 0, 0, 0},
                .enabled = items_config[i].default_enabled,
                .action = items_config[i].action,
                .sous_menu = NULL,
                .template_data = NULL
            };
        }
    }

    menu->visible = false;
    menu->hovered_item = -1;
    debug_printf("📋 Menu contextuel initialisé avec %d items (dont séparateurs et sous-menus)\n", menu->item_count);
}

// Calcule les dimensions du menu basées sur le texte
static void calculer_dimensions_menu(JsonEditor* editor) {
    ContextMenu* menu = &editor->context_menu;

    int max_width = 0;
    const int padding = 20; // Marge intérieure
    const int line_height = 25; // Hauteur par ligne normale
    const int separator_height = 10; // Hauteur des séparateurs

    // Calculer la largeur maximale basée sur le texte le plus long
    for (int i = 0; i < menu->item_count; i++) {
        // Ignorer les séparateurs pour le calcul de largeur
        if (menu->items[i].label != NULL) {
            int text_width = 0;
            if (editor->font_ui) {
                TTF_SizeUTF8(editor->font_ui, menu->items[i].label, &text_width, NULL);
            } else {
                // Fallback si pas de police
                text_width = strlen(menu->items[i].label) * 8;
            }

            if (text_width > max_width) {
                max_width = text_width;
            }
        }
    }

    menu->width = max_width + padding * 2;

    // Calculer la hauteur totale en comptant les séparateurs
    menu->height = 0;
    for (int i = 0; i < menu->item_count; i++) {
        if (menu->items[i].label == NULL) {
            // Séparateur
            menu->height += separator_height;
        } else {
            // Item normal
            menu->height += line_height;
        }
    }

    // Mettre à jour les rectangles des items
    int current_y = 0;
    for (int i = 0; i < menu->item_count; i++) {
        if (menu->items[i].label == NULL) {
            // Séparateur
            menu->items[i].rect = (SDL_Rect){
                0, // x sera calculé à l'affichage
                current_y, // y position cumulative
                menu->width,
                separator_height
            };
            current_y += separator_height;
        } else {
            // Item normal
            menu->items[i].rect = (SDL_Rect){
                0, // x sera calculé à l'affichage
                current_y, // y position cumulative
                menu->width,
                line_height
            };
            current_y += line_height;
        }
    }
}

// Calcule les dimensions d'un sous-menu (similaire à calculer_dimensions_menu)
// Cette fonction est nécessaire pour positionner correctement les sous-menus
static void calculer_dimensions_sous_menu(JsonEditor* editor, ContextMenu* sous_menu) {
    if (!sous_menu) return;

    int max_width = 0;
    const int padding = 20; // Marge intérieure
    const int line_height = 25; // Hauteur par ligne normale

    // Calculer la largeur maximale basée sur le texte le plus long
    for (int i = 0; i < sous_menu->item_count; i++) {
        if (sous_menu->items[i].label != NULL) {
            int text_width = 0;
            if (editor->font_ui) {
                TTF_SizeUTF8(editor->font_ui, sous_menu->items[i].label, &text_width, NULL);
            } else {
                // Fallback si pas de police
                text_width = strlen(sous_menu->items[i].label) * 8;
            }

            if (text_width > max_width) {
                max_width = text_width;
            }
        }
    }

    sous_menu->width = max_width + padding * 2;

    // Calculer la hauteur totale
    sous_menu->height = sous_menu->item_count * line_height;

    // Mettre à jour les rectangles des items
    int current_y = 0;
    for (int i = 0; i < sous_menu->item_count; i++) {
        sous_menu->items[i].rect = (SDL_Rect){
            0, // x sera calculé à l'affichage
            current_y, // y position cumulative
            sous_menu->width,
            line_height
        };
        current_y += line_height;
    }
}

void afficher_menu_contextuel(JsonEditor* editor, int x, int y) {
    ContextMenu* menu = &editor->context_menu;

    // Calculer les dimensions basées sur le contenu
    calculer_dimensions_menu(editor);

    // Mettre à jour la disponibilité des items
    mettre_a_jour_menu_contextuel(editor);

    // Positionner le menu (s'assurer qu'il reste dans la fenêtre)
    int window_width, window_height;
    SDL_GetWindowSize(editor->window, &window_width, &window_height);

    if (x + menu->width > window_width) {
        x = window_width - menu->width;
    }
    if (y + menu->height > window_height) {
        y = window_height - menu->height;
    }

    menu->x = x;
    menu->y = y;
    menu->visible = true;

    // Les rectangles des items sont déjà correctement calculés dans calculer_dimensions_menu()
    // avec des coordonnées relatives (x=0, y=position_cumulative)
    // Pas besoin de les recalculer ici

    debug_printf("📋 Menu contextuel affiché en (%d, %d), taille: %dx%d\n",
                 x, y, menu->width, menu->height);
}

void cacher_menu_contextuel(JsonEditor* editor) {
    editor->context_menu.visible = false;

    // Cacher tous les sous-menus
    for (int i = 0; i < editor->context_menu.item_count; i++) {
        if (editor->context_menu.items[i].sous_menu) {
            editor->context_menu.items[i].sous_menu->visible = false;
        }
    }
}

bool gerer_clic_menu_contextuel(JsonEditor* editor, int x, int y) {
    ContextMenu* menu = &editor->context_menu;

    if (!menu->visible) return false;

    // ─────────────────────────────────────────────────────────────────────────
    // VÉRIFIER D'ABORD LES CLICS DANS LES SOUS-MENUS
    // ─────────────────────────────────────────────────────────────────────────
    for (int i = 0; i < menu->item_count; i++) {
        ContextMenu* sous_menu = menu->items[i].sous_menu;

        if (!sous_menu || !sous_menu->visible) continue;

        // Vérifier si le clic est dans le sous-menu
        if (x >= sous_menu->x && x <= sous_menu->x + sous_menu->width &&
            y >= sous_menu->y && y <= sous_menu->y + sous_menu->height) {

            // Trouver l'item cliqué dans le sous-menu
            for (int j = 0; j < sous_menu->item_count; j++) {
                ContextMenuItem* sub_item = &sous_menu->items[j];

                SDL_Rect absolute_rect = {
                    sous_menu->x + sub_item->rect.x,
                    sous_menu->y + sub_item->rect.y,
                    sub_item->rect.w,
                    sub_item->rect.h
                };

                if (x >= absolute_rect.x && x <= absolute_rect.x + absolute_rect.w &&
                    y >= absolute_rect.y && y <= absolute_rect.y + absolute_rect.h) {

                    // Item cliqué trouvé
                    if (sub_item->enabled && sub_item->action != NULL) {
                        // Exécuter l'action
                        sub_item->action(editor);
                        return true;
                    }
                    break;
                    }
            }
            }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // VÉRIFIER LES CLICS DANS LE MENU PRINCIPAL
    // ─────────────────────────────────────────────────────────────────────────
    // Vérifier si le clic est dans le menu principal
    if (x >= menu->x && x <= menu->x + menu->width &&
        y >= menu->y && y <= menu->y + menu->height) {

        // Trouver l'item cliqué
        for (int i = 0; i < menu->item_count; i++) {
            ContextMenuItem* item = &menu->items[i];
            SDL_Rect absolute_rect = {
                menu->x + item->rect.x,
                menu->y + item->rect.y,
                item->rect.w,
                item->rect.h
            };

            if (x >= absolute_rect.x && x <= absolute_rect.x + absolute_rect.w &&
                y >= absolute_rect.y && y <= absolute_rect.y + absolute_rect.h) {

                // Ignorer les séparateurs et items désactivés
                // Les items avec sous-menu ne doivent pas être cliqués (juste survolés)
                if (item->label != NULL && item->enabled && item->action != NULL && !item->sous_menu) {
                    // Exécuter l'action et cacher le menu
                    item->action(editor);
                    cacher_menu_contextuel(editor);
                    return true;
                }
                break; // On a trouvé l'item, même si c'est un séparateur
                }
        }
        }

        return false;
}

// Gestion du survol pour la surbrillance
void gerer_survol_menu_contextuel(JsonEditor* editor) {
    ContextMenu* menu = &editor->context_menu;

    if (!menu->visible) return;

    // ─────────────────────────────────────────────────────────────────────────
    // GESTION DU SURVOL POUR LE MENU PRINCIPAL
    // ─────────────────────────────────────────────────────────────────────────
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    // Réinitialiser le hover
    menu->hovered_item = -1;

    // Vérifier si la souris est sur un item du menu principal
    for (int i = 0; i < menu->item_count; i++) {
        ContextMenuItem* item = &menu->items[i];

        // Ignorer les séparateurs
        if (item->label == NULL) continue;

        SDL_Rect absolute_rect = {
            menu->x + item->rect.x,
            menu->y + item->rect.y,
            item->rect.w,
            item->rect.h
        };

        // Vérifier si la souris est sur cet item
        if (mouse_x >= absolute_rect.x && mouse_x <= absolute_rect.x + absolute_rect.w &&
            mouse_y >= absolute_rect.y && mouse_y <= absolute_rect.y + absolute_rect.h) {

            menu->hovered_item = i;

        // Si l'item a un sous-menu, l'afficher
        if (item->sous_menu) {
            ContextMenu* sous_menu = item->sous_menu;

            // Calculer les dimensions du sous-menu
            calculer_dimensions_sous_menu(editor, sous_menu);

            // Positionner le sous-menu à droite de l'item
            int window_width, window_height;
            SDL_GetWindowSize(editor->window, &window_width, &window_height);

            // Position par défaut : à droite du menu principal
            sous_menu->x = menu->x + menu->width;
            sous_menu->y = absolute_rect.y;

            // Vérifier qu'il ne dépasse pas de la fenêtre
            if (sous_menu->x + sous_menu->width > window_width) {
                // Afficher à gauche du menu principal si pas de place à droite
                sous_menu->x = menu->x - sous_menu->width;
            }

            if (sous_menu->y + sous_menu->height > window_height) {
                sous_menu->y = window_height - sous_menu->height;
            }

            sous_menu->visible = true;
        }

        break;
            }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // GESTION DU SURVOL POUR LES SOUS-MENUS
    // ─────────────────────────────────────────────────────────────────────────

    // Parcourir tous les items pour trouver ceux avec sous-menu
    for (int i = 0; i < menu->item_count; i++) {
        ContextMenu* sous_menu = menu->items[i].sous_menu;

        if (!sous_menu || !sous_menu->visible) continue;

        // Réinitialiser le hover du sous-menu
        sous_menu->hovered_item = -1;

        // Vérifier si la souris est sur le sous-menu
        if (mouse_x >= sous_menu->x && mouse_x <= sous_menu->x + sous_menu->width &&
            mouse_y >= sous_menu->y && mouse_y <= sous_menu->y + sous_menu->height) {

            // Trouver quel item du sous-menu est survolé
            for (int j = 0; j < sous_menu->item_count; j++) {
                ContextMenuItem* sub_item = &sous_menu->items[j];

                SDL_Rect absolute_rect = {
                    sous_menu->x + sub_item->rect.x,
                    sous_menu->y + sub_item->rect.y,
                    sub_item->rect.w,
                    sub_item->rect.h
                };

                if (mouse_x >= absolute_rect.x && mouse_x <= absolute_rect.x + absolute_rect.w &&
                    mouse_y >= absolute_rect.y && mouse_y <= absolute_rect.y + absolute_rect.h) {

                    sous_menu->hovered_item = j;
                break;
                    }
            }
            } else {
                // La souris n'est plus sur le sous-menu ni sur l'item parent
                // Vérifier si on est toujours sur l'item parent qui ouvre le sous-menu
                if (menu->hovered_item != i) {
                    sous_menu->visible = false;
                }
            }
    }
}

void mettre_a_jour_menu_contextuel(JsonEditor* editor) {
    ContextMenu* menu = &editor->context_menu;

    for (int i = 0; i < menu->item_count; i++) {
        ContextMenuItem* item = &menu->items[i];

        // Ignorer les séparateurs
        if (item->label == NULL) continue;

        // Identifier l'item par son label et mettre à jour son état
        if (strcmp(item->label, "Copier") == 0) {
            item->enabled = editor->selection_active;
        }
        else if (strcmp(item->label, "Couper") == 0) {
            item->enabled = editor->selection_active;
        }
        else if (strcmp(item->label, "Coller") == 0) {
            item->enabled = (editor->clipboard[0] != '\0');
        }
        else if (strcmp(item->label, "Tout sélectionner") == 0) {
            item->enabled = (strlen(editor->buffer) > 0);
        }
        else if (strcmp(item->label, "Dupliquer") == 0) {
            item->enabled = true; // Toujours disponible
        }
    }
}

//  DESTRUCTION
void detruire_json_editor(JsonEditor* editor) {
    if (!editor) return;

    // ─────────────────────────────────────────────────────────────────────────
    // LIBÉRER LES SOUS-MENUS
    // ─────────────────────────────────────────────────────────────────────────
    for (int i = 0; i < editor->context_menu.item_count; i++) {
        if (editor->context_menu.items[i].sous_menu) {
            detruire_sous_menu(editor->context_menu.items[i].sous_menu);
        }
    }

    if (editor->renderer) SDL_DestroyRenderer(editor->renderer);
    if (editor->window) SDL_DestroyWindow(editor->window);

    detruire_boutons(editor);
    SAFE_FREE(editor);
    debug_printf("🗑️ Éditeur JSON détruit\n");
}
