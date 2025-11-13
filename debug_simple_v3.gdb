#!/usr/bin/gdb -x
# Script GDB V3 : Mémoire persistante
# Usage: gdb -x debug_simple_v3.gdb ./respire

break recalculate_widget_layout
commands
    printf "\n═══ recalculate_widget_layout ═══\n"
    printf "panel_width=%d stacked=%d when_stacked=%d\n", panel->rect.w, panel->widgets_stacked, panel->panel_width_when_stacked
    continue
end

break settings_panel.c:1178
commands
    printf "\n🔧 EMPILEMENT → true (panel_width=%d)\n", panel_width
    continue
end

break settings_panel.c:1187
commands
    if panel->panel_width_when_stacked == 0
        printf "   💾 SAUVEGARDE when_stacked=%d (PREMIER empilement)\n", panel_width
    else
        printf "   ♻️  when_stacked=%d déjà sauvegardé (ré-empilement)\n", panel->panel_width_when_stacked
    end
    continue
end

break settings_panel.c:1000
commands
    printf "\n🔄 DÉPILEMENT → false (panel_width=%d >= when_stacked=%d + 80)\n", panel_width, panel->panel_width_when_stacked
    printf "   📌 when_stacked=%d GARDÉ en mémoire\n", panel->panel_width_when_stacked
    continue
end

break settings_panel.c:331
commands
    printf "\n🚪 PANNEAU FERMÉ → Réinitialisation when_stacked=0\n"
    continue
end

printf "\n════════════════════════════════════════════════\n"
printf "SCRIPT GDB V3 : MÉMOIRE PERSISTANTE\n"
printf "════════════════════════════════════════════════\n"
printf "FIX:\n"
printf "  - Sauvegarde when_stacked lors du PREMIER empilement\n"
printf "  - NE réinitialise JAMAIS (sauf fermeture panneau)\n"
printf "  - Dépile si panel_width >= when_stacked + 80px\n"
printf "\n"
printf "Traces:\n"
printf "  - Appels recalculate_widget_layout\n"
printf "  - Empilements (premier vs ré-empilement)\n"
printf "  - Dépilements avec mémoire persistante\n"
printf "  - Fermeture panneau (réinitialisation)\n"
printf "════════════════════════════════════════════════\n"
printf "\n"

run
