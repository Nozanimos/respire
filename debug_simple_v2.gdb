#!/usr/bin/gdb -x
# Script GDB SIMPLE pour debug rapide (VERSION 2)
# Usage: gdb -x debug_simple_v2.gdb ./respire

# Breakpoint simple sur recalculate_widget_layout
break recalculate_widget_layout
commands
    printf "\n═══ recalculate_widget_layout ═══\n"
    printf "panel_width=%d stacked=%d min_width=%d\n", panel->rect.w, panel->widgets_stacked, panel->min_width_for_unstack
    continue
end

# Breakpoint sur l'empilement
break settings_panel.c:1169
commands
    printf "\n🔧 EMPILEMENT widgets_stacked → true (panel_width=%d)\n", panel_width
    continue
end

# Breakpoint sur le dépilement
break settings_panel.c:992
commands
    printf "\n🔄 DÉPILEMENT widgets_stacked → false (panel_width=%d >= min_width=%d)\n", panel_width, panel->min_width_for_unstack
    continue
end

printf "\n════════════════════════════════════════════════\n"
printf "SCRIPT GDB SIMPLE V2 CHARGÉ\n"
printf "════════════════════════════════════════════════\n"
printf "Ce script trace :\n"
printf "  - Chaque appel à recalculate_widget_layout\n"
printf "  - Quand widgets_stacked passe à true\n"
printf "  - Quand widgets_stacked passe à false\n"
printf "\n"
printf "FIX : Utilise min_width_for_unstack (calculé depuis JSON)\n"
printf "      au lieu de panel_width_when_stacked + marge\n"
printf "\n"
printf "ÉTAPES :\n"
printf "  1. Ouvrir le panneau (engrenage)\n"
printf "  2. Réduire/élargir la fenêtre\n"
printf "  3. Observer les traces\n"
printf "════════════════════════════════════════════════\n"
printf "\n"

run
