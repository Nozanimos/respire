# Script GDB simplifié - Affiche les propriétés de tous les widgets INCREMENT
# Usage: gdb -x debug_widgets_simple.gdb ./bin/respire

set pagination off
set print pretty on

# Breakpoint après la création de chaque widget INCREMENT
break create_config_widget
commands
    silent
    # On attend la fin de la fonction pour avoir toutes les valeurs
    finish
    printf "\n=== Widget INCREMENT créé ===\n"
    printf "Nom: %s\n", $retval->option_name
    printf "Position: x=%d, y=%d\n", $retval->base.x, $retval->base.y
    printf "Taille: w=%d, h=%d\n", $retval->base.width, $retval->base.height
    printf "Text size: %d\n", $retval->current_text_size
    printf "Arrow size: %d\n", $retval->arrow_size
    printf "Value: %d (min=%d, max=%d)\n", $retval->value, $retval->min_value, $retval->max_value
    printf "Layout interne:\n"
    printf "  local_text_x: %d\n", $retval->local_text_x
    printf "  local_arrows_x: %d\n", $retval->local_arrows_x
    printf "  local_value_x: %d\n", $retval->local_value_x
    printf "  base_espace_apres_texte: %d\n", $retval->base_espace_apres_texte
    printf "  base_espace_apres_fleches: %d\n", $retval->base_espace_apres_fleches
    printf "Calcul total_width devrait être: local_value_x(%d) + value_width + 10\n", $retval->local_value_x
    printf "--------------------------------------\n"
    continue
end

printf "\n======================================\n"
printf "GDB - Affichage widgets INCREMENT\n"
printf "======================================\n"
printf "Ce script affiche les propriétés de chaque widget INCREMENT créé\n"
printf "Observe particulièrement 'Vitesse respiration' vs les autres\n"
printf "======================================\n\n"

run
