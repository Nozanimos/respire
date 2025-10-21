// json_config_loader.h
#ifndef __JSON_CONFIG_LOADER_H__
#define __JSON_CONFIG_LOADER_H__

#include <cjson/cJSON.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "widget_list.h"

// ════════════════════════════════════════════════════════════════════════════
//  TYPES D'ÉLÉMENTS DANS LE PANNEAU
// ════════════════════════════════════════════════════════════════════════════
// Ces types correspondent aux différents éléments qu'on peut placer dans
// le panneau de configuration (pas seulement des widgets interactifs)
typedef enum {
    ELEMENT_TYPE_TITRE,         // Titre statique (ex: "Configuration")
    ELEMENT_TYPE_SEPARATEUR,    // Ligne horizontale de séparation
    ELEMENT_TYPE_INCREMENT,     // Widget numérique avec flèches
    ELEMENT_TYPE_TOGGLE,        // Interrupteur ON/OFF
    ELEMENT_TYPE_TEXTE          // Texte informatif simple
} ElementType;

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE POUR UN ÉLÉMENT TITRE
// ════════════════════════════════════════════════════════════════════════════
typedef struct {
    char texte[100];
    int x, y;
    int taille_police;
    bool souligne;
} TitreConfig;

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE POUR UN SÉPARATEUR
// ════════════════════════════════════════════════════════════════════════════
typedef struct {
    int x, y;
    int largeur;
    int hauteur;
    SDL_Color couleur;
} SeparateurConfig;

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE POUR UN TEXTE SIMPLE
// ════════════════════════════════════════════════════════════════════════════
typedef struct {
    char texte[200];
    int x, y;
    int taille_police;
    SDL_Color couleur;
} TexteConfig;

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
bool parser_titre(void* json_obj, LoaderContext* ctx);

// Parse un objet JSON représentant un séparateur
bool parser_separateur(void* json_obj, LoaderContext* ctx);

// ─────────────────────────────────────────────────────────────────────────
// FONCTIONS DE RENDU POUR LES ÉLÉMENTS NON-WIDGETS
// ─────────────────────────────────────────────────────────────────────────
// Ces éléments (titres, séparateurs) ne sont pas dans widget_list car
// ils ne sont pas interactifs. On les rend directement.

// Rend un titre (sera appelé depuis settings_panel.c)
void rendre_titre(SDL_Renderer* renderer, TTF_Font* font,
                  const TitreConfig* config, int offset_x, int offset_y);

// Rend un séparateur (ligne horizontale)
void rendre_separateur(SDL_Renderer* renderer,
                       const SeparateurConfig* config, int offset_x, int offset_y);

#endif
