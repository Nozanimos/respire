// SPDX-License-Identifier: GPL-3.0-or-later
// json_editor_window.h
#ifndef __JSON_EDITOR_H__
#define __JSON_EDITOR_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>

#define JSON_BUFFER_SIZE 8192    // Taille max du JSON (8 Ko)
#define EDITOR_WIDTH 600         // Largeur de la fenêtre
#define EDITOR_HEIGHT 800        // Hauteur de la fenêtre
#define LINE_HEIGHT 20           // Hauteur d'une ligne de texte
#define LEFT_MARGIN 50           // Marge pour les numéros de ligne

// Fonctions utilitaires mathématiques (inline pour performance)
static inline int min_int(int a, int b) { return a < b ? a : b; }
static inline int max_int(int a, int b) { return a > b ? a : b; }

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE POUR L'HISTORIQUE UNDO/REDO
// ════════════════════════════════════════════════════════════════════════════
typedef struct UndoNode {
    char* buffer_snapshot;          // Copie du buffer à cet état
    int curseur_position;           // Position du curseur
    int scroll_offset;              // Scroll vertical
    int scroll_offset_x;            // Scroll horizontal
    struct UndoNode* prev;          // État précédent
    struct UndoNode* next;          // État suivant (pour redo)
} UndoNode;

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
    char clipboard[JSON_BUFFER_SIZE];  // Presse-papier interne
    char filepath[256];             // Chemin du fichier JSON
    bool modified;                  // true si modifié depuis dernière sauvegarde
    bool json_valide;              // true si le JSON est bien formé

    // ─────────────────────────────────────────────────────────────────────────
    // CURSEUR ET NAVIGATION
    // ─────────────────────────────────────────────────────────────────────────
    int curseur_position;       // Position dans le buffer (en octets)
    int scroll_offset;          // Décalage vertical (pour scroller)
    int scroll_offset_x;        // Décalage Horizontal
    int nb_lignes;              // Nombre total de lignes

    // ─────────────────────────────────────────────────────────────────────────
    // SÉLECTION DE TEXTE
    // ─────────────────────────────────────────────────────────────────────────
    int selection_start;        // Début de la sélection (-1 si pas de sélection)
    int selection_end;          // Fin de la sélection
    bool selection_active;      // true si une sélection est en cours

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

    // ─────────────────────────────────────────────────────────────────────────
    // SYSTÈME UNDO/REDO
    // ─────────────────────────────────────────────────────────────────────────
    UndoNode* current_undo;             // Pointeur vers l'état actuel
    int undo_count;                     // Nombre d'états dans l'historique
    int max_undo_count;                 // Limite (ex: 100)

    // ─────────────────────────────────────────────────────────────────────────
    // AUTO-SAVE POUR HOT RELOAD
    // ─────────────────────────────────────────────────────────────────────────
    Uint32 last_modification_time;      // Timestamp de dernière modification
    float auto_save_delay;              // Délai avant auto-save (0.3 secondes)

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

// Supprime le caractère après le curseur (delete)
void supprimer_caractere_apres(JsonEditor* editor);

// Insère une nouvelle ligne
void inserer_nouvelle_ligne(JsonEditor* editor);

// Déplace le curseur (flèches clavier)
void deplacer_curseur(JsonEditor* editor, int dx, int dy);

// Copie de la sélection
void copier_selection(JsonEditor* editor);

// Coupe la sélection
void couper_selection(JsonEditor* editor);

// Colle la sélection
void coller_texte(JsonEditor* editor);

// Revenir en arrière
void faire_undo(JsonEditor* editor);

// Refaire
void faire_redo(JsonEditor* editor);

// Gestion de la sélection
void deselectionner(JsonEditor* editor);

// Gestion du undo/redo
void sauvegarder_etat_undo(JsonEditor* editor);

// Sélectionne le mot sous le curseur (double-clic)
void selectionner_mot_au_curseur(JsonEditor* editor);

// Sélectionne toute la ligne courante (triple-clic)
void selectionner_ligne_courante(JsonEditor* editor);

// Duplique la ligne courante (Ctrl+D)
void dupliquer_ligne_courante(JsonEditor* editor);

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

// Marque qu'une modification a eu lieu (pour auto-save)
void marquer_modification(JsonEditor* editor);

// Vérifie et sauvegarde automatiquement si nécessaire
void verifier_auto_save(JsonEditor* editor);

// Déplacer curseur avec les flèches
void deplacer_curseur_vertical(JsonEditor* editor, int direction);

// Scroll du curseur avec la souris
void auto_scroll_curseur(JsonEditor* editor);

// Compte le nombre de lignes dans le buffer
int compter_lignes(const char* buffer);

// Récupère la ligne à un index donné
const char* obtenir_ligne(const char* buffer, int index_ligne, char* dest, int max_len);

// Convertit position curseur → (ligne, colonne)
void position_vers_ligne_colonne(const char* buffer, int position, int* ligne, int* colonne);

// Trouve un nombre autour du curseur
bool trouver_nombre_au_curseur(JsonEditor* editor, int* debut, int* fin);

// Modifie un nombre au curseur (delta = +1 ou -1)
void modifier_nombre_au_curseur(JsonEditor* editor, int delta);

// ─────────────────────────────────────────────────────────────────────────
// UTILITAIRES UTF-8
// ─────────────────────────────────────────────────────────────────────────

// Retourne le nombre de caractères UTF-8 dans une chaîne
int utf8_strlen(const char* str);

// Avance de n caractères UTF-8, retourne le nombre d'octets avancés
int utf8_advance(const char* str, int n_chars);

// Copie n caractères UTF-8 (pas n octets!) dans dest
void utf8_strncpy(char* dest, const char* src, int n_chars, int max_bytes);

#endif
