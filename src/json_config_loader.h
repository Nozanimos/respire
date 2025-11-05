// SPDX-License-Identifier: GPL-3.0-or-later
// json_config_loader.h
#ifndef __JSON_CONFIG_LOADER_H__
#define __JSON_CONFIG_LOADER_H__

#include <cjson/cJSON.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "widget_types.h"
#include "widget_list.h"



// ════════════════════════════════════════════════════════════════════════════
//  CONTEXTE DE CHARGEMENT
// ════════════════════════════════════════════════════════════════════════════
// Cette structure contient tout ce dont on a besoin pour créer les widgets
// depuis le JSON (polices, renderer, etc.)
typedef struct {
    SDL_Renderer* renderer;
    TTF_Font* font_titre;
    TTF_Font* font_normal;
    TTF_Font* font_petit;

    // Table de correspondance nom_callback → pointeur fonction
    // (on verra ça plus tard pour simplifier)
} LoaderContext;

// ════════════════════════════════════════════════════════════════════════════
//  PROTOTYPES DES FONCTIONS
// ════════════════════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────────────────
// FONCTION PRINCIPALE
// ─────────────────────────────────────────────────────────────────────────
// Charge tous les widgets depuis un fichier JSON
//
// PARAMÈTRES :
//   - filename : Chemin du fichier JSON (ex: "../config/widgets.json")
//   - context : Contexte avec renderer et polices
//   - widget_list : Liste où ajouter les widgets
//
// RETOUR :
//   - true si succès
//   - false si erreur (fichier introuvable, JSON invalide, etc.)
bool charger_widgets_depuis_json(const char* filename,
                                 LoaderContext* context,
                                 WidgetList* widget_list);

// ─────────────────────────────────────────────────────────────────────────
// FONCTIONS HELPER (utilisées en interne)
// ─────────────────────────────────────────────────────────────────────────
// Parse un objet JSON représentant un widget increment
bool parser_widget_increment(cJSON* json_obj, LoaderContext* ctx, WidgetList* list);

// Parse un objet JSON représentant un widget toggle
bool parser_widget_toggle(cJSON* json_obj, LoaderContext* ctx, WidgetList* list);

// Parse un objet JSON représentant un titre
bool parser_titre(void* json_obj, LoaderContext* ctx, WidgetList* list);

// Parse un objet JSON représentant un séparateur
bool parser_separateur(void* json_obj, LoaderContext* ctx, WidgetList* list);

// Parse un objet JSON représentant le preview
bool parser_widget_preview(cJSON* json_obj, LoaderContext* ctx, WidgetList* list);

// Parse un objet JSON représentant un bouton
bool parser_widget_button(cJSON* json_obj, LoaderContext* ctx, WidgetList* list);

// ─────────────────────────────────────────────────────────────────────────
// GÉNÉRATION DES TEMPLATES
// ─────────────────────────────────────────────────────────────────────────
// Génère automatiquement le fichier templates.json depuis widgets_config.json
// Ce fichier contient des templates vierges pour chaque type de widget
//
// PARAMÈTRES :
//   - config_file : Chemin vers widgets_config.json
//   - output_file : Chemin vers templates.json à créer
//
// RETOUR :
//   - true si succès
//   - false si erreur
bool generer_templates_json(const char* config_file, const char* output_file);

#endif
