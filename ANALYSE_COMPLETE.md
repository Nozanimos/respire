# Analyse Complète : Chaîne de Gestion des Widgets

## 1. FLUX DE DONNÉES

### Chargement initial
```
JSON (widgets_config.json)
    ↓ json_config_loader.c : charger_widgets_depuis_json()
    ↓ Création des widgets avec positions base_x, base_y
widget_list (WidgetList)
    ↓ Chaque widget stocke:
        - base_x, base_y : positions JSON (IMMUABLES)
        - x, y : positions actuelles (MUTABLES)
        - width, height : dimensions
    ↓ settings_panel.c : recalculate_widget_layout()
    ↓ Affichage
```

### États du panneau
1. **Normal (dépilé)** : widgets aux positions JSON (base_x, base_y)
2. **Empilé** : widgets repositionnés verticalement (x, y modifiés)
3. **Transition** : dépilement en cours

## 2. PROBLÈMES IDENTIFIÉS

### 2.1 Logique de dépilement incohérente
**Fichier** : `settings_panel.c:938-983`

**Problème** :
```c
// Ligne 946-947 : Preview
w->base.x = w->base.base_x;  // OK : Position absolue
w->base.y = w->base.base_y;

// Ligne 953-954 : Increment
w->base.x = w->base.base_x;  // OK : Position absolue
w->base.y = w->base.base_y;

// Ligne 975-976 : Separator
w->base.y = w->base.base_y;  // OK
w->base.width = panel_width - w->base_start_margin - w->base_end_margin;  // DYNAMIQUE!
```

**Incohérence** : Le separator recalcule sa largeur dynamiquement, mais pas les autres widgets.

### 2.2 Appels multiples à recalculate_widget_layout()
**Fichier** : `settings_panel.c:904`

**Problème** : La fonction est appelée depuis :
- `update_panel_scale()` (ligne 714) : lors du resize
- `reload_widgets_from_json()` (ligne 1478)
- Potentiellement depuis d'autres endroits

**Conséquence** : Après un dépilement, la fonction est immédiatement rappelée → re-test de collision → ré-empilement.

### 2.3 Flag skip_collision_check ne persiste pas assez longtemps
**Fichier** : `settings_panel.c:913-917`

**Problème** :
```c
if (panel->skip_collision_check) {
    panel->skip_collision_check = false;  // Réinitialisé immédiatement!
    goto calculate_heights;
}
```

Le flag est réinitialisé **avant même** que la frame suivante soit rendue. Si `recalculate_widget_layout()` est appelé plusieurs fois dans la même frame (ce qui arrive avec le resize), le flag ne protège que le premier appel.

### 2.4 Mémoire persistante panel_width_when_stacked mal utilisée
**Fichier** : `settings_panel.c:931-933`

**Problème** :
```c
if (panel->widgets_stacked &&
    panel->panel_width_when_stacked > 0 &&
    panel_width >= panel->panel_width_when_stacked + UNSTACK_MARGIN) {
```

**Contradiction** : Si `panel_width_when_stacked` est sauvegardé lors du premier empilement (ex: 328px), et qu'on réduit ensuite à 300px puis qu'on élargit à 410px, on dépile car `410 >= 328+80`. Mais les widgets JSON peuvent nécessiter plus que 410px ! La largeur sauvegardée ne reflète **pas** la largeur minimale requise par le JSON.

### 2.5 Pas de réinitialisation des coordonnées après réouverture panneau
**Fichier** : `settings_panel.c:427-463`

**Problème** : Quand on rouvre le panneau (ligne 427), on ne réinitialise pas les positions des widgets. Si le panneau était empilé, les widgets gardent leurs positions empilées.

## 3. CAUSES RACINES

### 3.1 Confusion entre 3 types de largeurs
1. **panel_width** : Largeur actuelle du panneau
2. **panel_width_when_stacked** : Largeur au moment de l'empilement (dynamique)
3. **min_width_for_unstack** : Largeur minimale JSON (calculée une fois)

**Problème** : On utilise `panel_width_when_stacked` comme référence pour dépiler, alors qu'on devrait utiliser `min_width_for_unstack` (largeur minimale garantissant pas de collision).

### 3.2 Pas de notion d'état stable
Le code ne distingue pas :
- État "en transition" (dépilement en cours)
- État "stable" (positions finales atteintes)

**Conséquence** : Impossible de savoir si les widgets sont dans un état transitoire ou final.

### 3.3 Test de collision appliqué aux positions actuelles, pas aux positions cibles
**Fichier** : `settings_panel.c:1088-1140`

**Problème** : On construit les rectangles de collision (ligne 1092-1120) avec les positions **actuelles** des widgets (`w->base.x`, `w->base.y`).

Mais après un dépilement, ces positions sont les positions **JSON**, et on les teste pour collision ! Or si `panel_width < min_width_JSON`, il y aura forcément collision → ré-empilement.

## 4. SOLUTION PROPOSÉE : RÉÉCRITURE PROPRE

### 4.1 Principe de base : Séparation des responsabilités

```
recalculate_widget_layout() :
  - Lit widgets_stacked (état actuel)
  - Décide action (empiler/dépiler/rien)
  - Modifie positions (x, y)
  - Met à jour widgets_stacked (nouvel état)

update_panel_scale() :
  - Gère le redimensionnement du panneau
  - Appelle recalculate_widget_layout() UNE SEULE FOIS
```

### 4.2 Logique simplifiée

```c
void recalculate_widget_layout(SettingsPanel* panel) {
    int panel_width = panel->rect.w;

    // ÉTAPE 1 : Décider de l'action
    bool should_stack = false;
    bool should_unstack = false;

    if (!panel->widgets_stacked) {
        // État DÉPILÉ : Tester si on doit empiler
        if (panel_width < panel->min_width_for_unstack) {
            should_stack = true;
        }
    } else {
        // État EMPILÉ : Tester si on doit dépiler
        if (panel_width >= panel->min_width_for_unstack + MARGIN) {
            should_unstack = true;
        }
    }

    // ÉTAPE 2 : Appliquer l'action
    if (should_unstack) {
        restore_json_positions(panel);  // Restaurer positions JSON
        panel->widgets_stacked = false;
    } else if (should_stack) {
        stack_widgets_vertically(panel);  // Empiler verticalement
        panel->widgets_stacked = true;
    }
    // Sinon : ne rien faire (état stable)
}
```

### 4.3 Supprimer toutes les variables inutiles

**À SUPPRIMER** :
- `panel_width_when_stacked` : Remplacé par `min_width_for_unstack` (calculé une fois)
- `skip_collision_check` : Plus nécessaire avec la nouvelle logique
- Test de collision après dépilement : Plus nécessaire

**À GARDER** :
- `widgets_stacked` : Indique l'état actuel (empilé ou non)
- `min_width_for_unstack` : Largeur minimale JSON (calculée au chargement)

### 4.4 Hystérésis simple et claire

```c
// Empiler si largeur < min_width
if (panel_width < panel->min_width_for_unstack) {
    stack();
}

// Dépiler si largeur >= min_width + MARGIN
if (panel_width >= panel->min_width_for_unstack + MARGIN) {
    unstack();
}
```

**Avantage** : Pas de boucle infinie car :
- Si empilé à 380px (min_width=393), on dépile à 393+50=443px
- À 443px, pas de collision garantie (>= 393)
- Pas de ré-empilement

## 5. BUGS SPÉCIFIQUES À CORRIGER

### 5.1 Boutons Appliquer/Annuler
**Fichier** : `settings_panel.c:639-655`

**Problème** : Les boutons sont dans `widget_list` avec `BUTTON_ANCHOR_BOTTOM`, mais lors du dépilement (ligne 979), on ne restaure PAS les boutons ancrés en bas.

**Fix** :
```c
case WIDGET_TYPE_BUTTON:
    if (node->widget.button_widget) {
        ButtonWidget* w = node->widget.button_widget;
        w->base.x = w->base.base_x;

        // Gérer l'ancrage
        if (w->y_anchor == BUTTON_ANCHOR_BOTTOM) {
            w->base.y = screen_height - w->base_y - w->base_height / 2;
        } else {
            w->base.y = w->base.base_y;
        }
    }
    break;
```

### 5.2 Separator largeur dynamique incohérente
**Fichier** : `settings_panel.c:975-977`

**Problème** : Le separator recalcule sa largeur lors du dépilement, mais ça devrait être fait dans tous les cas (empilé ou dépilé).

**Fix** : Calculer la largeur du separator dans une fonction séparée, appelée après chaque modification de layout.

## 6. PLAN D'ACTION

### Phase 1 : Nettoyer settings_panel.c
1. Supprimer `panel_width_when_stacked`
2. Supprimer `skip_collision_check`
3. Simplifier `recalculate_widget_layout()` :
   - Une seule condition pour empiler : `panel_width < min_width`
   - Une seule condition pour dépiler : `panel_width >= min_width + MARGIN`
   - Supprimer test de collision après dépilement

### Phase 2 : Extraire fonctions
```c
static void restore_json_positions(SettingsPanel* panel);
static void stack_widgets_vertically(SettingsPanel* panel);
static void update_separator_width(SettingsPanel* panel);
static void update_button_positions(SettingsPanel* panel);
```

### Phase 3 : Garantir un seul appel par frame
- Ajouter flag `layout_dirty` dans SettingsPanel
- Marquer dirty lors du resize
- Recalculer seulement si dirty
- Reset dirty après recalcul

## 7. CODE PROPRE FINAL

### settings_panel.h
```c
typedef struct {
    // ... autres champs ...

    bool widgets_stacked;      // État actuel (empilé ou non)
    int min_width_for_unstack; // Largeur min JSON (calculée une fois)
    bool layout_dirty;         // Layout nécessite recalcul

    // SUPPRIMER :
    // int panel_width_when_stacked;
    // bool skip_collision_check;
} SettingsPanel;
```

### settings_panel.c : recalculate_widget_layout()
```c
void recalculate_widget_layout(SettingsPanel* panel) {
    if (!panel || !panel->widget_list) return;
    if (!panel->layout_dirty) return;  // Déjà à jour

    int panel_width = panel->rect.w;
    const int MARGIN = 60;  // Hystérésis

    // Décider de l'action selon l'état actuel
    if (!panel->widgets_stacked) {
        // Dépilé : tester si on doit empiler
        if (panel_width < panel->min_width_for_unstack) {
            stack_widgets_vertically(panel);
            panel->widgets_stacked = true;
        }
    } else {
        // Empilé : tester si on doit dépiler
        if (panel_width >= panel->min_width_for_unstack + MARGIN) {
            restore_json_positions(panel);
            panel->widgets_stacked = false;
        }
    }

    // Toujours recalculer éléments dynamiques
    update_separator_width(panel);
    update_button_positions(panel);
    calculate_content_height(panel);

    panel->layout_dirty = false;
}
```

## 8. RÉSUMÉ

**Problème principal** : Trop de variables d'état, logique complexe avec patches sur patches.

**Solution** :
- Simplifier à 2 variables : `widgets_stacked` (état) + `min_width_for_unstack` (référence)
- Hystérésis simple : empiler si < min, dépiler si >= min+marge
- Pas de test de collision après dépilement (inutile si largeur suffisante)
- Extraire fonctions pour clarté
- Flag `layout_dirty` pour éviter recalculs multiples

**Avantages** :
- Code simple et prévisible
- Pas de boucle infinie possible
- Facile à débugger
- Performant (un seul recalcul par frame)
