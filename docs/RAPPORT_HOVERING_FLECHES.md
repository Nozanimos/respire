# RAPPORT D'ANALYSE - Hovering des Flèches (Widgets INCREMENT)

## 🔍 PROBLÈME PRINCIPAL IDENTIFIÉ

**Symptôme** : Le hovering des flèches ne fonctionne que sur "un incrément sur deux"

**Cause racine** : **Désynchronisation entre le rendu et la détection de hovering**

---

## 📊 FLUX D'EXÉCUTION

### 1. Création des widgets (settings_panel.c)
- Chargement du JSON de configuration
- Création des widgets INCREMENT via `add_increment_widget()` → widget_list.c:50
- Positionnement initial avec coordonnées locales relatives

### 2. Rendu (settings_panel.c:441)
```c
render_all_widgets(renderer, panel->widget_list, panel_x, panel_y,
                   panel->rect.w, panel->scroll_offset);
```

### 3. Gestion événements (settings_panel.c:526)
```c
handle_widget_list_events(panel->widget_list, event, panel_x, panel_y,
                          panel->scroll_offset);
```

---

## 🐛 BUG #1 : POSITION DES FLÈCHES MAL CALCULÉE DANS LE HOVERING

### Localisation
**Fichier** : `src/widget.c`
**Fonction** : `handle_config_widget_events()`
**Lignes** : 435-448

### Le problème

**Dans le RENDU** (`render_config_widget()`, ligne 240-380) :
```c
// Ligne 257-279 : Calcul de arrows_x_offset avec alignement
int arrows_x_offset = widget->local_arrows_x;  // Position par défaut

if (container_width > 0) {
    // ✅ ALIGNEMENT À DROITE : les flèches sont décalées
    const int RIGHT_MARGIN = 10;
    const int ESTIMATED_VALUE_WIDTH = 40;
    int arrows_value_width = widget->arrow_size +
                            widget->base_espace_apres_fleches +
                            ESTIMATED_VALUE_WIDTH;

    arrows_x_offset = container_width - arrows_value_width - RIGHT_MARGIN;
    // ...
}

// Ligne 340 : RENDU avec arrows_x_offset
int arrows_screen_x = widget_screen_x + arrows_x_offset;  // ✅ Position ajustée
```

**Dans la DÉTECTION** (`handle_config_widget_events()`, ligne 385-481) :
```c
// Ligne 435 : ❌ ERREUR - Utilise local_arrows_x au lieu d'arrows_x_offset
int arrows_screen_x = widget_screen_x + widget->local_arrows_x;  // ❌ Position fixe

// Lignes 438-448 : Zones de hovering calculées avec la MAUVAISE position
widget->up_arrow_hovered = (mx >= arrows_screen_x - widget->arrow_size / 2 &&
                            mx <= arrows_screen_x + widget->arrow_size / 2 &&
                            my >= up_y &&
                            my <= up_y + widget->arrow_size);
```

### Conséquence

Quand `container_width > 0` (widgets groupés pour alignement) :
- Les flèches sont **rendues** à la position `arrows_x_offset` (alignées à droite)
- Mais les zones de hovering utilisent `local_arrows_x` (position par défaut, à gauche)
- **Résultat** : Les zones cliquables ne correspondent PAS aux flèches affichées

---

## 🔍 ANALYSE "UN SUR DEUX"

### Pourquoi ça fonctionne "un sur deux" ?

Le regroupement des widgets se fait dans `widget_list.c` (lignes 246-354) :

1. **Les widgets proches** (écart vertical < 30px) sont regroupés
2. **Dans chaque groupe** : calcul d'un `container_width` basé sur le widget le plus long
3. **Le widget le plus long** a `local_arrows_x ≈ arrows_x_offset` → hovering fonctionne ✅
4. **Les widgets plus courts** ont `local_arrows_x << arrows_x_offset` → hovering cassé ❌

**Exemple concret** :
```
Groupe 1 :
  - "Durée inspiration" (long texte)   → local_arrows_x = 180, arrows_x_offset = 180 ✅
  - "Rétention"        (court texte)   → local_arrows_x = 100, arrows_x_offset = 180 ❌
  - "Cycles"           (court texte)   → local_arrows_x = 80,  arrows_x_offset = 180 ❌
```

Seul le premier widget (le plus long) a les zones de hovering correctes !

---

## 🐛 BUG #2 : CALCUL DE LARGEUR DUPLIQUÉ

### Localisation
**Fichier** : `src/widget.c`
**Lignes** : 397-426 (dans `handle_config_widget_events`)

### Le problème

Le calcul de `value_x_offset` est **dupliqué** entre le rendu et la détection :

**Rendu** (widget.c:260-279) :
```c
if (container_width > 0) {
    arrows_x_offset = container_width - arrows_value_width - RIGHT_MARGIN;
    value_x_offset = arrows_x_offset + widget->arrow_size +
                     widget->base_espace_apres_fleches;
}
```

**Détection** (widget.c:414-426) :
```c
if (container_width > 0) {
    int arrows_x_offset = container_width - arrows_value_width - RIGHT_MARGIN;
    // ❌ Mais cette valeur n'est PAS utilisée pour le hovering des flèches !
    value_x_offset = arrows_x_offset + widget->arrow_size +
                     widget->base_espace_apres_fleches;
}
```

**Problème** :
- Le `arrows_x_offset` est calculé ligne 418 **MAIS** il n'est utilisé que pour `value_x_offset`
- Il n'est **PAS** utilisé pour calculer `arrows_screen_x` (ligne 435)
- C'est une variable locale qui n'affecte pas le hovering des flèches

---

## 🐛 BUG #3 : ZONES DE HOVERING MAL DIMENSIONNÉES

### Localisation
**Fichier** : `src/widget.c`
**Lignes** : 438-448

### Le problème

Les zones de hovering des flèches sont calculées ainsi :
```c
// Zone flèche UP
int up_y = arrows_screen_y - widget->arrow_size / 2;
widget->up_arrow_hovered = (mx >= arrows_screen_x - widget->arrow_size / 2 &&
                            mx <= arrows_screen_x + widget->arrow_size / 2 &&
                            my >= up_y &&
                            my <= up_y + widget->arrow_size);
```

**Problèmes** :
1. **Position X** : `arrows_screen_x` est la **position centrale** de la flèche
   - On teste `mx >= center - size/2` et `mx <= center + size/2`
   - Mais les flèches peuvent ne pas être exactement centrées sur `arrows_screen_x`

2. **Position Y** : `arrows_screen_y` est déjà le centre vertical de la flèche UP
   - On calcule `up_y = arrows_screen_y - arrow_size/2` (top de la flèche)
   - Mais dans le rendu (ligne 347), la flèche UP est créée à `up_y = arrows_screen_y` directement

**Incohérence rendu vs détection** :

**Rendu** (ligne 346-351) :
```c
int up_y = arrows_screen_y;  // ✅ Centre de la flèche UP
Triangle* up_arrow = create_up_arrow(arrows_screen_x, up_y,
                                     widget->arrow_size, up_color);
```

**Détection** (ligne 438-442) :
```c
int up_y = arrows_screen_y - widget->arrow_size / 2;  // ❌ Ajustement en trop
widget->up_arrow_hovered = (/* ... */ my >= up_y &&
                            my <= up_y + widget->arrow_size);
```

---

## 📝 LISTE COMPLÈTE DES PROBLÈMES

### 🔴 Critique (empêche le hovering)
1. **Position X des flèches** : `arrows_screen_x` utilise `local_arrows_x` au lieu de `arrows_x_offset`
2. **Calcul dupliqué** : `arrows_x_offset` recalculé mais non utilisé pour le hovering

### 🟡 Mineur (peut causer des imprécisions)
3. **Position Y UP** : Décalage vertical incohérent avec le rendu
4. **Position Y DOWN** : Même problème pour la flèche bas

---

## ✅ SOLUTIONS PROPOSÉES

### Solution 1 : Factoriser le calcul d'arrows_x_offset

Créer une fonction utilitaire :
```c
static int calculate_arrows_x_offset(ConfigWidget* widget, int container_width) {
    int arrows_x_offset = widget->local_arrows_x;

    if (container_width > 0) {
        const int RIGHT_MARGIN = 10;
        const int ESTIMATED_VALUE_WIDTH = 40;
        int arrows_value_width = widget->arrow_size +
                                widget->base_espace_apres_fleches +
                                ESTIMATED_VALUE_WIDTH;

        arrows_x_offset = container_width - arrows_value_width - RIGHT_MARGIN;

        int min_arrows_x = widget->local_arrows_x;
        if (arrows_x_offset < min_arrows_x) {
            arrows_x_offset = min_arrows_x;
        }
    }

    return arrows_x_offset;
}
```

Utiliser cette fonction dans :
- `render_config_widget()` ligne 257
- `handle_config_widget_events()` ligne 435

### Solution 2 : Corriger le calcul de hovering (RAPIDE)

Dans `handle_config_widget_events()`, ligne 411-435, remplacer :
```c
// ❌ ANCIEN CODE
int arrows_screen_x = widget_screen_x + widget->local_arrows_x;
```

Par :
```c
// ✅ NOUVEAU CODE - Même calcul que le rendu
int arrows_x_offset = widget->local_arrows_x;

if (container_width > 0) {
    const int RIGHT_MARGIN = 10;
    const int ESTIMATED_VALUE_WIDTH = 40;
    int arrows_value_width = widget->arrow_size +
                            widget->base_espace_apres_fleches +
                            ESTIMATED_VALUE_WIDTH;

    arrows_x_offset = container_width - arrows_value_width - RIGHT_MARGIN;

    int min_arrows_x = widget->local_arrows_x;
    if (arrows_x_offset < min_arrows_x) {
        arrows_x_offset = min_arrows_x;
    }
}

int arrows_screen_x = widget_screen_x + arrows_x_offset;  // ✅ Position ajustée
```

### Solution 3 : Corriger les zones Y

Ligne 438 et 444, utiliser directement `arrows_screen_y` sans ajustement :
```c
// ❌ ANCIEN
int up_y = arrows_screen_y - widget->arrow_size / 2;

// ✅ NOUVEAU - Cohérent avec create_up_arrow()
int up_y = arrows_screen_y;
```

---

## 🔬 TESTS RECOMMANDÉS

Après correction, tester :

1. **Widget court dans un groupe** : Hovering fonctionne sur tous les widgets
2. **Widget long (référence du groupe)** : Hovering toujours fonctionnel
3. **Widget hors groupe** (seul) : Hovering fonctionne
4. **Resize du panneau** : Hovering suit l'alignement dynamique
5. **Scroll** : Zones cliquables suivent le scroll

---

## 📊 PRIORITÉS

| Priorité | Bug | Impact | Difficulté |
|----------|-----|--------|------------|
| **P0** | Position X flèches | CRITIQUE - casse le hovering | FACILE |
| **P1** | Zones Y flèches | MINEUR - léger décalage | FACILE |
| **P2** | Factorisation code | QUALITÉ - évite duplication | MOYENNE |

---

## 🎯 RECOMMANDATION

**Action immédiate** : Appliquer la **Solution 2** (correction rapide)
- Temps : 5 minutes
- Risque : Faible
- Impact : Résout le problème principal

**Action follow-up** : Appliquer la **Solution 1** (refactoring)
- Temps : 15 minutes
- Risque : Faible
- Impact : Améliore la maintenabilité

---

**Rapport généré le** : 2025-11-15
**Fichiers analysés** : widget.c, widget_list.c, settings_panel.c
**Lignes de code examinées** : ~2500 lignes
