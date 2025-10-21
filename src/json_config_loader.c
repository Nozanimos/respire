// json_config_loader.c
#include "json_config_loader.h"
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
extern void alternate_cycles_changed(bool value);

// Fonction helper pour récupérer un callback INT par son nom
static void (*obtenir_callback_int(const char* nom))(int) {
    if (!nom) return NULL;

    if (strcmp(nom, "duration_value_changed") == 0) {
        return duration_value_changed;
    }
    if (strcmp(nom, "cycles_value_changed") == 0) {
        return cycles_value_changed;
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
        ctx->font_normal,
        callback_func
    );

    if (success) {
        debug_printf("✅ Widget toggle '%s' chargé depuis JSON\n", id->valuestring);
    }

    return success;
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
            // TODO: À implémenter (les titres seront gérés différemment)
            debug_printf("⚠️ Type 'titre' pas encore implémenté\n");
        }
        else if (strcmp(type_str, "separateur") == 0) {
            // TODO: À implémenter
            debug_printf("⚠️ Type 'separateur' pas encore implémenté\n");
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
//  RENDU D'UN TITRE
// ════════════════════════════════════════════════════════════════════════════
void rendre_titre(SDL_Renderer* renderer, TTF_Font* font,
                  const TitreConfig* config, int offset_x, int offset_y) {
    if (!renderer || !font || !config) return;

    if (config->souligne) {
        TTF_SetFontStyle(font, TTF_STYLE_UNDERLINE);
    }

    render_text(renderer, font, config->texte,
                config->x + offset_x, config->y + offset_y, 0xFF000000);

    if (config->souligne) {
        TTF_SetFontStyle(font, TTF_STYLE_NORMAL);
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  RENDU D'UN SÉPARATEUR
// ════════════════════════════════════════════════════════════════════════════
void rendre_separateur(SDL_Renderer* renderer,
                       const SeparateurConfig* config, int offset_x, int offset_y) {
    if (!renderer || !config) return;

    rectangleColor(renderer,
                   config->x + offset_x,
                   config->y + offset_y,
                   config->x + offset_x + config->largeur,
                   config->y + offset_y + config->hauteur,
                   (config->couleur.a << 24) | (config->couleur.r << 16) |
                   (config->couleur.g << 8) | config->couleur.b);
}
