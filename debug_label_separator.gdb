# ═══════════════════════════════════════════════════════════════════════════
# SCRIPT GDB : Débogage des positions des LABEL et SEPARATOR en mode stack
# ═══════════════════════════════════════════════════════════════════════════

file ./respire

# ─── Breakpoint dans stack_widgets_vertically au début de la boucle ───
break settings_panel.c:1067

commands
    silent

    # Afficher seulement pour les LABEL et SEPARATOR
    if r->type == 0
        printf "\n"
        printf "╔═══════════════════════════════════════════════════════════════════╗\n"
        printf "║ LABEL: "
        if r->node->widget.label_widget != 0
            printf "%s", r->node->widget.label_widget->text
        end
        printf "\n"
        printf "╚═══════════════════════════════════════════════════════════════════╝\n"

        if r->node->widget.label_widget != 0
            set $lbl = r->node->widget.label_widget
            printf "📍 Position AVANT stack:\n"
            printf "   base.x = %d\n", $lbl->base.x
            printf "   base.y = %d\n", $lbl->base.y
            printf "   base.base_x (JSON) = %d\n", $lbl->base.base_x
            printf "   base.base_y (JSON) = %d\n", $lbl->base.base_y
            printf "   alignment = %d", $lbl->alignment
            if $lbl->alignment == 0
                printf " (LEFT)\n"
            end
            if $lbl->alignment == 1
                printf " (CENTER)\n"
            end
            if $lbl->alignment == 2
                printf " (RIGHT)\n"
            end
        end
    end

    if r->type == 1
        printf "\n"
        printf "╔═══════════════════════════════════════════════════════════════════╗\n"
        printf "║ SEPARATOR\n"
        printf "╚═══════════════════════════════════════════════════════════════════╝\n"

        if r->node->widget.separator_widget != 0
            set $sep = r->node->widget.separator_widget
            printf "📍 Position AVANT stack:\n"
            printf "   base.x = %d\n", $sep->base.x
            printf "   base.y = %d\n", $sep->base.y
            printf "   base.base_y (JSON) = %d\n", $sep->base.base_y
            printf "   base.width = %d\n", $sep->base.width
        end
    end

    continue
end

# ─── Breakpoint après le traitement d'un LABEL (ligne 1063) ───
break settings_panel.c:1086

commands
    silent

    # Vérifier si on vient de traiter un LABEL
    if rects[$i].type == 0
        printf "\n"
        printf "📌 LABEL APRÈS stack:\n"
        if rects[$i].node->widget.label_widget != 0
            set $lbl = rects[$i].node->widget.label_widget
            printf "   base.x = %d (", $lbl->base.x
            if $lbl->alignment == 0
                printf "devrait être base_x=%d)\n", $lbl->base.base_x
            end
            if $lbl->alignment == 1
                printf "devrait être centré)\n"
            end
            if $lbl->alignment == 2
                printf "devrait être à droite)\n"
            end
        end
    end

    continue
end

# ─── Breakpoint après le traitement d'un SEPARATOR (ligne 1161) ───
break settings_panel.c:1161

commands
    silent

    # Vérifier si on vient de traiter un SEPARATOR
    if rects[$i].type == 1
        printf "\n"
        printf "📌 SEPARATOR APRÈS stack:\n"
        if rects[$i].node->widget.separator_widget != 0
            set $sep = rects[$i].node->widget.separator_widget
            printf "   base.x = %d\n", $sep->base.x
            printf "   base.y = %d\n", $sep->base.y
            printf "   base.width = %d\n", $sep->base.width
        end
    end

    continue
end

run
quit
