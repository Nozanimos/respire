# GUIDE DE DÉBOGAGE - Bug Compteur Responsive

## 🎯 Objectif

Déboguer le bug du compteur qui occupe tout l'hexagone au moment du changement de chiffre en mode responsive (fenêtre 200-300px).

## 📋 Prérequis

```bash
# S'assurer que l'application est compilée avec les symboles de debug
make clean
make
```

## 🚀 Utilisation du script GDB

### Méthode 1 : Mode interactif (recommandé)

```bash
gdb -x debug_counter_responsive.gdb ./bin/respire
```

Dans GDB, tapez :
```
(gdb) run
```

**Ensuite** :
1. Réduisez la fenêtre de l'application à environ 200-300px de largeur
2. Cliquez sur le bouton "Play" pour démarrer l'animation
3. Attendez que le compteur s'incrémente (1→2, 2→3, etc.)
4. Le script capturera automatiquement les données
5. Après 6 captures (≈1.8s), le traçage s'arrête automatiquement

Pour quitter :
```
(gdb) quit
```

### Méthode 2 : Mode batch (pour tests rapides)

```bash
gdb -batch -x debug_counter_responsive.gdb ./bin/respire < /dev/null 2>&1 | tee gdb_run.log
```

**Note** : En mode batch, l'application ne s'exécutera pas complètement car il n'y a pas d'interaction possible.

## 📊 Analyser les résultats

Les logs sont sauvegardés dans : `gdb_counter_trace.log`

### Structure d'une capture type

```
═══════════════════════════════════════════════════════════════════════
DETECTION CHANGEMENT DE CHIFFRE : 1 -> 2
═══════════════════════════════════════════════════════════════════════
Frame actuelle : 450 / 2400
Flag scale_min actuel : 1
Flag was_at_min_last  : 0

---------------------------------------------------------------------
CAPTURE #1 (T + 0.3s apres scale_min)
---------------------------------------------------------------------
Timing:
    Frames depuis scale_min : 18
    Secondes ecoulees       : 0.30 s

Etat du compteur:
    Chiffre affiche         : 2 / 10
    was_at_min_last_frame   : 1
    waiting_for_scale_min   : 0

Animation (frame actuelle):
    current_cycle           : 468 / 2400
    text_scale (precalcule) : 0.1523
    is_at_scale_min         : 0
    is_at_scale_max         : 0
    current_scale (node)    : 0.1523

Responsive:
    scale_factor            : 0.1953
    base_font_size          : 100

Calcul font_size:
    base_font_size * text_scale * scale_factor
    = 100 * 0.1523 * 0.1953
    = 2.97 pixels

Mesure texte Cairo (apres cairo_text_extents):
    extents.width           : 12.34
    extents.height          : 18.56
    extents.x_bearing       : 1.23
    extents.y_bearing       : -15.67

Surface Cairo creee:
    text_width  (width + 10) : 22 pixels
    text_height (height + 10): 28 pixels
```

## 🔍 Que chercher dans les logs ?

### 1. Vérifier la synchronisation

Comparer les valeurs entre **DETECTION CHANGEMENT DE CHIFFRE** et **CAPTURE #1** :

- `current_cycle` devrait augmenter progressivement
- `text_scale` devrait être **petit** (≈0.1-0.2) juste après scale_min
- `font_size` calculé devrait être **petit** (≈2-5 pixels pour scale_factor ≈ 0.2)

### 2. Détecter le bug

**Symptôme du bug** :
- `text_scale` est **grand** (≈0.8-1.0) juste après le changement de chiffre
- `font_size` calculé est **énorme** (≈15-20 pixels)
- `text_width` / `text_height` sont bien plus grands que l'hexagone

**Valeurs normales attendues** (fenêtre 250px) :
```
scale_factor     : 0.195
text_scale       : 0.100-0.200 (juste après scale_min)
font_size        : 2-4 pixels
text_width       : 15-25 pixels
text_height      : 20-30 pixels
```

**Valeurs bugguées** :
```
scale_factor     : 0.195
text_scale       : 0.800-1.000 (PROBLÈME : devrait être petit)
font_size        : 15-20 pixels (ÉNORME pour un petit hexagone)
text_width       : 80-120 pixels (DÉBORDE de l'hexagone)
text_height      : 100-150 pixels
```

### 3. Identifier la cause

Regarder l'évolution de `current_cycle` et `text_scale` sur les 6 captures :

- Si `text_scale` **démarre grand** puis **diminue** → Bug de décalage d'une frame
- Si `text_scale` **reste grand** → Bug de calcul dans precompute
- Si `text_scale` est **correct** mais `font_size` est **grand** → Bug dans le calcul responsive

## 🛠️ Après l'analyse

Une fois le bug identifié, consulter `ANALYSE_BUG_COMPTEUR_RESPONSIVE.md` pour les hypothèses de correction.

## 📝 Variables importantes

### Dans counter_render()
- `counter->current_breath` : Numéro du chiffre affiché (1, 2, 3...)
- `hex_node->current_cycle` : Index de la frame actuelle dans le tableau précomputé
- `current_frame->text_scale` : Scale du texte pour cette frame (0.1 à 1.0)
- `scale_factor` : Facteur responsive de la fenêtre (0.2 pour 250px)

### Dans apply_precomputed_frame()
- `node->current_cycle` : S'incrémente à chaque frame
- `node->current_scale` : Scale actuel de l'hexagone

## ⚙️ Personnaliser le script

Pour modifier le nombre de captures ou l'intervalle :

Éditer `debug_counter_responsive.gdb` :
```gdb
set $max_captures = 6                  # Nombre de captures
set $capture_interval_frames = 18      # Intervalle (18 frames = 0.3s à 60fps)
```

## 🆘 Résolution de problèmes

### Le script ne capture rien
- Vérifiez que vous avez bien lancé l'animation (bouton Play)
- Vérifiez que la fenêtre est bien réduite (200-300px)
- Attendez que le compteur s'incrémente

### GDB plante ou freeze
- Utilisez Ctrl+C pour interrompre
- Tapez `continue` pour reprendre
- Tapez `quit` pour quitter

### Les logs sont vides
- Vérifiez que `gdb_counter_trace.log` existe
- Le logging peut ne capturer que les sorties des breakpoints
- Utilisez `tail -f gdb_counter_trace.log` dans un autre terminal

## 📚 Fichiers liés

- `ANALYSE_BUG_COMPTEUR_RESPONSIVE.md` : Analyse détaillée du bug
- `debug_counter_responsive.gdb` : Script GDB automatisé
- `gdb_counter_trace.log` : Logs générés par le script
