#include "common.c"

typedef enum Ui_interaction_type {
    ui_interaction_none,
    ui_interaction_nop,
    ui_interaction_immediate_button
} Ui_interaction_type;

typedef enum Widget_status { widget_present, widget_not_present } Widget_status;

int g_widget_id_generator = 0;

typedef struct Widget {
    bool valid;
    int id;
    Ui_interaction_type type;
} Widget;

typedef struct Ui_context {
    //Widget got_clicked_ui_id, hovered_on_ui_id, got_activated_ui_id;
    //bool did_get_clicked, is_hovering, was_some_ui_id_activated;
    Widget half_clicked_widget, hovered_widget, fully_clicked_widget;
    
    Input *input;
    
    Command_buffer *command_buffer;
} Ui_context;

typedef struct Ui_layout {
    Rect rect;
} Ui_layout;

typedef struct Rect_triangles {
    v3 positions[6];
} Rect_triangles;

Rect_triangles create_rect_triangles(Rect rect) {
    Rect_triangles rect_triangles;
    rect_triangles.positions[0] = get_v3(rect.x_max, rect.y_min, 0);
    rect_triangles.positions[1] = get_v3(rect.x_min, rect.y_min, 0);
    rect_triangles.positions[2] = get_v3(rect.x_min, rect.y_max, 0);
    rect_triangles.positions[3] = get_v3(rect.x_min, rect.y_max, 0);
    rect_triangles.positions[4] = get_v3(rect.x_max, rect.y_max, 0);
    rect_triangles.positions[5] = get_v3(rect.x_max, rect.y_min, 0);
    
    return rect_triangles;
}

void draw_ui_rect_with_texture(Ui_context *ui_context, Rect rect, const char *texture_name) {
    Draw_rect_command_data data = {
        .rect = rect,
        .rect_type = texture_rect_type,
        .texture = get_texture_by_name(texture_name)
    };
    
    push_command(ui_context->command_buffer, draw_rect_command, &data, sizeof(Draw_rect_command_data));
}

bool did_mouse_go_up(Input *input) {
    return input->mouse_buttons_pressed_status[left_mouse_button];
}

bool did_mouse_go_down(Input *input) {
    return input->mouse_buttons_pressed_status[left_mouse_button];
}

void check_if_mouse_hovering_over_widget(Ui_context *context, Widget widget, Rect rect) {
    bool is_hovering = is_point_inside_rect(rect, context->input->mouse_pos);
    if (is_hovering) {
        context->hovered_widget = widget;
    }
}

void begin_ui_frame(Ui_context *context, Input *input) {
    
}

void end_ui_frame(Ui_context *context) {
    g_widget_id_generator = 0;
}

bool is_widget_equal(Widget a, Widget b) {
    if (a.id == b.id && a.valid && b.valid) 
        return true;
    return false;
}

Widget create_widget() {
    Widget widget = {0};
    widget.valid = true;
    widget.id = g_widget_id_generator++;
    
    return widget;
}

bool button(Ui_context *context, bool enabled, Rect rect) {
    Widget widget = create_widget(); 
    check_if_mouse_hovering_over_widget(context, widget, rect);
    
    if (is_widget_equal(context->fully_clicked_widget, widget)) {
        context->fully_clicked_widget.valid = false;
        return true;
    }
    
    return false;
}

void begin_layout_box(Ui_context *context, Rect rect, v4 color) {
    
}

void render_ui(Ui_context *context) {
    Ui_layout layout_ = {0}; //begin_layout_box(context, get_rect(context->ui_rect));
    Ui_layout *layout = &layout_;
    //to execute
    
    ///begin_row(layout);
    if (button(context, true, create_rect(0, 0, 200, 100))) {
        printf("shoveoff\n");
    }
    //end_row(layout);%
}


void draw_ui_rect_with_color(Ui_context *ui_context, Rect rect, v4 color) {
    Draw_rect_command_data data = {
        .rect = rect,
        .rect_type = color_rect_type,
        .color = color
    };
    
    push_command(ui_context->command_buffer, draw_rect_command, &data, sizeof(Draw_rect_command_data));
}


void interact_with_ui(Ui_context *context) {
    if (context->half_clicked_widget.valid == true) {
        if (is_widget_equal(context->half_clicked_widget, context->hovered_widget)) {
            if (did_mouse_go_up(context->input)) {
                context->fully_clicked_widget = context->half_clicked_widget;
            }
            context->half_clicked_widget.valid = false;
        }
    }
    else {
        if(context->hovered_widget.valid == true) {
            if(did_mouse_go_down(context->input)) {
                context->half_clicked_widget = context->hovered_widget;
            }
        }
    }
    
    context->hovered_widget.valid = false;
}

