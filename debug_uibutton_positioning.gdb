# ═══════════════════════════════════════════════════════════════════════════
# SCRIPT GDB : Débogage du positionnement des ButtonWidget (apply/cancel)
# ═══════════════════════════════════════════════════════════════════════════
# Analyse pourquoi modifier la position du bouton "Appliquer" dans le JSON
# à x=40, y=110, "top" provoque un empilement alors que la fenêtre n'est pas réduite
# ═══════════════════════════════════════════════════════════════════════════

file ./respire

# ─── Breakpoint dans check_and_stack_widgets_if_needed ───
break settings_panel.c:check_and_stack_widgets_if_needed

commands
    printf "\n"
    printf "╔═══════════════════════════════════════════════════════════════════════════╗\n"
    printf "║ ENTRÉE: check_and_stack_widgets_if_needed()                              ║\n"
    printf "╚═══════════════════════════════════════════════════════════════════════════╝\n"
    printf "\n"

    printf "📐 PANNEAU:\n"
    printf "   panel_width = %d\n", panel->rect.w
    printf "   screen_width = %d\n", panel->screen_width
    printf "   screen_height = %d\n", panel->screen_height
    printf "   widgets_stacked = %d\n", panel->widgets_stacked
    printf "   panel_width_when_stacked = %d\n", panel->panel_width_when_stacked
    printf "\n"

    continue
end

# ─── Breakpoint à la ligne de détection de collision des widgets ───
break settings_panel.c:1072

commands
    printf "\n"
    printf "╔═══════════════════════════════════════════════════════════════════════════╗\n"
    printf "║ DÉTECTION COLLISION                                                       ║\n"
    printf "╚═══════════════════════════════════════════════════════════════════════════╝\n"
    printf "\n"

    printf "🔍 RECHERCHE DE COLLISION AVEC BORD DROIT DU PANNEAU\n"
    printf "   panel_width = %d\n", panel_width
    printf "   Recherche dans widget_list...\n"
    printf "\n"

    # On va parcourir les widgets pour voir les positions
    set $node = panel->widget_list->first
    set $widget_num = 0

    while $node != 0
        set $widget_num = $widget_num + 1

        printf "   Widget %d: type=%d", $widget_num, $node->type

        # Afficher le type en clair
        if $node->type == 0
            printf " (LABEL)"
        end
        if $node->type == 1
            printf " (SEPARATOR)"
        end
        if $node->type == 2
            printf " (PREVIEW)"
        end
        if $node->type == 3
            printf " (INCREMENT)"
        end
        if $node->type == 4
            printf " (TOGGLE)"
        end
        if $node->type == 5
            printf " (SLIDER)"
        end
        if $node->type == 6
            printf " (BUTTON)"

            # Pour les boutons, afficher plus d'infos
            if $node->widget.button_widget != 0
                set $btn = $node->widget.button_widget
                printf " - id='%s'", $btn->id
                printf " base_x=%d base_y=%d", $btn->base.base_x, $btn->base.base_y
                printf " x=%d y=%d w=%d h=%d", $btn->base.x, $btn->base.y, $btn->base.width, $btn->base.height
                printf " y_anchor=%d", $btn->y_anchor

                # Calculer si le bouton dépasse
                set $right_edge = $btn->base.x + $btn->base.width
                printf " right_edge=%d", $right_edge

                if $right_edge > panel_width
                    printf " ⚠️ DÉPASSE LE PANNEAU!"
                end
            end
        end
        if $node->type == 7
            printf " (SELECTOR)"
        end

        printf "\n"

        set $node = $node->next
    end

    printf "\n"
    continue
end

# ─── Breakpoint quand on détecte qu'il faut empiler ───
break settings_panel.c:1120

commands
    printf "\n"
    printf "╔═══════════════════════════════════════════════════════════════════════════╗\n"
    printf "║ ⚠️  COLLISION DÉTECTÉE - EMPILEMENT DES WIDGETS                          ║\n"
    printf "╚═══════════════════════════════════════════════════════════════════════════╝\n"
    printf "\n"

    printf "📌 panel_width_when_stacked sera fixé à: %d\n", panel->rect.w
    printf "\n"

    continue
end

run
quit
