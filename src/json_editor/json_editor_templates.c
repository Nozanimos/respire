// SPDX-License-Identifier: GPL-3.0-or-later
// json_editor_templates.c
// Gestion des templates JSON (sous-menu en cascade)

#include "json_editor.h"
#include "core/debug.h"
#include "core/paths.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/memory/memory.h"

//  CHARGEMENT DES TEMPLATES DEPUIS LE FICHIER JSON

// Fonction helper : convertit un objet cJSON en string formaté
// Cette fonction prend un objet JSON et le convertit en texte bien indenté
static char* json_object_to_string(cJSON* obj) {
    if (!obj) return NULL;

    // Utiliser cJSON_Print pour avoir une indentation propre
    char* json_str = cJSON_Print(obj);

    if (!json_str) return NULL;

    // Dupliquer la string pour la stocker
    char* result = strdup(json_str);
    SAFE_FREE(json_str);

    return result;
}

// Crée le sous-menu des templates en lisant templates.json
ContextMenu* creer_sous_menu_templates(JsonEditor* editor) {
    debug_printf("🔧 Création du sous-menu templates...\n");

    // Utiliser la police de l'éditeur pour les calculs
    if (!editor || !editor->font_ui) {
        debug_printf("⚠️ Éditeur ou police UI non disponible\n");
        return NULL;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 1. CHARGER LE FICHIER templates.json
    // ─────────────────────────────────────────────────────────────────────────
    FILE* file = fopen(GENERATED_TEMPLATES_JSON, "r");

    if (!file) {
        debug_printf("⚠️ Impossible d'ouvrir %s\n", GENERATED_TEMPLATES_JSON);
        return NULL;
    }

    // Lire tout le contenu
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* json_string = SAFE_MALLOC(file_size + 1);
    if (!json_string) {
        fclose(file);
        debug_printf("❌ Erreur allocation mémoire\n");
        return NULL;
    }

    fread(json_string, 1, file_size, file);
    json_string[file_size] = '\0';
    fclose(file);

    // Parser le JSON
    cJSON* root = cJSON_Parse(json_string);
    SAFE_FREE(json_string);

    if (!root) {
        debug_printf("❌ JSON invalide dans %s\n", GENERATED_TEMPLATES_JSON);
        return NULL;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 2. EXTRAIRE LE TABLEAU DE TEMPLATES
    // ─────────────────────────────────────────────────────────────────────────
    cJSON* templates_array = cJSON_GetObjectItem(root, "templates");
    if (!cJSON_IsArray(templates_array)) {
        debug_printf("❌ Pas de tableau 'templates' trouvé\n");
        cJSON_Delete(root);
        return NULL;
    }

    int nb_templates = cJSON_GetArraySize(templates_array);
    if (nb_templates == 0) {
        debug_printf("⚠️ Aucun template trouvé\n");
        cJSON_Delete(root);
        return NULL;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 3. CRÉER LE SOUS-MENU
    // ─────────────────────────────────────────────────────────────────────────
    ContextMenu* sous_menu = SAFE_MALLOC(sizeof(ContextMenu));
    if (!sous_menu) {
        debug_printf("❌ Erreur allocation sous-menu\n");
        cJSON_Delete(root);
        return NULL;
    }

    // Initialiser les couleurs (même thème que le menu principal)
    sous_menu->bg_color = (SDL_Color){30, 30, 35, 255};
    sous_menu->border_color = (SDL_Color){100, 100, 110, 255};
    sous_menu->text_enabled_color = (SDL_Color){255, 255, 255, 255};
    sous_menu->text_disabled_color = (SDL_Color){128, 128, 128, 255};
    sous_menu->hover_color = (SDL_Color){181, 181, 181, 50};
    sous_menu->visible = false;
    sous_menu->hovered_item = -1;
    sous_menu->item_count = 0;

    // ─────────────────────────────────────────────────────────────────────────
    // 4. CRÉER UN ITEM PAR TEMPLATE
    // ─────────────────────────────────────────────────────────────────────────
    cJSON* template_obj = NULL;
    cJSON_ArrayForEach(template_obj, templates_array) {
        // Récupérer le champ "type" pour le label
        cJSON* type_field = cJSON_GetObjectItem(template_obj, "type");

        if (!type_field || !cJSON_IsString(type_field)) {
            debug_printf("⚠️ Template sans type valide, ignoré\n");
            continue;
        }

        // Créer le label (capitaliser la première lettre)
        const char* type_name = type_field->valuestring;
        char* label = SAFE_MALLOC(strlen(type_name) + 1);
        strcpy(label, type_name);

        // Capitaliser la première lettre
        if (label[0] >= 'a' && label[0] <= 'z') {
            label[0] = label[0] - 'a' + 'A';
        }

        // Convertir le template en string JSON formaté
        char* template_str = json_object_to_string(template_obj);

        if (!template_str) {
            debug_printf("⚠️ Impossible de convertir le template '%s' en string\n", type_name);
            SAFE_FREE(label);
            continue;
        }

        // Ajouter l'item au sous-menu
        sous_menu->items[sous_menu->item_count] = (ContextMenuItem){
            .label = label,
            .rect = (SDL_Rect){0, 0, 0, 0},  // Sera calculé plus tard
            .enabled = true,
            .action = action_inserer_template_contextuel,  // Action d'insertion
            .sous_menu = NULL,
            .template_data = template_str  // Stocker le JSON du template
        };

        sous_menu->item_count++;

        debug_printf("  ✓ Template '%s' ajouté au sous-menu\n", type_name);

        // Limiter le nombre de templates (sécurité)
        if (sous_menu->item_count >= 20) {
            debug_printf("⚠️ Limite de 20 templates atteinte\n");
            break;
        }
    }

    cJSON_Delete(root);

    debug_printf("✅ Sous-menu créé avec %d templates\n", sous_menu->item_count);
    return sous_menu;
}

//  LIBÉRATION DE LA MÉMOIRE

// Libère un sous-menu et toutes ses ressources
void detruire_sous_menu(ContextMenu* menu) {
    if (!menu) return;

    // Libérer tous les labels et template_data
    for (int i = 0; i < menu->item_count; i++) {
        if (menu->items[i].label) {
            SAFE_FREE(menu->items[i].label);
        }
        if (menu->items[i].template_data) {
            SAFE_FREE(menu->items[i].template_data);
        }
        // Libérer récursivement les sous-menus (si besoin plus tard)
        if (menu->items[i].sous_menu) {
            detruire_sous_menu(menu->items[i].sous_menu);
        }
    }

    SAFE_FREE(menu);
    debug_printf("🗑️ Sous-menu détruit\n");
}

//  ACTION D'INSERTION DE TEMPLATE

// Insère le template sélectionné à la position du curseur
void action_inserer_template_contextuel(JsonEditor* editor) {
    if (!editor) return;

    // Trouver quel item a été cliqué dans le sous-menu
    ContextMenu* sous_menu = NULL;

    // Parcourir le menu principal pour trouver l'item "Templates"
    for (int i = 0; i < editor->context_menu.item_count; i++) {
        if (editor->context_menu.items[i].sous_menu) {
            sous_menu = editor->context_menu.items[i].sous_menu;
            break;
        }
    }

    if (!sous_menu || !sous_menu->visible) return;

    // Trouver l'item survolé dans le sous-menu
    int hovered = sous_menu->hovered_item;
    if (hovered < 0 || hovered >= sous_menu->item_count) return;

    ContextMenuItem* item = &sous_menu->items[hovered];
    if (!item->template_data) return;

    debug_printf("📝 Insertion du template: %s\n", item->label);

    // ─────────────────────────────────────────────────────────────────────────
    // INSERTION DU TEMPLATE À LA POSITION DU CURSEUR
    // ─────────────────────────────────────────────────────────────────────────

    // Sauvegarder l'état pour undo
    sauvegarder_etat_undo(editor);

    // Calculer la longueur du template
    int template_len = strlen(item->template_data);
    int buffer_len = strlen(editor->buffer);

    // Vérifier qu'on ne dépasse pas la taille du buffer
    if (buffer_len + template_len >= JSON_BUFFER_SIZE - 1) {
        debug_printf("⚠️ Buffer plein, impossible d'insérer le template\n");
        return;
    }

    // Déplacer le texte après le curseur pour faire de la place
    memmove(
        editor->buffer + editor->curseur_position + template_len,
        editor->buffer + editor->curseur_position,
        buffer_len - editor->curseur_position + 1  // +1 pour le '\0'
    );

    // Insérer le template
    memcpy(
        editor->buffer + editor->curseur_position,
        item->template_data,
        template_len
    );

    // Déplacer le curseur à la fin du template inséré
    editor->curseur_position += template_len;

    // Marquer comme modifié
    editor->modified = true;

    // Valider le JSON
    valider_json(editor);

    // Recalculer le nombre de lignes
    editor->nb_lignes = compter_lignes(editor->buffer);

    // Auto-scroll pour montrer le curseur
    auto_scroll_curseur(editor);

    // Cacher les menus
    cacher_menu_contextuel(editor);

    debug_printf("✅ Template inséré avec succès\n");
}
