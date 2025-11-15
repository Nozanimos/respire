# Debug du hovering des widgets INCREMENT

## Problème
Le widget "Vitesse respiration" ne détecte pas le hover sur la valeur,
alors que tous les autres widgets INCREMENT fonctionnent correctement.

## Scripts GDB disponibles

### 1. debug_hover.gdb - Debug complet du hovering
Affiche tous les événements de souris et les calculs de collision.

**Usage:**
```bash
gdb -x debug_hover.gdb ./bin/respire
```

**Breakpoints:**
- `handle_config_widget_events` - Affiche chaque event souris
- `create_config_widget` - Affiche la création des widgets
- `widget.c:215` - Après calcul de total_width
- `widget.c:398` - Quand un widget devient hovered

**Instructions:**
1. Le programme démarre automatiquement
2. Ouvre le panneau de config (Ctrl+S ou clic sur l'icône)
3. Survole "Vitesse respiration" et les autres widgets
4. Compare les valeurs affichées dans GDB

---

### 2. debug_widgets_simple.gdb - Vue d'ensemble des widgets
Affiche les propriétés de tous les widgets INCREMENT à la création.

**Usage:**
```bash
gdb -x debug_widgets_simple.gdb ./bin/respire
```

**Utile pour:**
- Comparer les valeurs de "Vitesse respiration" avec les autres
- Voir si base.width est différent
- Vérifier les positions et tailles

---

### 3. debug_utf8.gdb - Vérification UTF-8
Vérifie les calculs de largeur avec TTF_SizeUTF8.

**Usage:**
```bash
gdb -x debug_utf8.gdb ./bin/respire
```

**Vérifie:**
- Calcul de text_width avec TTF_SizeUTF8
- Caractères UTF-8 dans le nom du widget
- Estimation de value_width
- Calcul final de total_width

---

### 4. debug_hover.sh - Script shell automatisé
Compile et lance GDB automatiquement.

**Usage:**
```bash
./debug_hover.sh
```

## Ce qu'il faut observer

### Valeurs importantes à comparer:
1. **base.width** - Doit couvrir nom + flèches + valeur
2. **local_value_x** - Position locale de la valeur
3. **text_width** - Largeur du texte mesurée avec UTF-8
4. **total_width** - Devrait être = local_value_x + value_width + 10

### Test de hovering:
Quand tu survoles un widget, GDB affiche:
- Position de la souris (mx, my)
- Position absolue du widget (abs_x, abs_y)
- Taille du widget (width, height)
- Résultat: hovered = 1 ou 0

### Comparer "Vitesse respiration" vs "Pause avant session":
Les deux devraient avoir des valeurs similaires, sauf:
- text_width (texte différent)
- valeur actuelle (3 vs 10)

## Hypothèses à vérifier

1. **Caractères UTF-8 dans "Vitesse respiration"** ?
   → Vérifier avec debug_utf8.gdb

2. **Calcul de value_width incorrect** ?
   → Ligne 203: `strlen(value_str) * (text_size / 2)`

3. **base.width pas mis à jour après container_width** ?
   → Déjà testé, pas la solution

4. **Offset ou position incorrecte** ?
   → Vérifier dans handle_config_widget_events

5. **Problème de rendu vs collision** ?
   → La valeur est affichée ailleurs que sa zone de collision

## Commandes GDB utiles

Dans GDB, tu peux aussi faire:
```gdb
# Afficher une variable
print widget->option_name
print widget->base.width

# Afficher toute la structure
print *widget

# Continuer jusqu'au prochain breakpoint
continue

# Sortir de GDB
quit
```

## Notes

- Les scripts utilisent `silent` et `continue` pour affichage automatique
- Les valeurs s'affichent au fur et à mesure
- Ctrl+C pour arrêter le programme
- `quit` pour sortir de GDB
