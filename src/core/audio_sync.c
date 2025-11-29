// SPDX-License-Identifier: GPL-3.0-or-later
// audio_sync.c - Système de synchronisation audio avec préchargement
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "audio_sync.h"
#include "debug.h"
#include "core/memory/memory.h"

// CRÉATION DU GESTIONNAIRE AUDIO
AudioSyncManager* audio_sync_create(const char** audio_files, int num_files, float master_volume) {
    if (num_files <= 0) {
        debug_printf("⚠️  Audio sync: aucun fichier à précharger\n");
        return NULL;
    }

    // Allouer la structure
    AudioSyncManager* manager = SAFE_MALLOC(sizeof(AudioSyncManager));
    if (!manager) {
        fprintf(stderr, "❌ Erreur allocation AudioSyncManager\n");
        return NULL;
    }

    // Initialiser les champs
    manager->num_chunks = num_files;
    manager->num_sync_points = 0;
    manager->sync_points = NULL;
    manager->last_frame_played = -1;
    manager->is_enabled = true;
    manager->master_volume = master_volume;

    // Allouer le tableau de chunks
    manager->preloaded_chunks = SAFE_MALLOC(num_files * sizeof(Mix_Chunk*));
    if (!manager->preloaded_chunks) {
        fprintf(stderr, "❌ Erreur allocation tableau de chunks\n");
        SAFE_FREE(manager);
        return NULL;
    }

    // Précharger tous les fichiers audio en mémoire
    debug_printf("🔊 PRÉCHARGEMENT AUDIO : %d fichiers...\n", num_files);
    int loaded_count = 0;

    for (int i = 0; i < num_files; i++) {
        manager->preloaded_chunks[i] = Mix_LoadWAV(audio_files[i]);

        if (manager->preloaded_chunks[i]) {
            loaded_count++;
            debug_printf("   ✅ [%d] %s\n", i, audio_files[i]);
        } else {
            fprintf(stderr, "   ❌ [%d] Erreur chargement: %s (%s)\n",
                    i, audio_files[i], Mix_GetError());
            // Continuer quand même (chunk NULL sera ignoré)
        }
    }

    debug_printf("✅ Audio préchargé: %d/%d fichiers chargés\n", loaded_count, num_files);

    return manager;
}

// AJOUT D'UN ÉVÉNEMENT AUDIO
bool audio_sync_add_event(AudioSyncManager* manager,
                          int frame_index,
                          AudioEventType event_type,
                          int audio_id,
                          float volume,
                          int channel,
                          bool loop) {
    if (!manager || audio_id < 0 || audio_id >= manager->num_chunks) {
        return false;
    }

    // Réallouer le tableau d'événements
    AudioSyncPoint* new_points = realloc(manager->sync_points,
                                         (manager->num_sync_points + 1) * sizeof(AudioSyncPoint));
    if (!new_points) {
        fprintf(stderr, "❌ Erreur réallocation sync_points\n");
        return false;
    }

    manager->sync_points = new_points;

    // Ajouter le nouvel événement
    AudioSyncPoint* point = &manager->sync_points[manager->num_sync_points];
    point->frame_index = frame_index;
    point->event_type = event_type;
    point->audio_file = NULL;  // Utilise l'audio_id pour indexer preloaded_chunks
    point->volume = volume;
    point->channel = channel;
    point->loop = loop;

    manager->num_sync_points++;

    debug_printf("🎵 Événement audio ajouté: frame %d, type %d, audio_id %d\n",
                 frame_index, event_type, audio_id);

    return true;
}

// MISE À JOUR (APPELÉE CHAQUE FRAME)
void audio_sync_update(AudioSyncManager* manager, int current_frame) {
    if (!manager || !manager->is_enabled) return;

    // Éviter de jouer plusieurs fois le même son sur la même frame
    if (current_frame == manager->last_frame_played) return;

    // Parcourir tous les points de synchronisation
    for (int i = 0; i < manager->num_sync_points; i++) {
        AudioSyncPoint* point = &manager->sync_points[i];

        // Si cette frame correspond à un événement
        if (point->frame_index == current_frame) {
            // Récupérer le chunk préchargé
            // NOTE: Dans cette version préparatoire, on utilise event_type comme index
            //       Dans une version complète, il faudrait un mapping event_type → audio_id
            int audio_id = (int)point->event_type;
            if (audio_id < 0 || audio_id >= manager->num_chunks) continue;

            Mix_Chunk* chunk = manager->preloaded_chunks[audio_id];
            if (!chunk) continue;

            // Calculer le volume final
            int mix_volume = (int)(point->volume * manager->master_volume * MIX_MAX_VOLUME);
            Mix_VolumeChunk(chunk, mix_volume);

            // Jouer le son
            int loops = point->loop ? -1 : 0;
            int played_channel = Mix_PlayChannel(point->channel, chunk, loops);

            if (played_channel == -1) {
                fprintf(stderr, "❌ Erreur lecture audio: %s\n", Mix_GetError());
            } else {
                debug_printf("🔊 Audio joué: frame %d, canal %d\n", current_frame, played_channel);
            }
        }
    }

    manager->last_frame_played = current_frame;
}

// ACTIVER/DÉSACTIVER L'AUDIO
void audio_sync_set_enabled(AudioSyncManager* manager, bool enabled) {
    if (!manager) return;

    manager->is_enabled = enabled;
    debug_printf("🔊 Audio sync %s\n", enabled ? "activé" : "désactivé");

    if (!enabled) {
        // Arrêter tous les sons en cours
        Mix_HaltChannel(-1);
    }
}

// CHANGER LE VOLUME GLOBAL
void audio_sync_set_volume(AudioSyncManager* manager, float volume) {
    if (!manager) return;

    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;

    manager->master_volume = volume;
    debug_printf("🔊 Volume global: %.1f%%\n", volume * 100.0f);
}

// DESTRUCTION DU GESTIONNAIRE
void audio_sync_destroy(AudioSyncManager* manager) {
    if (!manager) return;

    // Arrêter tous les sons
    Mix_HaltChannel(-1);

    // Libérer tous les chunks
    if (manager->preloaded_chunks) {
        for (int i = 0; i < manager->num_chunks; i++) {
            if (manager->preloaded_chunks[i]) {
                Mix_FreeChunk(manager->preloaded_chunks[i]);
            }
        }
        SAFE_FREE(manager->preloaded_chunks);
    }

    // Libérer les sync points
    if (manager->sync_points) {
        SAFE_FREE(manager->sync_points);
    }

    // Libérer la structure
    SAFE_FREE(manager);

    debug_printf("🧹 Gestionnaire audio détruit\n");
}
