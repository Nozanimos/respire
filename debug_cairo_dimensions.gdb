set pagination off
set confirm off

set logging file gdb_debug_V2.txt
set logging overwrite on
set logging enabled on

set $last_breath = 0

break counter.c:127
commands
    silent
    if counter->current_breath != $last_breath
        printf "\n========== CHANGEMENT %d -> %d ==========\n", $last_breath, counter->current_breath
        printf "cycle=%d text_scale=%.4f scale_factor=%.4f\n", hex_node->current_cycle, current_frame->text_scale, scale_factor
        set $last_breath = counter->current_breath
    end
    continue
end

break counter.c:207
commands
    silent
    if counter->is_active
        set $tw = (int)(extents.width + 10)
        set $th = (int)(extents.height + 10)
        printf "[AVANT surface] extents: w=%.2f h=%.2f -> surface: %dx%d | font_size=%.2f\n", extents.width, extents.height, $tw, $th, font_size
    end
    continue
end

break counter.c:214
commands
    silent
    if counter->is_active
        printf "[APRES surface] text_width=%d text_height=%d\n", text_width, text_height
    end
    continue
end

break counter.c:247
commands
    silent
    if counter->is_active && text_texture
        printf "[SDL_Rect] x=%d y=%d w=%d h=%d | hex_center=%d,%d\n", dest_rect.x, dest_rect.y, dest_rect.w, dest_rect.h, center_x, center_y
    end
    continue
end

printf "\nDebug dimensions Cairo + SDL\n"
printf "Trace: extents -> surface -> SDL_Rect\n\n"
