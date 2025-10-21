// json_editor_window.h
#ifndef __JSON_EDITOR_WINDOW_H__
#define __JSON_EDITOR_WINDOW_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

#define JSON_BUFFER_SIZE 8192    // Taille max du JSON (8 Ko)
#define EDITOR_WIDTH 600         // Largeur de la fenêtre
#define EDITOR_HEIGHT 800        // Hauteur de la fenêtre
#define LINE_HEIGHT 20           // Hauteur d'une ligne de texte
#define LEFT_MARGIN 50           // Marge pour les numéros de ligne

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE DE L'ÉDITEUR JSON
// ════════════════════════════════════════════════════════════════════════════
typedef struct {
    // ─────────────────────────────────────────────────────────────────────────
    // CONTEXTE SDL
    // ─────────────────────────────────────────────────────────────────────────
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font_mono;        // Police monospace pour le code
    TTF_Font* font_ui;          // Police pour les boutons

    // ─────────────────────────────────────────────────────────────────────────
    // CONTENU DU FICHIER
    // ─────────────────────────────────────────────────────────────────────────
    char buffer[JSON_BUFFER_SIZE];  // Contenu du JSON en mémoire
    char filepath[256];             // Chemin du fichier JSON
    bool modified;                  // true si modifié depuis dernière sauvegarde
    bool json_valide;              // true si le JSON est bien formé

    // ─────────────────────────────────────────────────────────────────────────
    // CURSEUR ET NAVIGATION
    // ─────────────────────────────────────────────────────────────────────────
    int curseur_position;       // Position dans le buffer (en octets)
    int scroll_offset;          // Décalage vertical (pour scroller)
    int nb_lignes;              // Nombre total de lignes

    // ─────────────────────────────────────────────────────────────────────────
    // BOUTONS
    // ─────────────────────────────────────────────────────────────────────────
    SDL_Rect bouton_recharger;
    SDL_Rect bouton_sauvegarder;
    bool bouton_recharger_survole;
    bool bouton_sauvegarder_survole;

    // ─────────────────────────────────────────────────────────────────────────
    // ÉTAT
    // ─────────────────────────────────────────────────────────────────────────
    bool est_ouvert;            // false si la fenêtre doit se fermer

} JsonEditor;

// ════════════════════════════════════════════════════════════════════════════
//  PROTOTYPES DES FONCTIONS
// ════════════════════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────────────────
// CYCLE DE VIE
// ─────────────────────────────────────────────────────────────────────────

// Crée et ouvre la fenêtre d'édition
// PARAMÈTRES :
//   - filepath : Chemin du fichier JSON à éditer
//   - pos_x, pos_y : Position de la fenêtre sur l'écran
JsonEditor* creer_json_editor(const char* filepath, int pos_x, int pos_y);

// Libère toutes les ressources de l'éditeur
void detruire_json_editor(JsonEditor* editor);

// ─────────────────────────────────────────────────────────────────────────
// FICHIER
// ─────────────────────────────────────────────────────────────────────────

// Charge le contenu du fichier dans le buffer
bool charger_fichier_json(JsonEditor* editor);

// Sauvegarde le buffer dans le fichier
bool sauvegarder_fichier_json(JsonEditor* editor);

// Valide la syntaxe JSON du buffer
bool valider_json(JsonEditor* editor);

// ─────────────────────────────────────────────────────────────────────────
// ÉDITION
// ─────────────────────────────────────────────────────────────────────────

// Insère un caractère à la position du curseur
void inserer_caractere(JsonEditor* editor, char c);

// Supprime le caractère avant le curseur (backspace)
void supprimer_caractere(JsonEditor* editor);

// Insère une nouvelle ligne
void inserer_nouvelle_ligne(JsonEditor* editor);

// Déplace le curseur (flèches clavier)
void deplacer_curseur(JsonEditor* editor, int dx, int dy);

// ─────────────────────────────────────────────────────────────────────────
// RENDU
// ─────────────────────────────────────────────────────────────────────────

// Rend toute la fenêtre de l'éditeur
void rendre_json_editor(JsonEditor* editor);

// Rend une ligne de texte avec numéro
void rendre_ligne_texte(JsonEditor* editor, const char* ligne, int numero, int y);

// Rend les boutons
void rendre_boutons(JsonEditor* editor);

// ─────────────────────────────────────────────────────────────────────────
// ÉVÉNEMENTS
// ─────────────────────────────────────────────────────────────────────────

// Gère les événements de la fenêtre d'édition
// RETOUR : true si l'événement concerne cette fenêtre
bool gerer_evenements_json_editor(JsonEditor* editor, SDL_Event* event);

// ─────────────────────────────────────────────────────────────────────────
// UTILITAIRES
// ─────────────────────────────────────────────────────────────────────────

// Compte le nombre de lignes dans le buffer
int compter_lignes(const char* buffer);

// Récupère la ligne à un index donné
const char* obtenir_ligne(const char* buffer, int index_ligne, char* dest, int max_len);

// Convertit position curseur → (ligne, colonne)
void position_vers_ligne_colonne(const char* buffer, int position, int* ligne, int* colonne);

#endif
