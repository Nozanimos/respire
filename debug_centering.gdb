# ═══════════════════════════════════════════════════════════════════════════
# SCRIPT GDB : Débogage simple du centrage en mode stack
# ═══════════════════════════════════════════════════════════════════════════

file ./respire

# ─── Breakpoint à l'entrée de stack_widgets_vertically ───
break settings_panel.c:stack_widgets_vertically

commands
    printf "\n"
    printf "╔═══════════════════════════════════════════════════════════════════════════╗\n"
    printf "║ ENTRÉE: stack_widgets_vertically                                          ║\n"
    printf "╚═══════════════════════════════════════════════════════════════════════════╝\n"
    printf "\n"
    printf "📦 rect_count = %d widgets\n", rect_count
    printf "\n"
    continue
end

# ─── Breakpoint après calcul de increment_start_x (ligne 1036) ───
break settings_panel.c:1036

commands
    printf "\n"
    printf "╔═══════════════════════════════════════════════════════════════════════════╗\n"
    printf "║ CALCUL CENTRAGE                                                           ║\n"
    printf "╚═══════════════════════════════════════════════════════════════════════════╝\n"
    printf "\n"
    printf "📐 panel_width = %d\n", panel_width
    printf "📏 max_increment_width = %d\n", max_increment_width
    printf "🎯 increment_start_x = %d\n", increment_start_x
    printf "\n"
    printf "📊 Marges:\n"
    printf "   Marge gauche = %d\n", increment_start_x
    printf "   Marge droite = %d\n", panel_width - increment_start_x - max_increment_width
    printf "\n"
    continue
end

# ─── Breakpoint dans render_config_widget (widget.c) ───
break widget.c:render_config_widget

commands
    silent

    # Vérifier si c'est "Vitesse respiration" en utilisant strncmp sur les premiers chars
    set $is_vitesse = 0
    if widget->option_name[0] == 'V'
        if widget->option_name[1] == 'i'
            if widget->option_name[2] == 't'
                set $is_vitesse = 1
            end
        end
    end

    if $is_vitesse == 1
        printf "\n"
        printf "╔═══════════════════════════════════════════════════════════════════════════╗\n"
        printf "║ RENDER: Vitesse respiration                                              ║\n"
        printf "╚═══════════════════════════════════════════════════════════════════════════╝\n"
        printf "\n"
        printf "📦 Position:\n"
        printf "   widget->base.x = %d\n", widget->base.x
        printf "   widget->base.y = %d\n", widget->base.y
        printf "\n"
        printf "📏 Dimensions:\n"
        printf "   widget->base.width = %d\n", widget->base.width
        printf "   container_width (param) = %d\n", container_width
        printf "\n"
        printf "🎯 Flèches:\n"
        printf "   local_arrows_x = %d\n", widget->local_arrows_x
        printf "   arrow_size = %d\n", widget->arrow_size
        printf "\n"
        printf "💡 Largeur totale calculée:\n"
        printf "   local_arrows_x + arrow_size + marge = %d + %d + 60 = %d\n", widget->local_arrows_x, widget->arrow_size, widget->local_arrows_x + widget->arrow_size + 60
        printf "\n"
    end

    continue
end

run
quit
