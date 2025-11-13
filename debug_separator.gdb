# Script GDB pour déboguer la logique du séparateur en mode stack
# Usage: gdb -x debug_separator.gdb ./bin/respire

set pagination off
set print pretty on

# Breakpoint dans stack_widgets_vertically au début du case SEPARATOR
break settings_panel.c:1083
commands
  silent
  printf "\n════════════════════════════════════════════════════════════\n"
  printf "🔍 SEPARATOR trouvé à l'index i=%d (rect_count=%d)\n", i, rect_count

  # Afficher le type et position Y de base du séparateur
  if r->node->widget.separator_widget
    printf "   Séparateur base_y=%d\n", r->node->widget.separator_widget->base.base_y
  end

  # Afficher les widgets autour dans la liste
  printf "\n📋 Ordre dans la liste des rects:\n"
  set $j = 0
  while $j < rect_count
    printf "   [%d] type=%d", $j, rects[$j].type

    # Afficher le type en clair
    if rects[$j].type == 0
      printf " (LABEL)"
    end
    if rects[$j].type == 1
      printf " (PREVIEW)"
    end
    if rects[$j].type == 2
      printf " (INCREMENT)"
    end
    if rects[$j].type == 3
      printf " (SELECTOR)"
    end
    if rects[$j].type == 4
      printf " (TOGGLE)"
    end
    if rects[$j].type == 5
      printf " (SEPARATOR)"
    end
    if rects[$j].type == 6
      printf " (BUTTON)"
    end

    if $j == i
      printf " <-- SÉPARATEUR ACTUEL"
    end
    if $j == i-1
      printf " <-- WIDGET PRÉCÉDENT (i-1)"
    end
    if $j == i-2
      printf " <-- i-2"
    end

    printf "\n"
    set $j = $j + 1
  end

  printf "════════════════════════════════════════════════════════════\n\n"
  continue
end

# Breakpoint après avoir trouvé le widget au-dessus (ligne 1105)
break settings_panel.c:1105
commands
  silent
  printf "\n✅ Résultat de la recherche dans la liste:\n"
  printf "   widget_above_index=%d\n", widget_above_index
  printf "   widget_above_type=%d", widget_above_type

  if widget_above_type == 0
    printf " (LABEL)\n"
  end
  if widget_above_type == 2
    printf " (INCREMENT)\n"
  end
  if widget_above_type == 3
    printf " (SELECTOR)\n"
  end
  if widget_above_type == 4
    printf " (TOGGLE)\n"
  end
  if widget_above_type == 6
    printf " (BUTTON)\n"
  end

  continue
end

# Breakpoint pour voir la décision finale (ligne 1112)
break settings_panel.c:1112
commands
  silent
  if widget_above_type == 0
    printf "   ⚡ DÉCISION: Widget au-dessus = LABEL → Y fixe\n"
  else
    printf "   ⚡ DÉCISION: Widget au-dessus = callback (type=%d) → empiler à current_y=%d\n", widget_above_type, current_y
  end
  continue
end

# Lancer le programme
run

# Quitter à la fin
quit
