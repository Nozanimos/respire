// SPDX-License-Identifier: GPL-3.0-or-later
// config_registry.h
// ════════════════════════════════════════════════════════════════════════════
// TABLE DE MAPPING CENTRALISÉE : ID widget <-> Champ AppConfig
// ════════════════════════════════════════════════════════════════════════════
// Ajouter un nouveau paramètre = ajouter UNE SEULE LIGNE dans cette table !

#ifndef __CONFIG_REGISTRY_H__
#define __CONFIG_REGISTRY_H__

#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include "config.h"

// ════════════════════════════════════════════════════════════════════════════
// TYPES DE PARAMÈTRES
// ════════════════════════════════════════════════════════════════════════════
typedef enum {
    CONFIG_TYPE_INT,
    CONFIG_TYPE_FLOAT,
    CONFIG_TYPE_BOOL,
} ConfigParamType;

// ════════════════════════════════════════════════════════════════════════════
// ENTRÉE DANS LA TABLE DE MAPPING
// ════════════════════════════════════════════════════════════════════════════
// NOTE : Les limites min/max sont DANS LE JSON (widgets_config.json)
//        Cette table sert UNIQUEMENT à faire le lien entre :
//        - widget_id (du JSON) ↔ champ dans AppConfig ↔ clé dans respiration.conf
typedef struct {
    const char* widget_id;        // ID du widget (ex: "nb_session")
    const char* file_key;         // Clé dans le fichier (ex: "nb_session")
    ConfigParamType type;         // Type du paramètre
    size_t offset;                // Offset dans AppConfig (calculé avec offsetof)

    // Valeur par défaut (utilisée si le fichier est absent/corrompu)
    union {
        int int_val;
        float float_val;
        bool bool_val;
    } default_value;
} ConfigParamEntry;

// ════════════════════════════════════════════════════════════════════════════
// TABLE DE MAPPING CENTRALISÉE
// ════════════════════════════════════════════════════════════════════════════
// 🎯 AJOUTER UN NOUVEAU PARAMÈTRE = AJOUTER UNE LIGNE ICI !
// ════════════════════════════════════════════════════════════════════════════

static const ConfigParamEntry CONFIG_PARAM_TABLE[] = {
    // ID widget          Clé fichier        Type              Offset                               Défaut
    { "nb_session",    "nb_session",   CONFIG_TYPE_INT,  offsetof(AppConfig, nb_session),    {.int_val = 1} },
    { "breath_duration",  "breath_duration", CONFIG_TYPE_FLOAT, offsetof(AppConfig, breath_duration), {.float_val = 3.0f} },
    { "start_duration",   "start_duration",  CONFIG_TYPE_INT,  offsetof(AppConfig, start_duration),   {.int_val = 10} },
    { "alternate_cycles", "alternate_cycles",CONFIG_TYPE_BOOL, offsetof(AppConfig, alternate_cycles), {.bool_val = false} },
    { "Nb_respiration",   "Nb_respiration",  CONFIG_TYPE_INT,  offsetof(AppConfig, Nb_respiration),   {.int_val = 20} },
};

static const int CONFIG_PARAMS_COUNT = sizeof(CONFIG_PARAM_TABLE) / sizeof(ConfigParamEntry);

// ════════════════════════════════════════════════════════════════════════════
// FONCTIONS UTILITAIRES
// ════════════════════════════════════════════════════════════════════════════

// Trouve une entrée par widget_id
static inline const ConfigParamEntry* find_param_by_widget_id(const char* id) {
    if (!id) return NULL;
    for (int i = 0; i < CONFIG_PARAMS_COUNT; i++) {
        if (strcmp(CONFIG_PARAM_TABLE[i].widget_id, id) == 0) {
            return &CONFIG_PARAM_TABLE[i];
        }
    }
    return NULL;
}

// Trouve une entrée par file_key
static inline const ConfigParamEntry* find_param_by_file_key(const char* key) {
    if (!key) return NULL;
    for (int i = 0; i < CONFIG_PARAMS_COUNT; i++) {
        if (strcmp(CONFIG_PARAM_TABLE[i].file_key, key) == 0) {
            return &CONFIG_PARAM_TABLE[i];
        }
    }
    return NULL;
}

// Récupère un pointeur vers le champ dans AppConfig
static inline void* get_config_field(AppConfig* config, size_t offset) {
    return (void*)((char*)config + offset);
}

#endif
