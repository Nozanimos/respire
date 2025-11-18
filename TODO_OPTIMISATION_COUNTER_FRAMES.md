# TODO - Optimisation precomputed_counter_frames

## 🐛 Problème actuel

**4 tableaux identiques alloués inutilement** :
```
✅ Compteur précompute : 1080 frames, flags transitions scale_min/max générés (×4)
```

### Code actuel (main.c:98-108)
```c
HexagoneNode* node = hex_list->first;
while (node) {
    precompute_counter_frames(node, ...);  // Appelé 4 fois
    node = node->next;
}
```

### Utilisation (renderer.c:649)
```c
HexagoneNode* first_node = app->hexagones->first;  // Utilise UNIQUEMENT le premier
counter_render(..., first_node, ...);
```

### Conséquence
- **4 allocations** : 4 × 1080 frames × 24 bytes = **103 KB gaspillés**
- **4 calculs identiques** de `precompute_counter_frames()`
- **3 tableaux jamais utilisés**

## ✅ Solution proposée

### Option A : Un seul tableau global (RECOMMANDÉE)

**1. Créer une structure globale dans AppState** (renderer.h) :
```c
typedef struct {
    CounterFrame* frames;  // Tableau unique partagé
    int total_frames;      // Nombre de frames (1080)
} GlobalCounterFrames;
```

**2. Modifier AppState** :
```c
typedef struct {
    ...
    CounterState* breath_counter;
    GlobalCounterFrames* counter_frames;  // 🆕 Tableau unique
    ...
} AppState;
```

**3. Allouer une seule fois** (main.c) :
```c
app.counter_frames = malloc(sizeof(GlobalCounterFrames));
app.counter_frames->frames = malloc(total_frames * sizeof(CounterFrame));
app.counter_frames->total_frames = total_frames;

// Calculer une seule fois (utiliser le premier hexagone comme référence)
precompute_counter_frames_global(
    hex_list->first,  // Hexagone de référence
    app.counter_frames,
    TARGET_FPS,
    config.breath_duration
);
```

**4. Modifier counter_render** pour utiliser le tableau global :
```c
void counter_render(CounterState* counter, SDL_Renderer* renderer,
                    int center_x, int center_y, int hex_radius,
                    GlobalCounterFrames* counter_frames,  // 🆕 Au lieu de HexagoneNode
                    int current_cycle,  // 🆕 Passé depuis l'extérieur
                    float scale_factor)
```

**Avantages** :
- ✅ Une seule allocation (26 KB au lieu de 103 KB)
- ✅ Un seul calcul (4× plus rapide à l'init)
- ✅ Plus clair : le compteur n'est pas lié à UN hexagone spécifique
- ✅ Évite les bugs si on lit le mauvais hexagone

### Option B : Calculer uniquement pour le premier hexagone

**Plus simple mais moins propre** :
```c
// main.c:98-108 - Modifier la boucle
HexagoneNode* node = hex_list->first;
precompute_counter_frames(node, ...);  // Une seule fois
```

**Problème** : Les autres hexagones ont un pointeur `precomputed_counter_frames` NULL ou invalide.

### Option C : Pointeurs partagés

**Tous les hexagones pointent vers le même tableau** :
```c
CounterFrame* shared_frames = malloc(total_frames * sizeof(CounterFrame));
// Calculer une fois
precompute_counter_frames_to_array(shared_frames, ...);

// Partager entre tous les hexagones
HexagoneNode* node = hex_list->first;
while (node) {
    node->precomputed_counter_frames = shared_frames;  // Pointeur partagé
    node = node->next;
}
```

**Problème** : Risque de double-free si on ne gère pas bien la destruction.

## 📝 Implémentation recommandée

**Option A** est la meilleure car :
1. Clarté du code (le compteur n'est pas dans HexagoneNode)
2. Pas de risque de double-free
3. Facile à debugger

## 🔧 Fichiers à modifier

1. **renderer.h** : Ajouter `GlobalCounterFrames`
2. **precompute_list.h** : Nouvelle fonction `precompute_counter_frames_global()`
3. **precompute_list.c** : Implémenter la fonction
4. **main.c** : Allouer et calculer une seule fois
5. **counter.h** : Modifier signature de `counter_render()`
6. **counter.c** : Utiliser `GlobalCounterFrames` au lieu de `HexagoneNode->precomputed_counter_frames`
7. **renderer.c** : Passer `app->counter_frames` et `first_node->current_cycle`

## ⚠️ Note importante

**Vérifier d'abord** : Est-ce que tous les hexagones ont exactement les mêmes scales ?
- Si OUI → Un seul tableau suffit
- Si NON → Il faut comprendre pourquoi et peut-être garder 4 tableaux

Dans le code actuel, `total_frames` est calculé une seule fois (precompute_list.c:115), donc tous les hexagones ont le même nombre de frames (1080). Les scales devraient aussi être identiques car ils utilisent la même formule `sinusoidal_movement()`.

**À confirmer** : Les 4 hexagones ont-ils des `animation->scale_min` et `animation->scale_max` différents ?
