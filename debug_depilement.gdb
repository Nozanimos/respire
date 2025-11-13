#!/usr/bin/gdb -x
# Script GDB pour tracer le problème de dépilement
# Usage: gdb -x debug_depilement.gdb ./respire

# ════════════════════════════════════════════════════════════════════════════
# BREAKPOINTS PRINCIPAUX
# ════════════════════════════════════════════════════════════════════════════

# 1. Entrée dans recalculate_widget_layout
break recalculate_widget_layout
commands
    silent
    printf "\n"
    printf "════════════════════════════════════════════════════════════════════════\n"
    printf ">>> APPEL recalculate_widget_layout()\n"
    printf "════════════════════════════════════════════════════════════════════════\n"
    printf "  panel_width           = %d\n", panel->rect.w
    printf "  widgets_stacked       = %d\n", panel->widgets_stacked
    printf "  panel_width_when_stacked = %d\n", panel->panel_width_when_stacked
    printf "════════════════════════════════════════════════════════════════════════\n"
    continue
end

# 2. Condition de DÉPILEMENT (ligne 922-924)
break settings_panel.c:922
commands
    silent
    printf "\n"
    printf "┌──────────────────────────────────────────────────────────────────────┐\n"
    printf "│ TEST CONDITION DÉPILEMENT (ligne 922)                               │\n"
    printf "└──────────────────────────────────────────────────────────────────────┘\n"
    printf "  widgets_stacked           = %d\n", panel->widgets_stacked
    printf "  panel_width_when_stacked  = %d\n", panel->panel_width_when_stacked
    printf "  panel_width               = %d\n", panel_width
    printf "  UNSTACK_MARGIN            = 50\n"
    printf "  panel_width >= (saved + MARGIN) ? %d >= %d ? ", panel_width, (panel->panel_width_when_stacked + 50)
    if panel_width >= (panel->panel_width_when_stacked + 50)
        printf "✅ OUI -> VA DÉPILER\n"
    else
        printf "❌ NON -> NE VA PAS DÉPILER\n"
    end
    printf "\n"
    continue
end

# 3. Entrée dans le bloc DÉPILEMENT (ligne 926)
break settings_panel.c:926
commands
    silent
    printf "\n"
    printf "🔄🔄🔄 DÉPILEMENT EN COURS 🔄🔄🔄\n"
    printf "  panel_width = %d, saved_width = %d\n", panel_width, panel->panel_width_when_stacked
    continue
end

# 4. Réinitialisation de panel_width_when_stacked après dépilement (ligne 991)
break settings_panel.c:991
commands
    silent
    printf "\n"
    printf "🔓 RÉINITIALISATION panel_width_when_stacked = 0\n"
    printf "   (widgets_stacked sera mis à false)\n"
    continue
end

# 5. Test condition EMPILEMENT (ligne 1157)
break settings_panel.c:1157
commands
    silent
    printf "\n"
    printf "┌──────────────────────────────────────────────────────────────────────┐\n"
    printf "│ TEST CONDITION EMPILEMENT (ligne 1157)                              │\n"
    printf "└──────────────────────────────────────────────────────────────────────┘\n"
    printf "  needs_reorganization = %d\n", needs_reorganization
    if needs_reorganization
        printf "  ✅ VA EMPILER\n"
    else
        printf "  ❌ NE VA PAS EMPILER\n"
    end
    printf "\n"
    continue
end

# 6. Sauvegarde de panel_width_when_stacked (ligne 1169)
break settings_panel.c:1169
commands
    silent
    printf "\n"
    printf "┌──────────────────────────────────────────────────────────────────────┐\n"
    printf "│ SAUVEGARDE panel_width_when_stacked (ligne 1169)                    │\n"
    printf "└──────────────────────────────────────────────────────────────────────┘\n"
    printf "  panel_width_when_stacked (avant) = %d\n", panel->panel_width_when_stacked
    printf "  panel_width (actuel)              = %d\n", panel_width
    if panel->panel_width_when_stacked == 0
        printf "  ✅ VA SAUVEGARDER (== 0)\n"
    else
        printf "  ⚠️  NE VA PAS SAUVEGARDER (déjà sauvegardé)\n"
    end
    printf "\n"
    continue
end

# 7. Après sauvegarde (ligne 1178)
break settings_panel.c:1178
commands
    silent
    printf "  💾 panel_width_when_stacked (après) = %d\n", panel->panel_width_when_stacked
    printf "\n"
    continue
end

# ════════════════════════════════════════════════════════════════════════════
# WATCHPOINT sur widgets_stacked (à activer manuellement)
# ════════════════════════════════════════════════════════════════════════════
# Après ouverture du panneau, tapez :
#   (gdb) watch panel->widgets_stacked
#   (gdb) continue

# ════════════════════════════════════════════════════════════════════════════
# INSTRUCTIONS
# ════════════════════════════════════════════════════════════════════════════
printf "\n"
printf "════════════════════════════════════════════════════════════════════════\n"
printf "  SCRIPT GDB CHARGÉ : Trace empilement/dépilement\n"
printf "════════════════════════════════════════════════════════════════════════\n"
printf "\n"
printf "ÉTAPES :\n"
printf "  1. (gdb) run\n"
printf "  2. Ouvrir le panneau de configuration (icône engrenage)\n"
printf "  3. CTRL+C pour interrompre\n"
printf "  4. (gdb) watch panel->widgets_stacked\n"
printf "  5. (gdb) continue\n"
printf "  6. Réduire/élargir la fenêtre et observer les traces\n"
printf "\n"
printf "BREAKPOINTS ACTIFS :\n"
printf "  - Entrée recalculate_widget_layout (affiche état)\n"
printf "  - Condition dépilement (ligne 922)\n"
printf "  - Entrée bloc dépilement (ligne 926)\n"
printf "  - Réinitialisation après dépilement (ligne 991)\n"
printf "  - Condition empilement (ligne 1157)\n"
printf "  - Sauvegarde panel_width_when_stacked (ligne 1169)\n"
printf "════════════════════════════════════════════════════════════════════════\n"
printf "\n"

# Lancer le programme
run
