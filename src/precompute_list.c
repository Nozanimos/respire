// SPDX-License-Identifier: GPL-3.0-or-later
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include "precompute_list.h"
#include "config.h"
#include "debug.h"

#define NB_SIDE 6


/*------------------------------ Nouvelle Liste ------------------------------------*/

HexagoneList* new_hexagone_list(void){
    HexagoneList* list = malloc(sizeof(HexagoneList));
    if (!list) return NULL;

    list->first = NULL;
    list->last = NULL;
    list->count = 0;
    return list;
}

/*-------------------------- Contrôle si liste vide --------------------------------*/

bool is_empty_hexagone_list(HexagoneList *li){
    return (li == NULL || li->first == NULL);
}

/*---------------------------- Compteur de listes ----------------------------------*/

int hexagone_list_count(HexagoneList *li){
    if (is_empty_hexagone_list(li)) return 0;
    return li->count;
}

/*-------------------------- Ajout hexagone à la liste ------------------------------*/

void add_hexagone(HexagoneList* list, Hexagon* hex, Animation* anim) {
    if (!list || !hex) return;

    HexagoneNode* new_node = malloc(sizeof(HexagoneNode));
    if (!new_node) return;

    new_node->data = hex;
    new_node->animation = anim;

    // Initialisation des champs pré-calcul
    new_node->precomputed_vx = NULL;
    new_node->precomputed_vy = NULL;
    new_node->total_cycles = 0;
    new_node->current_cycle = 0;
    new_node->is_frozen = false;  // 🆕 Animation active par défaut

    new_node->next = NULL;
    new_node->prev = list->last;

    if (list->last) {
        list->last->next = new_node;
    } else {
        list->first = new_node;
    }
    list->last = new_node;
    list->count++;
}

/*------------------------------- Match coordonnées -----------------------------------*/

bool are_coordinates_identical(Sint16* vx1, Sint16* vy1, Sint16* vx2, Sint16* vy2) {
    for (int i = 0; i < NB_SIDE; i++) {
        if (vx1[i] != vx2[i] || vy1[i] != vy2[i]) {
            return false;
        }
    }
    return true;
}

/*------------------------------- Mouvement Sinusoïdal -----------------------------------*/

void sinusoidal_movement(double frame_time, const SinusoidalConfig* config, SinusoidalResult* result) {
    double cycles_completed = frame_time / config->breath_duration;
    double progress_in_cycle = fmod(cycles_completed, 1.0);

    // SCALE
    double scale_progress = cos(progress_in_cycle * 2 * M_PI);
    result->scale = config->scale_min + (config->scale_max - config->scale_min) * (scale_progress + 1.0) / 2.0;

    // ROTATION
    double sinusoidal_progress;
    if (progress_in_cycle < 0.5) {
        sinusoidal_progress = 4.0 * progress_in_cycle * progress_in_cycle * progress_in_cycle;
    } else {
        double temp = 2.0 * progress_in_cycle - 2.0;
        sinusoidal_progress = 0.5 * temp * temp * temp + 1.0;
    }

    double base_rotation = config->angle_per_cycle * floor(cycles_completed);
    double current_cycle_rotation = config->angle_per_cycle * sinusoidal_progress;

    result->rotation = base_rotation + current_cycle_rotation;

    if (!config->clockwise) {
        result->rotation = -result->rotation;
    }
}

/*------------------------ Calcul de l'ensemble des cycles -----------------------------*/

void precompute_all_cycles(HexagoneList* list, int fps, float breath_duration) {
    if (!list) return;

    // CALCUL UNIQUE avant la boucle
    int cycles_for_alignment = calculate_alignment_cycles();
    int total_frames = (int)(cycles_for_alignment * fps * breath_duration);

    HexagoneNode* node = list->first;
    while (node) {
        if (node->data && node->animation) {
            // ✅ NOUVEAU : Sauvegarde des points RELATIFS de base
            Sint16 base_vx[NB_SIDE], base_vy[NB_SIDE];
            for (int i = 0; i < NB_SIDE; i++) {
                base_vx[i] = node->data->vx[i];  // Points relatifs
                base_vy[i] = node->data->vy[i];  // Points relatifs
            }

            // Sauvegarde du centre original
            int original_center_x = node->data->center_x;
            int original_center_y = node->data->center_y;

            // Allocation contiguë pour les transformations
            node->precomputed_vx = malloc(total_frames * NB_SIDE * sizeof(Sint16));
            node->precomputed_vy = malloc(total_frames * NB_SIDE * sizeof(Sint16));

            // 🆕 Allocation pour les scales (utilisés par le compteur de respirations)
            node->precomputed_scales = malloc(total_frames * sizeof(double));
            // 🆕 Allocation pour les frames du compteur (synchronisées avec l'hexagone)
            node->precomputed_counter_frames = malloc(total_frames * sizeof(CounterFrame));

            node->total_cycles = total_frames;
            node->current_cycle = 0;
            node->current_scale = 1.0;  // 🆕 Initialiser le scale actuel

            if (!node->precomputed_vx || !node->precomputed_vy ||
                !node->precomputed_scales || !node->precomputed_counter_frames) {
                fprintf(stderr, "Erreur d'allocation pour hexagone %d\n", node->data->element_id);
            free(node->precomputed_vx);
            free(node->precomputed_vy);
            free(node->precomputed_scales);
            free(node->precomputed_counter_frames);  // 🆕 Libérer aussi les frames du compteur
            continue;
                }

            // UTILISATION DE LA STRUCTURE GÉNÉRIQUE
            SinusoidalConfig config = {
                .angle_per_cycle = node->animation->angle_per_cycle,
                .scale_min = node->animation->scale_min,
                .scale_max = node->animation->scale_max,
                .clockwise = node->animation->clockwise,
                .breath_duration = breath_duration
            };

            // ✅ MODIFICATION : Pré-calcul en utilisant les points RELATIFS
            for (int frame = 0; frame < total_frames; frame++) {
                double time_in_seconds = (double)frame / fps;
                SinusoidalResult result;

                sinusoidal_movement(time_in_seconds, &config, &result);

                // 🆕 Stocker le scale précalculé pour cette frame (utilisé par le compteur)
                node->precomputed_scales[frame] = result.scale;

                double angle_rad = result.rotation * M_PI / 180.0;

                for (int point = 0; point < NB_SIDE; point++) {
                    int index = frame * NB_SIDE + point;

                    // ✅ UTILISER les points RELATIFS de base
                    double dx = base_vx[point];
                    double dy = base_vy[point];

                    // Appliquer rotation
                    double rotated_dx = dx * cos(angle_rad) - dy * sin(angle_rad);
                    double rotated_dy = dx * sin(angle_rad) + dy * cos(angle_rad);

                    // Appliquer scale
                    rotated_dx *= result.scale;
                    rotated_dy *= result.scale;

                    // ✅ STOCKER les points TRANSFORMÉS (mais toujours relatifs au centre)
                    node->precomputed_vx[index] = (Sint16)rotated_dx;
                    node->precomputed_vy[index] = (Sint16)rotated_dy;
                }
            } // Fin de la boucle for (int frame...)


            // ✅ RESTAURER le centre original (au cas où)
            node->data->center_x = original_center_x;
            node->data->center_y = original_center_y;

            debug_printf("✅ Hexagone %d: %d frames pré-calculées (système relatif)\n",
                   node->data->element_id, total_frames);
        }
        node = node->next;
    }
}

/*------------------------- Copie liste de points -----------------------------*/

//  Appliquer la frame actuelle depuis le pré-calcul
void apply_precomputed_frame(HexagoneNode* node) {
    if (!node || !node->precomputed_vx || !node->precomputed_vy) return;

    // 🆕 Si l'animation est figée, ne rien faire
    if (node->is_frozen) return;

    // Appliquer les points transformés AU CENTRE ACTUEL
    for (int i = 0; i < NB_SIDE; i++) {
        int index = node->current_cycle * NB_SIDE + i;

        // Les points précalculés sont relatifs, on les combine avec le centre actuel
        node->data->vx[i] = node->precomputed_vx[index];  // Points transformés relatifs
        node->data->vy[i] = node->precomputed_vy[index];  // Points transformés relatifs
    }

    // 🆕 Mettre à jour le scale actuel (utilisé par le compteur pour l'effet fish-eye)
    node->current_scale = node->precomputed_scales[node->current_cycle];

    /* NOTE : Le centre (center_x, center_y) reste inchangé pendant l'animation
     *  L'échelle (current_scale) est maintenant mise à jour et accessible au compteur */

    node->current_cycle++;
    if (node->current_cycle >= node->total_cycles) {
        node->current_cycle = 0;
    }
}

/*-------------------- Trouve le PPCM des angles de la liste ----------------------*/

int calculate_alignment_cycles(void) {
    double angles[] = {ANGLE_1, ANGLE_2, ANGLE_3, ANGLE_4};
    int nb_angles = 4;

    double ppcm_float = 1.0;

    for (int i = 0; i < nb_angles; i++) {
        double cycles_for_alignment = 60.0 / angles[i];
        ppcm_float = (ppcm_float * cycles_for_alignment) / gcd_fractional(ppcm_float, cycles_for_alignment);
    }

    int ppcm_result = (int)ceil(ppcm_float);

    debug_printf("🔢 PPCM: %.1f°, %.1f°, %.1f°, %.1f° -> %d cycles\n",
           ANGLE_1, ANGLE_2, ANGLE_3, ANGLE_4, ppcm_result);

    return ppcm_result;
}

/*------------------------------ Nettoyage ----------------------------------------*/


void free_hexagone_list(HexagoneList* list) {
    if (!list) return;

    HexagoneNode* current = list->first;
    while (current) {
        HexagoneNode* next = current->next;

        // Libération SIMPLIFIÉE - maintenant 3 free() pour inclure les scales
        free(current->precomputed_vx);
        free(current->precomputed_vy);
        free(current->precomputed_scales);  // 🆕 Libérer les scales précalculés
        // 🆕 Libérer les frames du compteur précomputé
        free(current->precomputed_counter_frames);

        if (current->animation) {
            free_animation(current->animation);
        }
        free_hexagon(current->data);
        free(current);
        current = next;
    }
    free(list);
}

/*---------------------------- Print de débogage ------------------------------------------*/

void debug_print_list_order(HexagoneList* list) {
    debug_printf("🔍 ORDRE RÉEL de la liste : ");
    HexagoneNode* node = list->first;
    while (node) {
        debug_printf("%d(", node->data->element_id);
        debug_printf("%s) ", node->animation->clockwise ? "H" : "A");
        node = node->next;
    }
    debug_printf("\n");
}

/*---------------------------- Print de débogage ------------------------------------------*/

//  Calculer et afficher les frames nécessaires pour la rotation
void print_rotation_frame_requirements(HexagoneList* list, int fps, float breath_duration) {
    if (!list) return;

    debug_printf("\n=== CALCUL DES FRAMES NÉCESSAIRES POUR LA ROTATION ===\n");

    HexagoneNode* node = list->first;
    while (node) {
        if (node->data && node->animation) {
            // Frames disponibles basées sur la respiration
            int available_frames = fps * breath_duration;

            // Calcul de la rotation par frame avec les frames disponibles
            double rotation_per_frame_available = node->animation->angle_per_cycle / available_frames;

            // Frames idéales pour une rotation fluide (1° par frame)
            double ideal_rotation_per_frame = 1.0;
            int required_frames_for_smooth = (int)(node->animation->angle_per_cycle / ideal_rotation_per_frame);

            // Frames pour éviter les saccades (0.5° par frame)
            double min_rotation_per_frame = 0.5;
            int required_frames_for_no_jitter = (int)(node->animation->angle_per_cycle / min_rotation_per_frame);

            debug_printf("Hexagone %d:\n", node->data->element_id);
            debug_printf("  Angle par cycle: %.1f°\n", node->animation->angle_per_cycle);
            debug_printf("  Frames disponibles: %d\n", available_frames);
            debug_printf("  Rotation par frame: %.3f°\n", rotation_per_frame_available);
            debug_printf("  Frames nécessaires (1°/frame): %d\n", required_frames_for_smooth);
            debug_printf("  Frames nécessaires (0.5°/frame): %d\n", required_frames_for_no_jitter);
            debug_printf("  Ratio disponible/nécessaire: %.2f\n", (double)available_frames / required_frames_for_no_jitter);

            if (available_frames < required_frames_for_no_jitter) {
                debug_printf("  ⚠️  ATTENTION: Frames insuffisantes pour rotation fluide!\n");
            } else {
                debug_printf("  ✅ Frames suffisantes\n");
            }
            debug_printf("\n");

        }
        node = node->next;
    }
}

/*----------------------------- Calcul PGCD ---------------------------------------*/

int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

/*----------------------------- Calcul PPCM ---------------------------------------*/

int lcm(int a, int b) {
    return (a * b) / gcd(a, b);
}

/*------------------------------ PGCD pour flottants -----------------------------------*/

double gcd_fractional(double a, double b) {
    while (fabs(b) > 1e-10) {
        double temp = b;
        b = fmod(a, b);
        a = temp;
    }
    return a;
}

/*──────────────────────────────────────────────────────────────────────────────
  PRÉCOMPUTING DES FRAMES DU COMPTEUR DE RESPIRATIONS
──────────────────────────────────────────────────────────────────────────────*/
// Cette fonction calcule pour chaque frame :
// 1. Le flag is_at_scale_min (true si on est à l'inspire)
// 2. Le scale du texte (synchronisé avec le scale de l'hexagone)
//
// Le compteur lui-même (incrémentation) se fait en temps réel dans counter_render
// en détectant les TRANSITIONS du flag (false → true)
//
// Paramètres :
// - node : Le nœud hexagone contenant les données précomputées
// - total_frames : Nombre total de frames précalculées
// - fps : Images par seconde
// - breath_duration : Durée d'un cycle complet (inspire + expire)
// - max_breaths : Nombre maximum de respirations (non utilisé ici, juste pour info)
void precompute_counter_frames(HexagoneNode* node, int total_frames, int fps,
                               float breath_duration, int max_breaths) {
    if (!node || !node->precomputed_counter_frames || !node->precomputed_scales) {
        return;
    }

    // Parcourir toutes les frames précalculées
    for (int frame = 0; frame < total_frames; frame++) {
        double current_scale = node->precomputed_scales[frame];

        // Calculer la progression dans le cycle
        double time_in_seconds = (double)frame / fps;
        double cycles_completed = time_in_seconds / breath_duration;
        double progress_in_cycle = fmod(cycles_completed, 1.0);

        // 🚩 DÉTECTER les deux positions clés du cycle de respiration :
        //
        // Timeline du cycle :
        // progress = 0.0-0.05  → scale_max (expire, position poumons_vides, position de départ)
        // progress = 0.45-0.55 → scale_min (inspire, poumons pleins)
        // progress = 0.95-1.0  → scale_max (inspire - position poumons pleins)
        //
        // Ces flags serviront pour :
        // - Incrémenter le compteur (à chaque passage au scale_min)
        // - Figer l'animation en position de repos (scale_max après la dernière respiration)
        // - Synchroniser l'audio plus tard (sons inspire/expire)

        bool is_at_min = (progress_in_cycle >= 0.45 && progress_in_cycle <= 0.55);
        bool is_at_max = (progress_in_cycle <= 0.05 || progress_in_cycle >= 0.95);

        // Enregistrer les drapeaux et le scale pour cette frame
        node->precomputed_counter_frames[frame].is_at_scale_min = is_at_min;
        node->precomputed_counter_frames[frame].is_at_scale_max = is_at_max;
        node->precomputed_counter_frames[frame].text_scale = current_scale;
    }

    debug_printf("✅ Compteur précompute : %d frames, flags scale_min générés\n",
                 total_frames);

    // Note : max_breaths est conservé pour info mais non utilisé dans le précomputing
    // Le compteur s'arrêtera quand current_breath atteindra max_breaths (logique dans counter_render)
    (void)max_breaths;  // Supprimer le warning de paramètre inutilisé
}


/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------------*/


