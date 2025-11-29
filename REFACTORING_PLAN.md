# Plan de Refactorisation - Projet Respire

## Vue d'ensemble

**Projet**: Application de respiration guidée en C avec SDL2/Cairo
**Taille**: ~14 000 lignes de code
**Date d'analyse**: 2025-11-29
**Niveau de dette technique**: **ÉLEVÉ**

---

## Statistiques du code

```
src/core/          : ~10 000 LOC (widgets, renderer, config)
src/instances/     : ~1 000 LOC (techniques de respiration)
src/json_editor/   : ~2 500 LOC (éditeur JSON intégré)
src/main.c         : 715 LOC

Fichiers les plus volumineux:
- settings_panel.c : 1235 LOC
- widget_list.c    : 792 LOC
- renderer.c       : 756 LOC
- main.c           : 715 LOC
```

---

## 🔴 PRIORITÉ CRITIQUE - Bugs potentiels

### 1. Fuites mémoire (50+ occurrences)

**Problème**: Pattern systématique de malloc() sans vérification complète du free correspondant.

**Fichiers affectés**:
- `src/core/geometry.c` (8 malloc)
- `src/core/precompute_list.c` (5 malloc)
- `src/core/stats_panel.c` (3 malloc, dont 1 dans boucle)
- `src/core/widget_list.c` (8 malloc)
- `src/core/counter_cache.c` (3 malloc, dont 1 dans boucle)

**Exemple problématique** (`widget_list.c:67-95`):
```c
WidgetNode* node = malloc(sizeof(WidgetNode));
if (!node) {
    debug_printf("❌ Erreur allocation nœud widget\n");
    return false;
}
node->widget.increment_widget = create_config_widget(...);
if (!node->widget.increment_widget) {
    // BUG: node n'est jamais libéré ici!
    free((void*)node->id);
    free((void*)node->display_name);
    free(node);  // ← Manquait dans certains cas
    return false;
}
```

**Action requise**:
- [ ] Auditer tous les malloc() et vérifier les chemins d'erreur
- [ ] Créer un wrapper `safe_malloc()` avec tracking
- [ ] Implémenter un système de cleanup automatique (arènes mémoire ou RAII-like)
- [ ] Utiliser Valgrind pour détecter les fuites

**Localisation**: `src/core/geometry.c:129,137,138,286,413,442` et 40+ autres

---

### 2. Variables globales statiques (anti-pattern)

**Problème**: Utilisation de variables globales statiques pour passer des callbacks, rendant le code non thread-safe et difficile à tester.

**Fichier**: `src/core/settings_panel.c:31-42`

```c
static SettingsPanel* current_panel_for_callbacks = NULL;
static AppConfig* current_main_config_for_callbacks = NULL;
static TimerState** current_session_timer = NULL;
static StopwatchState** current_session_stopwatch = NULL;
static TimerState** current_retention_timer = NULL;
static BreathCounter** current_breath_counter = NULL;
static int* current_total_sessions = NULL;
static HexagoneList** current_hexagones = NULL;
static int* current_screen_width = NULL;
static int* current_screen_height = NULL;
```

**Conséquences**:
- Non thread-safe
- Impossible à tester unitairement
- Couplage fort entre modules
- Violation du principe de responsabilité unique

**Action requise**:
- [ ] Créer une structure `CallbackContext` contenant tous ces pointeurs
- [ ] Passer le contexte en paramètre aux callbacks via `void* user_data`
- [ ] Supprimer toutes les variables globales statiques
- [ ] Refactorer les signatures de callbacks pour accepter un contexte

**Localisation**: `src/core/settings_panel.c:31-42,47-59`

---

### 3. Gestion d'erreurs incohérente

**Problème**: Pattern répété 40+ fois sans cleanup des allocations précédentes.

**Pattern détecté**:
```c
if (!ptr) {
    debug_printf("❌ Erreur allocation\n");
    return NULL;  // ← Pas de cleanup!
}
```

**Fichiers affectés** (40+ occurrences):
- widget_list.c, widget.c, toggle_widget.c, button_widget.c, label_widget.c
- json_editor (tous les fichiers)
- geometry.c, session_card.c

**Action requise**:
- [ ] Créer des macros `CLEANUP_ON_ERROR` ou `GOTO_CLEANUP`
- [ ] Utiliser un pattern `goto cleanup` cohérent
- [ ] Implémenter un système de destructeurs automatiques

**Localisation**: Voir résultats de recherche regex `if\s*\(!?\w+\)\s*\{\s*debug_printf\([^)]+\);\s*return`

---

## 🟠 PRIORITÉ MAJEURE - Dette technique

### 4. Fonctions monolithiques (>100 lignes)

**Problème**: Fonctions géantes impossible à maintenir et tester.

**Fonctions identifiées**:
- `initialize_app()` - `renderer.c:212-450+` (~200+ lignes)
  - Fait: SDL, TTF, fonts, window, renderer, images, panels, config
  - **Action**: Décomposer en 7 fonctions (`init_sdl`, `init_fonts`, `init_window`, etc.)

- `create_settings_panel()` - `settings_panel.c:318-450+` (~130+ lignes)
  - Fait: alloc, hot reload, scroll, polices, widgets JSON, fond, icône
  - **Action**: Décomposer en 5 fonctions (`init_panel_base`, `init_panel_fonts`, `load_panel_widgets`, etc.)

- `handle_app_events()` - `renderer.c:450-927` (~477 lignes!)
  - **Action**: Extraire les handlers par type d'événement

- `render_app()` - `renderer.c:946-1166` (~220 lignes)
  - **Action**: Décomposer par écran/état

**Action requise**:
- [ ] Appliquer la règle: 1 fonction = 1 responsabilité
- [ ] Limite stricte: 50 lignes max par fonction
- [ ] Utiliser des fonctions privées statiques pour découpage

**Localisation**: `src/core/renderer.c:212,450,946` + `src/core/settings_panel.c:318`

---

### 5. Fichiers trop volumineux (>700 LOC)

**Problème**: Fichiers difficiles à naviguer et maintenir.

**Fichiers concernés**:
1. `settings_panel.c` - 1235 LOC
   - **Action**: Séparer en `settings_panel_core.c`, `settings_panel_callbacks.c`, `settings_panel_layout.c`

2. `widget_list.c` - 792 LOC
   - **Action**: Séparer en `widget_list_core.c`, `widget_list_add.c`, `widget_list_query.c`

3. `renderer.c` - 756 LOC
   - **Action**: Séparer en `app_init.c`, `app_events.c`, `app_render.c`

4. `main.c` - 715 LOC
   - **Action**: Extraire la logique métier vers des modules dédiés

**Règle**: Maximum 500 LOC par fichier .c

**Action requise**:
- [ ] Découper les 4 fichiers monolithiques
- [ ] Créer des headers internes (`*_internal.h`) pour fonctions privées
- [ ] Maintenir la cohésion: un fichier = un module logique

---

### 6. Dépendances circulaires

**Problème**: Architecture avec dépendances bidirectionnelles empêchant la modularité.

**Cycles détectés**:

1. **core ↔ json_editor**
   ```
   core/renderer.h → json_editor/json_editor.h
   json_editor/*.c → core/debug.h, core/widget_base.h
   ```
   - **Action**: Core ne doit PAS dépendre de json_editor
   - **Solution**: Extraire json_editor en module externe, communiquer par interface

2. **core → instances (inversé)**
   ```
   core/renderer.c → instances/technique_instance.h
   core/renderer.c → instances/whm/whm.h
   ```
   - **Action**: Inverser la dépendance via polymorphisme
   - **Solution**: Créer interface `ITechnique` dans core, instances l'implémentent

**Action requise**:
- [ ] Dessiner le graphe de dépendances avec graphviz
- [ ] Appliquer le principe de dépendance inversée (DIP)
- [ ] Créer des interfaces claires entre modules
- [ ] Règle: `core` ne dépend de RIEN, tout dépend de `core`

**Localisation**: `src/core/renderer.h:12`, `src/core/renderer.c:8-9`

---

### 7. Duplication de code massive

**Problème**: Mêmes patterns répétés sans factorisation.

#### 7a. Pattern d'allocation de widgets (répété 8 fois)

**Fichiers**: `widget_list.c` - fonctions `add_*_widget()`

```c
// Répété dans: add_increment_widget, add_toggle_widget, add_label_widget, etc.
WidgetNode* node = malloc(sizeof(WidgetNode));
if (!node) {
    debug_printf("❌ Erreur allocation nœud widget\n");
    return false;
}
node->type = WIDGET_TYPE_XXX;
node->id = strdup(id);
node->display_name = strdup(display_name);
// ... configuration spécifique ...
```

**Action requise**:
- [ ] Créer fonction générique `create_widget_node_base(type, id, name)`
- [ ] Utiliser composition: chaque `add_*` appelle la base puis configure

#### 7b. Fonctions create_*_widget() (répété 8 fois)

**Fichiers**: `toggle_widget.c`, `button_widget.c`, `label_widget.c`, etc.

```c
Widget* widget = malloc(sizeof(Widget));
if (!widget) {
    debug_printf("❌ Erreur allocation Widget\n");
    return NULL;
}
// ... init commune ...
```

**Action requise**:
- [ ] Macro `DEFINE_WIDGET_CREATE(type)` ou fonction template-like
- [ ] Centraliser la logique d'allocation commune

#### 7c. Pattern debug_printf + return (répété 40+ fois)

**Action requise**:
- [ ] Macro `CHECK_ALLOC(ptr, msg)` ou `RETURN_IF_NULL(ptr, msg)`
- [ ] Réduire duplication de 40+ occurrences à <5

**Localisation**: Voir résultats recherche `if\s*\(!?\w+\)\s*\{\s*debug_printf`

---

## 🟡 PRIORITÉ MOYENNE - Maintenabilité

### 8. Constantes dupliquées

**Problème**: Mêmes constantes définies plusieurs fois.

**Cas identifiés**:

1. **NB_SIDE** défini 3 fois:
   - `src/core/config.h:12` → `#define NB_SIDE 6`
   - `src/core/geometry.c:15` → `#define NB_SIDE 6`
   - `src/core/precompute_list.c:10` → `#define NB_SIDE 6`

2. **PI** redéfini:
   - `src/core/geometry.c:17` → `#define PI 3.14159265358979323846`
   - Alors que `math.h` fournit `M_PI`

**Action requise**:
- [ ] Supprimer les duplications
- [ ] Centraliser dans `src/core/constants.h`
- [ ] Utiliser `M_PI` de `math.h` au lieu de redéfinir
- [ ] Ajouter `#ifndef` guards pour éviter redéfinitions futures

**Localisation**: `src/core/{config.h:12, geometry.c:15-17, precompute_list.c:10}`

---

### 9. Chemins hard-codés

**Problème**: Chemins absolus relatifs éparpillés dans le code.

**Occurrences**:
```c
// renderer.c:231
const char* font_path = "../fonts/arial/ARIAL.TTF";

// settings_panel.c:331
panel->json_config_path = "../config/widgets_config.json";

// settings_panel.c:411
SDL_Surface* bg_surface = IMG_Load("../img/settings_bg.png");

// main.c:94
initialize_app(&app, "Respiration guidée", "../img/nenuphar.jpg");
```

**Action requise**:
- [ ] Créer `src/core/paths.h` avec toutes les constantes de chemins
- [ ] Ou mieux: charger depuis fichier de config
- [ ] Utiliser un système de résolution de chemin robuste
- [ ] Supporter installation système (`/usr/share/respire/`, etc.)

**Localisation**: `src/core/renderer.c:231`, `src/core/settings_panel.c:331,411`, `src/main.c:94`

---

### 10. Includes avec chemins relatifs

**Problème**: 20+ occurrences de `#include "../xxx"` - fragile et dépendant de la structure.

**Exemples**:
```c
// json_editor/*.c
#include "../core/debug.h"
#include "../core/widget_base.h"

// instances/whm/*.c
#include "../../core/timer.h"
#include "../../core/counter.h"

// core/renderer.c
#include "../instances/technique_instance.h"
```

**Action requise**:
- [ ] Modifier `makefile` pour ajouter `-I$(SRC_DIR)` dans CFLAGS
- [ ] Remplacer tous les `../` par des chemins absolus depuis `src/`:
  ```c
  #include "core/debug.h"
  #include "core/widget_base.h"
  #include "instances/technique_instance.h"
  ```
- [ ] Vérifier que la compilation fonctionne après modification

**Localisation**: 20+ fichiers - voir résultats grep `^#include.*\.\.\//`

---

### 11. Commentaires décoratifs non standards

**Problème**: Utilisation excessive de séparateurs ASCII-art non conformes aux standards C.

**Exemples**:
```c
// ════════════════════════════════════════════════════════════════════════════
//  CALLBACKS POUR LES WIDGETS
// ════════════════════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────────────────
// CRÉATION DU NŒUD
// ─────────────────────────────────────────────────────────────────────────
```

**Problèmes**:
- Réduit la lisibilité (bruit visuel)
- Non standard (conventions C utilisent `/* */` ou `//`)
- Incompatible avec certains outils de documentation (Doxygen)

**Action requise**:
- [ ] Standardiser avec Doxygen ou commentaires simples:
  ```c
  /**
   * @brief Callbacks pour les widgets
   */
  ```
- [ ] Utiliser des séparateurs simples si nécessaire:
  ```c
  // --- Création du nœud ---
  ```

**Localisation**: `src/core/{settings_panel.c, widget_list.c, renderer.c}` (partout)

---

### 12. Constantes magiques

**Problème**: Nombres hard-codés sans explication.

**Exemples**:
```c
// renderer.c:254
1280, 720,  // Taille fixe pour dev

// settings_panel.c:341
panel->layout_threshold_width = 350;  // Seuil mode colonne

// session_card.c (nombreux exemples)
200, 280, 15, 0.67f, 10, 10, 10
```

**Action requise**:
- [ ] Convertir en `#define` avec noms explicites:
  ```c
  #define DEFAULT_WINDOW_WIDTH 1280
  #define DEFAULT_WINDOW_HEIGHT 720
  #define PANEL_LAYOUT_COLUMN_THRESHOLD 350
  ```
- [ ] Grouper par domaine dans headers appropriés

---

## 🟢 PRIORITÉ BASSE - Cosmétique

### 13. Code mort

**Problème**: Fichiers de backup et debug non supprimés.

**Fichiers**:
- `src/core/settings_panel.c.backup_current`
- `src/timer.c.verbose_bak`

**Action requise**:
- [ ] Supprimer tous les fichiers `.backup`, `.bak`, etc.
- [ ] Ajouter `*.backup` et `*.bak` au `.gitignore`
- [ ] Utiliser git pour l'historique au lieu de backups manuels

---

### 14. Commentaires contradictoires

**Problème**: Commentaires obsolètes ou contradictoires.

**Exemple** (`settings_panel.c:70-71`):
```c
// Sauvegarder immédiatement dans le fichier
// Sauvegarde uniquement lors du clic sur "Appliquer"
```

**Action requise**:
- [ ] Audit complet des commentaires
- [ ] Supprimer commentaires obsolètes
- [ ] Mettre à jour commentaires contradictoires

---

## Améliorations architecturales recommandées

### A. Séparation en modules clairs

**Structure actuelle**:
```
src/
├── core/       (mélange de tout)
├── instances/
└── json_editor/
```

**Structure proposée**:
```
src/
├── core/           (types de base, utilitaires)
│   ├── memory/     (gestion mémoire)
│   ├── error/      (gestion d'erreurs)
│   └── config/     (configuration)
├── ui/             (widgets, rendu)
│   ├── widgets/
│   ├── panels/
│   └── renderer/
├── techniques/     (ex-instances)
│   └── whm/
├── editor/         (ex-json_editor)
└── app/            (main, orchestration)
```

---

### B. Système de gestion d'erreurs cohérent

**Créer** `src/core/error/error.h`:
```c
typedef enum {
    ERR_NONE = 0,
    ERR_ALLOC,
    ERR_FILE_NOT_FOUND,
    ERR_INVALID_PARAM,
    // ...
} ErrorCode;

typedef struct {
    ErrorCode code;
    const char* message;
    const char* file;
    int line;
} Error;

#define SET_ERROR(err, code, msg) \
    do { \
        (err)->code = (code); \
        (err)->message = (msg); \
        (err)->file = __FILE__; \
        (err)->line = __LINE__; \
    } while(0)

#define CHECK_ALLOC(ptr, err) \
    do { \
        if (!(ptr)) { \
            SET_ERROR((err), ERR_ALLOC, "Memory allocation failed"); \
            goto cleanup; \
        } \
    } while(0)
```

---

### C. Système de contexte pour callbacks

**Créer** `src/ui/panels/settings_context.h`:
```c
typedef struct {
    SettingsPanel* panel;
    AppConfig* config;
    TimerState* session_timer;
    StopwatchState* session_stopwatch;
    TimerState* retention_timer;
    BreathCounter* breath_counter;
    int* total_sessions;
    HexagoneList* hexagones;
    int screen_width;
    int screen_height;
} SettingsContext;

// Callbacks prennent maintenant un contexte
typedef void (*IntCallback)(int value, void* context);
typedef void (*BoolCallback)(bool value, void* context);
```

---

### D. Pattern RAII-like pour C (cleanup automatique)

**Créer** `src/core/memory/auto_cleanup.h`:
```c
#define AUTO_FREE __attribute__((cleanup(auto_free_func)))

static inline void auto_free_func(void* ptr) {
    void** p = (void**)ptr;
    if (*p) {
        free(*p);
        *p = NULL;
    }
}

// Usage:
AUTO_FREE char* buffer = malloc(100);
// Sera automatiquement libéré en sortie de scope
```

---

## Plan d'exécution suggéré

### Phase 1 - Stabilisation (2-3 jours)
1. ✅ Fixer les fuites mémoire critiques
2. ✅ Supprimer variables globales statiques
3. ✅ Implémenter système d'erreurs cohérent
4. ✅ Tests avec Valgrind

### Phase 2 - Découpage (3-4 jours)
5. ✅ Séparer fonctions monolithiques
6. ✅ Découper fichiers >700 LOC
7. ✅ Réorganiser structure de dossiers

### Phase 3 - Dépendances (2-3 jours)
8. ✅ Résoudre dépendances circulaires
9. ✅ Fixer chemins d'includes
10. ✅ Tester compilation modulaire

### Phase 4 - Factorisation (2-3 jours)
11. ✅ Éliminer duplication code widgets
12. ✅ Centraliser constantes
13. ✅ Nettoyer commentaires

### Phase 5 - Polish (1 jour)
14. ✅ Supprimer code mort
15. ✅ Standardiser style
16. ✅ Documentation finale

**Durée totale estimée**: 10-14 jours de travail

---

## Métriques de succès

### Avant refactoring
- ❌ Lignes par fichier max: 1235
- ❌ Lignes par fonction max: ~477
- ❌ Fuites mémoire: 50+ potentielles
- ❌ Variables globales: 10
- ❌ Dépendances circulaires: 2
- ❌ Duplication: ~40+ patterns identiques

### Après refactoring (objectifs)
- ✅ Lignes par fichier max: <500
- ✅ Lignes par fonction max: <50
- ✅ Fuites mémoire: 0 (Valgrind clean)
- ✅ Variables globales: 0
- ✅ Dépendances circulaires: 0
- ✅ Duplication: <5 occurrences acceptables

---

## Notes importantes

1. **Tests**: Implémenter des tests unitaires AVANT de refactorer
2. **Git**: Faire des commits atomiques après chaque étape
3. **Revue**: Faire relire chaque PR de refactoring
4. **Documentation**: Mettre à jour la doc au fur et à mesure

---

**Généré le**: 2025-11-29
**Outils utilisés**: sequential-thinking, refactor, grep, cloc
**Status**: ⚠️ PRÊT POUR REFACTORING
