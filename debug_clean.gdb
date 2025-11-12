# ============================================================================
#  SCRIPT GDB POUR ANALYSER LE BUG DE RESTAURATION DES WIDGETS
# ============================================================================
# UTILISATION: gdb -x debug_clean.gdb ./respire
# ============================================================================

# Configuration
set pagination off
set print pretty on
set charset ASCII

# ----------------------------------------------------------------------------
# BREAKPOINT 1: Apres chargement JSON (positions initiales)
# ----------------------------------------------------------------------------
break json_config_loader.c:577
commands
    silent
    printf "\n"
    printf "====================================================================\n"
    printf "SNAPSHOT 1: APRES CHARGEMENT JSON\n"
    printf "====================================================================\n"

    set $node = list->first
    set $index = 0
    while ($node != 0)
        printf "[%d] ", $index

        if ($node->type == 2)
            printf "LABEL '%s': x=%d, y=%d (base_x=%d, base_y=%d) align=%d\n", $node->widget.label_widget->text, $node->widget.label_widget->base.x, $node->widget.label_widget->base.y, $node->widget.label_widget->base.base_x, $node->widget.label_widget->base.base_y, $node->widget.label_widget->alignment
        end
        if ($node->type == 7)
            printf "SELECTOR '%s': x=%d, y=%d (base_x=%d, base_y=%d)\n", $node->widget.selector_widget->nom_affichage, $node->widget.selector_widget->base.x, $node->widget.selector_widget->base.y, $node->widget.selector_widget->base.base_x, $node->widget.selector_widget->base.base_y
        end
        if ($node->type == 3)
            printf "SEPARATOR: x=%d, y=%d, width=%d (base_y=%d, margins=%d/%d)\n", $node->widget.separator_widget->base.x, $node->widget.separator_widget->base.y, $node->widget.separator_widget->base.width, $node->widget.separator_widget->base.base_y, $node->widget.separator_widget->base_start_margin, $node->widget.separator_widget->base_end_margin
        end
        if ($node->type == 4)
            printf "PREVIEW: x=%d, y=%d (base_x=%d, base_y=%d)\n", $node->widget.preview_widget->base.x, $node->widget.preview_widget->base.y, $node->widget.preview_widget->base.base_x, $node->widget.preview_widget->base.base_y
        end
        if ($node->type == 0)
            printf "INCREMENT: x=%d, y=%d (base_x=%d, base_y=%d)\n", $node->widget.increment_widget->base.x, $node->widget.increment_widget->base.y, $node->widget.increment_widget->base.base_x, $node->widget.increment_widget->base.base_y
        end
        if ($node->type == 1)
            printf "TOGGLE '%s': x=%d, y=%d (base_x=%d, base_y=%d)\n", $node->widget.toggle_widget->option_name, $node->widget.toggle_widget->base.x, $node->widget.toggle_widget->base.y, $node->widget.toggle_widget->base.base_x, $node->widget.toggle_widget->base.base_y
        end

        set $node = $node->next
        set $index = $index + 1
    end

    printf "====================================================================\n"
    continue
end

# ----------------------------------------------------------------------------
# BREAKPOINT 2: Calcul de needs_reorganization (ligne 988)
# ----------------------------------------------------------------------------
break settings_panel.c:988
commands
    silent
    printf "\n"
    printf "====================================================================\n"
    printf "SNAPSHOT 2: CALCUL DE needs_reorganization\n"
    printf "====================================================================\n"
    printf "panel_width = %d\n", panel_width
    printf "layout_threshold_width = %d\n", panel->layout_threshold_width
    printf "widgets_stacked (AVANT calcul) = %d\n", panel->widgets_stacked

    # Continuer jusqu'apres le calcul (ligne 1008)
    tbreak settings_panel.c:1008
    continue

    printf "\nRESULTAT:\n"
    printf "needs_reorganization = %d\n", needs_reorganization
    printf "====================================================================\n"
    continue
end

# ----------------------------------------------------------------------------
# BREAKPOINT 3: Debut de la restauration (ligne 1016)
# ----------------------------------------------------------------------------
break settings_panel.c:1016
commands
    silent
    printf "\n"
    printf "====================================================================\n"
    printf "SNAPSHOT 3: CONDITION DE RESTAURATION\n"
    printf "====================================================================\n"
    printf "widgets_stacked = %d\n", panel->widgets_stacked
    printf "needs_reorganization = %d\n", needs_reorganization
    printf "Condition (widgets_stacked && !needs_reorganization) = %d\n", (panel->widgets_stacked && !needs_reorganization)

    if (panel->widgets_stacked && !needs_reorganization)
        printf "\n>>> RESTAURATION VA SE DECLENCHER <<<\n"

        printf "\nAVANT RESTAURATION:\n"
        set $node = panel->widget_list->first
        set $index = 0
        while ($node != 0)
            if ($node->type == 2)
                printf "[%d] LABEL: x=%d, y=%d\n", $index, $node->widget.label_widget->base.x, $node->widget.label_widget->base.y
            end
            if ($node->type == 7)
                printf "[%d] SELECTOR: x=%d, y=%d\n", $index, $node->widget.selector_widget->base.x, $node->widget.selector_widget->base.y
            end
            set $node = $node->next
            set $index = $index + 1
        end

        # Aller jusqu'a la ligne 1094 (apres restauration)
        tbreak settings_panel.c:1094
        continue

        printf "\nAPRES RESTAURATION:\n"
        set $node = panel->widget_list->first
        set $index = 0
        while ($node != 0)
            if ($node->type == 2)
                printf "[%d] LABEL: x=%d, y=%d\n", $index, $node->widget.label_widget->base.x, $node->widget.label_widget->base.y
            end
            if ($node->type == 7)
                printf "[%d] SELECTOR: x=%d, y=%d\n", $index, $node->widget.selector_widget->base.x, $node->widget.selector_widget->base.y
            end
            set $node = $node->next
            set $index = $index + 1
        end

        printf "\nwidgets_stacked (apres restauration) = %d\n", panel->widgets_stacked
    else
        printf "\n>>> RESTAURATION NE SE DECLENCHE PAS <<<\n"
    end

    printf "====================================================================\n"
    continue
end

# ----------------------------------------------------------------------------
# BREAKPOINT 4: Debut de l'empilement (ligne 1099)
# ----------------------------------------------------------------------------
break settings_panel.c:1099
commands
    silent
    printf "\n"
    printf "====================================================================\n"
    printf "SNAPSHOT 4: CONDITION D'EMPILEMENT\n"
    printf "====================================================================\n"
    printf "needs_reorganization = %d\n", needs_reorganization

    if (needs_reorganization)
        printf "\n>>> EMPILEMENT VA SE DECLENCHER <<<\n"
        printf "widgets_stacked (avant empilement) = %d\n", panel->widgets_stacked

        # Aller jusqu'a la ligne 1204 (apres empilement)
        tbreak settings_panel.c:1204
        continue

        printf "\nAPRES EMPILEMENT:\n"
        printf "widgets_stacked (apres empilement) = %d\n", panel->widgets_stacked

        set $node = panel->widget_list->first
        set $index = 0
        printf "\nPositions apres empilement:\n"
        while ($node != 0)
            if ($node->type == 2)
                printf "[%d] LABEL: x=%d, y=%d\n", $index, $node->widget.label_widget->base.x, $node->widget.label_widget->base.y
            end
            if ($node->type == 7)
                printf "[%d] SELECTOR: x=%d, y=%d\n", $index, $node->widget.selector_widget->base.x, $node->widget.selector_widget->base.y
            end
            if ($node->type == 3)
                printf "[%d] SEPARATOR: x=%d, y=%d, width=%d\n", $index, $node->widget.separator_widget->base.x, $node->widget.separator_widget->base.y, $node->widget.separator_widget->base.width
            end
            set $node = $node->next
            set $index = $index + 1
        end
    else
        printf "\n>>> EMPILEMENT NE SE DECLENCHE PAS <<<\n"
    end

    printf "====================================================================\n"
    continue
end

# ----------------------------------------------------------------------------
# Lancer le programme
# ----------------------------------------------------------------------------
printf "\n"
printf "Script GDB charge - Breakpoints configures:\n"
printf "  1. Apres chargement JSON (json_config_loader.c:577)\n"
printf "  2. Calcul needs_reorganization (settings_panel.c:988)\n"
printf "  3. Condition restauration (settings_panel.c:1016)\n"
printf "  4. Condition empilement (settings_panel.c:1099)\n"
printf "\nLancement du programme...\n\n"

run
