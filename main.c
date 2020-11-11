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
#include "Parser.c"

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
#include "assets.c"
#include "global_data.c"

#define SAM_HYDE 1
#define SAM_HYDE_TOP_CORNER 0
#define SMALL_SQUARE 0

bool g_window_should_close = false;

typedef struct Image{
    u32 width, height, channels;
    u8 *buffer;
} Image;

typedef struct { 
    Window handle;
    XImage *ximage;
    Image *surface;
    
    /* common data */
#if 0
    int should_close;
    char keys[KEY_NUM];
    char buttons[BUTTON_NUM];
    callbacks_t callbacks;
    void *userdata;
#endif
} window;

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

static void image_release(Image *image) {
    free(image->buffer);
    free(image);
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

#if 0
static void handle_key_event(window_t *window, int virtual_key, char pressed) {
    KeySym *keysyms;
    KeySym keysym;
    keycode_t key;
    int dummy;
    
    keysyms = XGetKeyboardMapping(display, virtual_key, 1, &dummy);
    keysym = keysyms[0];
    XFree(keysyms);
    
    switch (keysym) {
        case XK_a:     key = KEY_A;     break;
        case XK_d:     key = KEY_D;     break;
        case XK_s:     key = KEY_S;     break;
        case XK_w:     key = KEY_W;     break;
        case XK_space: key = KEY_SPACE; break;
        default:       key = KEY_NUM;   break;
    }
    
    if (key < KEY_NUM) {
        window->keys[key] = pressed;
        if (window->callbacks.key_callback) {
            window->callbacks.key_callback(window, key, pressed);
        }
    }
}

static void handle_button_event(window_t *window, int xbutton, char pressed) {
    if (xbutton == Button1 || xbutton == Button3) {         /* mouse button */
        button_t button = xbutton == Button1 ? BUTTON_L : BUTTON_R;
        window->buttons[button] = pressed;
        if (window->callbacks.button_callback) {
            window->callbacks.button_callback(window, button, pressed);
        }
    } else if (xbutton == Button4 || xbutton == Button5) {  /* mouse wheel */
        if (window->callbacks.scroll_callback) {
            float offset = xbutton == Button4 ? 1 : -1;
            window->callbacks.scroll_callback(window, offset);
        }
    }
}
#endif

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

void process_event(Display *display, XEvent *event) {
    //Window handle;
    //window_t *window;
    //int error;
    
    //handle = event->xany.window;
    //error = XFindContext(display, handle, g_context, (XPointer*)&window);
    /*
    if (error != 0) {
        return;
    }
    */
    
    if (event->type == ClientMessage) {
        handle_client_event(display, &event->xclient);
    } 
#if 0
    else if (event->type == KeyPress) {
        handle_key_event(window, event->xkey.keycode, 1);
    } else if (event->type == KeyRelease) {
        handle_key_event(window, event->xkey.keycode, 0);
    } else if (event->type == ButtonPress) {
        handle_button_event(window, event->xbutton.button, 1);
    } else if (event->type == ButtonRelease) {
        handle_button_event(window, event->xbutton.button, 0);
    }
#endif
}


void input_poll_events(Display *display) {
    int count = XPending(display);
    while (count > 0) {
        XEvent event;
        XNextEvent(display, &event);
        process_event(display, &event);
        count -= 1;
    }
    XFlush(display);
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

int main() {
    
    //Buffer test_vertex_buffer = parse_vertices_file("test_vertices.pav");
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
    
    Vertex_buffer cube_vertex_buffer = convert_static_positions_and_uvs_to_vertices(g_cube_face, array_count(g_cube_face) / 5);
    
    change_object_size(cube_vertex_buffer.vertices, cube_vertex_buffer.num_vertices, 1);
    
#if 0
    const int stride = 8;
    //v4 vertex_positions[6];
    //v4 vertex_colors[6];
    v4 *vertex_positions = (v4 *)allocate_frame(allocator, sizeof(v4) * vertices_num);
    v4 *vertex_colors = (v4 *)allocate_frame(allocator, sizeof(v4) * vertices_num);
    v2 *vertex_uvs = (v2 *)allocate_frame(allocator, sizeof(v2) * vertices_num);
    
    u32 position_index = 0;
    u32 color_index = 3;
    u32 uv_index = 6;
    
    for (int i = 0; i  < vertices_num; ++i) {
        vertex_positions[i] = 
            get_v4(input_vertices[position_index], input_vertices[position_index + 1], input_vertices[position_index + 2], 1.0f),
        
        vertex_colors[i] = get_v4(input_vertices[color_index], input_vertices[color_index + 1], input_vertices[color_index + 2], 1.0f);
        
        vertex_uvs[i] = get_v2(input_vertices[uv_index], input_vertices[uv_index + 1]);
        
        position_index += stride;
        color_index += stride;
        uv_index += stride;
    }
#endif
    
    float float_temp_vertices[] = {
#if 1
        255, 0, 50,    1, 0,
        0, 0, 50,      0, 0,
        0, 255, 50,    0, 1,
#endif
#if 0
        10, 255, 15,    0, 1,
        255, 255, 15,  1, 1,
        255, 10, 15,    1, 0
#endif
    };
    
    Vertex_buffer temp_vertex_buffer = convert_static_positions_and_uvs_to_vertices(float_temp_vertices, array_count(float_temp_vertices) / 5);
    
    Renderer_settings renderer_settings = {
        .should_process_vertices = true,
        .run_once = false
    };
    
    u32 frame_number = 0;
    
    begin_frame_allocation(allocator);
    
    while(!g_window_should_close) {
        frame_number++;
        first_time = get_time();
        
        if (check_and_change_modification_time_of_file(&vertices_file)) {
            fprintf(stderr, "changed\n");
        }
        
        Pipeline_data pipeline_data = get_pipeline_data(&cube_vertex_buffer);
        
        if (renderer_settings.should_process_vertices) {
            process_vertices(main_worker, allocator, &pipeline_data, &scene, framebuffer.width, framebuffer.height);
        }
        
        rasterize(main_worker, &pipeline_data, &texture, &framebuffer);
        
        //printf("rasterizer: %lu\n", g_rasterizer_clock_cycles);
        
        //window draw buffer
        blit_image(&framebuffer, &surface);
        
        //int screen = XDefaultScreen(display);
        GC gc = XDefaultGC(display, screen);
        XPutImage(display, window_handle, gc, ximage,
                  0, 0, 0, 0, surface.width, surface.height);
        XFlush(display);
        
        //more stuff
        
        //input poll events
        input_poll_events(display);
        //g_window_should_close = true;
        
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
