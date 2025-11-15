# Guide Cairo - Manipulation du Rendu et des Couleurs

Ce guide explique comment utiliser Cairo pour personnaliser le rendu des animations dans le projet Respire.

## Table des matières
1. [Introduction à Cairo](#introduction-à-cairo)
2. [Système de couleurs](#système-de-couleurs)
3. [Antialiasing et qualité](#antialiasing-et-qualité)
4. [Personnalisation des hexagones](#personnalisation-des-hexagones)
5. [Personnalisation du texte](#personnalisation-du-texte)
6. [Optimisations](#optimisations)

---

## Introduction à Cairo

Cairo est une bibliothèque de rendu 2D vectoriel qui offre :
- **Antialiasing** : Lissage des bords pour un rendu plus net
- **Qualité** : Rendu haute qualité avec transparence (alpha)
- **Performance** : Rendu accéléré matériel (si disponible)
- **Flexibilité** : Contrôle fin sur tous les aspects du rendu

### Architecture Cairo dans Respire

```
SDL2 Renderer → Cairo Surface → Texture SDL → Affichage
     ↓              ↓                ↓
  Contexte      Dessin          Conversion
  d'affichage   vectoriel       pour SDL
```

---

## Système de couleurs

### Format des couleurs dans Cairo

Cairo utilise des valeurs normalisées entre **0.0 et 1.0** pour chaque composante :

```c
cairo_set_source_rgba(cr,
    rouge / 255.0,    // Rouge : 0.0 à 1.0
    vert / 255.0,     // Vert : 0.0 à 1.0
    bleu / 255.0,     // Bleu : 0.0 à 1.0
    alpha / 255.0     // Transparence : 0.0 (transparent) à 1.0 (opaque)
);
```

### Conversion SDL_Color → Cairo

Dans le code, nous utilisons `SDL_Color` (valeurs 0-255) puis convertissons :

```c
SDL_Color color = {235, 80, 245, 180};  // SDL format (0-255)

// Conversion pour Cairo
cairo_set_source_rgba(cr,
    color.r / 255.0,  // 235/255 = 0.92 (violet fort)
    color.g / 255.0,  // 80/255 = 0.31 (peu de vert)
    color.b / 255.0,  // 245/255 = 0.96 (violet fort)
    color.a / 255.0   // 180/255 = 0.71 (70% opaque)
);
```

### Comprendre les composantes RGBA

| Composante | Rôle | Valeur min | Valeur max | Exemple |
|------------|------|------------|------------|---------|
| **R** (Rouge) | Intensité du rouge | 0 (aucun) | 255 (max) | `{255, 0, 0, 255}` = Rouge pur |
| **G** (Vert) | Intensité du vert | 0 (aucun) | 255 (max) | `{0, 255, 0, 255}` = Vert pur |
| **B** (Bleu) | Intensité du bleu | 0 (aucun) | 255 (max) | `{0, 0, 255, 255}` = Bleu pur |
| **A** (Alpha) | Opacité | 0 (transparent) | 255 (opaque) | `{255, 0, 0, 128}` = Rouge 50% transparent |

### Exemples de couleurs courantes

```c
// Couleurs primaires
SDL_Color rouge_pur    = {255, 0,   0,   255};
SDL_Color vert_pur     = {0,   255, 0,   255};
SDL_Color bleu_pur     = {0,   0,   255, 255};

// Couleurs secondaires
SDL_Color jaune        = {255, 255, 0,   255};  // Rouge + Vert
SDL_Color cyan         = {0,   255, 255, 255};  // Vert + Bleu
SDL_Color magenta      = {255, 0,   255, 255};  // Rouge + Bleu

// Nuances de violet (comme dans Respire)
SDL_Color violet_fonce = {160, 50,  170, 180};  // Peu saturé, moyennement opaque
SDL_Color violet_vif   = {235, 80,  245, 180};  // Très saturé
SDL_Color violet_pale  = {245, 150, 250, 180};  // Clair et saturé

// Couleurs avec transparence
SDL_Color blanc_trans  = {255, 255, 255, 130};  // Blanc 50% transparent
SDL_Color blanc_opaque = {255, 255, 255, 200};  // Blanc 78% opaque
SDL_Color noir_leger   = {0,   0,   0,   50};   // Noir très transparent
```

### Règles pour créer des couleurs harmonieuses

**1. Pour plus de contraste :**
- Augmenter les valeurs RGB (vers 255)
- Augmenter l'alpha (vers 255)

**2. Pour des couleurs plus vives :**
- Maximiser 1-2 composantes RGB
- Minimiser les autres composantes

**3. Pour des couleurs pastel :**
- Garder toutes les composantes élevées (>200)
- Varier légèrement entre elles

**4. Pour la transparence :**
- Alpha < 150 : très transparent (laisse voir le fond)
- Alpha 150-200 : semi-transparent (mélange visible)
- Alpha > 200 : presque opaque (couleur pure)

---

## Antialiasing et qualité

### Niveaux d'antialiasing

Cairo offre plusieurs niveaux de qualité :

```c
// Dans geometry.c et counter.c, nous utilisons :
cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);
```

Niveaux disponibles :
- `CAIRO_ANTIALIAS_NONE` : Pas d'antialiasing (pixelisé)
- `CAIRO_ANTIALIAS_FAST` : Rapide mais moins beau
- `CAIRO_ANTIALIAS_GOOD` : Bon compromis (défaut)
- `CAIRO_ANTIALIAS_BEST` : Qualité maximale (utilisé dans Respire)

### Où modifier l'antialiasing

**Pour les hexagones** (`src/geometry.c:83`) :
```c
void make_hexagone(SDL_Renderer *renderer, Hexagon* hex) {
    // ...
    cairo_t* cr = cairo_create(surface);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);  // ← Ici
    // ...
}
```

**Pour le texte des compteurs** (`src/counter.c:189`) :
```c
void counter_render(...) {
    // ...
    cairo_t* cr = cairo_create(surface);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);  // ← Ici
    // ...
}
```

---

## Personnalisation des hexagones

### Emplacement du code

Les couleurs des hexagones sont définies dans **`src/geometry.c:162-167`**

```c
SDL_Color colors[] = {
    {160, 50, 170, 180},   // Hexagone 0 : Violet foncé
    {235, 80, 245, 180},   // Hexagone 1 : Violet vif
    {245, 150, 250, 180},  // Hexagone 2 : Violet pâle
    {255, 255, 255, 200}   // Hexagone 3 : Blanc (le plus visible)
};
hex->color = colors[element_id % 4];
```

### Ordre d'affichage des hexagones

Les hexagones sont superposés dans cet ordre (du fond vers le haut) :
1. **Hexagone 0** (violet foncé) - Le plus grand, au fond
2. **Hexagone 1** (violet vif) - Au-dessus du 0
3. **Hexagone 2** (violet pâle) - Au-dessus du 1
4. **Hexagone 3** (blanc) - Le plus petit, au premier plan

### Exemples de personnalisation

**Thème bleu océan :**
```c
SDL_Color colors[] = {
    {20, 50, 100, 180},    // Bleu foncé
    {40, 100, 180, 180},   // Bleu moyen
    {80, 150, 220, 180},   // Bleu clair
    {255, 255, 255, 200}   // Blanc
};
```

**Thème vert nature :**
```c
SDL_Color colors[] = {
    {50, 120, 50, 180},    // Vert foncé
    {80, 180, 80, 180},    // Vert moyen
    {120, 220, 120, 180},  // Vert clair
    {255, 255, 255, 200}   // Blanc
};
```

**Thème arc-en-ciel :**
```c
SDL_Color colors[] = {
    {255, 50, 50, 180},    // Rouge
    {255, 150, 50, 180},   // Orange
    {255, 255, 50, 180},   // Jaune
    {50, 200, 255, 180}    // Cyan
};
```

**Hexagone blanc plus visible (plus opaque) :**
```c
SDL_Color colors[] = {
    {160, 50, 170, 180},   // Violet foncé
    {235, 80, 245, 180},   // Violet vif
    {245, 150, 250, 180},  // Violet pâle
    {255, 255, 255, 240}   // Blanc très opaque (alpha 240 au lieu de 200)
};
```

### Transparence et superposition

La transparence (alpha) permet de voir les hexagones du dessous :

```
Alpha 130 (50%) : Très transparent, mélange fort avec le fond
   ↓
Alpha 180 (70%) : Semi-transparent, bon équilibre (valeur actuelle)
   ↓
Alpha 200 (78%) : Peu transparent, couleur bien visible
   ↓
Alpha 240 (94%) : Presque opaque, couleur pure
```

**Effet de superposition :**
- Si l'hexagone blanc (3) a alpha=130, il sera teinté par les hexagones violets en dessous
- Si l'hexagone blanc (3) a alpha=240, il sera presque pur blanc
- Valeur actuelle (200) : Bon compromis, légère teinte mais reste blanc

---

## Personnalisation du texte

### Emplacement du code

Le rendu du texte des compteurs se fait dans **`src/counter.c:169-230`**

### Couleur du texte

Définie dans `src/counter.c:32-35` :
```c
// Couleur bleu-nuit cendré (même que le timer)
counter->text_color.r = 70;
counter->text_color.g = 85;
counter->text_color.b = 110;
counter->text_color.a = 255;
```

**Exemples de couleurs pour le texte :**
```c
// Noir classique
counter->text_color = (SDL_Color){0, 0, 0, 255};

// Blanc (pour fond sombre)
counter->text_color = (SDL_Color){255, 255, 255, 255};

// Violet assorti aux hexagones
counter->text_color = (SDL_Color){160, 50, 170, 255};

// Bleu-nuit doux (actuel)
counter->text_color = (SDL_Color){70, 85, 110, 255};
```

### Taille de la police

La taille de police s'adapte automatiquement avec l'animation "fish-eye" :

```c
// src/counter.c:143-144
double font_size = counter->base_font_size * text_scale;
if (font_size < 12.0) font_size = 12.0;  // Minimum lisible
```

Pour changer la taille de base, modifier lors de la création du compteur dans `src/main.c` :
```c
CounterState* counter = counter_create(
    config.Nb_respiration,
    config.Retention,
    FONT_PATH,
    80  // ← Taille de base (augmenter pour texte plus grand)
);
```

### Qualité du texte

L'antialiasing du texte est activé dans `counter_render()` :
```c
// src/counter.c:189
cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);
```

---

## Optimisations

### Performance actuelle

Cairo crée des textures à la volée à chaque frame. Pour optimiser :

**1. Cache de textures (avancé)**
```c
// Créer un cache de textures précalculées pour éviter de recréer à chaque frame
static SDL_Texture* cached_textures[10];  // Pour chiffres 1-10
```

**2. Réduire la qualité sur machines lentes**
```c
// Utiliser CAIRO_ANTIALIAS_GOOD au lieu de BEST
cairo_set_antialias(cr, CAIRO_ANTIALIAS_GOOD);
```

**3. Limiter la fréquence de mise à jour**
```c
// Ne mettre à jour le texte que toutes les N frames
static int frame_counter = 0;
if (frame_counter++ % 2 == 0) {
    // Rendre le texte
}
```

### Mémoire

Cairo libère automatiquement les ressources avec :
```c
cairo_destroy(cr);
cairo_surface_destroy(surface);
cairo_font_face_destroy(cairo_face);
```

**Important :** Ne jamais oublier ces appels pour éviter les fuites mémoire !

---

## Références rapides

### Fichiers à modifier

| Fichier | Ligne | Modification |
|---------|-------|--------------|
| `src/geometry.c` | 162-167 | Couleurs des hexagones |
| `src/geometry.c` | 83 | Qualité antialiasing hexagones |
| `src/counter.c` | 32-35 | Couleur du texte compteur |
| `src/counter.c` | 189 | Qualité antialiasing texte |
| `src/counter.c` | 143-144 | Taille de police |

### Commandes utiles

```bash
# Recompiler après modifications
make clean && make

# Tester l'application
./bin/respire

# Revenir aux versions SDL2_gfx/TTF (si besoin)
cp src/geometry_sdl_gfx.c.bak src/geometry.c
cp src/counter_ttf.c.bak src/counter.c
```

### Ressources Cairo

- **Documentation officielle :** https://www.cairographics.org/manual/
- **Tutoriels Cairo :** https://www.cairographics.org/tutorial/
- **Référence API :** https://www.cairographics.org/manual/cairo-cairo-t.html

---

## Exemples pratiques

### Exemple 1 : Hexagones rouges progressifs

```c
// src/geometry.c:162-167
SDL_Color colors[] = {
    {100, 0, 0, 180},    // Rouge foncé
    {180, 0, 0, 180},    // Rouge moyen
    {240, 100, 100, 180}, // Rouge clair
    {255, 200, 200, 200} // Rose pâle
};
```

### Exemple 2 : Texte doré

```c
// src/counter.c:32-35
counter->text_color.r = 218;  // Or
counter->text_color.g = 165;
counter->text_color.b = 32;
counter->text_color.a = 255;
```

### Exemple 3 : Hexagone blanc très opaque

```c
// src/geometry.c:166
{255, 255, 255, 250}   // Blanc quasi-opaque (alpha 250)
```

---

**Bon réglage avec Cairo ! 🎨**
