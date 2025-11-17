# ════════════════════════════════════════════════════════════════════════════
# Script GDB pour analyser l'espacement des widgets INCREMENT
# ════════════════════════════════════════════════════════════════════════════
# Usage : gdb -x debug_widget_spacing.gdb ./bin/respire
# ════════════════════════════════════════════════════════════════════════════

set pagination off
set print pretty on

# ─────────────────────────────────────────────────────────────────────────
# BREAKPOINT 1 : Création du widget (calcul initial)
# ─────────────────────────────────────────────────────────────────────────
break create_config_widget
commands
    silent
    printf "\n"
    printf "════════════════════════════════════════════════════════════════\n"
    printf "🔧 CRÉATION WIDGET INCREMENT : %s\n", name
    printf "════════════════════════════════════════════════════════════════\n"
    printf "  📏 text_size = %d px\n", text_size

    # Continuer jusqu'après le calcul de base_espace_apres_texte
    break widget.c:153
    continue

    printf "  📐 base_espace_apres_texte = %d px  (text_size * 0.7)\n", widget->base_espace_apres_texte
    printf "  📐 base_roller_padding = %d px  (text_size * 0.4)\n", widget->base_roller_padding

    # Continuer jusqu'après la mesure du texte
    break widget.c:234
    continue

    printf "\n  📝 MESURE DU LABEL '%s' :\n", name
    printf "     text_width = %d px\n", text_width
    printf "     text_height = %d px\n", text_height

    printf "\n  🎯 CALCUL POSITION ROLLER :\n"
    printf "     local_roller_x = text_width + base_espace_apres_texte\n"
    printf "     local_roller_x = %d + %d = %d px\n", text_width, widget->base_espace_apres_texte, widget->local_roller_x

    # Continuer jusqu'après le calcul du roller_width
    break widget.c:241
    continue

    printf "\n  📦 DIMENSIONS ROLLER :\n"
    printf "     roller_width = %d px\n", widget->roller_width
    printf "     roller_height = %d px\n", widget->roller_height

    printf "\n  📊 LARGEUR TOTALE WIDGET :\n"
    printf "     total_width = local_roller_x + roller_width + 5\n"
    printf "     total_width = %d + %d + 5 = %d px\n", widget->local_roller_x, widget->roller_width, total_width

    printf "════════════════════════════════════════════════════════════════\n\n"

    # Effacer les breakpoints temporaires
    clear widget.c:153
    clear widget.c:234
    clear widget.c:241

    continue
end

# ─────────────────────────────────────────────────────────────────────────
# BREAKPOINT 2 : Calcul de la largeur max en mode STACK (settings_panel.c)
# ─────────────────────────────────────────────────────────────────────────
break settings_panel.c:1191
commands
    silent
    printf "\n"
    printf "════════════════════════════════════════════════════════════════\n"
    printf "📏 CALCUL LARGEUR MAX (MODE STACK)\n"
    printf "════════════════════════════════════════════════════════════════\n"
    printf "  Widget: %s\n", w->option_name

    printf "\n  📐 CALCUL real_width :\n"
    printf "     real_width = local_roller_x + roller_width + 10\n"
    printf "     real_width = %d + %d + 10 = %d px\n", w->local_roller_x, w->roller_width, real_width

    if real_width > max_increment_width
        printf "\n  ✅ NOUVEAU MAX : %d px (ancien: %d px)\n", real_width, max_increment_width
    end

    printf "════════════════════════════════════════════════════════════════\n\n"
    continue
end

# ─────────────────────────────────────────────────────────────────────────
# BREAKPOINT 3 : Calcul de increment_start_x (centrage)
# ─────────────────────────────────────────────────────────────────────────
break settings_panel.c:1200
commands
    silent
    printf "\n"
    printf "════════════════════════════════════════════════════════════════\n"
    printf "🎯 CENTRAGE INCREMENT (MODE STACK)\n"
    printf "════════════════════════════════════════════════════════════════\n"
    printf "  panel_width = %d px\n", panel_width
    printf "  max_increment_width = %d px\n", max_increment_width

    printf "\n  📐 CALCUL increment_start_x :\n"
    printf "     increment_start_x = (panel_width - max_increment_width) / 2\n"
    printf "     increment_start_x = (%d - %d) / 2 = %d px\n", panel_width, max_increment_width, increment_start_x

    printf "\n  📊 MARGES :\n"
    printf "     Gauche : %d px\n", increment_start_x
    printf "     Droite : %d px\n", panel_width - increment_start_x - max_increment_width

    printf "════════════════════════════════════════════════════════════════\n\n"
    continue
end

# ─────────────────────────────────────────────────────────────────────────
# BREAKPOINT 4 : Rendu du widget (avec alignement)
# ─────────────────────────────────────────────────────────────────────────
break render_config_widget
commands
    silent
    printf "\n"
    printf "════════════════════════════════════════════════════════════════\n"
    printf "🎨 RENDU WIDGET INCREMENT : %s\n", widget->option_name
    printf "════════════════════════════════════════════════════════════════\n"
    printf "  container_width = %d px\n", container_width

    # Continuer jusqu'au calcul de roller_x_offset
    break widget.c:324
    continue

    printf "\n  📐 CALCUL POSITION ROLLER (avec alignement) :\n"
    printf "     roller_x_offset = calculate_roller_x_offset(widget, container_width)\n"
    printf "     roller_x_offset = %d px\n", roller_x_offset

    if container_width > 0
        printf "\n  🔄 MODE ALIGNEMENT ACTIF (container_width > 0)\n"
        printf "     Roller aligné à droite avec marge 10px\n"
    else
        printf "\n  📍 MODE POSITION PAR DÉFAUT (container_width = 0)\n"
        printf "     Roller à position local_roller_x = %d px\n", widget->local_roller_x
    end

    # Continuer jusqu'au calcul des positions écran
    break widget.c:382
    continue

    printf "\n  🖥️  POSITIONS ÉCRAN :\n"
    printf "     widget_screen_x = %d px\n", widget_screen_x
    printf "     widget_screen_y = %d px\n", widget_screen_y
    printf "     roller_screen_x = widget_screen_x + roller_x_offset\n"
    printf "     roller_screen_x = %d + %d = %d px\n", widget_screen_x, roller_x_offset, roller_screen_x
    printf "     roller_screen_y = %d px\n", roller_screen_y

    printf "\n  📊 ESPACEMENT EFFECTIF LABEL-ROLLER :\n"
    printf "     espacement = roller_x_offset - text_width\n"
    printf "     espacement = %d - [text_width] px\n", roller_x_offset
    printf "     (Note: text_width non disponible ici, calculer manuellement)\n"

    printf "════════════════════════════════════════════════════════════════\n\n"

    # Effacer les breakpoints temporaires
    clear widget.c:324
    clear widget.c:382

    continue
end

# ─────────────────────────────────────────────────────────────────────────
# BREAKPOINT 5 : Fonction calculate_roller_x_offset (alignement)
# ─────────────────────────────────────────────────────────────────────────
break calculate_roller_x_offset
commands
    silent
    printf "\n"
    printf "════════════════════════════════════════════════════════════════\n"
    printf "🧮 CALCUL ALIGNEMENT ROLLER\n"
    printf "════════════════════════════════════════════════════════════════\n"
    printf "  Widget: %s\n", widget->option_name
    printf "  container_width = %d px\n", container_width
    printf "  local_roller_x (défaut) = %d px\n", widget->local_roller_x

    if container_width > 0
        # Continuer jusqu'au calcul de roller_x_offset
        break widget.c:294
        continue

        printf "\n  📐 CALCUL ALIGNEMENT À DROITE :\n"
        printf "     RIGHT_MARGIN = 10 px\n"
        printf "     roller_total_width = %d px\n", widget->roller_width
        printf "     roller_x_offset = container_width - roller_total_width - RIGHT_MARGIN\n"
        printf "     roller_x_offset = %d - %d - 10 = %d px\n", container_width, widget->roller_width, roller_x_offset

        if roller_x_offset < widget->local_roller_x
            printf "\n  ⚠️  SÉCURITÉ: roller_x_offset < local_roller_x\n"
            printf "     Utilisation de local_roller_x = %d px au lieu de %d px\n", widget->local_roller_x, roller_x_offset
        end

        clear widget.c:294
    else
        printf "\n  📍 PAS D'ALIGNEMENT (container_width = 0)\n"
        printf "     Utilisation de local_roller_x = %d px\n", widget->local_roller_x
    end

    printf "════════════════════════════════════════════════════════════════\n\n"
    continue
end

# ─────────────────────────────────────────────────────────────────────────
# COMMANDES INITIALES
# ─────────────────────────────────────────────────────────────────────────
printf "\n"
printf "════════════════════════════════════════════════════════════════════════════\n"
printf "🔍 SCRIPT DE DEBUG : ESPACEMENT WIDGETS INCREMENT\n"
printf "════════════════════════════════════════════════════════════════════════════\n"
printf "\n"
printf "Ce script va afficher :\n"
printf "  1️⃣  Création des widgets : calcul des espacements de base\n"
printf "  2️⃣  Mode STACK : calcul de la largeur max et centrage\n"
printf "  3️⃣  Rendu : positions effectives avec alignement\n"
printf "\n"
printf "📝 INSTRUCTIONS :\n"
printf "  - Lancez l'application normalement\n"
printf "  - Testez en mode UNSTACK (panneau large)\n"
printf "  - Testez en mode STACK (panneau étroit)\n"
printf "  - Observez les valeurs affichées\n"
printf "\n"
printf "⚠️  PROBLÈME ATTENDU :\n"
printf "  - Espacement label-roller trop grand (~15px en trop)\n"
printf "  - Vérifier : base_espace_apres_texte et calculs de position\n"
printf "\n"
printf "════════════════════════════════════════════════════════════════════════════\n"
printf "\nAppuyez sur ENTRÉE pour lancer l'application...\n"
# Ne pas mettre de "run" ici, laisser l'utilisateur le faire manuellement
