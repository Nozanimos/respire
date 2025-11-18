set pagination off
set confirm off

set logging file gdb_counter_trace_v2.txt
set logging overwrite on
set logging enabled on

set $last_breath = 0
set $capture_next_frames = 0
set $frames_captured = 0

break counter.c:127
commands
    silent
    if counter->current_breath != $last_breath
        printf "\n"
        printf "========================================================================\n"
        printf "INCREMENTATION DETECTEE : %d -> %d\n", $last_breath, counter->current_breath
        printf "========================================================================\n"
        printf "Frame au moment incrementation : %d\n", hex_node->current_cycle
        printf "text_scale a ce moment         : %.4f\n", current_frame->text_scale
        printf "scale_factor                   : %.4f\n", scale_factor
        set $last_breath = counter->current_breath
        set $capture_next_frames = 1
        set $frames_captured = 0
    end
    continue
end

break counter.c:172
commands
    silent
    if $capture_next_frames == 1
        printf "\n"
        printf "--- FRAME #%d apres incrementation ---\n", $frames_captured
        printf "current_cycle    : %d\n", hex_node->current_cycle
        printf "text_scale       : %.4f\n", text_scale
        printf "scale_factor     : %.4f\n", scale_factor
        printf "font_size calcule: %.2f pixels\n", counter->base_font_size * text_scale * scale_factor
        set $frames_captured = $frames_captured + 1
        if $frames_captured >= 10
            printf "\n>>> 10 frames capturees, arret du tracage de cette incrementation\n"
            set $capture_next_frames = 0
        end
    end
    continue
end

break precompute_list.c:232
commands
    silent
    if $capture_next_frames == 1
        printf "  [apply_precomputed_frame] cycle %d -> %d (scale=%.4f)\n", node->current_cycle, (node->current_cycle + 1) % node->total_cycles, node->current_scale
    end
    continue
end

break counter.c:91
commands
    silent
    if $capture_next_frames == 1
        set $current = hex_node->current_cycle
        set $frame = &hex_node->precomputed_counter_frames[$current]
        printf "  [counter_render entree] cycle=%d is_at_min=%d is_at_max=%d text_scale=%.4f\n", $current, $frame->is_at_scale_min, $frame->is_at_scale_max, $frame->text_scale
    end
    continue
end

printf "\n"
printf "========================================================================\n"
printf "DEBUG COMPTEUR V2 - TRACE FRAME PAR FRAME\n"
printf "========================================================================\n"
printf "Capture les 10 premieres frames apres chaque incrementation\n"
printf "Pour chaque frame, on voit:\n"
printf "  - current_cycle (index frame)\n"
printf "  - text_scale utilise\n"
printf "  - font_size calcule\n"
printf "  - Appels a apply_precomputed_frame\n"
printf "  - Entrees dans counter_render\n"
printf "\n"
printf "Logs dans: gdb_counter_trace_v2.txt\n"
printf "========================================================================\n\n"
