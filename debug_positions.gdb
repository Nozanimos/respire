# ═══════════════════════════════════════════════════════════════════════════
# SCRIPT GDB : Positions LABEL et SEPARATOR en mode stack
# ═══════════════════════════════════════════════════════════════════════════

file ./respire

# ─── Breakpoint AU DÉBUT de la boucle switch (après déclaration de r) ───
break settings_panel.c:1070

commands
    silent

    # Afficher seulement pour LABEL type=0 ou SEPARATOR type=1
    set $show = 0
    if rects[i].type == 0
        set $show = 1
        printf "\n╔═══════════════════════════════════════════════════════════════════╗\n"
        printf "║ Widget[%d] LABEL", i
    end
    if rects[i].type == 1
        set $show = 1
        printf "\n╔═══════════════════════════════════════════════════════════════════╗\n"
        printf "║ Widget[%d] SEPARATOR", i
    end

    if $show == 1
        printf "\n╚═══════════════════════════════════════════════════════════════════╝\n"

        # LABEL
        if rects[i].type == 0 && rects[i].node->widget.label_widget != 0
            set $w = rects[i].node->widget.label_widget
            printf "📝 LABEL '%s'\n", $w->text
            printf "   AVANT traitement stack:\n"
            printf "     base.x = %d\n", $w->base.x
            printf "     base.y = %d\n", $w->base.y
            printf "     base.base_x = %d\n", $w->base.base_x
            printf "     base.base_y = %d\n", $w->base.base_y
        end

        # SEPARATOR
        if rects[i].type == 1 && rects[i].node->widget.separator_widget != 0
            set $w = rects[i].node->widget.separator_widget
            printf "📏 SEPARATOR\n"
            printf "   AVANT traitement stack:\n"
            printf "     base.x = %d\n", $w->base.x
            printf "     base.y = %d\n", $w->base.y
            printf "     base.base_y = %d\n", $w->base.base_y
            printf "     base_start_margin = %d\n", $w->base_start_margin
            printf "     base.width = %d\n", $w->base.width
        end
    end

    continue
end

# ─── Breakpoint APRÈS le case LABEL (ligne 1079) ───
break settings_panel.c:1079

commands
    silent

    # Vérifier si widget courant est LABEL
    if rects[i].type == 0 && rects[i].node->widget.label_widget != 0
        set $w = rects[i].node->widget.label_widget
        printf "\n   APRÈS traitement stack:\n"
        printf "     base.x = %d (", $w->base.x
        if $w->base.x == $w->base.base_x
            printf "OK: inchangé)\n"
        else
            printf "❌ MODIFIÉ! devrait être %d)\n", $w->base.base_x
        end
        printf "     base.y = %d\n", $w->base.y
    end

    continue
end

# ─── Breakpoint APRÈS le case SEPARATOR (ligne 1174) ───
break settings_panel.c:1174

commands
    silent

    # Vérifier si widget courant est SEPARATOR
    if rects[i].type == 1 && rects[i].node->widget.separator_widget != 0
        set $w = rects[i].node->widget.separator_widget
        printf "\n   APRÈS traitement stack:\n"
        printf "     base.x = %d\n", $w->base.x
        printf "     base.y = %d\n", $w->base.y
        printf "     base.width = %d\n", $w->base.width
    end

    continue
end

run
quit
