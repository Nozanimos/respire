// SPDX-License-Identifier: GPL-3.0-or-later
// widget_types.h (VERSION ENRICHIE)
#ifndef __WIDGET_TYPES_H__
#define __WIDGET_TYPES_H__

#include <SDL2/SDL.h>
#include <stdbool.h>

// ════════════════════════════════════════════════════════════════════════════
//  TYPES D'ÉLÉMENTS DANS LE PANNEAU
// ════════════════════════════════════════════════════════════════════════════
typedef enum {
    // ─── ÉLÉMENTS NON-INTERACTIFS ───
    WIDGET_TYPE_LABEL,          // Label / Titre statique (ex: "Configuration")
    WIDGET_TYPE_SEPARATOR,      // Ligne horizontale de séparation
    WIDGET_TYPE_PREVIEW,        // Zone d'animation de prévisualisation

    // ─── WIDGETS INTERACTIFS ───
    WIDGET_TYPE_INCREMENT,      // Widget numérique avec flèches ↑↓
    WIDGET_TYPE_TOGGLE,         // Interrupteur ON/OFF
    WIDGET_TYPE_SLIDER,         // Curseur horizontal (pour l'audio)
    WIDGET_TYPE_BUTTON,         // Bouton cliquable
    WIDGET_TYPE_SELECTOR        // Liste déroulante (pour plus tard)
} WidgetType;

// ════════════════════════════════════════════════════════════════════════════
//  CONFIGURATION GÉNÉRIQUE D'UN ÉLÉMENT
// ════════════════════════════════════════════════════════════════════════════
typedef struct {
    WidgetType type;
    char id[50];              // Identifiant unique (pour callbacks)
    char label[100];          // Texte affiché
    int x, y;                 // Position dans le panneau

    // ───────────────────────────────────────────────────────────────────────
    // CONFIGURATION SPÉCIFIQUE AU TYPE (union pour économiser mémoire)
    // ───────────────────────────────────────────────────────────────────────
    union {
        // ─── TITRE ───
        struct {
            int taille_police;
            bool souligne;
        } titre;

        // ─── SEPARATEUR ───
        struct {
            int largeur;
            int hauteur;
            SDL_Color couleur;
        } separateur;

        // ─── TEXTE ───
        struct {
            int taille_police;
            SDL_Color couleur;
        } texte;

        // ─── INCREMENT (flèches ↑↓) ───
        struct {
            int min_value;
            int max_value;
            int default_value;
            int increment;
            int arrow_size;
            int text_size;
        } increment;

        // ─── TOGGLE (ON/OFF) ───
        struct {
            bool default_state;
        } toggle;

        // ─── SLIDER (curseur) ───
        struct {
            int min_value;
            int max_value;
            int default_value;
            int track_width;      // Largeur de la piste
            int thumb_size;       // Taille du curseur
        } slider;

        // ─── BUTTON (bouton simple) ───
        struct {
            int width;
            int height;
            SDL_Color bg_color;
        } button;

        // ─── SELECTOR (liste déroulante) ───
        struct {
            char options[10][50]; // Max 10 options
            int num_options;
            int default_index;
        } selector;
    };

} WidgetConfig;

#endif
