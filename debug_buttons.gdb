# Script GDB pour déboguer les boutons Appliquer/Annuler au dépilement
# Usage: gdb -x debug_buttons.gdb ./bin/respire

set pagination off
set print pretty on

# Breakpoint AVANT restore_json_positions (début du dépilement)
break settings_panel.c:1214
commands
  silent
  printf "\n════════════════════════════════════════════════════════════\n"
  printf "🔄 AVANT DÉPILEMENT (restore_json_positions)\n"
  printf "════════════════════════════════════════════════════════════\n"

  # Positions des UIButton (apply_button et cancel_button)
  printf "\n📍 Positions des UIButton du panneau:\n"
  printf "   apply_button.rect.x = %d\n", panel->apply_button.rect.x
  printf "   apply_button.rect.y = %d\n", panel->apply_button.rect.y
  printf "   apply_button.rect.w = %d\n", panel->apply_button.rect.w
  printf "   apply_button.rect.h = %d\n", panel->apply_button.rect.h
  printf "\n"
  printf "   cancel_button.rect.x = %d\n", panel->cancel_button.rect.x
  printf "   cancel_button.rect.y = %d\n", panel->cancel_button.rect.y
  printf "   cancel_button.rect.w = %d\n", panel->cancel_button.rect.w
  printf "   cancel_button.rect.h = %d\n", panel->cancel_button.rect.h

  # État du panel
  printf "\n📊 État du panneau:\n"
  printf "   widgets_stacked = %d\n", panel->widgets_stacked
  printf "   panel_width = %d\n", panel_width
  printf "   panel_width_when_stacked = %d\n", panel->panel_width_when_stacked
  printf "   screen_width = %d\n", panel->screen_width
  printf "   screen_height = %d\n", panel->screen_height

  printf "════════════════════════════════════════════════════════════\n\n"
  continue
end

# Breakpoint APRÈS restore_json_positions
break settings_panel.c:1219
commands
  silent
  printf "\n════════════════════════════════════════════════════════════\n"
  printf "✅ APRÈS restore_json_positions\n"
  printf "════════════════════════════════════════════════════════════\n"

  # Positions des UIButton après restauration
  printf "\n📍 Positions des UIButton après restauration:\n"
  printf "   apply_button.rect.x = %d\n", panel->apply_button.rect.x
  printf "   apply_button.rect.y = %d\n", panel->apply_button.rect.y
  printf "\n"
  printf "   cancel_button.rect.x = %d\n", panel->cancel_button.rect.x
  printf "   cancel_button.rect.y = %d\n", panel->cancel_button.rect.y

  # Vérifier si les positions ont changé
  printf "\n💡 Les positions des UIButton sont gérées dans update_panel_scale(),\n"
  printf "   pas dans restore_json_positions().\n"
  printf "   restore_json_positions() ne restaure que les ButtonWidget de la widget_list.\n"

  printf "════════════════════════════════════════════════════════════\n\n"
  continue
end

# Breakpoint dans restore_json_positions pour voir les ButtonWidget
break settings_panel.c:981
commands
  silent
  printf "   🔘 ButtonWidget trouvé dans restore_json_positions:\n"
  if node->widget.button_widget
    set $btn = node->widget.button_widget
    printf "      base_x=%d, base_y=%d\n", $btn->base_x, $btn->base_y
    printf "      Restauration: x=%d, y=%d\n", $btn->base.x, $btn->base.y
  end
  continue
end

# Breakpoint dans calculate_heights (fin de recalculate_widget_layout)
break settings_panel.c:1224
commands
  silent
  printf "\n════════════════════════════════════════════════════════════\n"
  printf "🏁 FIN recalculate_widget_layout (calculate_heights)\n"
  printf "════════════════════════════════════════════════════════════\n"

  printf "\n📍 Positions FINALES des UIButton:\n"
  printf "   apply_button.rect.x = %d\n", panel->apply_button.rect.x
  printf "   apply_button.rect.y = %d\n", panel->apply_button.rect.y
  printf "\n"
  printf "   cancel_button.rect.x = %d\n", panel->cancel_button.rect.x
  printf "   cancel_button.rect.y = %d\n", panel->cancel_button.rect.y

  printf "\n💡 Si les positions sont incorrectes, c'est que update_panel_scale()\n"
  printf "   n'a pas été appelé après le dépilement, ou sa logique est incorrecte.\n"

  printf "════════════════════════════════════════════════════════════\n\n"
  continue
end

# Lancer le programme
run

# Quitter à la fin
quit
