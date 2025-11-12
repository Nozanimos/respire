# ════════════════════════════════════════════════════════════════════════════
#  SCRIPT GDB POUR ANALYSER LES POSITIONS DES WIDGETS
# ════════════════════════════════════════════════════════════════════════════
#
# UTILISATION:
#   gdb -x debug_widgets.gdb ./bin/respire
#
# Ce script place des breakpoints pour capturer les positions des widgets:
# 1. Au chargement initial (après JSON)
# 2. Lors de l'empilement (réduction fenêtre)
# 3. Lors de la restauration (élargissement fenêtre)
# ════════════════════════════════════════════════════════════════════════════

# Désactiver la pagination pour éviter les pauses
set pagination off
set print pretty on

# ─────────────────────────────────────────────────────────────────────────
# BREAKPOINT 1 : Après chargement du JSON
# ─────────────────────────────────────────────────────────────────────────
break json_config_loader.c:577
commands
    silent
    printf "\n"
    printf "═══════════════════════════════════════════════════════════════════\n"
    printf "📷 SNAPSHOT 1: APRÈS CHARGEMENT JSON\n"
    printf "═══════════════════════════════════════════════════════════════════\n"

    # Parcourir la liste des widgets
    set $node = list->first
    set $index = 0
    while ($node != 0)
        printf "\n[Widget %d] Type: ", $index

        # Afficher selon le type
        if ($node->type == 2)
            # LABEL
            printf "LABEL '%s'\n", $node->widget.label_widget->text
            printf "  Position: x=%d, y=%d\n", $node->widget.label_widget->base.x, $node->widget.label_widget->base.y
            printf "  Base: base_x=%d, base_y=%d\n", $node->widget.label_widget->base.base_x, $node->widget.label_widget->base.base_y
            printf "  Alignment: "
            if ($node->widget.label_widget->alignment == 0)
                printf "LEFT\n"
            else
                if ($node->widget.label_widget->alignment == 1)
                    printf "CENTER\n"
                else
                    printf "RIGHT\n"
                end
            end
        end

        if ($node->type == 7)
            # SELECTOR
            printf "SELECTOR '%s'\n", $node->widget.selector_widget->nom_affichage
            printf "  Position: x=%d, y=%d\n", $node->widget.selector_widget->base.x, $node->widget.selector_widget->base.y
            printf "  Base: base_x=%d, base_y=%d\n", $node->widget.selector_widget->base.base_x, $node->widget.selector_widget->base.base_y
        end

        if ($node->type == 3)
            # SEPARATOR
            printf "SEPARATOR\n"
            printf "  Position: x=%d, y=%d\n", $node->widget.separator_widget->base.x, $node->widget.separator_widget->base.y
            printf "  Base Y: base_y=%d\n", $node->widget.separator_widget->base.base_y
            printf "  Width: %d\n", $node->widget.separator_widget->base.width
        end

        set $node = $node->next
        set $index = $index + 1
    end

    printf "\n═══════════════════════════════════════════════════════════════════\n"
    continue
end

# ─────────────────────────────────────────────────────────────────────────
# BREAKPOINT 2 : Lors de l'empilement (needs_reorganization = true)
# ─────────────────────────────────────────────────────────────────────────
break settings_panel.c:1087
condition 2 needs_reorganization == 1
commands
    silent
    printf "\n"
    printf "═══════════════════════════════════════════════════════════════════\n"
    printf "📷 SNAPSHOT 2: EMPILEMENT (réduction fenêtre)\n"
    printf "═══════════════════════════════════════════════════════════════════\n"
    printf "Panel width: %d\n", panel_width
    printf "widgets_stacked avant: %d\n", panel->widgets_stacked

    # Continuer jusqu'après l'empilement
    finish

    # Afficher les positions après empilement
    set $node = panel->widget_list->first
    set $index = 0
    while ($node != 0)
        if ($node->type == 2)
            # LABEL
            printf "\n[%d] LABEL '%s': x=%d, y=%d\n", $index, $node->widget.label_widget->text, $node->widget.label_widget->base.x, $node->widget.label_widget->base.y
        end
        if ($node->type == 7)
            # SELECTOR
            printf "\n[%d] SELECTOR '%s': x=%d, y=%d\n", $index, $node->widget.selector_widget->nom_affichage, $node->widget.selector_widget->base.x, $node->widget.selector_widget->base.y
        end
        if ($node->type == 3)
            # SEPARATOR
            printf "\n[%d] SEPARATOR: x=%d, y=%d, width=%d\n", $index, $node->widget.separator_widget->base.x, $node->widget.separator_widget->base.y, $node->widget.separator_widget->base.width
        end
        set $node = $node->next
        set $index = $index + 1
    end

    printf "\nwidgets_stacked après: %d\n", panel->widgets_stacked
    printf "═══════════════════════════════════════════════════════════════════\n"
    continue
end

# ─────────────────────────────────────────────────────────────────────────
# BREAKPOINT 3 : Lors de la restauration (widgets_stacked = true)
# ─────────────────────────────────────────────────────────────────────────
break settings_panel.c:855
condition 3 panel->widgets_stacked == 1
commands
    silent
    printf "\n"
    printf "═══════════════════════════════════════════════════════════════════\n"
    printf "📷 SNAPSHOT 3: RESTAURATION (élargissement fenêtre)\n"
    printf "═══════════════════════════════════════════════════════════════════\n"
    printf "Panel width: %d\n", panel_width
    printf "panel_ratio: %.3f\n", panel_ratio

    # Afficher AVANT restauration
    printf "\n--- AVANT RESTAURATION ---\n"
    set $node = panel->widget_list->first
    set $index = 0
    while ($node != 0)
        if ($node->type == 2)
            printf "[%d] LABEL '%s': x=%d, y=%d (base_x=%d, base_y=%d)\n", $index, $node->widget.label_widget->text, $node->widget.label_widget->base.x, $node->widget.label_widget->base.y, $node->widget.label_widget->base.base_x, $node->widget.label_widget->base.base_y
        end
        if ($node->type == 7)
            printf "[%d] SELECTOR '%s': x=%d, y=%d (base_x=%d, base_y=%d)\n", $index, $node->widget.selector_widget->nom_affichage, $node->widget.selector_widget->base.x, $node->widget.selector_widget->base.y, $node->widget.selector_widget->base.base_x, $node->widget.selector_widget->base.base_y
        end
        set $node = $node->next
        set $index = $index + 1
    end

    # Continuer jusqu'à la fin de la restauration
    finish

    # Afficher APRÈS restauration
    printf "\n--- APRÈS RESTAURATION ---\n"
    set $node = panel->widget_list->first
    set $index = 0
    while ($node != 0)
        if ($node->type == 2)
            printf "[%d] LABEL '%s': x=%d, y=%d\n", $index, $node->widget.label_widget->text, $node->widget.label_widget->base.x, $node->widget.label_widget->base.y
        end
        if ($node->type == 7)
            printf "[%d] SELECTOR '%s': x=%d, y=%d\n", $index, $node->widget.selector_widget->nom_affichage, $node->widget.selector_widget->base.x, $node->widget.selector_widget->base.y
        end
        set $node = $node->next
        set $index = $index + 1
    end

    printf "\nwidgets_stacked après: %d\n", panel->widgets_stacked
    printf "═══════════════════════════════════════════════════════════════════\n"
    continue
end

# ─────────────────────────────────────────────────────────────────────────
# LANCER LE PROGRAMME
# ─────────────────────────────────────────────────────────────────────────
printf "\n🔍 Script GDB chargé - Breakpoints configurés:\n"
printf "  1. Après chargement JSON (json_config_loader.c:577)\n"
printf "  2. Lors de l'empilement (settings_panel.c:1087)\n"
printf "  3. Lors de la restauration (settings_panel.c:855)\n"
printf "\n▶ Lancement du programme...\n\n"

run
