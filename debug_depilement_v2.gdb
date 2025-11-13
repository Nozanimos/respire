#!/usr/bin/gdb -x
# Script GDB pour tracer le problème de dépilement (VERSION 2)
# Usage: gdb -x debug_depilement_v2.gdb ./respire

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
    printf "  panel_width              = %d\n", panel->rect.w
    printf "  widgets_stacked          = %d\n", panel->widgets_stacked
    printf "  min_width_for_unstack    = %d\n", panel->min_width_for_unstack
    printf "════════════════════════════════════════════════════════════════════════\n"
    continue
end

# 2. Condition de DÉPILEMENT (ligne 921-922)
break settings_panel.c:921
commands
    silent
    printf "\n"
    printf "┌──────────────────────────────────────────────────────────────────────┐\n"
    printf "│ TEST CONDITION DÉPILEMENT (ligne 921)                               │\n"
    printf "└──────────────────────────────────────────────────────────────────────┘\n"
    printf "  widgets_stacked           = %d\n", panel->widgets_stacked
    printf "  min_width_for_unstack     = %d\n", panel->min_width_for_unstack
    printf "  panel_width               = %d\n", panel_width
    printf "  panel_width >= min_width ? %d >= %d ? ", panel_width, panel->min_width_for_unstack
    if panel_width >= panel->min_width_for_unstack
        printf "✅ OUI -> VA DÉPILER\n"
    else
        printf "❌ NON -> NE VA PAS DÉPILER (manque %dpx)\n", (panel->min_width_for_unstack - panel_width)
    end
    printf "\n"
    continue
end

# 3. Entrée dans le bloc DÉPILEMENT (ligne 924)
break settings_panel.c:924
commands
    silent
    printf "\n"
    printf "🔄🔄🔄 DÉPILEMENT EN COURS 🔄🔄🔄\n"
    printf "  panel_width = %d, min_width_for_unstack = %d\n", panel_width, panel->min_width_for_unstack
    continue
end

# 4. Fin du dépilement (ligne 992)
break settings_panel.c:992
commands
    silent
    printf "\n"
    printf "✅ DÉPILEMENT TERMINÉ (widgets_stacked → false)\n"
    continue
end

# 5. Test condition EMPILEMENT (ligne 1165)
break settings_panel.c:1165
commands
    silent
    printf "\n"
    printf "┌──────────────────────────────────────────────────────────────────────┐\n"
    printf "│ TEST CONDITION EMPILEMENT (ligne 1165)                              │\n"
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

# 6. Empilement effectif (ligne 1169)
break settings_panel.c:1169
commands
    silent
    printf "\n"
    printf "┌──────────────────────────────────────────────────────────────────────┐\n"
    printf "│ EMPILEMENT EN COURS (ligne 1169)                                    │\n"
    printf "└──────────────────────────────────────────────────────────────────────┘\n"
    printf "  panel_width              = %d\n", panel_width
    printf "  min_width_for_unstack    = %d\n", panel->min_width_for_unstack
    printf "  widgets_stacked → true\n"
    printf "\n"
    continue
end

# ════════════════════════════════════════════════════════════════════════════
# INSTRUCTIONS
# ════════════════════════════════════════════════════════════════════════════
printf "\n"
printf "════════════════════════════════════════════════════════════════════════\n"
printf "  SCRIPT GDB V2 CHARGÉ : Trace empilement/dépilement\n"
printf "  FIX : Utilise min_width_for_unstack au lieu de panel_width_when_stacked\n"
printf "════════════════════════════════════════════════════════════════════════\n"
printf "\n"
printf "ÉTAPES :\n"
printf "  1. (gdb) run\n"
printf "  2. Ouvrir le panneau de configuration (icône engrenage)\n"
printf "  3. Réduire/élargir la fenêtre et observer les traces\n"
printf "\n"
printf "BREAKPOINTS ACTIFS :\n"
printf "  - Entrée recalculate_widget_layout (affiche état)\n"
printf "  - Condition dépilement (ligne 921)\n"
printf "  - Entrée bloc dépilement (ligne 924)\n"
printf "  - Fin dépilement (ligne 992)\n"
printf "  - Condition empilement (ligne 1165)\n"
printf "  - Empilement effectif (ligne 1169)\n"
printf "════════════════════════════════════════════════════════════════════════\n"
printf "\n"

# Lancer le programme
run
