# Script GDB pour debugger le hovering des widgets INCREMENT
# Usage: gdb -x debug_hover.gdb ./bin/respire

# Compiler avec symboles debug
set pagination off
set print pretty on

# Breakpoint sur la fonction qui gère les events des ConfigWidget (INCREMENT)
break handle_config_widget_events
commands
    silent
    printf "\n=== handle_config_widget_events ===\n"
    printf "Widget: %s\n", widget->option_name
    printf "Event type: %d\n", event->type
    if event->type == 1024
        printf "  -> MOUSEMOTION: x=%d, y=%d\n", event->motion.x, event->motion.y
        printf "  Widget position (relative): x=%d, y=%d\n", widget->base.x, widget->base.y
        printf "  Widget size: w=%d, h=%d\n", widget->base.width, widget->base.height
        printf "  Offset: offset_x=%d, offset_y=%d\n", offset_x, offset_y
        printf "  Widget position (absolute): x=%d, y=%d\n", offset_x + widget->base.x, offset_y + widget->base.y
        printf "  Mouse in range X? %d <= %d < %d\n", offset_x + widget->base.x, event->motion.x, offset_x + widget->base.x + widget->base.width
        printf "  Mouse in range Y? %d <= %d < %d\n", offset_y + widget->base.y, event->motion.y, offset_y + widget->base.y + widget->base.height
        printf "  -> Hovered: %d\n", widget->base.hovered
    end
    continue
end

# Breakpoint sur la fonction de création pour voir les valeurs initiales
break create_config_widget
commands
    silent
    printf "\n=== create_config_widget ===\n"
    printf "Creating widget: %s\n", name
    printf "Position: x=%d, y=%d\n", x, y
    printf "Text size: %d\n", text_size
    printf "Arrow size: %d\n", arrow_size
    printf "Value: min=%d, max=%d, current=%d\n", min_value, max_value, initial_value
    continue
end

# Breakpoint après le calcul de total_width pour voir ce qui est stocké
break widget.c:215
commands
    silent
    printf "\n=== After total_width calculation ===\n"
    printf "Widget: %s\n", name
    printf "Text width measured: %d\n", text_width
    printf "Value string: '%s'\n", value_str
    printf "Value width estimated: %d\n", value_width
    printf "local_value_x: %d\n", widget->local_value_x
    printf "total_width: %d\n", total_width
    printf "total_height: %d\n", total_height
    printf "base.width will be set to: %d\n", total_width
    continue
end

# Breakpoint pour afficher quand un widget devient hovered
break widget.c:398
commands
    silent
    if widget->base.hovered
        printf "\n=== Widget HOVERED ===\n"
        printf "Widget: %s\n", widget->option_name
        printf "Mouse: (%d, %d)\n", mx, my
        printf "Widget abs pos: (%d, %d)\n", offset_x + widget->base.x, offset_y + widget->base.y
        printf "Widget size: %dx%d\n", widget->base.width, widget->base.height
        printf "base.width = %d\n", widget->base.width
        printf "local_value_x = %d\n", widget->local_value_x
    end
    continue
end

printf "\n======================================\n"
printf "GDB Debug Script for Widget Hovering\n"
printf "======================================\n"
printf "Breakpoints set:\n"
printf "  - handle_config_widget_events\n"
printf "  - create_config_widget\n"
printf "  - widget.c:215 (after total_width calc)\n"
printf "  - widget.c:398 (hovering detection)\n"
printf "\nStarting program...\n"
printf "Ouvre le panneau de config et survole les widgets INCREMENT\n"
printf "======================================\n\n"

run
