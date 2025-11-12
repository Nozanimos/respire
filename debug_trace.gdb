# ============================================================================
#  SCRIPT GDB - TRACER TOUTES LES FONCTIONS LORS DU RESIZE
# ============================================================================
# UTILISATION: gdb -x debug_trace.gdb ./bin/respire
#
# Ce script trace le flow complet lors du resize de la fenetre
# ============================================================================

set pagination off
set print pretty on
set charset ASCII

# ----------------------------------------------------------------------------
# BREAKPOINT 1: Detection SDL_WINDOWEVENT (resize fenetre)
# ----------------------------------------------------------------------------
break renderer.c:452
commands
    silent
    printf "\n"
    printf "====================================================================\n"
    printf ">>> SDL WINDOW RESIZE DETECTE <<<\n"
    printf "====================================================================\n"
    printf "screen_width = %d\n", app->screen_width
    printf "screen_height = %d\n", app->screen_height
    printf "scale_factor = %.3f\n", app->scale_factor

    if (app->settings_panel)
        printf "widgets_stacked AVANT update_panel_scale = %d\n", app->settings_panel->widgets_stacked
    end

    printf ">>> Appel a update_panel_scale() <<<\n"
    continue
end

# ----------------------------------------------------------------------------
# BREAKPOINT 2: Entree dans update_panel_scale
# ----------------------------------------------------------------------------
break settings_panel.c:485
commands
    silent
    printf "\n"
    printf "====================================================================\n"
    printf ">>> ENTREE: update_panel_scale() <<<\n"
    printf "====================================================================\n"
    printf "panel_width actuel = %d\n", panel->rect.w
    printf "widgets_stacked = %d\n", panel->widgets_stacked
    printf "\n"
    continue
end

# ----------------------------------------------------------------------------
# BREAKPOINT 3: Juste AVANT appel a recalculate_widget_layout
# ----------------------------------------------------------------------------
break settings_panel.c:706
commands
    silent
    printf "\n"
    printf "====================================================================\n"
    printf ">>> AVANT recalculate_widget_layout() <<<\n"
    printf "====================================================================\n"
    printf "panel_width = %d\n", panel->rect.w
    printf "panel_ratio = %.3f\n", panel->panel_ratio
    printf "widgets_stacked = %d\n", panel->widgets_stacked

    # Afficher positions actuelles de quelques widgets cles
    set $node = panel->widget_list->first
    set $idx = 0
    printf "\nPositions AVANT recalculate:\n"
    while ($node && $idx < 12)
        if ($node->type == 2 && $node->widget.label_widget)
            printf "[%d] LABEL: x=%d, y=%d\n", $idx, $node->widget.label_widget->base.x, $node->widget.label_widget->base.y
        end
        if ($node->type == 7 && $node->widget.selector_widget)
            printf "[%d] SELECTOR: x=%d, y=%d\n", $idx, $node->widget.selector_widget->base.x, $node->widget.selector_widget->base.y
        end
        if ($node->type == 3 && $node->widget.separator_widget)
            printf "[%d] SEPARATOR: x=%d, y=%d, width=%d\n", $idx, $node->widget.separator_widget->base.x, $node->widget.separator_widget->base.y, $node->widget.separator_widget->base.width
        end
        set $node = $node->next
        set $idx = $idx + 1
    end

    printf "\n>>> Appel a recalculate_widget_layout() <<<\n"
    continue
end

# ----------------------------------------------------------------------------
# BREAKPOINT 4: Entree dans recalculate_widget_layout
# ----------------------------------------------------------------------------
break settings_panel.c:832
commands
    silent
    printf "\n"
    printf "====================================================================\n"
    printf ">>> ENTREE: recalculate_widget_layout() <<<\n"
    printf "====================================================================\n"
    printf "panel_width = %d\n", panel->rect.w
    printf "widgets_stacked INITIAL = %d\n", panel->widgets_stacked
    printf "\n"
    continue
end

# ----------------------------------------------------------------------------
# BREAKPOINT 5: Dans la restauration preliminaire (ligne 852)
# ----------------------------------------------------------------------------
break settings_panel.c:852
commands
    silent
    printf "\n"
    printf "====================================================================\n"
    printf ">>> TEST RESTAURATION PRELIMINAIRE <<<\n"
    printf "====================================================================\n"
    printf "widgets_stacked = %d\n", panel->widgets_stacked

    if (panel->widgets_stacked)
        printf ">>> RESTAURATION PRELIMINAIRE SE DECLENCHE <<<\n"
        printf "Les widgets vont etre restaures aux positions JSON...\n"
    else
        printf ">>> PAS DE RESTAURATION (widgets deja aux positions JSON) <<<\n"
    end

    continue
end

# ----------------------------------------------------------------------------
# BREAKPOINT 6: Apres restauration preliminaire, avant detection collision
# ----------------------------------------------------------------------------
break settings_panel.c:1056
commands
    silent
    printf "\n"
    printf "====================================================================\n"
    printf ">>> DETECTION DE COLLISION <<<\n"
    printf "====================================================================\n"
    printf "panel_width = %d\n", panel_width
    printf "layout_threshold_width = %d\n", panel->layout_threshold_width
    printf "widgets_stacked = %d\n", panel->widgets_stacked

    # Aller jusqu'apres le calcul de needs_reorganization
    tbreak settings_panel.c:1076
    continue

    printf "\nRESULTAT:\n"
    printf "needs_reorganization = %d\n", needs_reorganization

    if (needs_reorganization)
        printf ">>> COLLISION DETECTEE - EMPILEMENT REQUIS <<<\n"
    else
        printf ">>> AUCUNE COLLISION - POSITIONS RESTAUREES OK <<<\n"
    end

    continue
end

# ----------------------------------------------------------------------------
# BREAKPOINT 7: Test empilement (ligne 1082)
# ----------------------------------------------------------------------------
break settings_panel.c:1082
commands
    silent
    printf "\n"
    printf "====================================================================\n"
    printf ">>> TEST EMPILEMENT <<<\n"
    printf "====================================================================\n"
    printf "needs_reorganization = %d\n", needs_reorganization

    if (needs_reorganization)
        printf ">>> EMPILEMENT VA SE DECLENCHER <<<\n"
        printf "widgets_stacked va passer a 1\n"
    else
        printf ">>> PAS D'EMPILEMENT <<<\n"
        printf "widgets_stacked va passer a 0 (ligne 1187)\n"
    end

    continue
end

# ----------------------------------------------------------------------------
# BREAKPOINT 8: Sortie de recalculate_widget_layout
# ----------------------------------------------------------------------------
break settings_panel.c:1262
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
    while ($node && $idx < 12)
        if ($node->type == 2 && $node->widget.label_widget)
            printf "[%d] LABEL: x=%d, y=%d (base_x=%d, base_y=%d)\n", $idx, $node->widget.label_widget->base.x, $node->widget.label_widget->base.y, $node->widget.label_widget->base.base_x, $node->widget.label_widget->base.base_y
        end
        if ($node->type == 7 && $node->widget.selector_widget)
            printf "[%d] SELECTOR: x=%d, y=%d (base_x=%d, base_y=%d)\n", $idx, $node->widget.selector_widget->base.x, $node->widget.selector_widget->base.y, $node->widget.selector_widget->base.base_x, $node->widget.selector_widget->base.base_y
        end
        if ($node->type == 3 && $node->widget.separator_widget)
            printf "[%d] SEPARATOR: x=%d, y=%d, width=%d\n", $idx, $node->widget.separator_widget->base.x, $node->widget.separator_widget->base.y, $node->widget.separator_widget->base.width
        end
        set $node = $node->next
        set $idx = $idx + 1
    end

    printf "\n====================================================================\n"
    continue
end

# ----------------------------------------------------------------------------
# WATCHPOINT: Surveiller changements de widgets_stacked
# ----------------------------------------------------------------------------
# NOTE: Le watchpoint sera active apres le premier run, une fois panel alloue

# ----------------------------------------------------------------------------
# Lancer le programme
# ----------------------------------------------------------------------------
printf "\n"
printf "Script GDB de trace charge\n"
printf "============================================================\n"
printf "Breakpoints configures:\n"
printf "  1. Detection resize (renderer.c:452)\n"
printf "  2. Entree update_panel_scale (settings_panel.c:485)\n"
printf "  3. Avant recalculate_widget_layout (settings_panel.c:706)\n"
printf "  4. Entree recalculate_widget_layout (settings_panel.c:832)\n"
printf "  5. Test restauration preliminaire (settings_panel.c:852)\n"
printf "  6. Detection collision (settings_panel.c:1056)\n"
printf "  7. Test empilement (settings_panel.c:1082)\n"
printf "  8. Sortie recalculate_widget_layout (settings_panel.c:1262)\n"
printf "============================================================\n"
printf "\nLancement du programme...\n"
printf "Apres ouverture panneau, vous pouvez activer watchpoint:\n"
printf "  (gdb) watch panel->widgets_stacked\n"
printf "\n"

run
