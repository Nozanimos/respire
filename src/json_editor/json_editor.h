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
//  FORWARD DECLARATION
// ════════════════════════════════════════════════════════════════════════════
// Déclaration anticipée pour éviter les dépendances circulaires
typedef struct JsonEditor_s JsonEditor;

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE POUR LE MENU CONTEXTUEL
// ════════════════════════════════════════════════════════════════════════════
typedef struct {
    char* label;
    SDL_Rect rect;
    bool enabled;
    void (*action)(JsonEditor*);
} ContextMenuItem;

typedef struct {
    ContextMenuItem items[10];          // Items du menu
    int item_count;                     // Nombre d'items
    bool visible;                      // true si le menu est affiché
    int x, y;                         // Position d'affichage
    int width, height;                // Dimensions calculées automatiquement
    SDL_Color bg_color;               // Couleur de fond
    SDL_Color border_color;           // Couleur de la bordure
    SDL_Color text_enabled_color;     // Couleur du texte activé
    SDL_Color text_disabled_color;    // Couleur du texte désactivé
    SDL_Color hover_color;            // Couleur de survol
} ContextMenu;

// ════════════════════════════════════════════════════════════════════════════
//  STRUCTURE POUR UN BOUTON GÉNÉRIQUE
// ════════════════════════════════════════════════════════════════════════════
// Fonction callback : sera appelée quand on clique sur le bouton
typedef void (*BoutonCallback)(JsonEditor*);

typedef struct {
    char label[64];                 // Texte du bouton
    SDL_Rect rect;                  // Position et taille
    Uint32 couleur_normale;         // Couleur de base
    Uint32 couleur_survol;          // Couleur au survol
    bool survole;                   // true si la souris est dessus
    BoutonCallback callback;        // Fonction à appeler au clic
} EditorButton;

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
struct JsonEditor_s {
    // ─────────────────────────────────────────────────────────────────────────
    // MENU CONTEXTUEL (CLIC DROIT)
    // ─────────────────────────────────────────────────────────────────────────
    ContextMenu context_menu;          // Menu contextuel

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
    int hauteur_fenetre;        // Hauteur actuelle de la fenêtre (mise à jour dynamiquement)

    // ─────────────────────────────────────────────────────────────────────────
    // SÉLECTION DE TEXTE
    // ─────────────────────────────────────────────────────────────────────────
    int selection_start;        // Début de la sélection (-1 si pas de sélection)
    int selection_end;          // Fin de la sélection
    bool selection_active;      // true si une sélection est en cours

    // ─────────────────────────────────────────────────────────────────────────
    // SYSTÈME DE BOUTONS DYNAMIQUE
    // ─────────────────────────────────────────────────────────────────────────
    EditorButton* boutons;          // Tableau dynamique de boutons
    int nb_boutons;                 // Nombre de boutons
    int capacite_boutons;           // Capacité du tableau

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

};
typedef struct JsonEditor_s JsonEditor;

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
// SYSTÈME DE BOUTONS
// ─────────────────────────────────────────────────────────────────────────

// Crée un nouveau bouton avec ses paramètres
// PARAMÈTRES :
//   - label : texte affiché sur le bouton
//   - largeur, hauteur : dimensions du bouton
//   - couleur_normale : couleur de base (format RGBA : 0xRRGGBBAA)
//   - couleur_survol : couleur au survol de la souris
//   - callback : fonction appelée au clic (ex: callback_recharger)
// RETOUR : le bouton créé (position sera calculée par ajouter_bouton)
EditorButton creer_bouton(const char* label, int largeur, int hauteur,
                          Uint32 couleur_normale, Uint32 couleur_survol,
                          BoutonCallback callback);

// Ajoute un bouton à l'éditeur et calcule sa position automatiquement
// Les boutons sont placés côte à côte avec un espacement de 10px
void ajouter_bouton(JsonEditor* editor, EditorButton bouton);

// Recalcule les positions de tous les boutons (appelé au redimensionnement)
void recalculer_tous_boutons(JsonEditor* editor);

// Gère les clics sur tous les boutons
// RETOUR : true si un bouton a été cliqué
bool gerer_clic_boutons(JsonEditor* editor, int x, int y);

// Gère le survol de tous les boutons
void gerer_survol_boutons(JsonEditor* editor, int x, int y);

// Rend tous les boutons de l'éditeur
void rendre_tous_boutons(JsonEditor* editor);

// Libère la mémoire des boutons
void detruire_boutons(JsonEditor* editor);

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
// MENU CONTEXTUEL
// ─────────────────────────────────────────────────────────────────────────

// Initialise le menu contextuel
void initialiser_menu_contextuel(JsonEditor* editor);

// Affiche le menu contextuel à la position (x,y)
void afficher_menu_contextuel(JsonEditor* editor, int x, int y);

// Cache le menu contextuel
void cacher_menu_contextuel(JsonEditor* editor);

// Gère le clic dans le menu contextuel
// RETOUR : true si un item a été cliqué
bool gerer_clic_menu_contextuel(JsonEditor* editor, int x, int y);

// Gestion du survol pour la surbrillance
void gerer_survol_menu_contextuel(JsonEditor* editor);

// Met à jour la disponibilité des items du menu
void mettre_a_jour_menu_contextuel(JsonEditor* editor);

// Dessine le menu contextuel
void dessiner_menu_contextuel(JsonEditor* editor);

// Actions du menu contextuel (déjà implémentées, on les réutilise)
void action_copier_contextuel(JsonEditor* editor);
void action_couper_contextuel(JsonEditor* editor);
void action_coller_contextuel(JsonEditor* editor);
void action_tout_selectionner_contextuel(JsonEditor* editor);
void action_dupliquer_contextuel(JsonEditor* editor);

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

// Recalcule la position des boutons après un redimensionnement de fenêtre
void recalculer_positions_boutons(JsonEditor* editor);

// Calcule le nombre de lignes visibles selon la taille actuelle de la fenêtre
int obtenir_nb_lignes_visibles(JsonEditor* editor);

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
