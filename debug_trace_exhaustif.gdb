# ============================================================================
#  SCRIPT GDB EXHAUSTIF - TRACER TOUTES LES FONCTIONS APPELÉES
# ============================================================================
# UTILISATION: gdb -x debug_trace_exhaustif.gdb ./bin/respire
# ============================================================================

set pagination off
set print pretty on
set charset ASCII

# Activer le traçage des appels de fonctions
set trace-commands on

# ----------------------------------------------------------------------------
# TRACER TOUTES LES FONCTIONS DE settings_panel.c
# ----------------------------------------------------------------------------

# Fonction principale de recalcul
break recalculate_widget_layout
commands
    silent
    printf "\n"
    printf "====================================================================\n"
    printf ">>> APPEL: recalculate_widget_layout()\n"
    printf "====================================================================\n"
    printf "panel_width = %d\n", panel->rect.w
    printf "panel_ratio = %.3f\n", panel->panel_ratio
    printf "widgets_stacked = %d\n", panel->widgets_stacked
    printf "min_width_for_unstack = %d\n", panel->min_width_for_unstack

    # Afficher le call stack pour savoir qui appelle cette fonction
    printf "\nCall stack:\n"
    backtrace 5

    continue
end

# Fonction update_panel_scale
break update_panel_scale
commands
    silent
    printf "\n"
    printf "====================================================================\n"
    printf ">>> APPEL: update_panel_scale()\n"
    printf "====================================================================\n"
    printf "screen_width = %d, screen_height = %d\n", screen_width, screen_height
    printf "widgets_stacked AVANT = %d\n", panel->widgets_stacked

    printf "\nCall stack:\n"
    backtrace 5

    continue
end

# ----------------------------------------------------------------------------
# TRACER TOUTES LES FONCTIONS DE widget_list.c QUI TOUCHENT AUX POSITIONS
# ----------------------------------------------------------------------------

# Rechercher les fonctions qui contiennent "scale", "position", "layout"
break render_all_widgets
commands
    silent
    printf "\n>>> APPEL: render_all_widgets()\n"
    backtrace 3
    continue
end

break update_widget_list_animations
commands
    silent
    printf "\n>>> APPEL: update_widget_list_animations()\n"
    backtrace 3
    continue
end

# ----------------------------------------------------------------------------
# WATCHPOINT sur widgets_stacked (s'active après premier breakpoint)
# ----------------------------------------------------------------------------
# Note: sera activé manuellement après que panel soit alloué

# ----------------------------------------------------------------------------
# BREAKPOINT sur la restauration (ligne ~916)
# ----------------------------------------------------------------------------
break settings_panel.c:916
commands
    silent
    printf "\n"
    printf "====================================================================\n"
    printf ">>> CONDITION DE DEPILEMENT <<<\n"
    printf "====================================================================\n"
    printf "widgets_stacked = %d\n", panel->widgets_stacked
    printf "panel_width = %d\n", panel_width
    printf "min_width_for_unstack = %d\n", panel->min_width_for_unstack
    printf "Condition (stacked && width >= min_width) = %d\n", (panel->widgets_stacked && panel_width >= panel->min_width_for_unstack)

    if (panel->widgets_stacked && panel_width >= panel->min_width_for_unstack)
        printf "\n>>> DEPILEMENT VA SE DECLENCHER <<<\n"

        # Aller jusqu'après la restauration (ligne 981)
        tbreak settings_panel.c:981
        continue

        printf "\nAPRES DEPILEMENT:\n"
        printf "widgets_stacked = %d\n", panel->widgets_stacked

        # Afficher quelques positions
        set $node = panel->widget_list->first
        set $idx = 0
        while ($node && $idx < 5)
            if ($node->type == 0 && $node->widget.increment_widget)
                printf "[%d] INCREMENT: x=%d, y=%d\n", $idx, $node->widget.increment_widget->base.x, $node->widget.increment_widget->base.y
            end
            if ($node->type == 7 && $node->widget.selector_widget)
                printf "[%d] SELECTOR: x=%d, y=%d\n", $idx, $node->widget.selector_widget->base.x, $node->widget.selector_widget->base.y
            end
            set $node = $node->next
            set $idx = $idx + 1
        end
    else
        printf "\n>>> DEPILEMENT NE SE DECLENCHE PAS <<<\n"
        if (!panel->widgets_stacked)
            printf "Raison: widgets pas empiles\n"
        end
        if (panel_width < panel->min_width_for_unstack)
            printf "Raison: panel_width (%d) < min_width (%d)\n", panel_width, panel->min_width_for_unstack
        end
    end

    continue
end

# ----------------------------------------------------------------------------
# BREAKPOINT sur l'empilement (ligne ~1147)
# ----------------------------------------------------------------------------
break settings_panel.c:1147
commands
    silent
    printf "\n"
    printf "====================================================================\n"
    printf ">>> CONDITION D'EMPILEMENT <<<\n"
    printf "====================================================================\n"
    printf "needs_reorganization = %d\n", needs_reorganization

    if (needs_reorganization)
        printf "\n>>> EMPILEMENT VA SE DECLENCHER <<<\n"
        printf "widgets_stacked va passer a 1\n"

        # Sauvegarder min_width_for_unstack qui va être calculé
        tbreak settings_panel.c:1260
        continue

        printf "\nAPRES EMPILEMENT:\n"
        printf "widgets_stacked = %d\n", panel->widgets_stacked
        printf "min_width_for_unstack calculé = %d\n", panel->min_width_for_unstack
    else
        printf "\n>>> PAS D'EMPILEMENT <<<\n"
    end

    continue
end

# ----------------------------------------------------------------------------
# BREAKPOINT après le goto calculate_heights (ligne 1262)
# ----------------------------------------------------------------------------
break settings_panel.c:1262
commands
    silent
    printf "\n"
    printf "====================================================================\n"
    printf ">>> LABEL calculate_heights atteint <<<\n"
    printf "====================================================================\n"
    printf "widgets_stacked = %d\n", panel->widgets_stacked

    # Afficher positions de quelques widgets
    set $node = panel->widget_list->first
    set $idx = 0
    printf "\nPositions actuelles:\n"
    while ($node && $idx < 8)
        if ($node->type == 0 && $node->widget.increment_widget)
            printf "[%d] INCREMENT: x=%d, y=%d (base_x=%d, base_y=%d)\n", $idx, $node->widget.increment_widget->base.x, $node->widget.increment_widget->base.y, $node->widget.increment_widget->base.base_x, $node->widget.increment_widget->base.base_y
        end
        if ($node->type == 7 && $node->widget.selector_widget)
            printf "[%d] SELECTOR: x=%d, y=%d (base_x=%d, base_y=%d)\n", $idx, $node->widget.selector_widget->base.x, $node->widget.selector_widget->base.y, $node->widget.selector_widget->base.base_x, $node->widget.selector_widget->base.base_y
        end
        if ($node->type == 3 && $node->widget.separator_widget)
            printf "[%d] SEPARATOR: x=%d, y=%d\n", $idx, $node->widget.separator_widget->base.x, $node->widget.separator_widget->base.y
        end
        set $node = $node->next
        set $idx = $idx + 1
    end

    continue
end

# ----------------------------------------------------------------------------
# BREAKPOINT sortie de recalculate_widget_layout (fin de fonction)
# ----------------------------------------------------------------------------
break settings_panel.c:1330
commands
    silent
    printf "\n"
    printf "====================================================================\n"
    printf ">>> SORTIE: recalculate_widget_layout() <<<\n"
    printf "====================================================================\n"
    printf "widgets_stacked FINAL = %d\n", panel->widgets_stacked

    # Afficher positions finales
    set $node = panel->widget_list->first
    set $idx = 0
    printf "\nPositions FINALES:\n"
    while ($node && $idx < 8)
        if ($node->type == 0 && $node->widget.increment_widget)
            printf "[%d] INCREMENT: x=%d, y=%d\n", $idx, $node->widget.increment_widget->base.x, $node->widget.increment_widget->base.y
        end
        if ($node->type == 7 && $node->widget.selector_widget)
            printf "[%d] SELECTOR: x=%d, y=%d\n", $idx, $node->widget.selector_widget->base.x, $node->widget.selector_widget->base.y
        end
        set $node = $node->next
        set $idx = $idx + 1
    end

    printf "\n====================================================================\n"
    continue
end

# ----------------------------------------------------------------------------
# Instructions pour activer le watchpoint
# ----------------------------------------------------------------------------
printf "\n"
printf "============================================================\n"
printf "Script GDB exhaustif charge\n"
printf "============================================================\n"
printf "Breakpoints configures:\n"
printf "  - recalculate_widget_layout()\n"
printf "  - update_panel_scale()\n"
printf "  - Condition depilement (ligne 916)\n"
printf "  - Condition empilement (ligne 1147)\n"
printf "  - Label calculate_heights (ligne 1262)\n"
printf "  - Sortie recalculate_widget_layout (ligne 1330)\n"
printf "============================================================\n"
printf "\nAPRES ouverture du panneau, activez le watchpoint:\n"
printf "  (gdb) watch panel->widgets_stacked\n"
printf "  (gdb) continue\n"
printf "\nPuis testez:\n"
printf "  1. Reduire fenetre -> empile\n"
printf "  2. Elargir fenetre -> devrait depiler\n"
printf "============================================================\n"
printf "\n"

run
