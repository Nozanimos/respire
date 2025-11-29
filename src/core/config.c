// SPDX-License-Identifier: GPL-3.0-or-later
// config.c
#include "config.h"
#include "config_registry.h"
#include "widget_list.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//  CHARGEMENT DEPUIS LE FICHIER (GÉNÉRIQUE)
// Parcourt la table CONFIG_PARAM_TABLE pour charger tous les paramètres
// Pour ajouter un nouveau paramètre : ajouter 1 ligne dans config_registry.h

void load_config(AppConfig* config) {
    // Initialiser avec les valeurs par défaut
    for (int i = 0; i < CONFIG_PARAMS_COUNT; i++) {
        const ConfigParamEntry* entry = &CONFIG_PARAM_TABLE[i];
        void* field = get_config_field(config, entry->offset);

        switch (entry->type) {
            case CONFIG_TYPE_INT:
                *(int*)field = entry->default_value.int_val;
                break;
            case CONFIG_TYPE_FLOAT:
                *(float*)field = entry->default_value.float_val;
                break;
            case CONFIG_TYPE_BOOL:
                *(bool*)field = entry->default_value.bool_val;
                break;
        }
    }

    // Valeurs par défaut pour les champs non sauvegardés
    config->fullscreen = false;

    FILE* file = fopen(CONFIG_FILE, "r");
    if (file) {
        char line[256];
        while (fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\n")] = 0;  // Retirer \n

            // Parser format "clé=valeur"
            char key[128];
            char value[128];
            if (sscanf(line, "%127[^=]=%127s", key, value) == 2) {
                // Chercher l'entrée correspondante dans la table
                const ConfigParamEntry* entry = find_param_by_file_key(key);
                if (entry) {
                    void* field = get_config_field(config, entry->offset);

                    // Parser selon le type
                    // NOTE: Les limites min/max sont validées par les widgets eux-mêmes
                    //       (qui ont chargé ces limites depuis widgets_config.json)
                    switch (entry->type) {
                        case CONFIG_TYPE_INT: {
                            int int_val = atoi(value);
                            *(int*)field = int_val;
                            break;
                        }
                        case CONFIG_TYPE_FLOAT: {
                            float float_val = atof(value);
                            *(float*)field = float_val;
                            break;
                        }
                        case CONFIG_TYPE_BOOL:
                            *(bool*)field = (atoi(value) != 0);
                            break;
                    }
                }
            }
        }
        fclose(file);
        debug_printf("✅ Config chargée: cycles=%d, durée=%.1fs, timer=%ds\n",
                     config->nb_session, config->breath_duration, config->start_duration);
    } else {
        debug_printf("⚠️ Fichier config absent, valeurs par défaut\n");
    }
}

//  SAUVEGARDE DANS LE FICHIER (GÉNÉRIQUE)
// Parcourt la table CONFIG_PARAM_TABLE pour sauvegarder tous les paramètres
// Pour ajouter un nouveau paramètre : ajouter 1 ligne dans config_registry.h

void save_config(const AppConfig* config) {
    FILE* file = fopen(CONFIG_FILE, "w");
    if (file) {
        // Parcourir la table et sauvegarder chaque paramètre
        for (int i = 0; i < CONFIG_PARAMS_COUNT; i++) {
            const ConfigParamEntry* entry = &CONFIG_PARAM_TABLE[i];
            const void* field = get_config_field((AppConfig*)config, entry->offset);

            // Écrire selon le type
            switch (entry->type) {
                case CONFIG_TYPE_INT:
                    fprintf(file, "%s=%d\n", entry->file_key, *(const int*)field);
                    break;
                case CONFIG_TYPE_FLOAT:
                    fprintf(file, "%s=%.1f\n", entry->file_key, *(const float*)field);
                    break;
                case CONFIG_TYPE_BOOL:
                    fprintf(file, "%s=%d\n", entry->file_key, *(const bool*)field ? 1 : 0);
                    break;
            }
        }
        fclose(file);
        debug_printf("💾 Config sauvegardée\n");
    }
}

//  SYNCHRONISATION CONFIG → WIDGETS (GÉNÉRIQUE)
// Parcourt la table CONFIG_PARAM_TABLE et synchronise automatiquement
// Pour ajouter un nouveau paramètre : ajouter 1 ligne dans config_registry.h

void sync_config_to_widgets(AppConfig* config, WidgetList* list) {
    if (!config || !list) return;

    debug_section("SYNC CONFIG → WIDGETS");
    int count = 0;

    // Parcourir tous les paramètres de la table
    for (int i = 0; i < CONFIG_PARAMS_COUNT; i++) {
        const ConfigParamEntry* entry = &CONFIG_PARAM_TABLE[i];
        const void* field = get_config_field(config, entry->offset);

        // Synchroniser selon le type
        bool success = false;
        switch (entry->type) {
            case CONFIG_TYPE_INT:
                success = set_widget_int_value(list, entry->widget_id, *(const int*)field);
                if (success) {
                    debug_printf("  🔄 %s = %d\n", entry->widget_id, *(const int*)field);
                    count++;
                }
                break;

            case CONFIG_TYPE_FLOAT:
                // Convertir float en int pour le widget (si nécessaire)
                success = set_widget_int_value(list, entry->widget_id, (int)(*(const float*)field));
                if (success) {
                    debug_printf("  🔄 %s = %.1f\n", entry->widget_id, *(const float*)field);
                    count++;
                }
                break;

            case CONFIG_TYPE_BOOL:
                success = set_widget_bool_value(list, entry->widget_id, *(const bool*)field);
                if (success) {
                    debug_printf("  🔄 %s = %s\n", entry->widget_id, *(const bool*)field ? "ON" : "OFF");
                    count++;
                }
                break;
        }
    }

    debug_printf("✅ %d widget(s) synchronisé(s)\n", count);
    debug_blank_line();
}

//  SYNCHRONISATION WIDGETS → CONFIG (GÉNÉRIQUE)
// Parcourt la table CONFIG_PARAM_TABLE et synchronise automatiquement
// Pour ajouter un nouveau paramètre : ajouter 1 ligne dans config_registry.h

void sync_widgets_to_config(WidgetList* list, AppConfig* config) {
    if (!list || !config) return;

    debug_section("SYNC WIDGETS → CONFIG");
    int count = 0;

    // Parcourir tous les paramètres de la table
    for (int i = 0; i < CONFIG_PARAMS_COUNT; i++) {
        const ConfigParamEntry* entry = &CONFIG_PARAM_TABLE[i];
        void* field = get_config_field(config, entry->offset);

        // Récupérer la valeur du widget selon le type
        switch (entry->type) {
            case CONFIG_TYPE_INT: {
                int int_val;
                if (get_widget_int_value(list, entry->widget_id, &int_val)) {
                    *(int*)field = int_val;
                    debug_printf("  💾 %s = %d\n", entry->widget_id, int_val);
                    count++;
                }
                break;
            }

            case CONFIG_TYPE_FLOAT: {
                int int_val;
                if (get_widget_int_value(list, entry->widget_id, &int_val)) {
                    *(float*)field = (float)int_val;
                    debug_printf("  💾 %s = %.1f\n", entry->widget_id, (float)int_val);
                    count++;
                }
                break;
            }

            case CONFIG_TYPE_BOOL: {
                bool bool_val;
                if (get_widget_bool_value(list, entry->widget_id, &bool_val)) {
                    *(bool*)field = bool_val;
                    debug_printf("  💾 %s = %s\n", entry->widget_id, bool_val ? "ON" : "OFF");
                    count++;
                }
                break;
            }
        }
    }

    debug_printf("✅ %d paramètre(s) récupéré(s)\n", count);

    // Sauvegarder dans le fichier
    save_config(config);
    debug_blank_line();
}
