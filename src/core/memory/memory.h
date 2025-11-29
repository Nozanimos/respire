// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Active le mode debug pour tracking des allocations
 *
 * En mode debug, toutes les allocations sont enregistrées avec
 * leur localisation (fichier:ligne) pour détecter les fuites.
 */
void memory_enable_tracking(bool enable);

/**
 * @brief Allocation mémoire sécurisée avec tracking optionnel
 *
 * @param size Taille en octets à allouer
 * @param file Nom du fichier source (automatique avec macro)
 * @param line Numéro de ligne (automatique avec macro)
 * @return Pointeur vers mémoire allouée, ou NULL si échec
 *
 * Note: Utiliser la macro SAFE_MALLOC au lieu d'appeler directement
 */
void* safe_malloc_impl(size_t size, const char* file, int line);

/**
 * @brief Libération mémoire sécurisée avec tracking
 *
 * @param ptr Pointeur à libérer (peut être NULL)
 * @param file Nom du fichier source (automatique avec macro)
 * @param line Numéro de ligne (automatique avec macro)
 *
 * Note: Utiliser la macro SAFE_FREE au lieu d'appeler directement
 * Cette fonction met automatiquement le pointeur à NULL après libération
 */
void safe_free_impl(void** ptr, const char* file, int line);

/**
 * @brief Affiche un rapport des allocations non libérées
 *
 * Utile en fin de programme pour détecter les fuites mémoire.
 * Nécessite que le tracking soit activé.
 */
void memory_report_leaks(void);

/**
 * @brief Retourne le nombre d'allocations actives
 *
 * @return Nombre de blocs mémoire actuellement alloués
 */
size_t memory_get_allocation_count(void);

/**
 * @brief Retourne la quantité de mémoire totale allouée
 *
 * @return Nombre d'octets actuellement alloués
 */
size_t memory_get_allocated_bytes(void);

/**
 * @brief Nettoie le système de tracking mémoire
 *
 * À appeler à la fin du programme pour libérer les ressources
 * utilisées par le système de tracking.
 */
void memory_cleanup_tracking(void);

// Macros pour utilisation simplifiée
#define SAFE_MALLOC(size) safe_malloc_impl((size), __FILE__, __LINE__)
#define SAFE_FREE(ptr) safe_free_impl((void**)&(ptr), __FILE__, __LINE__)

// Macro pour cleanup automatique en fin de scope (GCC/Clang)
#if defined(__GNUC__) || defined(__clang__)
    #define AUTO_FREE __attribute__((cleanup(auto_free_cleanup)))

    static inline void auto_free_cleanup(void* ptr) {
        void** p = (void**)ptr;
        if (*p) {
            safe_free_impl(p, __FILE__, __LINE__);
        }
    }
#else
    #define AUTO_FREE
#endif

// Macros utilitaires pour gestion d'erreurs avec cleanup
// Note: Si error.h est inclus, utiliser ses macros CHECK_ALLOC plus complètes
#ifndef CHECK_ALLOC
    #define CHECK_ALLOC(ptr) \
        do { \
            if (!(ptr)) { \
                goto cleanup; \
            } \
        } while(0)

    #define CHECK_ALLOC_MSG(ptr, msg) \
        do { \
            if (!(ptr)) { \
                debug_printf("❌ Allocation échouée: %s\n", (msg)); \
                goto cleanup; \
            } \
        } while(0)
#endif

#ifdef __cplusplus
}
#endif

#endif // MEMORY_H
