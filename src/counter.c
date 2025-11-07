// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
#include <math.h>
#include "counter.h"
#include "debug.h"

// ════════════════════════════════════════════════════════════════════════
// CRÉATION DU COMPTEUR
// ════════════════════════════════════════════════════════════════════════
CounterState* counter_create(int total_breaths, float breath_duration,
                             const SinusoidalConfig* sin_config,
                             const char* font_path, int base_font_size) {
    // Allocation de la structure
    CounterState* counter = malloc(sizeof(CounterState));
    if (!counter) {
        fprintf(stderr, "❌ Erreur allocation CounterState\n");
        return NULL;
    }

    // Initialisation des valeurs
    counter->current_breath = 0;
    counter->total_breaths = total_breaths;
    counter->is_active = false;
    counter->is_finished = false;

    // Calcul en temps réel
    counter->breath_duration = breath_duration;
    counter->start_time = 0;
    counter->first_min_reached = false;
    counter->was_at_min = false;

    // Copier la configuration sinusoïdale
    counter->sin_config = *sin_config;

    // Couleur bleu-nuit cendré (même que le timer)
    counter->text_color.r = 70;
    counter->text_color.g = 85;
    counter->text_color.b = 110;
    counter->text_color.a = 255;

    // Sauvegarder le chemin de la police (on ouvrira/fermera à chaque frame)
    counter->font_path = font_path;
    counter->base_font_size = base_font_size;
    counter->font = NULL;  // Sera ouvert/fermé à chaque rendu

    debug_printf("✅ Compteur créé: %d respirations max, %.1fs/cycle, police %s taille %d\n",
                 total_breaths, breath_duration, font_path, base_font_size);

    return counter;
}

// ════════════════════════════════════════════════════════════════════════
// DÉMARRER LE COMPTEUR
// ════════════════════════════════════════════════════════════════════════
void counter_start(CounterState* counter) {
    if (!counter) return;

    counter->is_active = true;
    counter->is_finished = false;
    counter->current_breath = 0;        // Recommence à 0
    counter->start_time = SDL_GetTicks(); // 🆕 Enregistrer le temps de démarrage
    counter->first_min_reached = false; // 🆕 Attendre le premier scale_min
    counter->was_at_min = false;        // 🆕 Réinitialiser la détection

    debug_printf("🫁 Compteur démarré: 0/%d respirations (%.1fs/cycle)\n",
                 counter->total_breaths, counter->breath_duration);
}

// ════════════════════════════════════════════════════════════════════════
// MISE À JOUR DU COMPTEUR (calcul en temps réel)
// ════════════════════════════════════════════════════════════════════════
// Logique :
// 1. Calcule le temps écoulé depuis le démarrage
// 2. Calcule la progression dans le cycle actuel
// 3. Détecte les PASSAGES au scale_min (transitions)
// 4. Incrémente le compteur à chaque passage
// 5. Arrête après avoir complété le nombre de respirations voulu
// ════════════════════════════════════════════════════════════════════════
bool counter_update(CounterState* counter) {
    if (!counter || !counter->is_active || counter->is_finished) {
        return false;
    }

    // Calculer le temps écoulé en secondes
    Uint32 current_time = SDL_GetTicks();
    double elapsed_seconds = (current_time - counter->start_time) / 1000.0;

    // Calculer la progression dans le cycle actuel
    // progress = 0.0 → scale_max (départ)
    // progress = 0.5 → scale_min
    // progress = 1.0 → scale_max (fin du cycle)
    double cycles_completed = elapsed_seconds / counter->breath_duration;
    double progress_in_cycle = fmod(cycles_completed, 1.0);

    // 🆕 Détecter si on est actuellement au scale_min (zone autour de 0.5)
    // On utilise une fenêtre de 0.45 à 0.55 pour être sûr de capturer le passage
    bool at_min_now = (progress_in_cycle >= 0.45 && progress_in_cycle <= 0.55);

    // 🆕 Détecter la TRANSITION vers le scale_min (on n'y était pas avant, mais maintenant oui)
    if (at_min_now && !counter->was_at_min) {
        // On vient d'arriver au scale_min !

        if (!counter->first_min_reached) {
            // Premier passage au scale_min → démarrer l'affichage
            counter->first_min_reached = true;
            counter->current_breath = 1;
            debug_printf("🎯 Premier scale_min atteint - compteur affiché: 1/%d\n",
                       counter->total_breaths);
        } else {
            // Passages suivants → incrémenter le compteur
            counter->current_breath++;
            debug_printf("🫁 Respiration %d/%d (%.1fs écoulées)\n",
                       counter->current_breath, counter->total_breaths, elapsed_seconds);

            // 🆕 Vérifier si on a DÉPASSÉ le nombre total de respirations
            // On arrête APRÈS avoir complété toutes les respirations
            if (counter->current_breath > counter->total_breaths) {
                counter->is_finished = true;
                counter->is_active = false;
                debug_printf("✅ Compteur terminé: %d respirations complétées\n",
                           counter->total_breaths);
                return false;
            }
        }
    }

    // Sauvegarder l'état actuel pour la prochaine frame
    counter->was_at_min = at_min_now;

    return true;  // Compteur toujours actif
}

// ════════════════════════════════════════════════════════════════════════
// RENDU DU COMPTEUR AVEC EFFET FISH-EYE (SDL_TTF haute qualité)
// ════════════════════════════════════════════════════════════════════════
// Le chiffre "respire" avec l'hexagone : sa taille varie selon le scale
// Scale max (expire) = texte agrandi (poumon qui se vide)
// Scale min (inspire) = texte réduit (poumon qui se remplit)
//
// Génère une texture TTF à la taille exacte calculée pour chaque frame
// → Qualité optimale sans pixelisation
// ════════════════════════════════════════════════════════════════════════
void counter_render(CounterState* counter, SDL_Renderer* renderer,
                    int center_x, int center_y, int hex_radius, double current_scale) {
    if (!counter || !renderer) return;

    // Ne rien afficher si on n'a pas encore atteint le premier scale_min
    if (!counter->first_min_reached) return;

    // Ne rien afficher si le compteur n'a pas encore démarré
    if (counter->current_breath == 0) return;

    // Formater le texte (numéro du cycle actuel)
    char count_text[8];
    snprintf(count_text, sizeof(count_text), "%d", counter->current_breath);

    // 🎨 EFFET FISH-EYE : Calculer le scale en temps réel avec sinusoidal_movement
    Uint32 current_time = SDL_GetTicks();
    double elapsed_seconds = (current_time - counter->start_time) / 1000.0;

    // Utiliser la fonction générique pour calculer le scale actuel
    SinusoidalResult result;
    sinusoidal_movement(elapsed_seconds, &counter->sin_config, &result);

    // Calculer la taille de police nécessaire (scale dynamique)
    int font_size = (int)(counter->base_font_size * result.scale);
    if (font_size < 12) font_size = 12;  // Minimum lisible

    // Ouvrir la police à la taille calculée
    TTF_Font* render_font = TTF_OpenFont(counter->font_path, font_size);
    if (!render_font) {
        fprintf(stderr, "❌ Erreur chargement police taille %d: %s\n", font_size, TTF_GetError());
        return;
    }

    // Générer la surface avec le texte (rendu haute qualité)
    SDL_Surface* text_surface = TTF_RenderUTF8_Blended(
        render_font,
        count_text,
        counter->text_color
    );

    if (!text_surface) {
        fprintf(stderr, "❌ Erreur rendu texte: %s\n", TTF_GetError());
        TTF_CloseFont(render_font);
        return;
    }

    // Créer texture depuis la surface
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    if (!text_texture) {
        fprintf(stderr, "❌ Erreur création texture: %s\n", SDL_GetError());
        SDL_FreeSurface(text_surface);
        TTF_CloseFont(render_font);
        return;
    }

    // Calculer la position centrée
    int text_width = text_surface->w;
    int text_height = text_surface->h;
    int text_x = center_x - (text_width / 2);
    int text_y = center_y - (text_height / 2);

    // Rectangle de destination
    SDL_Rect dest_rect = {text_x, text_y, text_width, text_height};

    // Dessiner le texte
    SDL_RenderCopy(renderer, text_texture, NULL, &dest_rect);

    // Nettoyage
    SDL_DestroyTexture(text_texture);
    SDL_FreeSurface(text_surface);
    TTF_CloseFont(render_font);
}

// ════════════════════════════════════════════════════════════════════════
// RÉINITIALISER LE COMPTEUR
// ════════════════════════════════════════════════════════════════════════
void counter_reset(CounterState* counter) {
    if (!counter) return;

    counter->current_breath = 0;
    counter->is_active = false;
    counter->is_finished = false;
    counter->start_time = 0;           // 🆕 Reset du temps de démarrage
    counter->first_min_reached = false; // 🆕 Reset de la détection du premier scale_min
    counter->was_at_min = false;       // 🆕 Reset de la détection de transition

    debug_printf("🔄 Compteur réinitialisé\n");
}

// ════════════════════════════════════════════════════════════════════════
// LIBÉRATION MÉMOIRE
// ════════════════════════════════════════════════════════════════════════
void counter_destroy(CounterState* counter) {
    if (!counter) return;

    // Pas besoin de fermer counter->font car il est NULL (ouvert/fermé à chaque rendu)
    free(counter);
    debug_printf("🧹 Compteur détruit\n");
}
