# Instructions de Debug - Problème de Dépilement

## Contexte

Le dépilement des widgets ne fonctionne pas quand on élargit la fenêtre après un empilement.

## Solution Implémentée

- **Nouvelle variable** : `panel_width_when_stacked` dans `SettingsPanel`
- **Sauvegarde** de la largeur du panneau au moment de l'empilement
- **Dépilement** si `panel_width >= panel_width_when_stacked + 50px` (hystérésis)
- **Réinitialisation** à 0 après dépilement

## Scripts GDB Disponibles

### 1. Script Simple (debug_simple.gdb)

**Usage** :
```bash
gdb -x debug_simple.gdb ./respire
```

**Ce qu'il trace** :
- Chaque appel à `recalculate_widget_layout` avec les valeurs clés
- Quand `widgets_stacked` passe à `true` (empilement)
- Quand `widgets_stacked` passe à `false` (dépilement)

**Idéal pour** : Un premier aperçu rapide du comportement

---

### 2. Script Détaillé (debug_depilement.gdb)

**Usage** :
```bash
gdb -x debug_depilement.gdb ./respire
```

**Ce qu'il trace** :
- Entrée dans `recalculate_widget_layout` (affiche l'état complet)
- **Condition de dépilement** (ligne 922) : affiche si la condition est vraie/fausse
- **Entrée dans le bloc dépilement** (ligne 926) : confirme que le dépilement a lieu
- **Réinitialisation** après dépilement (ligne 991)
- **Condition d'empilement** (ligne 1157) : affiche `needs_reorganization`
- **Sauvegarde** de `panel_width_when_stacked` (ligne 1169) : affiche si la sauvegarde a lieu

**Idéal pour** : Comprendre EXACTEMENT pourquoi le dépilement ne se produit pas

---

## Procédure de Test

1. **Lancer avec GDB** :
   ```bash
   gdb -x debug_simple.gdb ./respire
   # OU
   gdb -x debug_depilement.gdb ./respire
   ```

2. **Dans l'application** :
   - Ouvrir le panneau de configuration (icône engrenage)
   - Réduire la fenêtre jusqu'à ce que les widgets s'empilent
   - Élargir la fenêtre progressivement

3. **Observer les traces** dans la console GDB

4. **Capturer la sortie** :
   ```bash
   gdb -x debug_depilement.gdb ./respire > debug_output.txt 2>&1
   ```

---

## Variables Clés à Observer

| Variable | Signification | Valeur Attendue |
|----------|---------------|-----------------|
| `panel->widgets_stacked` | Widgets empilés ? | `0` (false) ou `1` (true) |
| `panel->panel_width_when_stacked` | Largeur sauvegardée | `> 0` si déjà empilé, `0` sinon |
| `panel_width` (ou `panel->rect.w`) | Largeur actuelle du panneau | Varie selon la fenêtre |
| `needs_reorganization` | Doit-on réorganiser ? | `0` (false) ou `1` (true) |

---

## Problèmes Potentiels à Identifier

### Problème 1 : panel_width_when_stacked jamais sauvegardé
**Symptôme** : `panel_width_when_stacked` reste toujours à `0`
**Cause possible** : La condition ligne 1185 (`if (panel->panel_width_when_stacked == 0)`) n'est jamais atteinte

### Problème 2 : Condition de dépilement jamais vraie
**Symptôme** : La trace "TEST CONDITION DÉPILEMENT" montre toujours "❌ NON"
**Cause possible** :
- `panel->widgets_stacked` est `false`
- `panel_width` n'atteint jamais `panel_width_when_stacked + 50`

### Problème 3 : recalculate_widget_layout pas appelé lors du resize
**Symptôme** : Aucune trace lors du redimensionnement
**Cause possible** : `update_panel_scale` n'est pas appelé depuis `renderer.c`

### Problème 4 : Sauvegarde écrasée
**Symptôme** : `panel_width_when_stacked` change alors qu'il ne devrait pas
**Cause possible** : Bug dans la logique de sauvegarde

---

## Analyse des Traces

### Exemple de trace CORRECTE (dépilement fonctionne) :

```
════════════════════════════════════════════════════════════════════════
>>> APPEL recalculate_widget_layout()
════════════════════════════════════════════════════════════════════════
  panel_width           = 340
  widgets_stacked       = 0
  panel_width_when_stacked = 0

[... réduction fenêtre ...]

🔧 EMPILEMENT widgets_stacked → true
💾 SAUVEGARDE panel_width_when_stacked = 340

[... élargissement fenêtre ...]

┌──────────────────────────────────────────────────────────────────────┐
│ TEST CONDITION DÉPILEMENT (ligne 922)                               │
└──────────────────────────────────────────────────────────────────────┘
  widgets_stacked           = 1
  panel_width_when_stacked  = 340
  panel_width               = 390
  panel_width >= (saved + MARGIN) ? 390 >= 390 ? ✅ OUI -> VA DÉPILER

🔄 DÉPILEMENT widgets_stacked → false
🔓 panel_width_when_stacked réinitialisé à 0
```

### Exemple de trace INCORRECTE (dépilement ne fonctionne pas) :

```
[... élargissement fenêtre ...]

┌──────────────────────────────────────────────────────────────────────┐
│ TEST CONDITION DÉPILEMENT (ligne 922)                               │
└──────────────────────────────────────────────────────────────────────┘
  widgets_stacked           = 1
  panel_width_when_stacked  = 340
  panel_width               = 370
  panel_width >= (saved + MARGIN) ? 370 >= 390 ? ❌ NON -> NE VA PAS DÉPILER
```
→ **Diagnostic** : La fenêtre n'est pas assez large (370 < 390). Il faut élargir davantage.

OU

```
┌──────────────────────────────────────────────────────────────────────┐
│ TEST CONDITION DÉPILEMENT (ligne 922)                               │
└──────────────────────────────────────────────────────────────────────┘
  widgets_stacked           = 0
  panel_width_when_stacked  = 0
  panel_width               = 390
  panel_width >= (saved + MARGIN) ? 390 >= 50 ? ✅ OUI -> VA DÉPILER
```
→ **Diagnostic** : `widgets_stacked = 0` donc la condition échoue. Les widgets ne sont pas marqués comme empilés !

---

## Fichiers Modifiés

- `src/settings_panel.h` : Ajout de `panel_width_when_stacked`
- `src/settings_panel.c` :
  - Lignes 922-925 : Condition de dépilement
  - Lignes 1169-1186 : Sauvegarde lors de l'empilement
  - Ligne 998 : Réinitialisation après dépilement

---

## Prochaines Étapes

Après avoir exécuté les scripts GDB et capturé les traces :

1. Copier la sortie dans un fichier `debug_output_DATE.txt`
2. Chercher les anomalies par rapport aux traces CORRECTES ci-dessus
3. Identifier le problème exact
4. Proposer un correctif ciblé

---

## Contact

Si les traces révèlent un problème, envoie-moi :
- Le fichier de sortie GDB complet
- Une description de ce que tu as fait (réduire, élargir, etc.)
- La version du script utilisé (simple ou détaillé)
