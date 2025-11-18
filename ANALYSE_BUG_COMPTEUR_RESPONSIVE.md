# ANALYSE DU BUG - Compteur en mode responsive

## 🐛 DESCRIPTION DU PROBLÈME

Lorsque la fenêtre est réduite à 200-300px :
- L'hexagone réduit correctement sa taille
- Le texte du compteur réduit également sa taille
- **BUG** : Au moment du changement de chiffre (1→2, 2→3, etc.), le compteur occupe **tout l'espace de l'hexagone** au lieu d'appliquer le ratio directement en apparaissant

## 📊 FLUX D'EXÉCUTION IDENTIFIÉ

### 1. Calcul du scale_factor responsive (`src/renderer.c:44-56`)
```c
float calculate_scale_factor(int width, int height) {
    float width_ratio = (float)width / REFERENCE_WIDTH;   // REFERENCE_WIDTH = 1280
    float height_ratio = (float)height / REFERENCE_HEIGHT; // REFERENCE_HEIGHT = 720
    float scale = (width_ratio < height_ratio) ? width_ratio : height_ratio;
    // Pour 200-300px → scale ≈ 0.15-0.23
}
```

### 2. Incrémentation du compteur (`src/counter.c:125-139`)
Le compteur s'incrémente quand :
```c
if (is_at_min_now && !counter->was_at_min_last_frame) {
    counter->current_breath++;  // 1→2, 2→3, etc.
}
```
- **Timing** : Détection à `scale_min` (poumons pleins, hexagone au minimum)
- **Source** : `is_at_min_now` vient de `current_frame->is_at_scale_min`
- **Frame** : Utilise `hex_node->current_cycle` comme index

### 3. Calcul de la taille du texte (`src/counter.c:172`)
```c
double font_size = counter->base_font_size * text_scale * scale_factor;
// base_font_size = taille de base (ex: 100)
// text_scale = valeur précomputée (entre scale_min=0.1 et scale_max=1.0)
// scale_factor = facteur responsive (0.15-0.23 pour 200-300px)
```

### 4. Rendu Cairo (`src/counter.c:194-259`)
- Surface temporaire 1x1 créée pour mesurer le texte (ligne 195)
- Mesure des extents avec `cairo_text_extents()` (ligne 204)
- Surface finale créée avec `text_width = extents.width + 10` (ligne 207)
- Texture SDL créée et affichée (ligne 238-251)

## 🔍 BUGS POTENTIELS IDENTIFIÉS

### BUG #1 : Désynchronisation frame/compteur au moment de l'incrémentation
**Localisation** : `src/counter.c:110-113`
```c
CounterFrame* current_frame = &hex_node->precomputed_counter_frames[hex_node->current_cycle];
bool is_at_min_now = current_frame->is_at_scale_min;
double text_scale = current_frame->text_scale;
```

**Problème suspecté** :
1. Le compteur s'incrémente à la détection de `scale_min` (ligne 127)
2. À ce moment, `text_scale` devrait être au **minimum** (≈0.1)
3. MAIS `hex_node->current_cycle` pourrait pointer vers une frame avec un `text_scale` différent
4. Décalage possible d'une frame entre :
   - L'incrémentation du compteur (`current_breath++`)
   - La lecture du bon `text_scale` depuis `precomputed_counter_frames`

**Impact** :
- Si `text_scale` est lu AVANT que `current_cycle` soit mis à jour → taille incorrecte
- Le texte pourrait être rendu avec le `text_scale` de la frame précédente (plus grand)

### BUG #2 : Application du scale_factor au moment du changement
**Localisation** : `src/counter.c:172`

**Problème suspecté** :
1. Le `scale_factor` est multiplié APRÈS le `text_scale`
2. Si le `text_scale` est incorrect (voir BUG #1), le problème est amplifié
3. En mode très réduit (200-300px), même une petite erreur de `text_scale` devient visible

**Exemple** :
```
Fenêtre 250px → scale_factor ≈ 0.195
base_font_size = 100

CAS NORMAL (text_scale = 0.1 au scale_min) :
font_size = 100 * 0.1 * 0.195 = 1.95 → très petit (correct)

CAS BUG (text_scale = 1.0 de la frame précédente) :
font_size = 100 * 1.0 * 0.195 = 19.5 → beaucoup trop grand !
```

### BUG #3 : Ordre d'exécution apply_precomputed_frame() vs counter_render()
**Localisation** : Boucle principale (probablement dans `src/main.c`)

**Problème suspecté** :
1. `apply_precomputed_frame()` incrémente `current_cycle` (ligne 232 de `precompute_list.c`)
2. `counter_render()` lit `current_cycle` pour obtenir `text_scale`
3. Si l'ordre d'appel est incorrect, décalage d'une frame

**Scénario problématique** :
```
Frame N : apply_precomputed_frame() → current_cycle = N (scale proche de max)
          counter détecte scale_min → current_breath++
          counter_render() lit current_cycle = N → text_scale = 0.9 (TROP GRAND)

Frame N+1 : current_cycle = N+1 → text_scale = 0.1 (CORRECT)
```

### BUG #4 : Précision de la détection de scale_min
**Localisation** : `src/precompute_list.c:428-432`
```c
bool close_to_min = fabs(current_scale - scale_min) < threshold;  // threshold = 3%
bool scale_increasing = current_scale > prev_scale;
bool is_at_min = close_to_min && scale_increasing;
```

**Problème suspecté** :
- La détection utilise un seuil de 3% de la plage
- Si le scale augmente rapidement, plusieurs frames consécutives pourraient être `is_at_min = true`
- Le flag `was_at_min_last_frame` devrait empêcher ça, MAIS si `text_scale` n'est pas synchronisé...

### BUG #5 : Recréation texture Cairo à chaque frame
**Localisation** : `src/counter.c:194-259`

**Problème suspecté** :
- La texture est recréée COMPLÈTEMENT à chaque frame
- FreeType + Cairo initialisés/détruits à chaque appel
- Au moment du changement de chiffre, la nouvelle texture est créée
- Possible lag d'une frame où la texture n'a pas encore la bonne taille

## 🎯 ZONES À TRACER AVEC GDB

Pour identifier le bug précisément, il faut tracer :

### 1. Au moment du changement de chiffre (scale_min)
- `hex_node->current_cycle` (index de frame)
- `current_frame->text_scale` (scale lu depuis precomputed)
- `current_frame->is_at_scale_min` (flag de détection)
- `counter->current_breath` (numéro affiché)
- `scale_factor` (facteur responsive)
- `font_size` calculé

### 2. Timeline à capturer (6 mesures × 0.3s après scale_min)
```
T0    : Juste AVANT scale_min détecté
T_min : Frame où is_at_scale_min = true (incrémentation)
T+0.3s: Première mesure après
T+0.6s: Deuxième mesure
T+0.9s: Troisième mesure
T+1.2s: Quatrième mesure
T+1.5s: Cinquième mesure
T+1.8s: Sixième mesure
```

### 3. Pour chaque mesure, capturer :
```
- hex_node->current_cycle
- hex_node->precomputed_counter_frames[current_cycle].text_scale
- hex_node->precomputed_counter_frames[current_cycle].is_at_scale_min
- hex_node->precomputed_counter_frames[current_cycle].is_at_scale_max
- counter->current_breath
- counter->was_at_min_last_frame
- scale_factor
- font_size (calculé)
- text_width, text_height (extents Cairo)
```

## 📝 HYPOTHÈSE PRINCIPALE

Le bug vient probablement d'un **décalage d'une frame** entre :
1. La détection du changement de chiffre (à scale_min)
2. L'application du bon `text_scale` correspondant à ce scale_min

**Pourquoi visible uniquement en mode réduit (200-300px) ?**
- En mode normal (1280x720) : scale_factor ≈ 1.0
  - Erreur : 100 * (1.0 - 0.1) * 1.0 = 90 pixels de différence → peu visible
- En mode réduit (250px) : scale_factor ≈ 0.195
  - Erreur : 100 * (1.0 - 0.1) * 0.195 = 17.55 pixels de différence
  - L'hexagone est TRÈS petit (rayon ≈ 30-40px), donc 17px = **presque tout l'hexagone** !

## ✅ PROCHAINES ÉTAPES

1. ✅ Script GDB pour capturer les valeurs au moment du bug
2. Analyse des logs GDB pour confirmer l'hypothèse
3. Correction ciblée selon les résultats
