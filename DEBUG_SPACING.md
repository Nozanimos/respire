# 🔍 Guide de Debug : Espacement Widgets INCREMENT

## 📋 Problème

Les cadres affichant les valeurs (rollers) des widgets INCREMENT sont trop distants de leurs noms d'options (**environ 15px en trop**).

## 🎯 Objectif

Analyser le calcul de positionnement pour identifier où l'espacement excessif est ajouté.

## 🛠️ Utilisation du script GDB

### 1️⃣ Compilation avec symboles de debug

```bash
make clean && make
```

*(Le Makefile utilise déjà `-g` pour les symboles de debug)*

### 2️⃣ Lancer GDB avec le script

```bash
gdb -x debug_widget_spacing.gdb ./bin/respire
```

### 3️⃣ Dans GDB, lancer l'application

```gdb
(gdb) run
```

### 4️⃣ Scénarios de test

#### Mode UNSTACK (panneau large)
1. Ouvrir l'application
2. Panneau settings large → widgets non empilés
3. Observer les valeurs affichées dans le terminal

#### Mode STACK (panneau étroit)
1. Réduire la largeur du panneau
2. Les widgets s'empilent et se centrent
3. Observer les valeurs de calcul de centrage

### 5️⃣ Commandes GDB utiles

```gdb
# Continuer l'exécution
(gdb) continue

# Afficher une variable
(gdb) print widget->base_espace_apres_texte
(gdb) print widget->local_roller_x
(gdb) print container_width

# Lister les breakpoints
(gdb) info breakpoints

# Désactiver temporairement un breakpoint
(gdb) disable 1

# Quitter
(gdb) quit
```

## 📊 Valeurs à surveiller

### À la création du widget (create_config_widget)

| Variable | Formule | Valeur attendue |
|----------|---------|-----------------|
| `text_size` | Config JSON | 14 px |
| `base_espace_apres_texte` | `text_size * 0.7` | **~10 px** ⚠️ (TROP ?) |
| `base_roller_padding` | `text_size * 0.4` | ~5-6 px |
| `text_width` | TTF_SizeUTF8() | Variable selon texte |
| `local_roller_x` | `text_width + base_espace_apres_texte` | Variable |
| `roller_width` | Calculé selon valeur max | Variable |
| `total_width` | `local_roller_x + roller_width + 5` | Variable |

### En mode STACK (settings_panel.c)

| Variable | Formule | Description |
|----------|---------|-------------|
| `real_width` | `local_roller_x + roller_width + 10` | Largeur réelle du widget |
| `max_increment_width` | `max(real_width)` | Widget le plus long |
| `increment_start_x` | `(panel_width - max_width) / 2` | Position X pour centrage |

### Au rendu (render_config_widget)

| Variable | Formule | Description |
|----------|---------|-------------|
| `container_width` | Passé en paramètre | 0 = unstack, >0 = stack |
| `roller_x_offset` | `calculate_roller_x_offset()` | Position X du roller |
| `roller_screen_x` | `widget_screen_x + roller_x_offset` | Position écran finale |

## 🔍 Points de suspicion

### 1. Espacement de base trop grand

```c
// src/widget.c:152
widget->base_espace_apres_texte = (int)(text_size * 0.7);  // 14 * 0.7 = 9-10px
```

**Hypothèse** : Ce coefficient 0.7 est peut-être trop élevé ?

### 2. Marge supplémentaire dans le calcul

```c
// src/widget.c:246
int total_width = widget->local_roller_x + widget->roller_width + 5;  // +5 px

// src/settings_panel.c:1191
int real_width = w->local_roller_x + w->roller_width + 10;  // +10 px
```

**Hypothèse** : Marges différentes selon le contexte ?

### 3. Alignement à droite

```c
// src/widget.c:294 (dans calculate_roller_x_offset)
roller_x_offset = container_width - roller_total_width - RIGHT_MARGIN;  // RIGHT_MARGIN = 10px
```

**Hypothèse** : L'alignement à droite ajoute-t-il de l'espace inutile ?

## 📝 Analyse attendue

### Exemple de sortie GDB

```
════════════════════════════════════════════════════════════════
🔧 CRÉATION WIDGET INCREMENT : Vitesse respiration
════════════════════════════════════════════════════════════════
  📏 text_size = 14 px
  📐 base_espace_apres_texte = 9 px  (text_size * 0.7)    ← SUSPECT !
  📐 base_roller_padding = 5 px  (text_size * 0.4)

  📝 MESURE DU LABEL 'Vitesse respiration' :
     text_width = 120 px
     text_height = 14 px

  🎯 CALCUL POSITION ROLLER :
     local_roller_x = 120 + 9 = 129 px    ← ESPACEMENT ICI

  📦 DIMENSIONS ROLLER :
     roller_width = 40 px
     roller_height = 18 px

  📊 LARGEUR TOTALE WIDGET :
     total_width = 129 + 40 + 5 = 174 px
════════════════════════════════════════════════════════════════
```

### Diagnostic

Si `base_espace_apres_texte = 9-10 px` et que visuellement on observe **15px en trop**, alors :
- **Espacement réel** = `base_espace_apres_texte` + autres marges
- **Espacement souhaité** = ~3-5 px

**Solution probable** : Réduire le coefficient de 0.7 à 0.2-0.3, ou passer en valeur fixe.

## 🎯 Actions correctives possibles

Selon les résultats du debug :

1. **Réduire base_espace_apres_texte**
   ```c
   widget->base_espace_apres_texte = 3;  // Fixe, minimal
   ```

2. **Ajuster le calcul de real_width** (si marge +10 est excessive)
   ```c
   int real_width = w->local_roller_x + w->roller_width + 3;
   ```

3. **Vérifier la logique d'alignement** (calculate_roller_x_offset)

---

**Date** : Janvier 2025
**Version analysée** : 19ac307
