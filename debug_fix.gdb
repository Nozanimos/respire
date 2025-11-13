# ═══════════════════════════════════════════════════════════════════════════
# SCRIPT GDB : Positions LABEL et SEPARATOR en mode stack (CORRIGÉ)
# ═══════════════════════════════════════════════════════════════════════════

file ./respire

# ─── Breakpoint AU DÉBUT du case LABEL (ligne 1072) ───
break settings_panel.c:1072

commands
    silent

    # Vérifier que c'est bien un LABEL
    if rects[i].node->widget.label_widget != 0
        set $w = rects[i].node->widget.label_widget
        printf "\n╔═══════════════════════════════════════════════════════════════════╗\n"
        printf "║ Widget[%d] LABEL '%s'\n", i, $w->text
        printf "╚═══════════════════════════════════════════════════════════════════╝\n"
        printf "📝 AVANT traitement switch:\n"
        printf "   base.x = %d\n", $w->base.x
        printf "   base.y = %d\n", $w->base.y
        printf "   base.base_x = %d\n", $w->base.base_x
        printf "   base.base_y = %d\n", $w->base.base_y
    end

    continue
end

# ─── Breakpoint À LA FIN du case LABEL (ligne 1078 - juste avant break) ───
break settings_panel.c:1078

commands
    silent

    # Vérifier que c'est bien un LABEL
    if rects[i].node->widget.label_widget != 0
        set $w = rects[i].node->widget.label_widget
        printf "\n   APRÈS traitement switch:\n"
        printf "   base.x = %d ", $w->base.x
        if $w->base.x == $w->base.base_x
            printf "(✅ OK: inchangé)\n"
        else
            printf "(❌ MODIFIÉ! devrait être %d)\n", $w->base.base_x
        end
        printf "   base.y = %d\n", $w->base.y
    end

    continue
end

# ─── Breakpoint AU DÉBUT du case SEPARATOR (ligne 1128) ───
break settings_panel.c:1128

commands
    silent

    # Vérifier que c'est bien un SEPARATOR
    if rects[i].node->widget.separator_widget != 0
        set $w = rects[i].node->widget.separator_widget
        printf "\n╔═══════════════════════════════════════════════════════════════════╗\n"
        printf "║ Widget[%d] SEPARATOR\n", i
        printf "╚═══════════════════════════════════════════════════════════════════╝\n"
        printf "📏 AVANT traitement switch:\n"
        printf "   base.x = %d\n", $w->base.x
        printf "   base.y = %d\n", $w->base.y
        printf "   base.base_y = %d\n", $w->base.base_y
        printf "   base_start_margin = %d\n", $w->base_start_margin
        printf "   base.width = %d\n", $w->base.width
    end

    continue
end

# ─── Breakpoint À LA FIN du case SEPARATOR (ligne 1173 - juste avant break) ───
break settings_panel.c:1173

commands
    silent

    # Vérifier que c'est bien un SEPARATOR
    if rects[i].node->widget.separator_widget != 0
        set $w = rects[i].node->widget.separator_widget
        printf "\n   APRÈS traitement switch:\n"
        printf "   base.x = %d\n", $w->base.x
        printf "   base.y = %d\n", $w->base.y
        printf "   base.width = %d\n", $w->base.width
    end

    continue
end

run
quit
