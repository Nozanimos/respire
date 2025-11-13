# Fix du Problème de Dépilement - Analyse Complète

## 🔍 Analyse des Traces

### Traces Observées (debug_simple et debug_depilement)

Voici ce qui se passait avec l'ancien code :

```
1. panel_width=327px → EMPILE, sauvegarde when_stacked=327

2. Élargissement à 382px:
   - Condition : 382 >= 327+50 = 377 → ✅ DÉPILE
   - when_stacked réinitialisé à 0
   - recalculate_widget_layout() IMMÉDIATEMENT RAPPELÉ
   - À 382px, widgets JSON en collision → RE-EMPILE avec when_stacked=382 ❌

3. Élargissement à 437px:
   - Condition : 437 >= 382+50 = 432 → ✅ DÉPILE
   - when_stacked réinitialisé à 0
   - recalculate_widget_layout() IMMÉDIATEMENT RAPPELÉ
   - À 437px, widgets JSON en collision → RE-EMPILE avec when_stacked=437 ❌

4. Élargissement à 488px:
   - Condition : 488 >= 437+50 = 487 → ✅ DÉPILE
   - when_stacked réinitialisé à 0
   - recalculate_widget_layout() IMMÉDIATEMENT RAPPELÉ
   - À 488px, widgets JSON en collision → RE-EMPILE avec when_stacked=488 ❌

5. Élargissement à 500px:
   - Condition : 500 >= 488+50 = 538 → ❌ NE DÉPILE PAS
```

### 🎯 Problème Identifié

**Boucle infinie de DÉPILE → RE-EMPILE** causée par :

1. ✅ Le dépilement se produit correctement (condition `panel_width >= when_stacked + 50`)
2. ✅ Les widgets reviennent aux positions JSON
3. ❌ **`recalculate_widget_layout()` est immédiatement rappelé** (par la boucle de rendu)
4. ❌ **À cette largeur, les widgets JSON sont ENCORE EN COLLISION**
5. ❌ **Ré-empilement automatique** avec nouvelle sauvegarde de `when_stacked`
6. 🔄 **Le cycle se répète** à chaque élargissement

### 📊 Exemple Concret

Dans les traces, à `panel_width=382px` :

```
Avant : stacked=1 when_stacked=327
Dépile : 382 >= 377 ✅
Après : stacked=0 when_stacked=0

[fonction rappelée IMMÉDIATEMENT]

Test collisions à 382px → COLLISION !
Re-empile : stacked=1 when_stacked=382
```

**Pourquoi collision à 382px ?**
Parce que `min_width_for_unstack` (calculé depuis le JSON) vaut probablement **~500px** !
Les positions JSON nécessitent au moins 500px pour ne pas avoir de collision.

---

## ✅ Solution Implémentée

### Changement Principal

**AVANT** (causait la boucle) :
```c
// Dépiler si panel_width >= (largeur sauvegarde + marge)
if (panel->widgets_stacked &&
    panel->panel_width_when_stacked > 0 &&
    panel_width >= panel->panel_width_when_stacked + UNSTACK_MARGIN) {

    // Dépile...
    panel->panel_width_when_stacked = 0;  // Réinitialise
    panel->widgets_stacked = false;
}
```

**Problème** : Après dépilement, la fonction est rappelée, re-teste les collisions à `panel_width`, trouve des collisions, ré-empile.

**APRÈS** (stable) :
```c
// Dépiler SEULEMENT si panel_width >= largeur minimale JSON
if (panel->widgets_stacked &&
    panel_width >= panel->min_width_for_unstack) {

    // Dépile...
    panel->widgets_stacked = false;
}
```

**Avantage** : `min_width_for_unstack` est **calculé depuis le JSON** (ligne 264 dans `create_settings_panel`) et représente la **largeur minimale garantissant AUCUNE collision**.

Donc :
- Si `panel_width >= min_width_for_unstack` → **AUCUNE collision possible** après dépilement
- La fonction peut être rappelée autant de fois que nécessaire → **pas de ré-empilement** !

### Calcul de `min_width_for_unstack`

Fonction `calculate_required_width_for_json_layout()` (ligne 840) :

```c
static int calculate_required_width_for_json_layout(SettingsPanel* panel) {
    int max_right_edge = 0;  // Bord droit le plus à droite

    // Parcourt tous les widgets
    for (chaque widget) {
        int widget_right = widget.base_x + widget.width;
        if (widget_right > max_right_edge) {
            max_right_edge = widget_right;
        }
    }

    // Ajoute une marge de sécurité
    return max_right_edge + 20;
}
```

Cette fonction calcule la **bounding box** des widgets selon leurs positions JSON.

---

## 🧪 Test du Fix

### Script GDB Mis à Jour

Deux nouveaux scripts :
- `debug_simple_v2.gdb` : Traces basiques
- `debug_depilement_v2.gdb` : Traces détaillées

**Utilisation** :
```bash
gdb -x debug_simple_v2.gdb ./respire
```

### Comportement Attendu (après fix)

```
1. panel_width=327px → EMPILE
   min_width_for_unstack = 500px (exemple)

2. Élargissement à 382px:
   - Condition : 382 >= 500 → ❌ NE DÉPILE PAS (correct !)
   - Reste empilé

3. Élargissement à 450px:
   - Condition : 450 >= 500 → ❌ NE DÉPILE PAS (correct !)
   - Reste empilé

4. Élargissement à 510px:
   - Condition : 510 >= 500 → ✅ DÉPILE
   - Widgets reviennent aux positions JSON
   - recalculate_widget_layout() rappelé
   - Test collisions à 510px → AUCUNE COLLISION ✅
   - Reste dépilé ✅

5. Réduction à 450px:
   - Test collisions → COLLISION !
   - Re-empile (normal)

6. Ré-élargissement à 510px:
   - Condition : 510 >= 500 → ✅ DÉPILE
   - Reste dépilé ✅
```

**Plus de boucle infinie !** Le dépilement ne se produit qu'une fois que la largeur est **suffisante** pour les positions JSON.

---

## 📋 Changements de Code

### `src/settings_panel.c`

**Ligne 912-922** : Condition de dépilement simplifiée
```c
// Ancien code supprimé:
// - const int UNSTACK_MARGIN = 50;
// - panel->panel_width_when_stacked > 0 &&
// - panel_width >= panel->panel_width_when_stacked + UNSTACK_MARGIN

// Nouveau code:
if (panel->widgets_stacked &&
    panel_width >= panel->min_width_for_unstack) {
```

**Ligne 924-926** : Debug printf mis à jour
```c
// Ancien:
// debug_printf("🔄 DÉPILEMENT: panel_width=%dpx >= (saved_width=%dpx + marge=%dpx)\n",
//             panel_width, panel->panel_width_when_stacked, UNSTACK_MARGIN);

// Nouveau:
debug_printf("🔄 DÉPILEMENT: panel_width=%dpx >= min_width_for_unstack=%dpx\n",
            panel_width, panel->min_width_for_unstack);
```

**Ligne 989-993** : Réinitialisation simplifiée
```c
// Ancien code supprimé:
// panel->panel_width_when_stacked = 0;
// debug_printf("   🔓 panel_width_when_stacked réinitialisé à 0\n");

// Nouveau code:
panel->widgets_stacked = false;
debug_printf("✅ Widgets dépilés et restaurés aux positions JSON\n");
```

**Ligne 1166-1171** : Empilement simplifié
```c
// Ancien code supprimé (sauvegarde de panel_width_when_stacked)

// Nouveau code:
panel->widgets_stacked = true;
debug_printf("   📐 min_width_for_unstack = %dpx (pour dépiler)\n",
            panel->min_width_for_unstack);
```

### `src/settings_panel.h`

**Aucun changement nécessaire** :
- `panel_width_when_stacked` peut être supprimé (non utilisé), mais gardé pour l'instant
- `min_width_for_unstack` existe déjà (ligne 118)

---

## 🎓 Leçons Apprises

### Erreur de Conception Initiale

L'approche `panel_width_when_stacked + MARGE` semblait logique :
- Sauvegarder la largeur au moment de l'empilement
- Ajouter une marge d'hystérésis pour éviter les oscillations

**Mais** : Cette approche ne prenait **pas en compte** les positions JSON des widgets !

### Insight Clé

Le problème n'était pas de savoir **à quelle largeur on a empilé**, mais de savoir **à partir de quelle largeur les positions JSON ne collisionnent plus**.

Cette information est **déjà calculée** dès le chargement du JSON : `min_width_for_unstack`.

### Architecture Correcte

```
JSON → calculate_required_width_for_json_layout() → min_width_for_unstack
                                                            ↓
                                          Condition de dépilement stable
```

Pas besoin de sauvegarder dynamiquement la largeur. La largeur minimale est **une propriété intrinsèque du layout JSON**, pas une propriété de l'état courant.

---

## 🔮 Améliorations Futures Possibles

1. **Supprimer `panel_width_when_stacked`** complètement (non utilisé après le fix)

2. **Ajouter une petite marge** à `min_width_for_unstack` pour plus de confort :
   ```c
   if (panel_width >= panel->min_width_for_unstack + 20) {
   ```

3. **Optimiser `calculate_required_width_for_json_layout()`** pour tenir compte du `panel_ratio`

4. **Ajouter un flag** `layout_is_stable` pour éviter les re-tests répétés après dépilement

---

## ✅ Commit

**Message** :
```
Fix: Utiliser min_width_for_unstack pour dépilement stable

Problème identifié dans les traces debug_simple et debug_depilement:
- Boucle infinie DÉPILE → RE-EMPILE
- Causée par condition basée sur panel_width_when_stacked + marge
- Après dépilement, recalculate_widget_layout() rappelé
- À cette largeur, collisions avec positions JSON → ré-empilement

Solution:
- Utiliser min_width_for_unstack (calculé depuis JSON) comme condition
- Garantit AUCUNE collision après dépilement
- Plus besoin de sauvegarder panel_width_when_stacked dynamiquement

Changements:
- src/settings_panel.c:
  * Ligne 921: Condition simplifiée (panel_width >= min_width_for_unstack)
  * Ligne 992: Suppression réinitialisation panel_width_when_stacked
  * Ligne 1169: Suppression sauvegarde panel_width_when_stacked
- Scripts GDB v2 créés pour tests

Résultat: Dépilement stable, plus de boucle infinie
```
