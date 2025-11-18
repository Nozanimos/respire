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
# Usage : gdb -x debug_counter_responsive.gdb ./bin/respire
# ════════════════════════════════════════════════════════════════════════════

set pagination off
set confirm off

set logging file gdb_counter_trace.log
set logging overwrite on
set logging on

# ════════════════════════════════════════════════════════════════════════════
# VARIABLES GLOBALES POUR LE TRAÇAGE
# ════════════════════════════════════════════════════════════════════════════

set $capture_count = 0
set $max_captures = 6
set $capture_interval_frames = 18
set $frames_since_scale_min = 0
set $scale_min_detected = 0
set $last_breath_count = 0

# ════════════════════════════════════════════════════════════════════════════
# BREAKPOINT 1 : Détection du changement de compteur (incrémentation)
# ════════════════════════════════════════════════════════════════════════════

break counter.c:127
commands
    silent
    if counter->current_breath != $last_breath_count
        printf "\n"
        printf "═══════════════════════════════════════════════════════════════════════\n"
        printf "DETECTION CHANGEMENT DE CHIFFRE : %d -> %d\n", $last_breath_count, counter->current_breath
        printf "═══════════════════════════════════════════════════════════════════════\n"
        printf "Frame actuelle : %d / %d\n", hex_node->current_cycle, hex_node->total_cycles
        printf "Flag scale_min actuel : %d\n", current_frame->is_at_scale_min
        printf "Flag was_at_min_last  : %d\n", counter->was_at_min_last_frame
        printf "\n"
        set $scale_min_detected = 1
        set $frames_since_scale_min = 0
        set $capture_count = 0
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
    if !counter->is_active
        continue
    end
    set $current_cycle = hex_node->current_cycle
    set $total_cycles = hex_node->total_cycles
    if $current_cycle < 0 || $current_cycle >= $total_cycles
        continue
    end
    set $frame_ptr = &hex_node->precomputed_counter_frames[$current_cycle]
    set $text_scale = $frame_ptr->text_scale
    set $is_at_min = $frame_ptr->is_at_scale_min
    set $is_at_max = $frame_ptr->is_at_scale_max
    if $scale_min_detected == 1
        set $frames_since_scale_min = $frames_since_scale_min + 1
        if $frames_since_scale_min % $capture_interval_frames == 0
            if $capture_count < $max_captures
                printf "\n"
                printf "---------------------------------------------------------------------\n"
                printf "CAPTURE #%d (T + %.1fs apres scale_min)\n", $capture_count + 1, ($frames_since_scale_min / 60.0)
                printf "---------------------------------------------------------------------\n"
                printf "Timing:\n"
                printf "    Frames depuis scale_min : %d\n", $frames_since_scale_min
                printf "    Secondes ecoulees       : %.2f s\n", ($frames_since_scale_min / 60.0)
                printf "\n"
                printf "Etat du compteur:\n"
                printf "    Chiffre affiche         : %d / %d\n", counter->current_breath, counter->total_breaths
                printf "    was_at_min_last_frame   : %d\n", counter->was_at_min_last_frame
                printf "    waiting_for_scale_min   : %d\n", counter->waiting_for_scale_min
                printf "\n"
                printf "Animation (frame actuelle):\n"
                printf "    current_cycle           : %d / %d\n", $current_cycle, $total_cycles
                printf "    text_scale (precalcule) : %.4f\n", $text_scale
                printf "    is_at_scale_min         : %d\n", $is_at_min
                printf "    is_at_scale_max         : %d\n", $is_at_max
                printf "    current_scale (node)    : %.4f\n", hex_node->current_scale
                printf "\n"
                printf "Responsive:\n"
                printf "    scale_factor            : %.4f\n", scale_factor
                printf "    base_font_size          : %d\n", counter->base_font_size
                printf "\n"
                set $capture_count = $capture_count + 1
                if $capture_count >= $max_captures
                    printf "---------------------------------------------------------------------\n"
                    printf "Toutes les captures effectuees. Desactivation du tracage.\n"
                    printf "---------------------------------------------------------------------\n\n"
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
    if $scale_min_detected == 1
        set $calculated_font_size = counter->base_font_size * text_scale * scale_factor
        if $frames_since_scale_min % $capture_interval_frames == 0 && $capture_count < $max_captures
            printf "Calcul font_size:\n"
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
    if $scale_min_detected == 1
        if $frames_since_scale_min % $capture_interval_frames == 0 && $capture_count < $max_captures
            printf "Mesure texte Cairo (apres cairo_text_extents):\n"
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
    if $scale_min_detected == 1
        if $frames_since_scale_min % $capture_interval_frames == 0 && $capture_count < $max_captures
            printf "Surface Cairo creee:\n"
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
    if $scale_min_detected == 1
        if node->current_cycle >= 0 && node->current_cycle < node->total_cycles
            set $old_cycle = node->current_cycle
            set $new_cycle = ($old_cycle + 1) % node->total_cycles
            if $frames_since_scale_min % $capture_interval_frames == 0 && $capture_count < $max_captures
                printf "apply_precomputed_frame (incrementation cycle):\n"
                printf "    current_cycle : %d -> %d\n", $old_cycle, $new_cycle
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
    if is_at_min_now && !counter->was_at_min_last_frame && $scale_min_detected == 0
        printf "\n"
        printf "═══════════════════════════════════════════════════════════════════════\n"
        printf "ETAT JUSTE AVANT LE CHANGEMENT DE CHIFFRE\n"
        printf "═══════════════════════════════════════════════════════════════════════\n"
        printf "Compteur actuel         : %d / %d\n", counter->current_breath, counter->total_breaths
        printf "Frame actuelle          : %d / %d\n", hex_node->current_cycle, hex_node->total_cycles
        printf "text_scale (precalcule) : %.4f\n", current_frame->text_scale
        printf "scale_factor            : %.4f\n", scale_factor
        printf "═══════════════════════════════════════════════════════════════════════\n\n"
    end
    continue
end

# ════════════════════════════════════════════════════════════════════════════
# MESSAGES DE DÉMARRAGE
# ════════════════════════════════════════════════════════════════════════════

printf "\n"
printf "═══════════════════════════════════════════════════════════════════════════\n"
printf "DEBUG COMPTEUR RESPONSIVE - MODE TRACE ACTIVE\n"
printf "═══════════════════════════════════════════════════════════════════════════\n"
printf "\n"
printf "Configuration du tracage :\n"
printf "  - Capture au changement de chiffre (scale_min)\n"
printf "  - 6 captures espacees de 0.3s apres le changement\n"
printf "  - Logs sauvegardes dans : gdb_counter_trace.log\n"
printf "\n"
printf "Points de trace actifs :\n"
printf "  - Incrementation compteur (counter.c:127)\n"
printf "  - Entree counter_render (counter.c:91)\n"
printf "  - Calcul font_size (counter.c:172)\n"
printf "  - Mesure texte Cairo (counter.c:205)\n"
printf "  - Creation surface Cairo (counter.c:214)\n"
printf "  - Incrementation cycle (precompute_list.c:232)\n"
printf "  - Etat avant changement (counter.c:125)\n"
printf "\n"
printf "Instructions :\n"
printf "  1. Lancez l'application : run\n"
printf "  2. Reduisez la fenetre a 200-300px\n"
printf "  3. Lancez l'animation et attendez le changement de chiffre\n"
printf "  4. Le script capturera automatiquement les donnees\n"
printf "  5. Consultez gdb_counter_trace.log pour l'analyse\n"
printf "\n"
printf "═══════════════════════════════════════════════════════════════════════════\n\n"
