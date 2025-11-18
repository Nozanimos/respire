# ════════════════════════════════════════════════════════════════════════════
# SCRIPT GDB - DEBUG COMPTEUR RESPONSIVE
# ════════════════════════════════════════════════════════════════════════════
#
# Objectif : Tracer l'affichage du compteur en mode responsive (200-300px)
#            au moment du changement de chiffre pour identifier le bug
#
# Bug observé : Le compteur occupe tout l'hexagone au moment du changement
#               de chiffre au lieu d'appliquer le ratio directement
#
# Usage : gdb -x debug_counter_responsive.gdb ./build/respire
# ════════════════════════════════════════════════════════════════════════════

# Désactiver la pagination pour un log continu
set pagination off
set confirm off

# Activer les logs détaillés
set logging file gdb_counter_trace.log
set logging overwrite on
set logging on

# ════════════════════════════════════════════════════════════════════════════
# VARIABLES GLOBALES POUR LE TRAÇAGE
# ════════════════════════════════════════════════════════════════════════════

# Compteur de captures après scale_min
set $capture_count = 0
set $max_captures = 6
set $capture_interval_frames = 18  # 0.3s à 60fps = 18 frames
set $frames_since_scale_min = 0
set $scale_min_detected = 0
set $last_breath_count = 0

# ════════════════════════════════════════════════════════════════════════════
# BREAKPOINT 1 : Détection du changement de compteur (incrémentation)
# ════════════════════════════════════════════════════════════════════════════

break counter.c:127
commands
    silent

    # Vérifier si c'est vraiment une incrémentation
    if counter->current_breath != $last_breath_count
        printf "\n"
        printf "═══════════════════════════════════════════════════════════════════════\n"
        printf "🔔 DÉTECTION CHANGEMENT DE CHIFFRE : %d → %d\n", $last_breath_count, counter->current_breath
        printf "═══════════════════════════════════════════════════════════════════════\n"
        printf "Frame actuelle : %d / %d\n", hex_node->current_cycle, hex_node->total_cycles
        printf "Flag scale_min actuel : %d\n", current_frame->is_at_scale_min
        printf "Flag was_at_min_last  : %d\n", counter->was_at_min_last_frame
        printf "\n"

        # Marquer qu'on a détecté scale_min pour commencer les captures
        set $scale_min_detected = 1
        set $frames_since_scale_min = 0
        set $capture_count = 0

        # Mettre à jour le dernier compteur
        set $last_breath_count = counter->current_breath
    end

    continue
end

# ════════════════════════════════════════════════════════════════════════════
# BREAKPOINT 2 : Entrée dans counter_render (à chaque frame)
# ════════════════════════════════════════════════════════════════════════════

break counter.c:91
commands
    silent

    # Si le compteur n'est pas actif, ignorer
    if !counter->is_active
        continue
    end

    # Calculer les valeurs importantes
    set $current_cycle = hex_node->current_cycle
    set $total_cycles = hex_node->total_cycles

    # Vérifier que current_cycle est dans les limites
    if $current_cycle < 0 || $current_cycle >= $total_cycles
        continue
    end

    # Récupérer la frame actuelle
    set $frame_ptr = &hex_node->precomputed_counter_frames[$current_cycle]
    set $text_scale = $frame_ptr->text_scale
    set $is_at_min = $frame_ptr->is_at_scale_min
    set $is_at_max = $frame_ptr->is_at_scale_max

    # ════════════════════════════════════════════════════════════════════════
    # CAPTURE : Si on vient de détecter scale_min, commencer les captures
    # ════════════════════════════════════════════════════════════════════════

    if $scale_min_detected == 1
        # Incrémenter le compteur de frames
        set $frames_since_scale_min = $frames_since_scale_min + 1

        # Capturer tous les N frames (pour avoir 6 captures espacées de 0.3s)
        if $frames_since_scale_min % $capture_interval_frames == 0
            if $capture_count < $max_captures
                printf "\n"
                printf "─────────────────────────────────────────────────────────────────────\n"
                printf "📸 CAPTURE #%d (T + %.1fs après scale_min)\n", $capture_count + 1, ($frames_since_scale_min / 60.0)
                printf "─────────────────────────────────────────────────────────────────────\n"
                printf "⏱️  Timing:\n"
                printf "    Frames depuis scale_min : %d\n", $frames_since_scale_min
                printf "    Secondes écoulées       : %.2f s\n", ($frames_since_scale_min / 60.0)
                printf "\n"
                printf "🔢 État du compteur:\n"
                printf "    Chiffre affiché         : %d / %d\n", counter->current_breath, counter->total_breaths
                printf "    was_at_min_last_frame   : %d\n", counter->was_at_min_last_frame
                printf "    waiting_for_scale_min   : %d\n", counter->waiting_for_scale_min
                printf "\n"
                printf "🎬 Animation (frame actuelle):\n"
                printf "    current_cycle           : %d / %d\n", $current_cycle, $total_cycles
                printf "    text_scale (précalculé) : %.4f\n", $text_scale
                printf "    is_at_scale_min         : %d\n", $is_at_min
                printf "    is_at_scale_max         : %d\n", $is_at_max
                printf "    current_scale (node)    : %.4f\n", hex_node->current_scale
                printf "\n"
                printf "📐 Responsive:\n"
                printf "    scale_factor            : %.4f\n", scale_factor
                printf "    base_font_size          : %d\n", counter->base_font_size
                printf "\n"

                # Incrémenter le compteur de captures
                set $capture_count = $capture_count + 1

                # Si on a fait toutes les captures, arrêter le traçage
                if $capture_count >= $max_captures
                    printf "─────────────────────────────────────────────────────────────────────\n"
                    printf "✅ Toutes les captures effectuées. Désactivation du traçage.\n"
                    printf "─────────────────────────────────────────────────────────────────────\n\n"
                    set $scale_min_detected = 0
                end
            end
        end
    end

    continue
end

# ════════════════════════════════════════════════════════════════════════════
# BREAKPOINT 3 : Calcul du font_size (ligne critique)
# ════════════════════════════════════════════════════════════════════════════

break counter.c:172
commands
    silent

    # Seulement si on est en mode capture
    if $scale_min_detected == 1
        # Calculer le font_size ici
        set $calculated_font_size = counter->base_font_size * text_scale * scale_factor

        # Afficher uniquement si c'est une frame de capture
        if $frames_since_scale_min % $capture_interval_frames == 0 && $capture_count < $max_captures
            printf "💡 Calcul font_size:\n"
            printf "    base_font_size * text_scale * scale_factor\n"
            printf "    = %d * %.4f * %.4f\n", counter->base_font_size, text_scale, scale_factor
            printf "    = %.2f pixels\n", $calculated_font_size
            printf "\n"
        end
    end

    continue
end

# ════════════════════════════════════════════════════════════════════════════
# BREAKPOINT 4 : Mesure du texte Cairo (capture extents)
# ════════════════════════════════════════════════════════════════════════════

break counter.c:205
commands
    silent

    # Seulement si on est en mode capture
    if $scale_min_detected == 1
        # Afficher uniquement si c'est une frame de capture
        if $frames_since_scale_min % $capture_interval_frames == 0 && $capture_count < $max_captures
            printf "📏 Mesure texte Cairo (après cairo_text_extents):\n"
            printf "    extents.width           : %.2f\n", extents.width
            printf "    extents.height          : %.2f\n", extents.height
            printf "    extents.x_bearing       : %.2f\n", extents.x_bearing
            printf "    extents.y_bearing       : %.2f\n", extents.y_bearing
            printf "\n"
        end
    end

    continue
end

# ════════════════════════════════════════════════════════════════════════════
# BREAKPOINT 5 : Taille finale de la surface Cairo
# ════════════════════════════════════════════════════════════════════════════

break counter.c:214
commands
    silent

    # Seulement si on est en mode capture
    if $scale_min_detected == 1
        # Afficher uniquement si c'est une frame de capture
        if $frames_since_scale_min % $capture_interval_frames == 0 && $capture_count < $max_captures
            printf "🎨 Surface Cairo créée:\n"
            printf "    text_width  (width + 10) : %d pixels\n", text_width
            printf "    text_height (height + 10): %d pixels\n", text_height
            printf "\n"
        end
    end

    continue
end

# ════════════════════════════════════════════════════════════════════════════
# BREAKPOINT 6 : Application de la frame précalculée (synchronisation)
# ════════════════════════════════════════════════════════════════════════════

break precompute_list.c:232
commands
    silent

    # Seulement si on est en mode capture ET compteur actif
    if $scale_min_detected == 1
        # Vérifier si le node a un cycle actuel valide
        if node->current_cycle >= 0 && node->current_cycle < node->total_cycles
            set $old_cycle = node->current_cycle
            set $new_cycle = ($old_cycle + 1) % node->total_cycles

            # Afficher uniquement si c'est une frame de capture
            if $frames_since_scale_min % $capture_interval_frames == 0 && $capture_count < $max_captures
                printf "🔄 apply_precomputed_frame (incrémentation cycle):\n"
                printf "    current_cycle : %d → %d\n", $old_cycle, $new_cycle
                printf "    current_scale : %.4f\n", node->current_scale
                printf "\n"
            end
        end
    end

    continue
end

# ════════════════════════════════════════════════════════════════════════════
# BREAKPOINT 7 : Capture initiale AVANT le premier scale_min
# ════════════════════════════════════════════════════════════════════════════

break counter.c:125
commands
    silent

    # Capturer l'état juste AVANT l'incrémentation
    if is_at_min_now && !counter->was_at_min_last_frame && $scale_min_detected == 0
        printf "\n"
        printf "═══════════════════════════════════════════════════════════════════════\n"
        printf "⏮️  ÉTAT JUSTE AVANT LE CHANGEMENT DE CHIFFRE\n"
        printf "═══════════════════════════════════════════════════════════════════════\n"
        printf "🔢 Compteur actuel         : %d / %d\n", counter->current_breath, counter->total_breaths
        printf "🎬 Frame actuelle          : %d / %d\n", hex_node->current_cycle, hex_node->total_cycles
        printf "📐 text_scale (précalculé) : %.4f\n", current_frame->text_scale
        printf "📏 scale_factor            : %.4f\n", scale_factor
        printf "═══════════════════════════════════════════════════════════════════════\n\n"
    end

    continue
end

# ════════════════════════════════════════════════════════════════════════════
# MESSAGES DE DÉMARRAGE
# ════════════════════════════════════════════════════════════════════════════

printf "\n"
printf "═══════════════════════════════════════════════════════════════════════════\n"
printf "🐛 DEBUG COMPTEUR RESPONSIVE - MODE TRACE ACTIVÉ\n"
printf "═══════════════════════════════════════════════════════════════════════════\n"
printf "\n"
printf "Configuration du traçage :\n"
printf "  - Capture au changement de chiffre (scale_min)\n"
printf "  - 6 captures espacées de 0.3s après le changement\n"
printf "  - Logs sauvegardés dans : gdb_counter_trace.log\n"
printf "\n"
printf "Points de trace actifs :\n"
printf "  ✓ Incrémentation compteur (counter.c:127)\n"
printf "  ✓ Entrée counter_render (counter.c:91)\n"
printf "  ✓ Calcul font_size (counter.c:172)\n"
printf "  ✓ Mesure texte Cairo (counter.c:205)\n"
printf "  ✓ Création surface Cairo (counter.c:214)\n"
printf "  ✓ Incrémentation cycle (precompute_list.c:232)\n"
printf "  ✓ État avant changement (counter.c:125)\n"
printf "\n"
printf "Instructions :\n"
printf "  1. Lancez l'application : run\n"
printf "  2. Réduisez la fenêtre à 200-300px\n"
printf "  3. Lancez l'animation et attendez le changement de chiffre\n"
printf "  4. Le script capturera automatiquement les données\n"
printf "  5. Consultez gdb_counter_trace.log pour l'analyse\n"
printf "\n"
printf "═══════════════════════════════════════════════════════════════════════════\n\n"

# Démarrage automatique (commenté par défaut - décommenter pour auto-run)
# run

# Note : Pour une utilisation interactive, lancez manuellement avec "run"
# puis réduisez la fenêtre et observez les traces
