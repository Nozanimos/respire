// SPDX-License-Identifier: GPL-3.0-or-later
// json_config_loader.c
#include "json_config_loader.h"
//#include "settings_panel.h"
#include "debug.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <string.h>
#include <SDL2/SDL2_gfxPrimitives.h>

// ════════════════════════════════════════════════════════════════════════════
//  TABLE DE CORRESPONDANCE : NOM CALLBACK → POINTEUR FONCTION
// ════════════════════════════════════════════════════════════════════════════
// PROBLÈME : Dans le JSON on a des strings comme "duration_value_changed"
// mais on a besoin de pointeurs de fonction réels.
//
// SOLUTION TEMPORAIRE : On va déclarer les fonctions en extern et faire
// un simple if/strcmp pour matcher les noms.
//
// TODO PLUS TARD : Créer une vraie table de hashmap pour être plus propre

// Déclarations externes des callbacks (définis dans settings_panel.c)
extern void duration_value_changed(int value);
extern void cycles_value_changed(int value);
extern void nb_breath(int value);
extern void start_value_changed(int value);
extern void session_value_changed(int value);
extern void alternate_cycles_changed(bool value);
extern void apply_button_clicked(void);
extern void cancel_button_clicked(void);

// Callbacks pour le selector type de rétention
extern void retention_full(void);
extern void retention_empty(void);
extern void retention_alternate(void);

// Fonction helper pour récupérer un callback INT par son nom
static void (*obtenir_callback_int(const char* nom))(int) {
    if (!nom) return NULL;

    if (strcmp(nom, "duration_value_changed") == 0) {
        return duration_value_changed;
    }
    if (strcmp(nom, "cycles_value_changed") == 0) {
        return cycles_value_changed;
    }
    if (strcmp(nom, "nb_breath") == 0) {
        return nb_breath;
    }
    if (strcmp(nom, "start_value_changed") == 0) {
        return start_value_changed;
    }
    if (strcmp(nom, "session_value_changed") == 0) {
        return session_value_changed;
    }

    debug_printf("⚠️ Callback INT inconnu: '%s'\n", nom);
    return NULL;
}

// Fonction helper pour récupérer un callback BOOL par son nom
static void (*obtenir_callback_bool(const char* nom))(bool) {
    if (!nom) return NULL;

    if (strcmp(nom, "alternate_cycles_changed") == 0) {
        return alternate_cycles_changed;
    }

    debug_printf("⚠️ Callback BOOL inconnu: '%s'\n", nom);
    return NULL;
}

// Fonction helper pour récupérer un callback VOID (pour les boutons et selectors) par son nom
static void (*obtenir_callback_void(const char* nom))(void) {
    if (!nom) return NULL;

    if (strcmp(nom, "apply_button_clicked") == 0) {
        return apply_button_clicked;
    }
    if (strcmp(nom, "cancel_button_clicked") == 0) {
        return cancel_button_clicked;
    }

    // Callbacks du selector type de rétention
    if (strcmp(nom, "retention_full") == 0) {
        return retention_full;
    }
    if (strcmp(nom, "retention_empty") == 0) {
        return retention_empty;
    }
    if (strcmp(nom, "retention_alternate") == 0) {
        return retention_alternate;
    }

    debug_printf("⚠️ Callback VOID inconnu: '%s'\n", nom);
    return NULL;
}

// ════════════════════════════════════════════════════════════════════════════
//  PARSING D'UN WIDGET INCREMENT
// ════════════════════════════════════════════════════════════════════════════
bool parser_widget_increment(cJSON* json_obj, LoaderContext* ctx, WidgetList* list) {
    // Récupération des champs obligatoires
    cJSON* id = cJSON_GetObjectItem(json_obj, "id");
    cJSON* nom_affichage = cJSON_GetObjectItem(json_obj, "nom_affichage");
    cJSON* x = cJSON_GetObjectItem(json_obj, "x");
    cJSON* y = cJSON_GetObjectItem(json_obj, "y");
    cJSON* valeur_min = cJSON_GetObjectItem(json_obj, "valeur_min");
    cJSON* valeur_max = cJSON_GetObjectItem(json_obj, "valeur_max");
    cJSON* valeur_depart = cJSON_GetObjectItem(json_obj, "valeur_depart");
    cJSON* increment = cJSON_GetObjectItem(json_obj, "increment");
    cJSON* taille_fleche = cJSON_GetObjectItem(json_obj, "taille_fleche");
    cJSON* taille_texte = cJSON_GetObjectItem(json_obj, "taille_texte");
    cJSON* callback = cJSON_GetObjectItem(json_obj, "callback");

    // Validation
    if (!cJSON_IsString(id) || !cJSON_IsString(nom_affichage) ||
        !cJSON_IsNumber(x) || !cJSON_IsNumber(y) ||
        !cJSON_IsNumber(valeur_min) || !cJSON_IsNumber(valeur_max) ||
        !cJSON_IsNumber(valeur_depart)) {
        debug_printf("❌ Widget increment invalide : champs manquants\n");
        return false;
    }

    // Récupération du callback
    void (*callback_func)(int) = NULL;
    if (cJSON_IsString(callback)) {
        callback_func = obtenir_callback_int(callback->valuestring);
    }

    // Création du widget
    bool success = add_increment_widget(
        list,
        id->valuestring,
        nom_affichage->valuestring,
        x->valueint,
        y->valueint,
        valeur_min->valueint,
        valeur_max->valueint,
        valeur_depart->valueint,
        cJSON_IsNumber(increment) ? increment->valueint : 1,
        cJSON_IsNumber(taille_fleche) ? taille_fleche->valueint : 6,
        cJSON_IsNumber(taille_texte) ? taille_texte->valueint : 18,
        ctx->font_normal,
        callback_func
    );

    if (success) {
        debug_printf("✅ Widget increment '%s' chargé depuis JSON\n", id->valuestring);
    }

    return success;
}

// ════════════════════════════════════════════════════════════════════════════
//  PARSING D'UN WIDGET TOGGLE
// ════════════════════════════════════════════════════════════════════════════
bool parser_widget_toggle(cJSON* json_obj, LoaderContext* ctx, WidgetList* list) {
    (void)ctx;  // Paramètre non utilisé

    // Récupération des champs
    cJSON* id = cJSON_GetObjectItem(json_obj, "id");
    cJSON* nom_affichage = cJSON_GetObjectItem(json_obj, "nom_affichage");
    cJSON* x = cJSON_GetObjectItem(json_obj, "x");
    cJSON* y = cJSON_GetObjectItem(json_obj, "y");
    cJSON* etat_depart = cJSON_GetObjectItem(json_obj, "etat_depart");
    cJSON* largeur_toggle = cJSON_GetObjectItem(json_obj, "largeur_toggle");
    cJSON* hauteur_toggle = cJSON_GetObjectItem(json_obj, "hauteur_toggle");
    cJSON* taille_curseur = cJSON_GetObjectItem(json_obj, "taille_curseur");
    cJSON* taille_texte = cJSON_GetObjectItem(json_obj, "taille_texte");
    cJSON* callback = cJSON_GetObjectItem(json_obj, "callback");

    // Validation
    if (!cJSON_IsString(id) || !cJSON_IsString(nom_affichage) ||
        !cJSON_IsNumber(x) || !cJSON_IsNumber(y)) {
        debug_printf("❌ Widget toggle invalide : champs manquants\n");
        return false;
    }

    // Récupération du callback
    void (*callback_func)(bool) = NULL;
    if (cJSON_IsString(callback)) {
        callback_func = obtenir_callback_bool(callback->valuestring);
    }

    // Création du widget
    bool success = add_toggle_widget(
        list,
        id->valuestring,
        nom_affichage->valuestring,
        x->valueint,
        y->valueint,
        cJSON_IsBool(etat_depart) ? cJSON_IsTrue(etat_depart) : false,
        cJSON_IsNumber(largeur_toggle) ? largeur_toggle->valueint : 40,
        cJSON_IsNumber(hauteur_toggle) ? hauteur_toggle->valueint : 18,
        cJSON_IsNumber(taille_curseur) ? taille_curseur->valueint : 18,
        cJSON_IsNumber(taille_texte) ? taille_texte->valueint : 18,
        callback_func
    );

    if (success) {
        debug_printf("✅ Widget toggle '%s' chargé depuis JSON\n", id->valuestring);
    }

    return success;
}
// ════════════════════════════════════════════════════════════════════════════
//  PARSING D'UN TITRE (éléments statiques)
// ════════════════════════════════════════════════════════════════════════════
bool parser_titre(void* json_obj, LoaderContext* ctx, WidgetList* list) {
    if (!json_obj || !ctx || !list) return false;

    // Cast du void* vers cJSON*
    cJSON* obj = (cJSON*)json_obj;

    // Récupération des champs
    cJSON* texte = cJSON_GetObjectItem(obj, "texte");
    cJSON* x = cJSON_GetObjectItem(obj, "x");
    cJSON* y = cJSON_GetObjectItem(obj, "y");
    cJSON* taille = cJSON_GetObjectItem(obj, "taille_texte");
    cJSON* souligne = cJSON_GetObjectItem(obj, "souligne");
    cJSON* alignement = cJSON_GetObjectItem(obj, "alignment");

    // Validation minimale
    if (!cJSON_IsString(texte) || !cJSON_IsNumber(x) || !cJSON_IsNumber(y)) {
        debug_printf("❌ Titre invalide : champs manquants\n");
        return false;
    }

    // Paramètres
    int text_size = cJSON_IsNumber(taille) ? taille->valueint : 24;
    bool underlined = (cJSON_IsBool(souligne) && cJSON_IsTrue(souligne));
    SDL_Color color = {0, 0, 0, 255};  // Noir par défaut

    // Parser l'alignement
    LabelAlignment alignment = LABEL_ALIGN_CENTER;  // Par défaut centré
    if (cJSON_IsString(alignement)) {
        const char* align_str = alignement->valuestring;
        if (strcmp(align_str, "left") == 0) {
            alignment = LABEL_ALIGN_LEFT;
        } else if (strcmp(align_str, "right") == 0) {
            alignment = LABEL_ALIGN_RIGHT;
        } else if (strcmp(align_str, "center") == 0) {
            alignment = LABEL_ALIGN_CENTER;
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // CALCULER LA POSITION X FINALE SELON L'ALIGNEMENT
    // ═══════════════════════════════════════════════════════════════════════
    int x_final = x->valueint;  // Par défaut, utiliser la valeur du JSON

    if (alignment == LABEL_ALIGN_CENTER) {
        // Pour CENTER, calculer la position pour centrer le texte dans la largeur du panneau
        TTF_Font* font = ctx->font_titre;  // Police pour les titres
        if (font) {
            int text_width = 0;
            if (TTF_SizeUTF8(font, texte->valuestring, &text_width, NULL) == 0) {
                x_final = (ctx->panel_width - text_width) / 2;
                debug_printf("📐 LABEL CENTER '%s': largeur=%d, x_calculé=%d (panel_width=%d)\n",
                            texte->valuestring, text_width, x_final, ctx->panel_width);
            } else {
                debug_printf("⚠️ Impossible de mesurer '%s', x=%d par défaut\n",
                            texte->valuestring, x_final);
            }
        }
    } else if (alignment == LABEL_ALIGN_RIGHT) {
        // Pour RIGHT, calculer depuis le bord droit
        TTF_Font* font = ctx->font_titre;
        if (font) {
            int text_width = 0;
            if (TTF_SizeUTF8(font, texte->valuestring, &text_width, NULL) == 0) {
                x_final = ctx->panel_width - text_width - 20;  // 20px de marge
                debug_printf("📐 LABEL RIGHT '%s': largeur=%d, x_calculé=%d (panel_width=%d)\n",
                            texte->valuestring, text_width, x_final, ctx->panel_width);
            }
        }
    }
    // Pour LEFT, garder x_final = x->valueint (pas de calcul)

    // ═══ AJOUTER À LA WIDGET LIST ═══
    // Générer un id unique pour le titre
    char id[50];
    snprintf(id, sizeof(id), "titre_%d", list->count);

    bool success = add_label_widget(
        list,
        id,
        texte->valuestring,
        x_final,  // ← Utiliser x_final calculé au lieu de x->valueint
        y->valueint,
        text_size,
        color,
        underlined,
        alignment
    );

    if (success) {
        debug_printf("✅ Titre '%s' ajouté à la liste\n", texte->valuestring);
    }

    return success;
}

// ════════════════════════════════════════════════════════════════════════════
//  PARSING D'UN SÉPARATEUR (éléments statiques)
// ════════════════════════════════════════════════════════════════════════════
bool parser_separateur(void* json_obj, LoaderContext* ctx, WidgetList* list) {
    if (!json_obj || !ctx || !list) return false;

    // Cast du void* vers cJSON*
    cJSON* obj = (cJSON*)json_obj;

    // Récupération des champs
    cJSON* x = cJSON_GetObjectItem(obj, "x");
    cJSON* y = cJSON_GetObjectItem(obj, "y");
    cJSON* largeur = cJSON_GetObjectItem(obj, "largeur");
    cJSON* hauteur = cJSON_GetObjectItem(obj, "hauteur");
    cJSON* couleur = cJSON_GetObjectItem(obj, "couleur");

    // Validation minimale
    if (!cJSON_IsNumber(x) || !cJSON_IsNumber(y)) {
        debug_printf("❌ Séparateur invalide : coordonnées manquantes\n");
        return false;
    }

    // Paramètres
    int x_val = x->valueint;
    int y_val = y->valueint;
    int width = cJSON_IsNumber(largeur) ? largeur->valueint : 460;
    int thickness = cJSON_IsNumber(hauteur) ? hauteur->valueint : 1;

    // Couleur par défaut : gris clair
    SDL_Color color = {200, 200, 200, 255};
    if (cJSON_IsObject(couleur)) {
        cJSON* r = cJSON_GetObjectItem(couleur, "r");
        cJSON* g = cJSON_GetObjectItem(couleur, "g");
        cJSON* b = cJSON_GetObjectItem(couleur, "b");
        cJSON* a = cJSON_GetObjectItem(couleur, "a");
        if (cJSON_IsNumber(r)) color.r = r->valueint;
        if (cJSON_IsNumber(g)) color.g = g->valueint;
        if (cJSON_IsNumber(b)) color.b = b->valueint;
        if (cJSON_IsNumber(a)) color.a = a->valueint;
    }

    // ═══ CONVERTIR x,largeur EN marges ═══
    // add_separator_widget() attend start_margin et end_margin
    // Avec PANEL_WIDTH = 500 (largeur de référence du panneau)
    const int PANEL_WIDTH = 500;
    int start_margin = x_val;
    int end_margin = PANEL_WIDTH - (x_val + width);

    // Générer un id unique
    char id[50];
    snprintf(id, sizeof(id), "sep_%d", list->count);

    // ═══ AJOUTER À LA WIDGET LIST ═══
    bool success = add_separator_widget(
        list,
        id,
        y_val,
        start_margin,
        end_margin,
        thickness,
        color
    );

    if (success) {
        debug_printf("✅ Séparateur ajouté à la liste\n");
    }

    return success;
}

// ════════════════════════════════════════════════════════════════════════════
//  PARSING D'UN WIDGET PREVIEW
// ════════════════════════════════════════════════════════════════════════════
bool parser_widget_preview(cJSON* json_obj, LoaderContext* ctx, WidgetList* list) {
    if (!json_obj || !ctx || !list) return false;

    // Récupération des champs
    cJSON* id = cJSON_GetObjectItem(json_obj, "id");
    cJSON* x = cJSON_GetObjectItem(json_obj, "x");
    cJSON* y = cJSON_GetObjectItem(json_obj, "y");
    cJSON* frame_size = cJSON_GetObjectItem(json_obj, "frame_size");
    cJSON* size_ratio = cJSON_GetObjectItem(json_obj, "size_ratio");
    cJSON* breath_duration = cJSON_GetObjectItem(json_obj, "breath_duration");

    // Validation
    if (!cJSON_IsString(id) || !cJSON_IsNumber(x) || !cJSON_IsNumber(y)) {
        debug_printf("❌ Widget preview invalide : champs manquants\n");
        return false;
    }

    // Valeurs par défaut
    int size = cJSON_IsNumber(frame_size) ? frame_size->valueint : 100;
    float ratio = cJSON_IsNumber(size_ratio) ? (float)size_ratio->valuedouble : 0.90f;
    float duration = cJSON_IsNumber(breath_duration) ? (float)breath_duration->valuedouble : 3.0f;

    // Création du widget
    bool success = add_preview_widget(
        list,
        id->valuestring,
        x->valueint,
        y->valueint,
        size,
        ratio,
        duration
    );

    if (success) {
        debug_printf("✅ Widget preview '%s' chargé depuis JSON\n", id->valuestring);
    }

    return success;
}

// ════════════════════════════════════════════════════════════════════════════
//  PARSING D'UN WIDGET BUTTON
// ════════════════════════════════════════════════════════════════════════════
bool parser_widget_button(cJSON* json_obj, LoaderContext* ctx, WidgetList* list) {
    if (!json_obj || !ctx || !list) return false;

    // Récupération des champs
    cJSON* id = cJSON_GetObjectItem(json_obj, "id");
    cJSON* texte = cJSON_GetObjectItem(json_obj, "texte");
    cJSON* x = cJSON_GetObjectItem(json_obj, "x");
    cJSON* y = cJSON_GetObjectItem(json_obj, "y");
    cJSON* largeur = cJSON_GetObjectItem(json_obj, "largeur");
    cJSON* hauteur = cJSON_GetObjectItem(json_obj, "hauteur");
    cJSON* taille_texte = cJSON_GetObjectItem(json_obj, "taille_texte");
    cJSON* couleur = cJSON_GetObjectItem(json_obj, "couleur");
    cJSON* y_anchor_json = cJSON_GetObjectItem(json_obj, "y_anchor");
    cJSON* callback = cJSON_GetObjectItem(json_obj, "callback");

    // Validation
    if (!cJSON_IsString(id) || !cJSON_IsString(texte) ||
        !cJSON_IsNumber(x) || !cJSON_IsNumber(y) ||
        !cJSON_IsNumber(largeur) || !cJSON_IsNumber(hauteur)) {
        debug_printf("❌ Widget button invalide : champs manquants\n");
        return false;
    }

    // Taille de texte par défaut
    int text_size = cJSON_IsNumber(taille_texte) ? taille_texte->valueint : 16;

    // Couleur de fond par défaut : bleu
    SDL_Color bg_color = {70, 130, 180, 255};
    if (cJSON_IsObject(couleur)) {
        cJSON* r = cJSON_GetObjectItem(couleur, "r");
        cJSON* g = cJSON_GetObjectItem(couleur, "g");
        cJSON* b = cJSON_GetObjectItem(couleur, "b");
        cJSON* a = cJSON_GetObjectItem(couleur, "a");
        if (cJSON_IsNumber(r)) bg_color.r = r->valueint;
        if (cJSON_IsNumber(g)) bg_color.g = g->valueint;
        if (cJSON_IsNumber(b)) bg_color.b = b->valueint;
        if (cJSON_IsNumber(a)) bg_color.a = a->valueint;
    }

    // Ancrage Y (par défaut TOP)
    ButtonYAnchor y_anchor = BUTTON_ANCHOR_TOP;
    if (cJSON_IsString(y_anchor_json)) {
        if (strcmp(y_anchor_json->valuestring, "bottom") == 0) {
            y_anchor = BUTTON_ANCHOR_BOTTOM;
        }
    }

    // Récupération du callback
    void (*callback_func)(void) = NULL;
    if (cJSON_IsString(callback)) {
        callback_func = obtenir_callback_void(callback->valuestring);
    }

    // Création du widget
    bool success = add_button_widget(
        list,
        id->valuestring,
        texte->valuestring,
        x->valueint,
        y->valueint,
        largeur->valueint,
        hauteur->valueint,
        text_size,
        bg_color,
        y_anchor,
        callback_func
    );

    if (success) {
        debug_printf("✅ Widget button '%s' chargé depuis JSON\n", id->valuestring);
    }

    return success;
}

// ════════════════════════════════════════════════════════════════════════════
//  PARSING D'UN WIDGET SELECTOR
// ════════════════════════════════════════════════════════════════════════════
bool parser_widget_selector(cJSON* json_obj, LoaderContext* ctx, WidgetList* list) {
    (void)ctx;  // Paramètre non utilisé

    // Récupération des champs obligatoires
    cJSON* id = cJSON_GetObjectItem(json_obj, "id");
    cJSON* nom_affichage = cJSON_GetObjectItem(json_obj, "nom_affichage");
    cJSON* x = cJSON_GetObjectItem(json_obj, "x");
    cJSON* y = cJSON_GetObjectItem(json_obj, "y");
    cJSON* index_depart = cJSON_GetObjectItem(json_obj, "index_depart");
    cJSON* taille_texte = cJSON_GetObjectItem(json_obj, "taille_texte");
    cJSON* options_array = cJSON_GetObjectItem(json_obj, "options");

    // Validation des champs de base
    if (!cJSON_IsString(id) || !cJSON_IsString(nom_affichage) ||
        !cJSON_IsNumber(x) || !cJSON_IsNumber(y) ||
        !cJSON_IsArray(options_array)) {
        debug_printf("❌ Widget selector invalide : champs manquants\n");
    return false;
        }

        // Valeurs par défaut
        int default_index = cJSON_IsNumber(index_depart) ? index_depart->valueint : 0;
        int arrow_size = 6;  // Taille fixe des flèches
        int text_size = cJSON_IsNumber(taille_texte) ? taille_texte->valueint : 14;

        // ─────────────────────────────────────────────────────────────────────────
        // CRÉATION DU WIDGET SELECTOR
        // ─────────────────────────────────────────────────────────────────────────
        bool success = add_selector_widget(
            list,
            id->valuestring,
            nom_affichage->valuestring,
            x->valueint,
            y->valueint,
            default_index,
            arrow_size,
            text_size,
            ctx->font_normal  // ← Police pour le rendu du texte
        );

        if (!success) {
            debug_printf("❌ Échec création SelectorWidget '%s'\n", id->valuestring);
            return false;
        }

        // ─────────────────────────────────────────────────────────────────────────
        // RÉCUPÉRATION DU WIDGET POUR AJOUTER LES OPTIONS
        // ─────────────────────────────────────────────────────────────────────────
        WidgetNode* node = find_widget_by_id(list, id->valuestring);
        if (!node || !node->widget.selector_widget) {
            debug_printf("❌ Widget selector '%s' introuvable après création\n", id->valuestring);
            return false;
        }

        SelectorWidget* selector = node->widget.selector_widget;

        // ─────────────────────────────────────────────────────────────────────────
        // PARSING DU TABLEAU D'OPTIONS
        // ─────────────────────────────────────────────────────────────────────────
        int num_options = cJSON_GetArraySize(options_array);
        debug_printf("📋 Parsing de %d options pour selector '%s'\n", num_options, id->valuestring);

        for (int i = 0; i < num_options; i++) {
            cJSON* option = cJSON_GetArrayItem(options_array, i);
            if (!cJSON_IsObject(option)) {
                debug_printf("⚠️ Option %d invalide (pas un objet)\n", i);
                continue;
            }

            // Récupérer texte et callback de l'option
            cJSON* texte = cJSON_GetObjectItem(option, "texte");
            cJSON* callback_name = cJSON_GetObjectItem(option, "callback");

            if (!cJSON_IsString(texte) || !cJSON_IsString(callback_name)) {
                debug_printf("⚠️ Option %d: 'texte' ou 'callback' manquant\n", i);
                continue;
            }

            // Ajouter l'option au widget
            bool added = add_selector_option(selector, texte->valuestring, callback_name->valuestring);
            if (!added) {
                debug_printf("⚠️ Impossible d'ajouter option '%s'\n", texte->valuestring);
                continue;
            }

            // Récupérer le callback VOID associé
            void (*callback_func)(void) = obtenir_callback_void(callback_name->valuestring);
            if (callback_func) {
                set_selector_option_callback(selector, i, callback_func);
                debug_printf("✅ Option '%s' → callback '%s' défini\n",
                             texte->valuestring, callback_name->valuestring);
            } else {
                debug_printf("⚠️ Callback '%s' introuvable pour option '%s'\n",
                             callback_name->valuestring, texte->valuestring);
            }
        }

        // ─────────────────────────────────────────────────────────────────────────
        // APPELER LE CALLBACK DE L'OPTION PAR DÉFAUT
        // ─────────────────────────────────────────────────────────────────────────
        if (default_index >= 0 && default_index < selector->num_options) {
            if (selector->options[default_index].callback) {
                selector->options[default_index].callback();
                debug_printf("✅ Callback de l'option par défaut appelé: %s\n",
                             selector->options[default_index].callback_name);
            }
        }

        // ─────────────────────────────────────────────────────────────────────────
        // INITIALISATION DU LAYOUT (créer flèches et zones cliquables)
        // IMPORTANT: Doit être fait APRÈS l'ajout de toutes les options pour
        // calculer correctement la largeur maximale des choix
        // ─────────────────────────────────────────────────────────────────────────
        rescale_selector_widget(selector, 1.0f);
        debug_printf("✅ Widget selector '%s' chargé avec %d options (layout initialisé)\n",
                     id->valuestring, selector->num_options);

        return true;
}

// ════════════════════════════════════════════════════════════════════════════
//  FONCTION PRINCIPALE : CHARGER TOUS LES WIDGETS
// ════════════════════════════════════════════════════════════════════════════
bool charger_widgets_depuis_json(const char* filename,
                                 LoaderContext* context,
                                 WidgetList* widget_list) {
    if (!filename || !context || !widget_list) {
        debug_printf("❌ Paramètres invalides pour charger_widgets_depuis_json\n");
        return false;
    }

    debug_printf("📂 Chargement de la configuration depuis: %s\n", filename);

    // ─────────────────────────────────────────────────────────────────────────
    // 1. LECTURE DU FICHIER
    // ─────────────────────────────────────────────────────────────────────────
    FILE* file = fopen(filename, "r");
    if (!file) {
        debug_printf("❌ Impossible d'ouvrir le fichier: %s\n", filename);
        return false;
    }

    // Déterminer la taille du fichier
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allouer un buffer pour tout le contenu
    char* json_string = malloc(file_size + 1);
    if (!json_string) {
        debug_printf("❌ Erreur allocation mémoire pour JSON\n");
        fclose(file);
        return false;
    }

    // Lire le fichier
    fread(json_string, 1, file_size, file);
    json_string[file_size] = '\0';
    fclose(file);

    debug_printf("✅ Fichier JSON lu (%ld octets)\n", file_size);

    // ─────────────────────────────────────────────────────────────────────────
    // 2. PARSING DU JSON
    // ─────────────────────────────────────────────────────────────────────────
    cJSON* root = cJSON_Parse(json_string);
    free(json_string);

    if (!root) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr) {
            debug_printf("❌ Erreur parsing JSON avant: %s\n", error_ptr);
        }
        return false;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 3. RÉCUPÉRATION DU TABLEAU "widgets"
    // ─────────────────────────────────────────────────────────────────────────
    cJSON* widgets_array = cJSON_GetObjectItem(root, "widgets");
    if (!cJSON_IsArray(widgets_array)) {
        debug_printf("❌ Pas de tableau 'widgets' trouvé dans le JSON\n");
        cJSON_Delete(root);
        return false;
    }

    int nb_widgets = cJSON_GetArraySize(widgets_array);
    debug_printf("📋 Nombre d'éléments à charger: %d\n", nb_widgets);

    // ─────────────────────────────────────────────────────────────────────────
    // 4. BOUCLE SUR TOUS LES WIDGETS
    // ─────────────────────────────────────────────────────────────────────────
    int compteur_success = 0;

    for (int i = 0; i < nb_widgets; i++) {
        cJSON* widget = cJSON_GetArrayItem(widgets_array, i);
        if (!cJSON_IsObject(widget)) continue;

        // Récupérer le type
        cJSON* type = cJSON_GetObjectItem(widget, "type");
        if (!cJSON_IsString(type)) {
            debug_printf("⚠️ Widget %d sans type valide\n", i);
            continue;
        }

        const char* type_str = type->valuestring;
        debug_printf("🔍 Traitement widget %d de type '%s'\n", i, type_str);

        // ─────────────────────────────────────────────────────────────────────
        // DISPATCH selon le type
        // ─────────────────────────────────────────────────────────────────────
        bool success = false;

        if (strcmp(type_str, "increment") == 0) {
            success = parser_widget_increment(widget, context, widget_list);
        }
        else if (strcmp(type_str, "toggle") == 0) {
            success = parser_widget_toggle(widget, context, widget_list);
        }
        else if (strcmp(type_str, "titre") == 0) {
            success = parser_titre(widget, context, widget_list);
        }
        else if (strcmp(type_str, "separateur") == 0) {
            success = parser_separateur(widget, context, widget_list);
        }
        else if (strcmp(type_str, "preview") == 0) {
            success = parser_widget_preview(widget, context, widget_list);
        }
        else if (strcmp(type_str, "button") == 0) {
            success = parser_widget_button(widget, context, widget_list);
        }
        else if (strcmp(type_str, "selector") == 0) {
            success = parser_widget_selector(widget, context, widget_list);
        }
        else {
            debug_printf("⚠️ Type de widget inconnu: '%s'\n", type_str);
        }

        if (success) compteur_success++;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 5. NETTOYAGE ET RÉSULTAT
    // ─────────────────────────────────────────────────────────────────────────
    cJSON_Delete(root);

    debug_printf("✅ Chargement terminé : %d/%d éléments créés avec succès\n",
                 compteur_success, nb_widgets);

    return (compteur_success > 0);
}

// ════════════════════════════════════════════════════════════════════════════
//  GÉNÉRATION DES TEMPLATES
// ════════════════════════════════════════════════════════════════════════════

// Fonction helper : Remplace toutes les valeurs numériques par 0 et les strings par "a_modifier"
// (sauf le champ "type")
static void nettoyer_template_recursif(cJSON* item) {
    if (!item) return;

    cJSON* child = item->child;
    while (child) {
        // Si c'est un nombre, on le met à 0
        if (cJSON_IsNumber(child)) {
            cJSON_SetNumberValue(child, 0);
        }
        // Si c'est une string ET que ce n'est pas le champ "type"
        else if (cJSON_IsString(child) && strcmp(child->string, "type") != 0) {
            cJSON_SetValuestring(child, "a_modifier");
        }
        // Si c'est un objet ou un array, on descend récursivement
        else if (cJSON_IsObject(child) || cJSON_IsArray(child)) {
            nettoyer_template_recursif(child);
        }

        child = child->next;
    }
}

bool generer_templates_json(const char* config_file, const char* output_file) {
    debug_printf("🔧 Génération des templates depuis %s...\n", config_file);

    // ─────────────────────────────────────────────────────────────────────
    // 1. CHARGER LE FICHIER widgets_config.json
    // ─────────────────────────────────────────────────────────────────────
    FILE* file = fopen(config_file, "r");
    if (!file) {
        debug_printf("❌ Impossible d'ouvrir %s\n", config_file);
        return false;
    }

    // Lire tout le contenu du fichier
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* json_string = malloc(file_size + 1);
    if (!json_string) {
        fclose(file);
        debug_printf("❌ Erreur allocation mémoire\n");
        return false;
    }

    fread(json_string, 1, file_size, file);
    json_string[file_size] = '\0';
    fclose(file);

    // Parser le JSON
    cJSON* root = cJSON_Parse(json_string);
    free(json_string);

    if (!root) {
        debug_printf("❌ JSON invalide dans %s\n", config_file);
        return false;
    }

    // ─────────────────────────────────────────────────────────────────────
    // 2. EXTRAIRE LES TEMPLATES (sections "_template")
    // ─────────────────────────────────────────────────────────────────────
    cJSON* widgets_array = cJSON_GetObjectItem(root, "widgets");
    if (!cJSON_IsArray(widgets_array)) {
        debug_printf("❌ Pas de tableau 'widgets' trouvé\n");
        cJSON_Delete(root);
        return false;
    }

    // Créer le tableau qui contiendra les templates
    cJSON* templates_array = cJSON_CreateArray();

    // Parcourir tous les widgets pour trouver les sections "_template"
    cJSON* widget = NULL;
    cJSON_ArrayForEach(widget, widgets_array) {
        cJSON* template_obj = cJSON_GetObjectItem(widget, "_template");

        if (template_obj) {
            // Dupliquer le template
            cJSON* template_copy = cJSON_Duplicate(template_obj, 1);

            // Nettoyer les valeurs (0 pour les nombres, "a_modifier" pour les strings)
            nettoyer_template_recursif(template_copy);

            // Ajouter au tableau de templates
            cJSON_AddItemToArray(templates_array, template_copy);

            // Log pour debug
            cJSON* type_field = cJSON_GetObjectItem(template_copy, "type");
            if (type_field && cJSON_IsString(type_field)) {
                debug_printf("  ✓ Template '%s' extrait\n", type_field->valuestring);
            }
        }
    }

    cJSON_Delete(root);

    // ─────────────────────────────────────────────────────────────────────
    // 3. CRÉER LE FICHIER templates.json
    // ─────────────────────────────────────────────────────────────────────
    cJSON* output_root = cJSON_CreateObject();
    cJSON_AddStringToObject(output_root, "_commentaire",
                            "Fichier généré automatiquement - Templates de widgets");
    cJSON_AddStringToObject(output_root, "_note",
                            "Utilisez ces templates dans l'éditeur JSON. "
                            "Les valeurs 'a_modifier' doivent être personnalisées.");
    cJSON_AddItemToObject(output_root, "templates", templates_array);

    // Convertir en string avec indentation
    char* json_output = cJSON_Print(output_root);
    if (!json_output) {
        debug_printf("❌ Erreur lors de la conversion JSON\n");
        cJSON_Delete(output_root);
        return false;
    }

    // Écrire dans le fichier
    FILE* output = fopen(output_file, "w");
    if (!output) {
        debug_printf("❌ Impossible de créer %s\n", output_file);
        free(json_output);
        cJSON_Delete(output_root);
        return false;
    }

    fprintf(output, "%s", json_output);
    fclose(output);

    free(json_output);
    cJSON_Delete(output_root);

    debug_printf("✅ Templates générés avec succès dans %s\n", output_file);
    return true;
}
