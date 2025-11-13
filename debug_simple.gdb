# Script GDB simple pour tracer empilement/depilement
set pagination off
set print pretty on

# Breakpoint sur empilement
break settings_panel.c:1161
commands
    silent
    printf "\n=== EMPILEMENT ===\n"
    printf "panel_width = %d, min_width = %d\n", panel_width, panel->min_width_for_unstack
    printf "widgets_stacked: 0 -> 1\n"
    continue
end

# Breakpoint sur depilement
break settings_panel.c:984
commands
    silent
    printf "\n=== DEPILEMENT ===\n"
    printf "panel_width = %d, min_width = %d\n", panel_width, panel->min_width_for_unstack
    printf "widgets_stacked: 1 -> 0\n"
    continue
end

printf "\nScript GDB simple charge\n"
printf "Trace uniquement empilement/depilement\n\n"

run
