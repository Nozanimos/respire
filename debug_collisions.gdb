#!/usr/bin/gdb -x
# Script GDB pour tracer les COLLISIONS détectées
# Usage: gdb -x debug_collisions.gdb ./respire

# Breakpoint sur la détection de collision (ligne 1139-1150)
break settings_panel.c:1139
commands
    silent
    printf "\n"
    printf "════════════════════════════════════════════════════════════════════════\n"
    printf ">>> TEST COLLISION (ligne 1139)\n"
    printf "════════════════════════════════════════════════════════════════════════\n"
    printf "  panel_width = %d\n", panel_width
    printf "  widgets_stacked = %d\n", panel->widgets_stacked
    printf "  min_width_for_unstack = %d\n", panel->min_width_for_unstack
    printf "  Nombre de widgets testés: %d\n", rect_count
    printf "\n"
    continue
end

# Breakpoint quand collision détectée (ligne 1141-1147)
break settings_panel.c:1141
commands
    silent
    printf "\n"
    printf "⚠️  COLLISION DÉTECTÉE !\n"
    printf "  Widget[%d] (type=%d) : x=%d y=%d w=%d h=%d\n", i, rects[i].type, rects[i].x, rects[i].y, rects[i].width, rects[i].height
    printf "  Widget[%d] (type=%d) : x=%d y=%d w=%d h=%d\n", j, rects[j].type, rects[j].x, rects[j].y, rects[j].width, rects[j].height
    printf "\n"
    continue
end

# Breakpoint après test de collision (ligne 1152-1157)
break settings_panel.c:1152
commands
    silent
    if needs_reorganization
        printf "✅ DÉCISION: Besoin de réorganisation (collision(s) trouvée(s))\n"
    else
        printf "✅ DÉCISION: Pas de réorganisation nécessaire\n"
    end
    printf "\n"
    continue
end

# Breakpoint sur le calcul de min_width_for_unstack (ligne 264)
break settings_panel.c:264
commands
    silent
    printf "\n"
    printf "════════════════════════════════════════════════════════════════════════\n"
    printf ">>> CALCUL min_width_for_unstack (ligne 264)\n"
    printf "════════════════════════════════════════════════════════════════════════\n"
    continue
end

# Breakpoint après calcul de min_width_for_unstack (ligne 265)
break settings_panel.c:265
commands
    silent
    printf "  min_width_for_unstack = %d\n", panel->min_width_for_unstack
    printf "════════════════════════════════════════════════════════════════════════\n"
    printf "\n"
    continue
end

printf "\n"
printf "════════════════════════════════════════════════════════════════════════\n"
printf "  SCRIPT GDB COLLISIONS CHARGÉ\n"
printf "════════════════════════════════════════════════════════════════════════\n"
printf "\n"
printf "Ce script trace:\n"
printf "  - Calcul de min_width_for_unstack au démarrage\n"
printf "  - Test de collision dans recalculate_widget_layout\n"
printf "  - Chaque collision détectée (widgets en conflit)\n"
printf "  - Décision de réorganisation\n"
printf "\n"
printf "ÉTAPES:\n"
printf "  1. Ouvrir le panneau\n"
printf "  2. Réduire/élargir la fenêtre\n"
printf "  3. Observer quelles collisions sont détectées\n"
printf "════════════════════════════════════════════════════════════════════════\n"
printf "\n"

run
