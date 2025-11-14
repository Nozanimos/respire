# Script GDB - Vérification calculs UTF-8 pour "Vitesse respiration"
# Usage: gdb -x debug_utf8.gdb ./bin/respire

set pagination off
set print pretty on

# Breakpoint sur le calcul de text_width avec condition
break widget.c:172
commands
    silent
    printf "\n=== Calcul text_width (ligne 172) ===\n"
    printf "Widget name: %s\n", name
    printf "strlen(name): %lu bytes\n", strlen(name)
    printf "Font: %p (text_size=%d)\n", correct_font, text_size
    # Après TTF_SizeUTF8
    next
    printf "text_width mesuré: %d pixels\n", text_width
    printf "text_height mesuré: %d pixels\n", text_height

    # Vérifier s'il y a des caractères non-ASCII
    set $i = 0
    set $has_utf8 = 0
    while $i < strlen(name)
        if (unsigned char)name[$i] > 127
            set $has_utf8 = 1
            printf "  ATTENTION: Caractère UTF-8 à position %d: 0x%02x\n", $i, (unsigned char)name[$i]
        end
        set $i = $i + 1
    end
    if $has_utf8 == 0
        printf "  -> Tous les caractères sont ASCII\n"
    end
    printf "--------------------------------------\n"
    continue
end

# Breakpoint sur le calcul de value_width (ligne 203)
break widget.c:203
commands
    silent
    printf "\n=== Calcul value_width (ligne 203) ===\n"
    printf "Widget name: %s\n", name
    printf "Value string: '%s'\n", value_str
    printf "strlen(value_str): %lu\n", strlen(value_str)
    printf "text_size: %d\n", text_size
    printf "Estimation: strlen * (text_size/2) = %lu * %d = %d\n", strlen(value_str), text_size/2, strlen(value_str) * (text_size / 2)
    continue
end

# Breakpoint final pour voir le résultat
break widget.c:215
commands
    silent
    printf "\n=== RÉSULTAT FINAL (ligne 215) ===\n"
    printf "Widget: %s\n", name
    printf "text_width (mesuré UTF-8): %d\n", text_width
    printf "local_arrows_x: %d\n", widget->local_arrows_x
    printf "  = text_width(%d) + base_espace_apres_texte(%d)\n", text_width, widget->base_espace_apres_texte
    printf "local_value_x: %d\n", widget->local_value_x
    printf "  = local_arrows_x(%d) + arrow_size(%d) + base_espace_apres_fleches(%d)\n", widget->local_arrows_x, arrow_size, widget->base_espace_apres_fleches
    printf "value_width (estimé): %d\n", value_width
    printf "total_width: %d\n", total_width
    printf "  = local_value_x(%d) + value_width(%d) + 10\n", widget->local_value_x, value_width
    printf "base.width sera: %d\n", total_width
    printf "======================================\n"
    continue
end

printf "\n======================================\n"
printf "GDB - Vérification UTF-8\n"
printf "======================================\n"
printf "Vérifie les calculs de largeur avec UTF-8\n"
printf "Observe 'Vitesse respiration' en particulier\n"
printf "======================================\n\n"

run
