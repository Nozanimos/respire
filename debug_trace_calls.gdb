# ═══════════════════════════════════════════════════════════════════════════
# SCRIPT GDB : Tracer les appels de fonctions et positions LABEL
# ═══════════════════════════════════════════════════════════════════════════

file ./respire

# ─── Tracer TOUTES les entrées dans recalculate_widget_layout ───
break recalculate_widget_layout
commands
    silent
    printf "\n╔══════════════════════════════════════════════════════════════╗\n"
    printf "║ 🔄 ENTRÉE recalculate_widget_layout()\n"
    printf "║    panel->rect.w = %d\n", panel->rect.w
    printf "║    panel->widgets_stacked = %d\n", panel->widgets_stacked
    printf "║    panel->panel_width_when_stacked = %d\n", panel->panel_width_when_stacked
    printf "╚══════════════════════════════════════════════════════════════╝\n"
    continue
end

# ─── Tracer les appels à restore_json_positions ───
break restore_json_positions
commands
    silent
    printf "\n   📍 APPEL restore_json_positions()\n"
    printf "      panel_width = %d\n", panel->rect.w
    continue
end

# ─── Tracer les entrées dans restore_json_positions pour "Configuration" ───
break settings_panel.c:925
commands
    silent
    if node->widget.label_widget != 0
        set $w = node->widget.label_widget
        if $w->alignment == 1
            printf "      └─ LABEL '%s' (CENTER) AVANT: base.x=%d, base.base_x=%d, base.width=%d\n", $w->text, $w->base.x, $w->base.base_x, $w->base.width
        end
    end
    continue
end

# ─── Tracer APRÈS calcul CENTER dans restore_json_positions ───
break settings_panel.c:934
commands
    silent
    if node->widget.label_widget != 0
        set $w = node->widget.label_widget
        if $w->alignment == 1
            printf "      └─ LABEL '%s' (CENTER) APRÈS calcul: base.x=%d (formule: (panel_width=%d - width=%d) / 2)\n", $w->text, $w->base.x, panel_width, $w->base.width
        end
    end
    continue
end

# ─── Tracer les appels à stack_widgets_vertically ───
break stack_widgets_vertically
commands
    silent
    printf "\n   🔧 APPEL stack_widgets_vertically()\n"
    printf "      panel_width = %d, rect_count = %d\n", panel->rect.w, rect_count
    continue
end

# ─── Tracer "Configuration" dans stack_widgets_vertically AVANT traitement ───
break settings_panel.c:1072
commands
    silent
    if rects[i].type == 0 && rects[i].node->widget.label_widget != 0
        set $w = rects[i].node->widget.label_widget
        if $w->alignment == 1
            printf "      └─ Widget[%d] LABEL '%s' (CENTER) AVANT: base.x=%d, base.width=%d\n", i, $w->text, $w->base.x, $w->base.width
        end
    end
    continue
end

# ─── Tracer "Configuration" APRÈS calcul CENTER dans stack_widgets_vertically ───
break settings_panel.c:1084
commands
    silent
    if rects[i].type == 0 && rects[i].node->widget.label_widget != 0
        set $w = rects[i].node->widget.label_widget
        if $w->alignment == 1
            printf "      └─ LABEL '%s' (CENTER) APRÈS calcul: base.x=%d (formule: (panel_width=%d - width=%d) / 2)\n", $w->text, $w->base.x, panel_width, $w->base.width
        end
    end
    continue
end

# ─── Tracer la fin de recalculate_widget_layout ───
break settings_panel.c:1576
commands
    silent
    printf "\n   ✅ FIN recalculate_widget_layout() - layout_dirty=false\n"
    continue
end

run
quit
