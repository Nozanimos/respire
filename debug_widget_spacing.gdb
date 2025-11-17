# ════════════════════════════════════════════════════════════════════════════
# Script GDB pour analyser l'espacement des widgets INCREMENT
# ════════════════════════════════════════════════════════════════════════════
# Usage : gdb -x debug_widget_spacing.gdb ./bin/respire
# ════════════════════════════════════════════════════════════════════════════

set pagination off
set print pretty on

# ─────────────────────────────────────────────────────────────────────────
# BREAKPOINT 1 : Fin de création du widget (après tous les calculs)
# ─────────────────────────────────────────────────────────────────────────
break widget.c:260
commands
    silent
    printf "\n"
    printf "════════════════════════════════════════════════════════════════\n"
    printf "🔧 WIDGET INCREMENT CRÉÉ : %s\n", widget->option_name
    printf "════════════════════════════════════════════════════════════════\n"
    printf "  📏 text_size = %d px\n", widget->current_text_size
    printf "  📐 base_espace_apres_texte = %d px  (text_size * 0.7)\n", widget->base_espace_apres_texte
    printf "  📐 base_roller_padding = %d px  (text_size * 0.4)\n", widget->base_roller_padding
    printf "\n  📦 DIMENSIONS :\n"
    printf "     local_roller_x = %d px  (position roller depuis début widget)\n", widget->local_roller_x
    printf "     roller_width = %d px\n", widget->roller_width
    printf "     roller_height = %d px\n", widget->roller_height
    printf "════════════════════════════════════════════════════════════════\n\n"
    continue
end

# ─────────────────────────────────────────────────────────────────────────
# BREAKPOINT 2 : Calcul de la largeur max en mode STACK (settings_panel.c)
# ─────────────────────────────────────────────────────────────────────────
break settings_panel.c:1194
commands
    silent
    printf "\n"
    printf "════════════════════════════════════════════════════════════════\n"
    printf "📏 CALCUL LARGEUR (MODE STACK) : %s\n", w->option_name
    printf "════════════════════════════════════════════════════════════════\n"
    printf "  real_width = local_roller_x + roller_width + 10\n"
    printf "  real_width = %d + %d + 10 = %d px\n", w->local_roller_x, w->roller_width, real_width
    if real_width > max_increment_width
        printf "  ✅ NOUVEAU MAX : %d px\n", real_width
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
    printf "  increment_start_x = (panel_width - max_width) / 2 = %d px\n", increment_start_x
    printf "════════════════════════════════════════════════════════════════\n\n"
    continue
end

# ─────────────────────────────────────────────────────────────────────────
# BREAKPOINT 4 : Calcul roller_x_offset dans render
# ─────────────────────────────────────────────────────────────────────────
break widget.c:325
commands
    silent
    printf "\n"
    printf "════════════════════════════════════════════════════════════════\n"
    printf "🎨 RENDU WIDGET : %s\n", widget->option_name
    printf "════════════════════════════════════════════════════════════════\n"
    printf "  container_width = %d px ", container_width
    if container_width > 0
        printf "(MODE STACK - alignement actif)\n"
    else
        printf "(MODE UNSTACK - position par défaut)\n"
    end
    printf "  roller_x_offset = %d px\n", roller_x_offset
    printf "  local_roller_x (défaut) = %d px\n", widget->local_roller_x
    printf "\n  📊 ESPACEMENT label-roller = %d px\n", roller_x_offset
    printf "════════════════════════════════════════════════════════════════\n\n"
    continue
end

# ─────────────────────────────────────────────────────────────────────────
# BREAKPOINT 5 : Fonction calculate_roller_x_offset (alignement)
# ─────────────────────────────────────────────────────────────────────────
break widget.c:295
commands
    silent
    printf "\n"
    printf "════════════════════════════════════════════════════════════════\n"
    printf "🧮 CALCUL ALIGNEMENT : %s\n", widget->option_name
    printf "════════════════════════════════════════════════════════════════\n"
    printf "  container_width = %d px\n", container_width
    printf "  roller_width = %d px\n", widget->roller_width
    printf "  RIGHT_MARGIN = 10 px\n"
    printf "  roller_x_offset = container_width - roller_width - RIGHT_MARGIN\n"
    printf "  roller_x_offset = %d - %d - 10 = %d px\n", container_width, widget->roller_width, roller_x_offset
    printf "  local_roller_x (min) = %d px\n", widget->local_roller_x
    if roller_x_offset < widget->local_roller_x
        printf "  ⚠️  AJUSTÉ à local_roller_x (sécurité)\n"
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
printf "Ce script s'exécute en continu et affiche automatiquement :\n"
printf "  1️⃣  Création des widgets : calcul des espacements de base\n"
printf "  2️⃣  Mode STACK : calcul de la largeur max et centrage\n"
printf "  3️⃣  Rendu : positions effectives avec alignement\n"
printf "\n"
printf "📝 L'application va se lancer automatiquement.\n"
printf "   Ouvrez le panneau Settings pour voir les calculs.\n"
printf "\n"
printf "⚠️  PROBLÈME ATTENDU :\n"
printf "  - Espacement label-roller trop grand (~15px en trop)\n"
printf "  - Vérifier : base_espace_apres_texte et calculs de position\n"
printf "\n"
printf "════════════════════════════════════════════════════════════════════════════\n"
printf "\n🚀 Lancement automatique de l'application...\n\n"

# Lancer l'application automatiquement
run
