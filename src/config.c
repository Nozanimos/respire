// SPDX-License-Identifier: GPL-3.0-or-later
// config.c
#include "config.h"
#include <stdio.h>



void load_config(AppConfig* config) {
    FILE* file = fopen(CONFIG_FILE, "r");
    if (file) {
        fscanf(file, "breath_cycles=%d\n", &config->breath_cycles);
        fscanf(file, "breath_duration=%f\n", &config->breath_duration);
        fscanf(file, "startup_delay=%d\n", &config->startup_delay);
        fclose(file);

        // Validation des valeurs chargées
        if (config->breath_duration < 1 || config->breath_duration > 10) {
            config->breath_duration = 3.0f;
        }
        if (config->breath_cycles < 1 || config->breath_cycles > 16) {
            config->breath_cycles = 1;

        }
    } else {
        // Valeurs par défaut
        *config = (AppConfig){
            .breath_cycles = 1,
            .breath_duration = 3.0f,
            .startup_delay = 10
        };
    }
}

void save_config(const AppConfig* config) {
    FILE* file = fopen(CONFIG_FILE, "w");
    if (file) {
        fprintf(file, "breath_cycles=%d\n", config->breath_cycles);
        fprintf(file, "breath_duration=%.1f\n", config->breath_duration);
        fprintf(file, "startup_delay=%d\n", config->startup_delay);
        fclose(file);
    }
}
