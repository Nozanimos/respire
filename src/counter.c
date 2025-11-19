// SPDX-License-Identifier: GPL-3.0-or-later
// counter.c - VERSION OPTIMISÉE AVEC CACHE DE TEXTURES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "counter.h"
#include "counter_cache.h"
#include "debug.h"

// ════════════════════════════════════════════════════════════════════════
// CRÉATION DU COMPTEUR
// ════════════════════════════════════════════════════════════════════════
CounterState* counter_create(SDL_Renderer* renderer, int total_breaths, int retention_type,
                             const char* font_path, int base_font_size,
                             double scale_min, double scale_max,
                             int fps, float breath_duration) {
    if (!renderer) {
        fprintf(stderr, "❌ Renderer NULL dans counter_create\n");
        return NULL;
    }

    // Allocation de la structure
    CounterState* counter = malloc(sizeof(CounterState));
    if (!counter) {
        fprintf(stderr, "❌ Erreur allocation CounterState\n");
        return NULL;
    }

    // Initialisation des valeurs
    counter->total_breaths = total_breaths;
    counter->retention_type = retention_type;
    counter->is_active = false;

    // 🆕 Initialiser l'état du compteur
    counter->current_breath = 0;
    counter->was_at_min_last_frame = false;
    counter->waiting_for_scale_min = false;
    counter->was_at_max_last_frame = false;

    // Couleur bleu-nuit cendré (même que le timer)
    counter->text_color.r = 70;
    counter->text_color.g = 85;
    counter->text_color.b = 110;
    counter->text_color.a = 255;

    // Sauvegarder le chemin de la police
    counter->font_path = font_path;
    counter->base_font_size = base_font_size;

    // 🎨 CRÉER LE CACHE DE TEXTURES (précalcul complet avec Cairo + sinusoïdale)
    counter->cache = counter_cache_create(renderer, total_breaths, font_path,
                                          base_font_size, counter->text_color,
                                          scale_min, scale_max,
                                          fps, breath_duration);

    if (!counter->cache) {
        fprintf(stderr, "❌ Erreur création cache de textures\n");
        free(counter);
        return NULL;
    }

    const char* retention_name = (retention_type == 0) ? "poumons pleins" : "poumons vides";
    debug_printf("✅ Compteur créé: %d respirations max (%s), cache initialisé\n",
                 total_breaths, retention_name);

    return counter;
}



// ════════════════════════════════════════════════════════════════════════
// RENDU DU COMPTEUR AVEC CACHE DE TEXTURES (ULTRA-LIGHT)
// ════════════════════════════════════════════════════════════════════════
// Le chiffre "respire" avec l'hexagone : sa taille vient du cache précompilé
// Scale max (inspire) = texte agrandi (poumons pleins)
// Scale min (expire) = texte réduit (poumons vides)
//
// Logique de fin de session :
// 1. Compter normalement jusqu'à total_breaths (ex: 10)
// 2. Incrémenter à chaque scale_min (expire = poumons vides)
// 3. Après le 10ème scale_min, passer en mode "attente"
// 4. Continuer à afficher le dernier chiffre jusqu'à la position finale
// 5. Signaler que la session est terminée
// ════════════════════════════════════════════════════════════════════════

void counter_render(CounterState* counter, SDL_Renderer* renderer,
                    int center_x, int center_y, int hex_radius, HexagoneNode* hex_node,
                    float scale_factor) {
    if (!counter || !renderer || !hex_node) return;

    // Supprimer le warning de paramètre inutilisé
    (void)hex_radius;

    // Ne rien afficher si le compteur n'est pas actif
    if (!counter->is_active) return;

    // 🎯 RÉCUPÉRER LES DONNÉES PRÉCOMPUTÉES depuis l'hexagone
    if (!hex_node->precomputed_counter_frames) return;

    // Vérifier que current_cycle est dans les limites
    if (hex_node->current_cycle < 0 || hex_node->current_cycle >= hex_node->total_cycles) {
        return;
    }

    CounterFrame* current_frame = &hex_node->precomputed_counter_frames[hex_node->current_cycle];
    bool is_at_min_now = current_frame->is_at_scale_min;
    bool is_at_max_now = current_frame->is_at_scale_max;

    // Flag pour éviter de terminer dans le même frame où on atteint total_breaths
    bool just_completed = false;

    // 🚩 LOGIQUE DE COMPTAGE : TOUJOURS SUR SCALE_MIN
    // Le compteur COMPTE toujours sur scale_min (expire) : 1, 2, 3... 10
    // Seule la CONDITION DE FIN varie selon le type de rétention :
    //   - retention_type=0 (poumons pleins) : après le 10ème scale_min, attendre scale_MAX pour finir
    //   - retention_type=1 (poumons vides) : après le 10ème scale_min, faire le cycle complet puis attendre scale_MIN pour finir

    // 📊 COMPTAGE : toujours sur scale_min (expire)
    if (is_at_min_now && !counter->was_at_min_last_frame) {
        if (!counter->waiting_for_scale_min) {
            counter->current_breath++;
            debug_printf("🫁 Respiration %d/%d détectée à scale_min (expire) (frame %d)\n",
                         counter->current_breath, counter->total_breaths, hex_node->current_cycle);

            if (counter->current_breath >= counter->total_breaths) {
                counter->waiting_for_scale_min = true;
                just_completed = true;  // On vient juste d'atteindre le total, ne pas terminer maintenant
                const char* target = (counter->retention_type == 0) ? "scale_max" : "scale_min";
                debug_printf("✅ %d respirations complétées - attente du prochain %s...\n",
                             counter->total_breaths, target);
            }
        }
    }

    // 🎯 CONDITION DE FIN : varie selon le type de rétention
    // Ne pas terminer dans le même frame où on vient d'atteindre total_breaths (just_completed)
    if (counter->waiting_for_scale_min && !just_completed) {
        if (counter->retention_type == 0) {
            // POUMONS PLEINS : terminer au scale_max (inspire final)
            if (is_at_max_now && !counter->was_at_max_last_frame) {
                counter->is_active = false;
                debug_printf("🎯 Scale_max final atteint - session terminée (poumons pleins)\n");
            }
        } else {
            // POUMONS VIDES : terminer au PROCHAIN scale_min (expire final après cycle complet)
            if (is_at_min_now && !counter->was_at_min_last_frame) {
                counter->is_active = false;
                debug_printf("🎯 Scale_min final atteint - session terminée (poumons vides)\n");
            }
        }
    }

    // Sauvegarder les états pour la prochaine frame
    counter->was_at_min_last_frame = is_at_min_now;
    counter->was_at_max_last_frame = is_at_max_now;

    // Ne rien afficher si breath_number est 0 (pas encore démarré)
    if (counter->current_breath == 0) return;

    // ═══════════════════════════════════════════════════════════════════════════
    // 🚀 RENDU ULTRA-LIGHT AVEC CACHE COMPLET PAR FRAME
    // ═══════════════════════════════════════════════════════════════════════════
    // La texture est déjà rendue à la taille exacte pour cette frame
    // On fait juste un lookup direct et un blit avec scale_factor (responsive)

    int texture_width, texture_height;
    SDL_Texture* cached_texture = counter_cache_get(counter->cache,
                                                     counter->current_breath,
                                                     hex_node->current_cycle,
                                                     &texture_width,
                                                     &texture_height);

    if (!cached_texture) {
        // Fallback si texture manquante (ne devrait jamais arriver)
        return;
    }

    // 🎯 APPLIQUER UNIQUEMENT LE SCALE_FACTOR pour le responsive
    // Le breathing est déjà dans la texture (précalculée avec sinusoïdale)
    int scaled_width = (int)(texture_width * scale_factor);
    int scaled_height = (int)(texture_height * scale_factor);

    // Calculer la position centrée
    int text_x = center_x - (scaled_width / 2);
    int text_y = center_y - (scaled_height / 2);

    SDL_Rect dest_rect = {text_x, text_y, scaled_width, scaled_height};

    // 🎨 BLIT de la texture (instantané !)
    SDL_RenderCopy(renderer, cached_texture, NULL, &dest_rect);
}

// ════════════════════════════════════════════════════════════════════════
// LIBÉRATION MÉMOIRE
// ════════════════════════════════════════════════════════════════════════
void counter_destroy(CounterState* counter) {
    if (!counter) return;

    // Détruire le cache de textures
    if (counter->cache) {
        counter_cache_destroy(counter->cache);
    }

    free(counter);
    debug_printf("🧹 Compteur détruit\n");
}
