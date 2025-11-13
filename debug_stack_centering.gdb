# ═══════════════════════════════════════════════════════════════════════════
# SCRIPT GDB : Débogage du centrage en mode stack
# ═══════════════════════════════════════════════════════════════════════════
# Analyse:
# 1. Le calcul de max_increment_width et increment_start_x
# 2. Les marges gauche/droite par rapport aux widgets
# 3. Le problème de hovering sur "vitesse respiration"
# ═══════════════════════════════════════════════════════════════════════════

file ./respire

# ─── Breakpoint après le calcul de max_increment_width (ligne 1038) ───
break settings_panel.c:1038

commands
    printf "\n"
    printf "╔═══════════════════════════════════════════════════════════════════════════╗\n"
    printf "║ CALCUL DU CENTRAGE - max_increment_width                                 ║\n"
    printf "╚═══════════════════════════════════════════════════════════════════════════╝\n"
    printf "\n"

    printf "📐 PANNEAU:\n"
    printf "   panel_width = %d\n", panel_width
    printf "   center_x = %d\n", center_x
    printf "\n"

    printf "🔢 INCREMENT WIDGETS:\n"
    printf "   max_increment_width = %d\n", max_increment_width
    printf "   increment_start_x = %d\n", increment_start_x
    printf "\n"

    printf "📋 Détail des widgets INCREMENT:\n"
    set $i = 0
    while $i < rect_count
        if rects[$i].type == 3
            printf "   Widget[%d]: x=%d y=%d w=%d h=%d", $i, rects[$i].x, rects[$i].y, rects[$i].width, rects[$i].height

            # Calculer les marges
            set $left_margin = increment_start_x - 0
            set $right_margin = panel_width - (increment_start_x + rects[$i].width)
            printf " → marges: left=%d right=%d", $left_margin, $right_margin

            # Afficher le nom du widget si disponible
            if rects[$i].node->widget.increment_widget != 0
                printf " '%s'", rects[$i].node->widget.increment_widget->option_name
            end

            printf "\n"
        end
        set $i = $i + 1
    end

    printf "\n"
    printf "💡 VÉRIFICATION DU CENTRAGE:\n"
    printf "   increment_start_x = %d\n", increment_start_x
    printf "   max_increment_width = %d\n", max_increment_width
    printf "   Bord droit = increment_start_x + max_width = %d\n", increment_start_x + max_increment_width
    printf "   Marge gauche = %d\n", increment_start_x
    printf "   Marge droite = %d\n", panel_width - (increment_start_x + max_increment_width)
    printf "\n"

    continue
end

# ─── Breakpoint dans render_config_widget pour voir le container_width ───
break config_widget.c:render_config_widget

commands
    silent

    # Afficher uniquement pour "vitesse respiration"
    if strcmp(widget->option_name, "Vitesse respiration") == 0
        printf "\n"
        printf "╔═══════════════════════════════════════════════════════════════════════════╗\n"
        printf "║ RENDER: '%s'                                            ║\n", widget->option_name
        printf "╚═══════════════════════════════════════════════════════════════════════════╝\n"
        printf "\n"

        printf "📦 Widget base:\n"
        printf "   widget->base.x = %d\n", widget->base.x
        printf "   widget->base.y = %d\n", widget->base.y
        printf "   widget->base.width = %d\n", widget->base.width
        printf "   widget->base.height = %d\n", widget->base.height
        printf "\n"

        printf "🎯 Container width passé en paramètre:\n"
        printf "   container_width = %d\n", container_width
        printf "\n"

        printf "📏 Dimensions internes:\n"
        printf "   local_arrows_x = %d\n", widget->local_arrows_x
        printf "   arrow_size = %d\n", widget->arrow_size
        printf "\n"

        printf "🖱️  Zone de hover (si is_hovered):\n"
        printf "   is_hovered = %d\n", widget->base.is_hovered
        printf "\n"
    end

    continue
end

# ─── Breakpoint dans handle_config_widget_hover ───
break config_widget.c:handle_config_widget_hover

commands
    silent

    if strcmp(widget->option_name, "Vitesse respiration") == 0
        printf "\n"
        printf "╔═══════════════════════════════════════════════════════════════════════════╗\n"
        printf "║ HOVER CHECK: '%s'                                       ║\n", widget->option_name
        printf "╚═══════════════════════════════════════════════════════════════════════════╝\n"
        printf "\n"

        printf "🖱️  Position souris:\n"
        printf "   mouse_x = %d, mouse_y = %d\n", mouse_x, mouse_y
        printf "\n"

        printf "📦 Zone de hover calculée:\n"
        printf "   abs_x = offset_x + widget->base.x = %d + %d = %d\n", offset_x, widget->base.x, offset_x + widget->base.x
        printf "   abs_y = offset_y + widget->base.y = %d + %d = %d\n", offset_y, widget->base.y, offset_y + widget->base.y
        printf "\n"

        printf "📏 Container width pour hover:\n"
        printf "   container_width = %d\n", container_width
        printf "   widget->base.width = %d\n", widget->base.width
        printf "   Largeur effective hover = %d\n", container_width > 0 ? container_width : widget->base.width
        printf "\n"
    end

    continue
end

run
quit
