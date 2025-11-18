# ANALYSE DU CYCLE DE RESPIRATION

## 🔍 Ce que dit le code actuel

### Formule du scale (precompute_list.c:86-87)

```c
scale_progress = cos(progress_in_cycle * 2 * M_PI);
result->scale = scale_min + (scale_max - scale_min) * (scale_progress + 1.0) / 2.0;
```

Avec `scale_min = 0.1` et `scale_max = 1.0` :

| Progress | cos(2πp) | scale_progress | **scale** | Visuel hexagone |
|----------|----------|----------------|-----------|-----------------|
| 0.00     | 1        | 1              | **1.0**   | GRAND (max)     |
| 0.25     | 0        | 0              | **0.55**  | Moyen           |
| 0.50     | -1       | -1             | **0.1**   | PETIT (min)     |
| 0.75     | 0        | 0              | **0.55**  | Moyen           |
| 1.00     | 1        | 1              | **1.0**   | GRAND (max)     |

**Cycle visuel** : Grand → Moyen → Petit → Moyen → Grand (rebouclage)

### Sémantique selon les commentaires (precompute_list.c:413-417)

```
progress = 0.0     → scale_max (1.0) = poumons VIDES
progress = 0.0→0.5 → scale_max → scale_min = INSPIRE (poumons se remplissent)
progress = 0.5     → scale_min (0.1) = poumons PLEINS
progress = 0.5→1.0 → scale_min → scale_max = EXPIRE (poumons se vident)
progress = 1.0     → scale_max (1.0) = poumons VIDES (rebouclage)
```

### Interprétation actuelle

| Phase               | Scale | Hexagone | Poumons | Logique métier |
|---------------------|-------|----------|---------|----------------|
| Départ (p=0.0)      | 1.0   | GRAND    | VIDES   | ✓              |
| INSPIRE (p=0.0→0.5) | 1.0→0.1 | Rétrécit | Se remplissent | ⚠️ Contre-intuitif |
| Milieu (p=0.5)      | 0.1   | PETIT    | PLEINS  | ✓              |
| EXPIRE (p=0.5→1.0)  | 0.1→1.0 | Grandit  | Se vident | ⚠️ Contre-intuitif |
| Fin (p=1.0)         | 1.0   | GRAND    | VIDES   | ✓              |

## 🤔 Le problème sémantique

**Visuellement, quand on inspire (remplit les poumons), on s'attend à ce que l'hexagone GRANDISSE, pas rétrécisse !**

Actuellement :
- **INSPIRE** (remplir poumons) → hexagone **RÉTRÉCIT** (1.0 → 0.1)
- **EXPIRE** (vider poumons) → hexagone **GRANDIT** (0.1 → 1.0)

C'est l'inverse de l'intuition naturelle !

## 🎯 Détection des flags (precompute_list.c:428-438)

```c
// is_at_scale_min : on est proche de 0.1 ET scale commence à augmenter
bool is_at_min = close_to_min && scale_increasing;

// is_at_scale_max : on est proche de 1.0 ET scale commence à diminuer
bool is_at_max = close_to_max && scale_decreasing;
```

| Flag            | Moment détecté                    | Progress | Transition       |
|-----------------|-----------------------------------|----------|------------------|
| is_at_scale_min | Quand on quitte 0.1 vers 1.0      | ≈0.5     | Début EXPIRE     |
| is_at_scale_max | Quand on quitte 1.0 vers 0.1      | ≈0.0     | Début INSPIRE    |

## 🔢 Logique du compteur (counter.c:125-127)

```c
if (is_at_min_now && !counter->was_at_min_last_frame) {
    counter->current_breath++;  // Incrémentation
}
```

Le compteur s'incrémente quand **is_at_scale_min = true**, c'est-à-dire :
- Au **début de l'expiration** (quand on quitte scale_min vers scale_max)
- Progress ≈ 0.5
- Hexagone est PETIT (0.1) et commence à GRANDIR

## 📊 Ce qui se passe avec text_scale (d'après GDB)

Au moment de l'incrémentation (is_at_scale_min détecté) :
```
Frame 271 : text_scale = 0.1003 (PETIT, correct ✓)
```

**Puis le compteur continue à être rendu avec text_scale qui suit l'animation :**
```
Frame 289 (T+0.3s) : text_scale = 0.1954 (grandit)
Frame 307 (T+0.6s) : text_scale = 0.4260 (grandit encore)
Frame 325 (T+0.9s) : text_scale = 0.7039 (grandit beaucoup)
Frame 343 (T+1.2s) : text_scale = 0.9231 (presque au max)
Frame 361 (T+1.5s) : text_scale = 0.9997 (au MAX, is_at_scale_max = 1)
```

**Le chiffre "pulse" avec l'hexagone : 0.1 → 1.0 → 0.1 → ...**

## 🐛 Pourquoi c'est visible en mode réduit ?

**Fenêtre normale (1280x720, scale_factor = 1.0)** :
- Hexagone rayon ≈ 200px
- Compteur oscille entre 10px (scale_min) et 100px (scale_max)
- Ratio compteur/hexagone : 10/200 à 100/200 = 5% à 50%
- Visuellement acceptable

**Fenêtre réduite (≈300px, scale_factor ≈ 0.3)** :
- Hexagone rayon ≈ 60px (200 × 0.3)
- Compteur oscille entre 3px (0.1 × 100 × 0.3) et 30px (1.0 × 100 × 0.3)
- MAIS à base_font_size=189 : entre 6px et **57px** !
- Ratio compteur/hexagone : 6/60 à 57/60 = 10% à **95%** !
- **Le compteur occupe presque TOUT l'hexagone au pic !**

## ✅ Solutions possibles

### Option 1 : Figer text_scale au moment de l'incrémentation

Sauvegarder le text_scale quand le chiffre s'incrémente et l'utiliser pour tout le cycle :

```c
// Dans CounterState, ajouter :
double fixed_text_scale;

// Au moment de l'incrémentation (counter.c:127) :
counter->current_breath++;
counter->fixed_text_scale = current_frame->text_scale;  // Figer à 0.1

// Au rendu (counter.c:172) :
double font_size = counter->base_font_size * counter->fixed_text_scale * scale_factor;
```

**Résultat** : Le compteur reste à taille constante (petit) pendant tout le cycle.

### Option 2 : Utiliser scale_max au lieu de scale_min

Inverser la logique : figer à scale_max (1.0) au lieu de scale_min (0.1) :

```c
counter->fixed_text_scale = 1.0;  // Toujours pleine taille
```

Puis réduire base_font_size pour compenser.

### Option 3 : Utiliser un scale intermédiaire fixe

```c
counter->fixed_text_scale = 0.5;  // Taille moyenne constante
```

### Option 4 : Inverser scale_min et scale_max sémantiquement

Renommer les variables pour clarifier :
- `scale_min` (0.1) → `scale_exhale` (expire, petit)
- `scale_max` (1.0) → `scale_inhale` (inspire, grand)

Mais ça ne résout pas le problème de pulsation.

## 🔬 Prochaine étape : GDB v2

Le script `debug_counter_responsive_v2.gdb` va capturer **frame par frame** :
- Les 10 premières frames après l'incrémentation
- L'ordre exact des appels (apply_precomputed_frame vs counter_render)
- Le text_scale utilisé à chaque frame

Cela confirmera si le problème est bien le **pulsing** du text_scale.
