#ifndef PATHS_H
#define PATHS_H

/* ═══════════════════════════════════════════════════════════════════════════
 * CHEMINS DE FICHIERS ET RÉPERTOIRES
 * ═══════════════════════════════════════════════════════════════════════════
 * Ce fichier centralise tous les chemins de fichiers de l'application.
 *
 * Organisation :
 *   - Tous les chemins sont relatifs à la racine du projet
 *   - Utiliser ces constantes au lieu de chemins hard-codés
 *   - Facilite le portage et l'installation système
 * ═══════════════════════════════════════════════════════════════════════════ */

/* ═══════════════════════════════════════════════════════════════════════════
 * POLICES (FONTS)
 * ═══════════════════════════════════════════════════════════════════════════ */
#define FONT_ARIAL_REGULAR  "../fonts/arial/ARIAL.TTF"
#define FONT_ARIAL_BOLD     "../fonts/arial/ARIALBD.TTF"

/* ═══════════════════════════════════════════════════════════════════════════
 * IMAGES
 * ═══════════════════════════════════════════════════════════════════════════ */
#define IMG_NENUPHAR        "../img/nenuphar.jpg"
#define IMG_WIM             "../img/wim.png"
#define IMG_VERT            "../img/vert.jpg"
#define IMG_SETTINGS_BG     "../img/settings_bg.png"
#define IMG_SETTINGS_ICON   "../img/settings.png"

/* ═══════════════════════════════════════════════════════════════════════════
 * CONFIGURATION
 * ═══════════════════════════════════════════════════════════════════════════ */
#define CONFIG_FILE             "../config/respiration.conf"
#define CONFIG_WIDGETS          "../config/widgets_config.json"
#define CONFIG_STATS_DIR        "../config/stats"

/* ═══════════════════════════════════════════════════════════════════════════
 * FICHIERS GÉNÉRÉS (pour l'éditeur JSON)
 * ═══════════════════════════════════════════════════════════════════════════ */
#define GENERATED_WIDGETS_C     "../src/generated_widgets.c"
#define GENERATED_TEMPLATES_JSON "../src/json_editor/templates.json"

#endif /* PATHS_H */
