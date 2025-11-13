#!/usr/bin/gdb -x
# Script GDB SIMPLE pour debug rapide
# Usage: gdb -x debug_simple.gdb ./respire

# Breakpoint simple sur recalculate_widget_layout
break recalculate_widget_layout
commands
    printf "\n═══ recalculate_widget_layout ═══\n"
    printf "panel_width=%d stacked=%d when_stacked=%d\n", panel->rect.w, panel->widgets_stacked, panel->panel_width_when_stacked
    continue
end

# Breakpoint sur l'empilement
break settings_panel.c:1177
commands
    printf "\n🔧 EMPILEMENT widgets_stacked → true\n"
    continue
end

# Breakpoint sur le dépilement
break settings_panel.c:999
commands
    printf "\n🔄 DÉPILEMENT widgets_stacked → false\n"
    continue
end

printf "\n════════════════════════════════════════════════\n"
printf "SCRIPT GDB SIMPLE CHARGÉ\n"
printf "════════════════════════════════════════════════\n"
printf "Ce script trace :\n"
printf "  - Chaque appel à recalculate_widget_layout\n"
printf "  - Quand widgets_stacked passe à true\n"
printf "  - Quand widgets_stacked passe à false\n"
printf "\n"
printf "ÉTAPES :\n"
printf "  1. Ouvrir le panneau (engrenage)\n"
printf "  2. Réduire/élargir la fenêtre\n"
printf "  3. Observer les traces\n"
printf "════════════════════════════════════════════════\n"
printf "\n"

run
