// SPDX-License-Identifier: GPL-3.0-or-later
// json_editor_codegen.c
// Génération de code C à partir de la configuration JSON des widgets

#include "json_editor.h"
#include <stdio.h>
#include <string.h>
#include <cjson/cJSON.h>

// Structure pour collecter les callbacks uniques
typedef struct {
    char* callbacks_int[20];
    int count_int;
    char* callbacks_bool[20];
    int count_bool;
    char* callbacks_void[20];
    int count_void;
} CallbackCollection;

// Fonction pour ajouter un callback à la collection (évite les doublons)
static void ajouter_callback_unique(char** array, int* count, const char* callback_name) {
    if (!callback_name || *count >= 20) return;

    // Vérifier si déjà présent
    for (int i = 0; i < *count; i++) {
        if (strcmp(array[i], callback_name) == 0) {
            return; // Déjà présent
        }
    }

    // Ajouter
    array[*count] = strdup(callback_name);
    (*count)++;
}

// ════════════════════════════════════════════════════════════════════════════
//  COLLECTE DES CALLBACKS DEPUIS LE JSON
// ════════════════════════════════════════════════════════════════════════════
static void collecter_callbacks_depuis_json(cJSON* widgets, CallbackCollection* collection) {
    collection->count_int = 0;
    collection->count_bool = 0;
    collection->count_void = 0;

    int widget_count = cJSON_GetArraySize(widgets);
    for (int i = 0; i < widget_count; i++) {
        cJSON* widget = cJSON_GetArrayItem(widgets, i);
        cJSON* type = cJSON_GetObjectItem(widget, "type");
        cJSON* callback = cJSON_GetObjectItem(widget, "callback");

        if (!callback || !cJSON_IsString(callback) || !type || !cJSON_IsString(type)) {
            continue;
        }

        const char* callback_name = callback->valuestring;
        const char* type_str = type->valuestring;

        // Déterminer le type de callback selon le type de widget
        if (strcmp(type_str, "increment") == 0) {
            ajouter_callback_unique(collection->callbacks_int, &collection->count_int, callback_name);
        } else if (strcmp(type_str, "toggle") == 0) {
            ajouter_callback_unique(collection->callbacks_bool, &collection->count_bool, callback_name);
        } else if (strcmp(type_str, "button") == 0) {
            ajouter_callback_unique(collection->callbacks_void, &collection->count_void, callback_name);
        }
    }
}

// Fonction pour libérer la mémoire de la collection
static void liberer_callback_collection(CallbackCollection* collection) {
    for (int i = 0; i < collection->count_int; i++) {
        free(collection->callbacks_int[i]);
    }
    for (int i = 0; i < collection->count_bool; i++) {
        free(collection->callbacks_bool[i]);
    }
    for (int i = 0; i < collection->count_void; i++) {
        free(collection->callbacks_void[i]);
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  GÉNÉRATION DES FONCTIONS HELPER POUR LES CALLBACKS
// ════════════════════════════════════════════════════════════════════════════
static void generer_helpers_callbacks(FILE* f, CallbackCollection* collection) {
    // Déclarations externes
    fprintf(f, "// ════════════════════════════════════════════════════════════════════════════\n");
    fprintf(f, "//  DÉCLARATIONS EXTERNES DES CALLBACKS\n");
    fprintf(f, "// ════════════════════════════════════════════════════════════════════════════\n");
    fprintf(f, "// Ces fonctions doivent être définies dans settings_panel.c\n");

    for (int i = 0; i < collection->count_int; i++) {
        fprintf(f, "extern void %s(int value);\n", collection->callbacks_int[i]);
    }
    for (int i = 0; i < collection->count_bool; i++) {
        fprintf(f, "extern void %s(bool value);\n", collection->callbacks_bool[i]);
    }
    for (int i = 0; i < collection->count_void; i++) {
        fprintf(f, "extern void %s(void);\n", collection->callbacks_void[i]);
    }
    fprintf(f, "\n");

    // Fonction helper pour callbacks INT
    fprintf(f, "// ════════════════════════════════════════════════════════════════════════════\n");
    fprintf(f, "//  FONCTIONS HELPER POUR RÉSOUDRE LES CALLBACKS\n");
    fprintf(f, "// ════════════════════════════════════════════════════════════════════════════\n");
    fprintf(f, "// Ces fonctions convertissent les noms de callbacks en pointeurs de fonction\n\n");

    fprintf(f, "static void (*obtenir_callback_int(const char* nom))(int) {\n");
    fprintf(f, "    if (!nom) return NULL;\n\n");
    for (int i = 0; i < collection->count_int; i++) {
        fprintf(f, "    if (strcmp(nom, \"%s\") == 0) return %s;\n",
                collection->callbacks_int[i], collection->callbacks_int[i]);
    }
    fprintf(f, "\n    return NULL;\n");
    fprintf(f, "}\n\n");

    // Fonction helper pour callbacks BOOL
    fprintf(f, "static void (*obtenir_callback_bool(const char* nom))(bool) {\n");
    fprintf(f, "    if (!nom) return NULL;\n\n");
    for (int i = 0; i < collection->count_bool; i++) {
        fprintf(f, "    if (strcmp(nom, \"%s\") == 0) return %s;\n",
                collection->callbacks_bool[i], collection->callbacks_bool[i]);
    }
    fprintf(f, "\n    return NULL;\n");
    fprintf(f, "}\n\n");

    // Fonction helper pour callbacks VOID
    fprintf(f, "static void (*obtenir_callback_void(const char* nom))(void) {\n");
    fprintf(f, "    if (!nom) return NULL;\n\n");
    for (int i = 0; i < collection->count_void; i++) {
        fprintf(f, "    if (strcmp(nom, \"%s\") == 0) return %s;\n",
                collection->callbacks_void[i], collection->callbacks_void[i]);
    }
    fprintf(f, "\n    return NULL;\n");
    fprintf(f, "}\n\n");
}

// ════════════════════════════════════════════════════════════════════════════
//  GÉNÉRATION DE CODE C POUR UN WIDGET INCREMENT
// ════════════════════════════════════════════════════════════════════════════
static void generer_code_increment(FILE* f, cJSON* widget) {
    cJSON* id = cJSON_GetObjectItem(widget, "id");
    cJSON* nom = cJSON_GetObjectItem(widget, "nom_affichage");
    cJSON* x = cJSON_GetObjectItem(widget, "x");
    cJSON* y = cJSON_GetObjectItem(widget, "y");
    cJSON* min = cJSON_GetObjectItem(widget, "valeur_min");
    cJSON* max = cJSON_GetObjectItem(widget, "valeur_max");
    cJSON* depart = cJSON_GetObjectItem(widget, "valeur_depart");
    cJSON* inc = cJSON_GetObjectItem(widget, "increment");
    cJSON* fleche = cJSON_GetObjectItem(widget, "taille_fleche");
    cJSON* texte = cJSON_GetObjectItem(widget, "taille_texte");
    cJSON* callback = cJSON_GetObjectItem(widget, "callback");

    if (!id || !nom || !x || !y || !min || !max || !depart) {
        return; // Widget invalide
    }

    // Commentaire descriptif
    fprintf(f, "    // Widget increment : %s\n", nom->valuestring);

    // Appel de la fonction
    fprintf(f, "    add_increment_widget(\n");
    fprintf(f, "        list,\n");
    fprintf(f, "        \"%s\",              // id\n", id->valuestring);
    fprintf(f, "        \"%s\",  // nom_affichage\n", nom->valuestring);
    fprintf(f, "        %d, %d,              // x, y\n", x->valueint, y->valueint);
    fprintf(f, "        %d, %d, %d,          // valeur_min, valeur_max, valeur_depart\n",
            min->valueint, max->valueint, depart->valueint);
    fprintf(f, "        %d,                  // increment\n",
            inc && cJSON_IsNumber(inc) ? inc->valueint : 1);
    fprintf(f, "        %d,                  // taille_fleche\n",
            fleche && cJSON_IsNumber(fleche) ? fleche->valueint : 6);
    fprintf(f, "        %d,                  // taille_texte\n",
            texte && cJSON_IsNumber(texte) ? texte->valueint : 18);
    fprintf(f, "        font_normal,         // font\n");
    fprintf(f, "        obtenir_callback_int(\"%s\")  // callback\n",
            callback && cJSON_IsString(callback) ? callback->valuestring : "");
    fprintf(f, "    );\n\n");
}

// ════════════════════════════════════════════════════════════════════════════
//  GÉNÉRATION DE CODE C POUR UN WIDGET TOGGLE
// ════════════════════════════════════════════════════════════════════════════
static void generer_code_toggle(FILE* f, cJSON* widget) {
    cJSON* id = cJSON_GetObjectItem(widget, "id");
    cJSON* nom = cJSON_GetObjectItem(widget, "nom_affichage");
    cJSON* x = cJSON_GetObjectItem(widget, "x");
    cJSON* y = cJSON_GetObjectItem(widget, "y");
    cJSON* etat = cJSON_GetObjectItem(widget, "etat_depart");
    cJSON* largeur = cJSON_GetObjectItem(widget, "largeur_toggle");
    cJSON* hauteur = cJSON_GetObjectItem(widget, "hauteur_toggle");
    cJSON* curseur = cJSON_GetObjectItem(widget, "taille_curseur");
    cJSON* texte = cJSON_GetObjectItem(widget, "taille_texte");
    cJSON* callback = cJSON_GetObjectItem(widget, "callback");

    if (!id || !nom || !x || !y) {
        return; // Widget invalide
    }

    // Commentaire descriptif
    fprintf(f, "    // Widget toggle : %s\n", nom->valuestring);

    // Appel de la fonction
    fprintf(f, "    add_toggle_widget(\n");
    fprintf(f, "        list,\n");
    fprintf(f, "        \"%s\",              // id\n", id->valuestring);
    fprintf(f, "        \"%s\",  // nom_affichage\n", nom->valuestring);
    fprintf(f, "        %d, %d,              // x, y\n", x->valueint, y->valueint);
    fprintf(f, "        %s,                  // etat_depart\n",
            etat && cJSON_IsTrue(etat) ? "true" : "false");
    fprintf(f, "        %d,                  // largeur_toggle\n",
            largeur && cJSON_IsNumber(largeur) ? largeur->valueint : 40);
    fprintf(f, "        %d,                  // hauteur_toggle\n",
            hauteur && cJSON_IsNumber(hauteur) ? hauteur->valueint : 18);
    fprintf(f, "        %d,                  // taille_curseur\n",
            curseur && cJSON_IsNumber(curseur) ? curseur->valueint : 18);
    fprintf(f, "        %d,                  // taille_texte\n",
            texte && cJSON_IsNumber(texte) ? texte->valueint : 18);
    fprintf(f, "        obtenir_callback_bool(\"%s\")  // callback\n",
            callback && cJSON_IsString(callback) ? callback->valuestring : "");
    fprintf(f, "    );\n\n");
}

// ════════════════════════════════════════════════════════════════════════════
//  GÉNÉRATION DE CODE C POUR UN TITRE (LABEL)
// ════════════════════════════════════════════════════════════════════════════
static void generer_code_titre(FILE* f, cJSON* widget) {
    cJSON* texte = cJSON_GetObjectItem(widget, "texte");
    cJSON* x = cJSON_GetObjectItem(widget, "x");
    cJSON* y = cJSON_GetObjectItem(widget, "y");
    cJSON* taille = cJSON_GetObjectItem(widget, "taille_texte");
    cJSON* couleur = cJSON_GetObjectItem(widget, "couleur");
    cJSON* souligne = cJSON_GetObjectItem(widget, "souligne");

    if (!texte || !x || !y) {
        return; // Widget invalide
    }

    // Commentaire descriptif
    fprintf(f, "    // Titre : %s\n", texte->valuestring);

    // Appel de la fonction
    fprintf(f, "    add_label_widget(\n");
    fprintf(f, "        list,\n");
    fprintf(f, "        NULL,                // id (pas besoin pour un titre)\n");
    fprintf(f, "        \"%s\",  // texte\n", texte->valuestring);
    fprintf(f, "        %d, %d,              // x, y\n", x->valueint, y->valueint);
    fprintf(f, "        %d,                  // taille_texte\n",
            taille && cJSON_IsNumber(taille) ? taille->valueint : 24);

    // Gestion de la couleur
    if (couleur && cJSON_IsObject(couleur)) {
        cJSON* r = cJSON_GetObjectItem(couleur, "r");
        cJSON* g = cJSON_GetObjectItem(couleur, "g");
        cJSON* b = cJSON_GetObjectItem(couleur, "b");
        cJSON* a = cJSON_GetObjectItem(couleur, "a");
        fprintf(f, "        (SDL_Color){%d, %d, %d, %d},  // couleur\n",
                r ? r->valueint : 255,
                g ? g->valueint : 255,
                b ? b->valueint : 255,
                a ? a->valueint : 255);
    } else {
        fprintf(f, "        (SDL_Color){255, 255, 255, 255},  // couleur par défaut\n");
    }

    fprintf(f, "        %s                   // souligne\n",
            souligne && cJSON_IsTrue(souligne) ? "true" : "false");
    fprintf(f, "    );\n\n");
}

// ════════════════════════════════════════════════════════════════════════════
//  GÉNÉRATION DE CODE C POUR UN SÉPARATEUR
// ════════════════════════════════════════════════════════════════════════════
static void generer_code_separator(FILE* f, cJSON* widget) {
    cJSON* id = cJSON_GetObjectItem(widget, "id");
    cJSON* y = cJSON_GetObjectItem(widget, "y");
    cJSON* marge_debut = cJSON_GetObjectItem(widget, "marge_debut");
    cJSON* marge_fin = cJSON_GetObjectItem(widget, "marge_fin");
    cJSON* epaisseur = cJSON_GetObjectItem(widget, "epaisseur");
    cJSON* couleur = cJSON_GetObjectItem(widget, "couleur");

    if (!y) {
        return; // Widget invalide
    }

    // Commentaire descriptif
    fprintf(f, "    // Séparateur\n");

    // Appel de la fonction
    fprintf(f, "    add_separator_widget(\n");
    fprintf(f, "        list,\n");
    if (id && cJSON_IsString(id)) {
        fprintf(f, "        \"%s\",              // id\n", id->valuestring);
    } else {
        fprintf(f, "        NULL,                // id\n");
    }
    fprintf(f, "        %d,                  // y\n", y->valueint);
    fprintf(f, "        %d,                  // marge_debut\n",
            marge_debut && cJSON_IsNumber(marge_debut) ? marge_debut->valueint : 20);
    fprintf(f, "        %d,                  // marge_fin\n",
            marge_fin && cJSON_IsNumber(marge_fin) ? marge_fin->valueint : 20);
    fprintf(f, "        %d,                  // epaisseur\n",
            epaisseur && cJSON_IsNumber(epaisseur) ? epaisseur->valueint : 2);

    // Gestion de la couleur
    if (couleur && cJSON_IsObject(couleur)) {
        cJSON* r = cJSON_GetObjectItem(couleur, "r");
        cJSON* g = cJSON_GetObjectItem(couleur, "g");
        cJSON* b = cJSON_GetObjectItem(couleur, "b");
        cJSON* a = cJSON_GetObjectItem(couleur, "a");
        fprintf(f, "        (SDL_Color){%d, %d, %d, %d}   // couleur\n",
                r ? r->valueint : 100,
                g ? g->valueint : 100,
                b ? b->valueint : 100,
                a ? a->valueint : 255);
    } else {
        fprintf(f, "        (SDL_Color){100, 100, 100, 255}  // couleur par défaut\n");
    }

    fprintf(f, "    );\n\n");
}

// ════════════════════════════════════════════════════════════════════════════
//  GÉNÉRATION DE CODE C POUR UN PREVIEW
// ════════════════════════════════════════════════════════════════════════════
static void generer_code_preview(FILE* f, cJSON* widget) {
    cJSON* id = cJSON_GetObjectItem(widget, "id");
    cJSON* x = cJSON_GetObjectItem(widget, "x");
    cJSON* y = cJSON_GetObjectItem(widget, "y");
    cJSON* taille_cadre = cJSON_GetObjectItem(widget, "taille_cadre");
    cJSON* ratio_taille = cJSON_GetObjectItem(widget, "ratio_taille");
    cJSON* duree_respiration = cJSON_GetObjectItem(widget, "duree_respiration");

    if (!x || !y) {
        return; // Widget invalide
    }

    // Commentaire descriptif
    fprintf(f, "    // Widget preview (animation)\n");

    // Appel de la fonction
    fprintf(f, "    add_preview_widget(\n");
    fprintf(f, "        list,\n");
    if (id && cJSON_IsString(id)) {
        fprintf(f, "        \"%s\",              // id\n", id->valuestring);
    } else {
        fprintf(f, "        NULL,                // id\n");
    }
    fprintf(f, "        %d, %d,              // x, y\n", x->valueint, y->valueint);
    fprintf(f, "        %d,                  // taille_cadre\n",
            taille_cadre && cJSON_IsNumber(taille_cadre) ? taille_cadre->valueint : 200);
    fprintf(f, "        %.2ff,               // ratio_taille\n",
            ratio_taille && cJSON_IsNumber(ratio_taille) ? ratio_taille->valuedouble : 0.8);
    fprintf(f, "        %.1ff                // duree_respiration\n",
            duree_respiration && cJSON_IsNumber(duree_respiration) ? duree_respiration->valuedouble : 4.0);
    fprintf(f, "    );\n\n");
}

// ════════════════════════════════════════════════════════════════════════════
//  GÉNÉRATION DE CODE C POUR UN BOUTON
// ════════════════════════════════════════════════════════════════════════════
static void generer_code_button(FILE* f, cJSON* widget) {
    cJSON* id = cJSON_GetObjectItem(widget, "id");
    cJSON* nom = cJSON_GetObjectItem(widget, "nom_affichage");
    cJSON* x = cJSON_GetObjectItem(widget, "x");
    cJSON* y = cJSON_GetObjectItem(widget, "y");
    cJSON* largeur = cJSON_GetObjectItem(widget, "largeur");
    cJSON* hauteur = cJSON_GetObjectItem(widget, "hauteur");
    cJSON* taille_texte = cJSON_GetObjectItem(widget, "taille_texte");
    cJSON* couleur_fond = cJSON_GetObjectItem(widget, "couleur_fond");
    cJSON* ancrage_y = cJSON_GetObjectItem(widget, "ancrage_y");
    cJSON* callback = cJSON_GetObjectItem(widget, "callback");

    if (!id || !nom || !x || !y) {
        return; // Widget invalide
    }

    // Commentaire descriptif
    fprintf(f, "    // Bouton : %s\n", nom->valuestring);

    // Appel de la fonction
    fprintf(f, "    add_button_widget(\n");
    fprintf(f, "        list,\n");
    fprintf(f, "        \"%s\",              // id\n", id->valuestring);
    fprintf(f, "        \"%s\",  // nom_affichage\n", nom->valuestring);
    fprintf(f, "        %d, %d,              // x, y\n", x->valueint, y->valueint);
    fprintf(f, "        %d, %d,              // largeur, hauteur\n",
            largeur && cJSON_IsNumber(largeur) ? largeur->valueint : 100,
            hauteur && cJSON_IsNumber(hauteur) ? hauteur->valueint : 40);
    fprintf(f, "        %d,                  // taille_texte\n",
            taille_texte && cJSON_IsNumber(taille_texte) ? taille_texte->valueint : 16);

    // Gestion de la couleur de fond
    if (couleur_fond && cJSON_IsObject(couleur_fond)) {
        cJSON* r = cJSON_GetObjectItem(couleur_fond, "r");
        cJSON* g = cJSON_GetObjectItem(couleur_fond, "g");
        cJSON* b = cJSON_GetObjectItem(couleur_fond, "b");
        cJSON* a = cJSON_GetObjectItem(couleur_fond, "a");
        fprintf(f, "        (SDL_Color){%d, %d, %d, %d},  // couleur_fond\n",
                r ? r->valueint : 50,
                g ? g->valueint : 100,
                b ? b->valueint : 200,
                a ? a->valueint : 255);
    } else {
        fprintf(f, "        (SDL_Color){50, 100, 200, 255},  // couleur_fond par défaut\n");
    }

    // Ancrage Y
    const char* ancrage_str = "BUTTON_ANCHOR_TOP";
    if (ancrage_y && cJSON_IsString(ancrage_y)) {
        if (strcmp(ancrage_y->valuestring, "bottom") == 0) {
            ancrage_str = "BUTTON_ANCHOR_BOTTOM";
        }
    }
    fprintf(f, "        %s,           // ancrage_y\n", ancrage_str);

    fprintf(f, "        obtenir_callback_void(\"%s\")  // callback\n",
            callback && cJSON_IsString(callback) ? callback->valuestring : "");
    fprintf(f, "    );\n\n");
}

// ════════════════════════════════════════════════════════════════════════════
//  FONCTION PRINCIPALE : GÉNÉRATION DU FICHIER C COMPLET
// ════════════════════════════════════════════════════════════════════════════
bool generer_code_c_depuis_json(JsonEditor* editor) {
    if (!editor) {
        fprintf(stderr, "❌ Éditeur invalide pour la génération de code C\n");
        return false;
    }

    // Parser le JSON actuel
    cJSON* root = cJSON_Parse(editor->buffer);
    if (!root) {
        fprintf(stderr, "❌ JSON invalide, impossible de générer le code C\n");
        return false;
    }

    cJSON* widgets = cJSON_GetObjectItem(root, "widgets");
    if (!widgets || !cJSON_IsArray(widgets)) {
        fprintf(stderr, "❌ Pas de tableau 'widgets' trouvé dans le JSON\n");
        cJSON_Delete(root);
        return false;
    }

    // Collecter tous les callbacks utilisés
    CallbackCollection callbacks;
    collecter_callbacks_depuis_json(widgets, &callbacks);

    // Ouvrir le fichier de sortie
    FILE* f = fopen("../src/generated_widgets.c", "w");
    if (!f) {
        fprintf(stderr, "❌ Impossible de créer le fichier ../src/generated_widgets.c\n");
        liberer_callback_collection(&callbacks);
        cJSON_Delete(root);
        return false;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // EN-TÊTE DU FICHIER GÉNÉRÉ
    // ═══════════════════════════════════════════════════════════════════════
    fprintf(f, "// SPDX-License-Identifier: GPL-3.0-or-later\n");
    fprintf(f, "// generated_widgets.c\n");
    fprintf(f, "// ⚠️  FICHIER GÉNÉRÉ AUTOMATIQUEMENT - NE PAS MODIFIER MANUELLEMENT\n");
    fprintf(f, "// ⚠️  Ce fichier est généré par le JSON Editor via \"Générer Code C\"\n");
    fprintf(f, "// ⚠️  À CONSERVER : Servira pour la version finale du projet (sans JSON Editor)\n");
    fprintf(f, "//\n");
    fprintf(f, "// Ce fichier contient tous les appels add_*_widget() pour recréer\n");
    fprintf(f, "// l'interface de configuration sans dépendre du système JSON.\n");
    fprintf(f, "// Pour l'utiliser : copier la fonction init_widgets_from_json() dans\n");
    fprintf(f, "// settings_panel.c et l'appeler à la place de load_widgets_from_json().\n\n");

    fprintf(f, "#include \"widget_list.h\"\n");
    fprintf(f, "#include \"button_widget.h\"\n");
    fprintf(f, "#include <SDL2/SDL.h>\n");
    fprintf(f, "#include <SDL2/SDL_ttf.h>\n");
    fprintf(f, "#include <string.h>\n");
    fprintf(f, "#include <stdio.h>\n\n");

    // Générer les déclarations extern et les fonctions helper
    generer_helpers_callbacks(f, &callbacks);

    fprintf(f, "// ════════════════════════════════════════════════════════════════════════════\n");
    fprintf(f, "//  FONCTION D'INITIALISATION DES WIDGETS (VERSION HARDCODÉE)\n");
    fprintf(f, "// ════════════════════════════════════════════════════════════════════════════\n");
    fprintf(f, "// Cette fonction remplace load_widgets_from_json() pour la version finale.\n");
    fprintf(f, "// Elle recrée exactement la même interface que le JSON, mais en dur dans le code.\n");
    fprintf(f, "//\n");
    fprintf(f, "// Paramètres :\n");
    fprintf(f, "//   - list : La liste de widgets à remplir\n");
    fprintf(f, "//   - font_normal : Police à utiliser pour le texte des widgets\n");
    fprintf(f, "void init_widgets_from_json(WidgetList* list, TTF_Font* font_normal) {\n");
    fprintf(f, "    if (!list || !font_normal) {\n");
    fprintf(f, "        fprintf(stderr, \"❌ Paramètres invalides pour init_widgets_from_json\\n\");\n");
    fprintf(f, "        return;\n");
    fprintf(f, "    }\n\n");

    // ═══════════════════════════════════════════════════════════════════════
    // GÉNÉRATION DU CODE POUR CHAQUE WIDGET
    // ═══════════════════════════════════════════════════════════════════════
    int widget_count = cJSON_GetArraySize(widgets);
    for (int i = 0; i < widget_count; i++) {
        cJSON* widget = cJSON_GetArrayItem(widgets, i);
        cJSON* type = cJSON_GetObjectItem(widget, "type");

        if (!type || !cJSON_IsString(type)) {
            continue;
        }

        const char* type_str = type->valuestring;

        if (strcmp(type_str, "increment") == 0) {
            generer_code_increment(f, widget);
        } else if (strcmp(type_str, "toggle") == 0) {
            generer_code_toggle(f, widget);
        } else if (strcmp(type_str, "titre") == 0) {
            generer_code_titre(f, widget);
        } else if (strcmp(type_str, "separator") == 0) {
            generer_code_separator(f, widget);
        } else if (strcmp(type_str, "preview") == 0) {
            generer_code_preview(f, widget);
        } else if (strcmp(type_str, "button") == 0) {
            generer_code_button(f, widget);
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // FIN DE LA FONCTION
    // ═══════════════════════════════════════════════════════════════════════
    fprintf(f, "    // Tous les widgets ont été créés avec succès\n");
    fprintf(f, "}\n");

    fclose(f);
    liberer_callback_collection(&callbacks);
    cJSON_Delete(root);

    printf("✅ Code C généré avec succès : ../src/generated_widgets.c\n");
    printf("   %d widgets générés\n", widget_count);
    printf("   %d callbacks INT, %d callbacks BOOL, %d callbacks VOID\n",
           callbacks.count_int, callbacks.count_bool, callbacks.count_void);

    return true;
}
