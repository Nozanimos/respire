// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef __AUDIO_SYNC_H__
#define __AUDIO_SYNC_H__

#include <stdbool.h>
#include <SDL2/SDL_mixer.h>

// SYSTÈME DE SYNCHRONISATION AUDIO AVEC PRÉCHARGEMENT
// Cette structure prépare l'infrastructure pour synchroniser l'audio
// avec les animations précomputées.
//
// Fonctionnement :
// 1. Au démarrage : Précharger tous les chunks audio en mémoire
// 2. À chaque frame : Vérifier si un événement audio doit être déclenché
// 3. Si oui : Jouer le chunk instantanément (déjà en RAM)
//
// Avantages :
// - Latence zéro (audio déjà en mémoire)
// - Synchronisation parfaite avec les frames
// - Prévisibilité (tous les sons connus à l'avance)

// TYPES D'ÉVÉNEMENTS AUDIO
typedef enum {
    AUDIO_EVENT_INHALE_START,    // Début inspire (scale_max → scale_min)
    AUDIO_EVENT_INHALE_END,      // Fin inspire (atteint scale_min)
    AUDIO_EVENT_EXHALE_START,    // Début expire (scale_min → scale_max)
    AUDIO_EVENT_EXHALE_END,      // Fin expire (atteint scale_max)
    AUDIO_EVENT_BREATH_COUNT,    // Son de comptage (1, 2, 3...)
    AUDIO_EVENT_SESSION_START,   // Début de session
    AUDIO_EVENT_SESSION_END,     // Fin de session
    AUDIO_EVENT_CUSTOM           // Événement personnalisé
} AudioEventType;

// POINT DE SYNCHRONISATION AUDIO
// Représente un événement audio déclenché à une frame précise
typedef struct {
    int frame_index;             // À quelle frame déclencher (0 à total_cycles-1)
    AudioEventType event_type;   // Type d'événement
    const char* audio_file;      // Chemin vers le fichier audio (si CUSTOM)
    float volume;                // Volume relatif (0.0 à 1.0)
    int channel;                 // Canal SDL_mixer (-1 = auto)
    bool loop;                   // true = boucle infinie
} AudioSyncPoint;

// GESTIONNAIRE DE SYNCHRONISATION AUDIO
typedef struct {
    // Chunks audio préchargés en mémoire
    Mix_Chunk** preloaded_chunks;  // Tableau de chunks (indexé par audio_id)
    int num_chunks;                // Nombre de chunks préchargés

    // Points de synchronisation (frame → audio)
    AudioSyncPoint* sync_points;   // Tableau d'événements
    int num_sync_points;           // Nombre d'événements

    // État de lecture
    int last_frame_played;         // Dernière frame où un son a été joué
    bool is_enabled;               // Audio actif/inactif

    // Volume global
    float master_volume;           // Volume global (0.0 à 1.0)
} AudioSyncManager;

// PROTOTYPES

/**
 * Créer et initialiser le gestionnaire de synchronisation audio
 * Précharge tous les chunks audio en mémoire
 *
 * @param audio_files Tableau de chemins vers les fichiers audio à précharger
 * @param num_files Nombre de fichiers à précharger
 * @param master_volume Volume global (0.0 à 1.0)
 * @return Pointeur vers le gestionnaire créé, NULL si erreur
 */
AudioSyncManager* audio_sync_create(const char** audio_files, int num_files, float master_volume);

/**
 * Ajouter un point de synchronisation audio
 * Définit qu'un son doit être joué à une frame précise
 *
 * @param manager Pointeur vers le gestionnaire
 * @param frame_index Frame où déclencher l'audio
 * @param event_type Type d'événement
 * @param audio_id ID du chunk préchargé (index dans preloaded_chunks)
 * @param volume Volume relatif (0.0 à 1.0)
 * @param channel Canal SDL_mixer (-1 = auto)
 * @param loop true pour boucler le son
 * @return true si succès, false si erreur
 */
bool audio_sync_add_event(AudioSyncManager* manager,
                          int frame_index,
                          AudioEventType event_type,
                          int audio_id,
                          float volume,
                          int channel,
                          bool loop);

/**
 * Mettre à jour le gestionnaire audio (à appeler chaque frame)
 * Déclenche les événements audio si la frame correspond
 *
 * @param manager Pointeur vers le gestionnaire
 * @param current_frame Frame actuelle de l'animation
 */
void audio_sync_update(AudioSyncManager* manager, int current_frame);

/**
 * Activer/désactiver l'audio
 *
 * @param manager Pointeur vers le gestionnaire
 * @param enabled true pour activer, false pour désactiver
 */
void audio_sync_set_enabled(AudioSyncManager* manager, bool enabled);

/**
 * Changer le volume global
 *
 * @param manager Pointeur vers le gestionnaire
 * @param volume Volume global (0.0 à 1.0)
 */
void audio_sync_set_volume(AudioSyncManager* manager, float volume);

/**
 * Détruire le gestionnaire et libérer tous les chunks audio
 *
 * @param manager Pointeur vers le gestionnaire à détruire
 */
void audio_sync_destroy(AudioSyncManager* manager);

#endif
