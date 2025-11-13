# ═══════════════════════════════════════════════════════════════════════════
# SCRIPT GDB : Débogage du positionnement des ButtonWidget (apply/cancel)
# ═══════════════════════════════════════════════════════════════════════════
# Analyse pourquoi modifier la position du bouton "Appliquer" dans le JSON
# à x=40, y=110, "top" provoque un empilement alors que la fenêtre n'est pas réduite
# ═══════════════════════════════════════════════════════════════════════════

file ./respire

# ─── Breakpoint dans recalculate_widget_layout ───
break settings_panel.c:recalculate_widget_layout

commands
    printf "\n"
    printf "╔═══════════════════════════════════════════════════════════════════════════╗\n"
    printf "║ ENTRÉE: recalculate_widget_layout()                                      ║\n"
    printf "╚═══════════════════════════════════════════════════════════════════════════╝\n"
    printf "\n"

    printf "📐 PANNEAU:\n"
    printf "   panel_width = %d\n", panel->rect.w
    printf "   screen_width = %d\n", panel->screen_width
    printf "   screen_height = %d\n", panel->screen_height
    printf "   widgets_stacked = %d\n", panel->widgets_stacked
    printf "   panel_width_when_stacked = %d\n", panel->panel_width_when_stacked
    printf "   layout_dirty = %d\n", panel->layout_dirty
    printf "\n"

    continue
end

# ─── Breakpoint à la ligne de détection de collision (ligne 1388) ───
break settings_panel.c:1388

commands
    printf "\n"
    printf "╔═══════════════════════════════════════════════════════════════════════════╗\n"
    printf "║ DÉTECTION COLLISION - Liste des widgets                                  ║\n"
    printf "╚═══════════════════════════════════════════════════════════════════════════╝\n"
    printf "\n"

    printf "📐 panel_width = %d\n", panel_width
    printf "📦 rect_count = %d widgets\n", rect_count
    printf "\n"

    # Afficher tous les widgets avec leurs positions
    set $i = 0
    while $i < rect_count
        printf "   Widget %d: type=%d", $i, rects[$i].type

        # Afficher le type en clair
        if rects[$i].type == 0
            printf " (LABEL)"
        end
        if rects[$i].type == 1
            printf " (SEPARATOR)"
        end
        if rects[$i].type == 2
            printf " (PREVIEW)"
        end
        if rects[$i].type == 3
            printf " (INCREMENT)"
        end
        if rects[$i].type == 4
            printf " (TOGGLE)"
        end
        if rects[$i].type == 5
            printf " (SLIDER)"
        end
        if rects[$i].type == 6
            printf " (BUTTON)"
        end
        if rects[$i].type == 7
            printf " (SELECTOR)"
        end

        printf " x=%d y=%d w=%d h=%d", rects[$i].x, rects[$i].y, rects[$i].width, rects[$i].height

        # Calculer le bord droit
        set $right_edge = rects[$i].x + rects[$i].width
        printf " right_edge=%d", $right_edge

        if $right_edge > panel_width
            printf " ⚠️ DÉPASSE!"
        end

        printf "\n"

        set $i = $i + 1
    end

    printf "\n"
    continue
end

# ─── Breakpoint quand on détecte qu'il faut empiler (ligne 1408) ───
break settings_panel.c:1408

commands
    printf "\n"
    printf "╔═══════════════════════════════════════════════════════════════════════════╗\n"
    printf "║ ⚠️  needs_reorganization = true - EMPILEMENT DES WIDGETS                 ║\n"
    printf "╚═══════════════════════════════════════════════════════════════════════════╝\n"
    printf "\n"

    printf "📌 panel_width actuel = %d\n", panel_width
    printf "📌 panel_width_when_stacked = %d\n", panel->panel_width_when_stacked
    printf "\n"

    continue
end

run
quit
