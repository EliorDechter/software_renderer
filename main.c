//TODO_LIST:
//-hierarchical rasterizer
//-fix multithreading
//-AA
//-multithreading memory allocation
//-make sure every thread has its own cahceline aligned memory
//-texture filtering
//-put a cap on number of jobs & finish with the multithreading code already
//-make sure each job operates on a single cache line - which is not the case right now
//-idea: nested iterator
//-should the buffer default depth be 1? what's the difference between 32 bit and 24 bit depth buffer?
//-frustrum culling before perpsective divide
//-resize window
//-handle different resolutions

//RESEARCH_LIST
//-use optional type?
//-is bounds checking expensive?
//-per vognsen's descriptors

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdbool.h>
#include <x86intrin.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h> 

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-parameter"

#pragma GCC diagnostic pop

#include "common.c"
#include "math.c"
#include "multithreading.c"
#include "renderer.c"

#include "global_data.c"
#include "Parser.c"

#define SAM_HYDE 1
#define SAM_HYDE_TOP_CORNER 0
#define SMALL_SQUARE 0

bool g_window_should_close = false;

typedef struct Image{
    u32 width, height, channels;
    u8 *buffer;
} Image;

typedef enum Key { key_a, key_d, key_s, key_w, key_r, key_f, key_escape, key_unknown, key_num } Key;
typedef enum Mouse_button { left_mouse_button, right_mouse_button, scroll_mouse_up, scroll_mouse_down, num_mouse_buttons } Mouse_button;

typedef struct Input {
    v2 mouse_pos;
    Mouse_button mouse_buttons_pressed_status[num_mouse_buttons];
} Input;

typedef struct Os_context {
    Window window_handle;
    XImage *ximage;
    Display *display;
    
    Input input;
    //char keys[KEY_NUM];
    //char buttons[BUTTON_num];
    //callbacks
    void *userdata;
} Os_context;

typedef struct Os_window{ 
    Window handle;
    XImage *ximage;
    Image *surface;
    
#if 0
    int should_close;
    char keys[KEY_NUM];
    char buttons[BUTTON_NUM];
    callbacks_t callbacks;
    void *userdata;
#endif
} Os_window;

static Image create_image(Allocator *allocator, u32 width, u32 height, u32 channels) {
    u32 num_elements = width * height * channels;
    
    assert(width > 0 && height > 0 && channels >= 1 && channels <= 4);
    
    Image image;
    image.width = width;
    image.height = height;
    image.channels = channels;
    image.buffer = NULL;
    
    u32 buffer_size = (u32)sizeof(unsigned char) * num_elements;
    image.buffer = allocate_perm(allocator, buffer_size);
    memset(image.buffer, 0, buffer_size);
    
    return image;
}

static void blit_image(Framebuffer *framebuffer, Image *image) {
    u32 tile_width = 2;
    u32 tile_height = 2;
    u32 width_in_tiles = (framebuffer->width + tile_width - 1) / tile_width;
    
    for (u32 y = 0; y < framebuffer->height; ++y) {
        for (u32 x = 0; x < framebuffer->width; ++x) {
            
            u32 tile_x = x / tile_width;
            u32 tile_y = y / tile_height;
            u32 in_tile_x = x % tile_width;
            u32 in_tile_y = y % tile_height;
            u32 src_index = (tile_y * width_in_tiles + tile_x) * (tile_width * tile_height) + in_tile_y * tile_width + in_tile_x;
            u32 dst_index = y * framebuffer->width + x;
            
            u32 *src_pixel = (u32 *)framebuffer->color_buffer + src_index;
            u32 *dst_pixel = (u32 *)image->buffer + dst_index;
            
            u8 *src_pixel_elements = (u8 *)src_pixel;
            u8 *dst_pixel_elements = (u8 *)dst_pixel;
            
            dst_pixel_elements[0] = src_pixel_elements[2];  /* blue */
            dst_pixel_elements[1] = src_pixel_elements[1];  /* green */
            dst_pixel_elements[2] = src_pixel_elements[0];  /* red */
        }
    }
}

static Key handle_key_event(Os_context *os_context, int first_key_code) {
    int keysyms_per_keycode_return;
    KeySym *key_sym;
    
    key_sym = XGetKeyboardMapping(os_context->display, first_key_code, 1, &keysyms_per_keycode_return);
    
    Key key;
    switch (key_sym[0]) {
        case XK_a:     key = key_a;     break;
        case XK_d:     key = key_d;     break;
        case XK_s:     key = key_s;     break;
        case XK_w:     key = key_w;     break;
        case XK_r:     key = key_s;     break;
        case XK_f:     key = key_w;     break;
        case XK_Escape: key = key_escape; break;
        default:       key = key_unknown;   break;
    }
    
    XFree(key_sym);
    
    return key;
}

static Mouse_button handle_mouse_button_event(Os_context *os_context, int xbutton, bool is_pressed) {
    Mouse_button button;
    switch(xbutton) {
        case Button1: button = left_mouse_button; break;
        case Button2: button = right_mouse_button; break;
        case Button4: button = scroll_mouse_up; break; //TODO: not sure if button 4 and 5 actually represent mouse up or down
        case Button5: button = scroll_mouse_down; break;
    }
    
    os_context->input.mouse_buttons_pressed_status[button] = is_pressed;
    
    return button;
}

static v2 get_mouse_position(Os_context *context) {
    Window root, child;
    int root_x, root_y, window_x, window_y;
    u32 mask;
    XQueryPointer(context->display, context->window_handle, &root, &child, &root_x, &root_y, &window_x, &window_y, &mask);
    
    return get_v2(window_x, window_y);
}

static void handle_client_event(Display *display, XClientMessageEvent *event) {
    static Atom protocols = None;
    static Atom delete_window = None;
    if (protocols == None) {
        protocols = XInternAtom(display, "WM_PROTOCOLS", True);
        delete_window = XInternAtom(display, "WM_DELETE_WINDOW", True);
        assert(protocols != None);
        assert(delete_window != None);
    }
    if (event->message_type == protocols) {
        Atom protocol = event->data.l[0];
        if (protocol == delete_window) {
            g_window_should_close = true;
        }
    }
}

void handle_input(Os_context *os_context) {
    for (int i = XPending(os_context->display); i > 0;  --i) {
        XEvent event;
        XNextEvent(os_context->display, &event);
        
        if (event.type == ClientMessage) {
            handle_client_event(os_context->display, &event.xclient);
        } 
        else if (event.type == KeyPress) {
            Key key = handle_key_event(os_context, event.xkey.keycode);
            if (key == key_escape) {
                g_window_should_close = true;
            }
            if (key == key_a) {
                move_camera(get_v3(-0.1, 0, 0));
            }
            else if (key == key_d) {
                move_camera(get_v3(0.1, 0, 0));
            }
            else if (key == key_w) {
                move_camera(get_v3(0, 0.1, 0));
            }
            else if (key == key_s) {
                move_camera(get_v3(0, -0.1, 0));
            }
            else if (key == key_r) {
                move_camera(get_v3(0, 0, 0.1));
            }
            else if (key == key_f) {
                move_camera(get_v3(0, 0, -0.1));
            }
            else if (key == key_unknown) {
                //do nothing
            }
        }
        else if (event.type == KeyRelease) {
            handle_key_event(os_context, event.xkey.keycode);
        }
        else if (event.type == ButtonPress) {
            Mouse_button mouse_button = handle_mouse_button_event(os_context, event.xbutton.button, true);
        } 
        else if (event.type == ButtonRelease) {
            Mouse_button mouse_button = handle_mouse_button_event(os_context, event.xbutton.button, false);
        }
    }
    
    os_context->input.mouse_pos = get_mouse_position(os_context);
    
    XFlush(os_context->display); //NOTE: no idea what this does
}

static double get_time() {
    struct timespec time_spec;
    clock_gettime(CLOCK_MONOTONIC, &time_spec);
    
    return (double)time_spec.tv_sec + (double)time_spec.tv_nsec / 1e9;
}

FILE *g_output_file;

typedef struct Renderer_settings {
    //debug stuff
    bool should_process_vertices;
    bool run_once;
    enum vertices_source { vertices_source_model, vertices_source_simple_image } vertices_source;
} Renderer_settings;

typedef struct File {
    const char *name;
    time_t modification_time;
} File;

bool check_and_change_modification_time_of_file(File *file) {
    struct stat new_modification_time;
    int error = stat(file->name, &new_modification_time);
    if (error != 0) { 
        //error
    }
    
    if (new_modification_time.st_mtime > file->modification_time) {
        file->modification_time = new_modification_time.st_mtime;
        return true;
    }
    
    return false;
}

void clear_framebuffer(Framebuffer *framebuffer) {
    framebuffer_clear_color(framebuffer, g_default_framebuffer_color);
    framebuffer_clear_depth(framebuffer, g_default_framebuffer_depth);
}

typedef struct Render_result {
    Framebuffer framebuffer;
} Render_result;


typedef enum Command { draw_rect_command } Command;
typedef enum Render_space { world_space, screen_space } Render_space;

typedef struct Draw_rect_command_data {
    Rect rect;
    enum { color_rect_type, texture_rect_type } rect_type;
    union {
        Texture *texture;
        v4 color;
    };
} Draw_rect_command_data;

typedef struct Command_buffer {
    Command *commands;
    int num_commands;
    int max_num_commands;
    void *data;
    void *data_pointer;
    size_t max_data_size;
} Command_buffer;

Command_buffer create_command_buffer() {
    Command_buffer command_buffer = {0};
    command_buffer.max_num_commands = 100;
    command_buffer.max_data_size = convert_gibibytes_to_bytes(0.3);
    command_buffer.commands = (Command *)allocate_perm(g_allocator, sizeof(Command) * command_buffer.max_num_commands);
    command_buffer.data = (void *)allocate_perm(g_allocator, command_buffer.max_data_size);
    
    return command_buffer;
}

void push_command(Command_buffer *command_buffer, Command command, void *data, size_t data_size) {
    assert(command_buffer->num_commands + 1 <= command_buffer->max_num_commands);
    assert(command_buffer->max_data_size + data_size <= command_buffer->max_data_size);
    command_buffer->commands[command_buffer->num_commands++] = command;
    memcpy(command_buffer->data_pointer, data, data_size);
    command_buffer->data_pointer+=data_size;
}

typedef struct Color_vertex_buffer {
    int num;
    int max_num_vertices;
    Color_vertex *vertices;
} Color_vertex_buffer;

typedef struct Texture_vertex_buffer {
    int num;
    int max_num_vertices;
    Texture_vertex *vertices;
} Texture_vertex_buffer;

void triangulate_rect(Rect rect, v3 *positions) {
    positions[0] = get_v3(rect.x_max, rect.y_min, 0);
    positions[1] = get_v3(rect.x_min, rect.y_min, 0);
    positions[2] = get_v3(rect.x_min, rect.y_max, 0);
    positions[3] = get_v3(rect.x_min, rect.y_max, 0);
    positions[4] = get_v3(rect.x_max, rect.y_max, 0);
    positions[5] = get_v3(rect.x_max, rect.y_min, 0);
}

Color_vertex create_color_vertex(v3 pos, v4 color) {
    Color_vertex vertex = {
        .pos = pos,
        .color = color
    };
    
    return vertex;
}

Texture_vertex create_texture_vertex(v3 pos, v2 uv) {
    Texture_vertex vertex = {
        .pos = pos,
        .uv = uv
    };
    
    return vertex;
}

void execute_commands(Command_buffer *command_buffer, Color_vertex_buffer *color_vertex_buffer,
                      Texture_vertex_buffer *texture_vertex_buffer) {
    //NOTE: this function will rape the branch predictor (I belive)
    for (int i = 0; i < command_buffer->num_commands; ++i) {
        Command command = command_buffer->commands[i];
        void *current_command_data = command_buffer->data;
        switch(command) {
            case draw_rect_command: {
                Draw_rect_command_data *data = (Draw_rect_command_data *)current_command_data;
                current_command_data += sizeof(Draw_rect_command_data);
                v3 positions[6];
                triangulate_rect(data->rect, positions);
                if (data->rect_type == color_rect_type) {
                    color_vertex_buffer->vertices[color_vertex_buffer->num++] = create_color_vertex(positions[0], data->color);
                    color_vertex_buffer->vertices[color_vertex_buffer->num++] = create_color_vertex(positions[1], data->color);
                    color_vertex_buffer->vertices[color_vertex_buffer->num++] = create_color_vertex(positions[2], data->color);
                    color_vertex_buffer->vertices[color_vertex_buffer->num++] = create_color_vertex(positions[3], data->color);
                    color_vertex_buffer->vertices[color_vertex_buffer->num++] = create_color_vertex(positions[4], data->color);
                    color_vertex_buffer->vertices[color_vertex_buffer->num++] = create_color_vertex(positions[5], data->color);
                }
                else if (data->rect_type == texture_rect_type) {
                    texture_vertex_buffer->vertices[texture_vertex_buffer->num++] = create_texture_vertex(positions[0], get_v2(1, 0));
                    texture_vertex_buffer->vertices[texture_vertex_buffer->num++] = create_texture_vertex(positions[1], get_v2(0, 0));
                    texture_vertex_buffer->vertices[texture_vertex_buffer->num++] = create_texture_vertex(positions[2], get_v2(0, 1));
                    texture_vertex_buffer->vertices[texture_vertex_buffer->num++] = create_texture_vertex(positions[3], get_v2(0, 1));
                    texture_vertex_buffer->vertices[texture_vertex_buffer->num++] = create_texture_vertex(positions[4], get_v2(1, 1));
                    texture_vertex_buffer->vertices[texture_vertex_buffer->num++] = create_texture_vertex(positions[5], get_v2(1, 0));
                }
                break;
            }
            default: {
                //none
                break;
            }
        }
    }
}

#include "assets.c"
#include "ui.c" //TODO: move this up when all the dependencies on this file get worked out

Render_result render(const Vertex_buffer *vertex_buffer, const Renderer_settings *renderer_settings, Allocator *allocator, Worker *main_worker, Framebuffer *framebuffer, Scene *scene, Texture *texture) {
    Pipeline_data pipeline_data = get_pipeline_data(vertex_buffer);
    
    if (renderer_settings->should_process_vertices) {
        process_vertices(main_worker, allocator, &pipeline_data, scene, framebuffer->width, framebuffer->height);
    }
    
    rasterize(main_worker, &pipeline_data, texture, framebuffer);
    
    Render_result render_result = {0};
    
    return render_result;
    
}

int main() {
    g_default_framebuffer_color = get_v4(0.9f, 0.9f, 0.9f, 1.0f);
    g_default_framebuffer_depth = 1.0f; 
    
    Buffer_with_name loaded_vertex_buffer = {0};
    //parse_test_vertices_file("test_vertices.pav", &loaded_vertex_buffer);
    test_parser();
    File vertices_file = {.name = "test_vertices.pav", .modification_time = 0};
    
    g_output_file = fopen("output_file", "w");
    
    const char *const WINDOW_TITLE = "Viewer";
    //TODO: assert we arent drawing beyond the screen size
#if SAM_HYDE
    const int SCREEN_WIDTH = 512;
    const int SCREEN_HEIGHT = 512; 
#elif SMALL_SQUARE
    const int SCREEN_WIDTH = 16;
    const int SCREEN_HEIGHT = 16; 
#endif
    
    //allocate memory
    Allocator *allocator = create_allocator(1, 1);
    
    Display *display = XOpenDisplay(NULL);
    assert(display != NULL);
    //XContext context = XUniqueContext();
    //initialize path?
    
    //create_scene()
    
    //test_ enter_ mainloop()
    //window_t *window;
    
    //create window
    int screen = XDefaultScreen(display);
    unsigned long border = XWhitePixel(display, screen);
    unsigned long background = XBlackPixel(display, screen);
    Window root_window = XRootWindow(display, screen);
    
    Window window_handle = XCreateSimpleWindow(display, root_window, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0,
                                               border, background);
    
    /* not resizable */
    XSizeHints *size_hints = XAllocSizeHints();
    size_hints->flags = PMinSize | PMaxSize;
    size_hints->min_width = SCREEN_WIDTH;
    size_hints->max_width = SCREEN_WIDTH;
    size_hints->min_height = SCREEN_HEIGHT;
    size_hints->max_height = SCREEN_HEIGHT;
    XSetWMNormalHints(display, window_handle, size_hints);
    XFree(size_hints);
    
    /* application name */
    XClassHint *class_hint = XAllocClassHint();
    class_hint->res_name = (char*)WINDOW_TITLE;
    class_hint->res_class = (char*)WINDOW_TITLE;
    XSetClassHint(display, window_handle, class_hint);
    XFree(class_hint);
    
    /* event subscription */
    long mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask;
    XSelectInput(display, window_handle, mask);
    Atom delete_window = XInternAtom(display, "WM_DELETE_WINDOW", True);
    XSetWMProtocols(display, window_handle, &delete_window, 1);
    
    //create surface
    int depth = XDefaultDepth(display, screen);
    Visual *visual = XDefaultVisual(display, screen);
    
    assert(depth == 24 || depth == 32);
    Image surface = create_image(allocator, SCREEN_WIDTH, SCREEN_HEIGHT, 4);
    XImage *ximage = XCreateImage(display, visual, depth, ZPixmap, 0,
                                  (char*)surface.buffer, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0);
    
    //XSaveContext(display, window_handle, context, (XPointer)window);
    XMapWindow(display, window_handle);
    XFlush(display);
    
    //camera create
    //more stuff
    
    //release scene
    Scene scene = create_scene(SCREEN_WIDTH, SCREEN_HEIGHT);
    g_camera = &scene.camera;
    Framebuffer framebuffer = create_framebuffer(allocator, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    double lowest_fps = 1000.0f;
    double highest_fps = 0.0f;
    
    double first_time = get_time();
    double delta_time  = 0;
    double second_time = first_time;
    
#if SAM_HYDE
    Texture texture = load_texture(allocator, "index.jpeg");
#elif SMALL_SQUARE
    Texture texture = load_texture(allocator, "Untitled.png");
#endif
    
    float average_fps = 0;
    u32 average_fps_counter = 0;
    
    Worker_group worker_group;
    Worker *main_worker;
    init_multithreading(allocator, true, &worker_group, &main_worker);
    
    Model model = load_model_from_obj();
    Vertex_buffer model_vertex_buffer = {
        .num_vertices = model.num_vertices,
        .vertices = model.vertices
    };
    
    Renderer_settings renderer_settings = {
        .should_process_vertices = false,
        .run_once = false
    };
    
    u32 frame_number = 0;
    
    Texture model_texture = load_texture(allocator, "viking_room.png");
    
#if 0
    Vertex_buffer cube_vertex_buffer = convert_static_positions_and_uvs_to_vertices(g_cube, array_count(g_cube) / 5);
    
    change_object_size(cube_vertex_buffer.vertices, cube_vertex_buffer.num_vertices, 1);
#endif
    
    Vertex_buffer cube_face_vertex_buffer = convert_static_positions_and_uvs_to_vertices(g_cube_face_screen_space, array_count(g_cube_face_screen_space) / 5);
    
    Os_context os_context = {0};
    os_context.display = display;
    os_context.window_handle = window_handle;
    
    Vertex_buffer button_vertex_buffer = convert_static_positions_and_uvs_to_vertices(g_button_vertices, array_count(g_button_vertices) / 5);
    
    Command_buffer command_buffer = create_command_buffer();
    
    Ui_context ui_context = {0};
    ui_context.input = &os_context.input;
    ui_context.command_buffer = &command_buffer;
    
    begin_frame_allocation(allocator);
    while(!g_window_should_close) {
        frame_number++;
        first_time = get_time();
        
        handle_input(&os_context);
        
        if (check_and_change_modification_time_of_file(&vertices_file)) {
            fprintf(stderr, "changed\n");
        }
        
        //execute_commands(command_buffer, texture_vertex_buffer);
        
        render(&cube_face_vertex_buffer, &renderer_settings, allocator, main_worker, &framebuffer, &scene, &texture);
        
        //printf("rasterizer: %lu\n", g_rasterizer_clock_cycles);
        
        //window draw buffer
        blit_image(&framebuffer, &surface);
        clear_framebuffer(&framebuffer);
        
        //int screen = XDefaultScreen(display);
        GC gc = XDefaultGC(display, screen);
        XPutImage(display, window_handle, gc, ximage,
                  0, 0, 0, 0, surface.width, surface.height);
        XFlush(display);
        
        
        second_time = get_time();
        delta_time = second_time - first_time;
        //printf("%f\n", 1 / delta_time);
        if (1 /delta_time >= highest_fps) {
            highest_fps = 1 / delta_time;
        }
        if (1 /delta_time <= lowest_fps) {
            lowest_fps = 1 /delta_time;
        }
        
        if (average_fps_counter <= 1000) {
            average_fps += delta_time;
            average_fps_counter++;
        }
        
        reset_frame_memory(allocator);
        
        if (renderer_settings.run_once)
            break;
    }
    
    deinit_multithreading(&worker_group);
    
    //printf("median: %f\n", 1 / (median_fps / (double)max_frames));
    printf("lowest fps: %f, highest_fps: %f, average fps: %f\n", lowest_fps, highest_fps, 1.0f / (average_fps / average_fps_counter));
    
    assert(ximage);
    XDestroyImage(ximage);
    assert(display != NULL);
    XCloseDisplay(display);
    display = NULL;
    
    free_allocator(allocator);
    
    
    return 0;
}
