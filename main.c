//TODO_LIST:
//-hierarchical rasterizer
//-fix multithreading
//-AA
//-multithreading memory allocation
//-make sure every thread has its own cahceline aligned memory

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

size_t convert_gibibytes_to_bytes(f32 size) {
    return (size_t)(size * (f32)pow(1024, 3));
}

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

int main() {
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
    size_t perm_memory_size =  convert_gibibytes_to_bytes(0.1);
    size_t frame_memory_size =  convert_gibibytes_to_bytes(0.1);
    u8 *memory = malloc(perm_memory_size + frame_memory_size);
    assert(memory);
    Allocator allocator = create_allocator(memory, perm_memory_size, frame_memory_size);
    g_allocator = &allocator;
    
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
    Image surface = create_image(&allocator, SCREEN_WIDTH, SCREEN_HEIGHT, 4);
    XImage *ximage = XCreateImage(display, visual, depth, ZPixmap, 0,
                                  (char*)surface.buffer, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0);
    
    //XSaveContext(display, window_handle, context, (XPointer)window);
    XMapWindow(display, window_handle);
    XFlush(display);
    
    //camera create
    //more stuff
    
    //release scene
    Scene scene = create_scene(SCREEN_WIDTH, SCREEN_HEIGHT);
    Framebuffer framebuffer = create_framebuffer(&allocator, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    double lowest_fps = 1000.0f;
    double highest_fps = 0.0f;
    
    double first_time = get_time();
    double delta_time  = 0;
    double second_time = first_time;
    
#if SAM_HYDE
    Texture texture = load_texture(&allocator, "index.jpeg");
#elif SMALL_SQUARE
    Texture texture = load_texture(&allocator, "Untitled.png");
#endif
    
    float average_fps = 0;
    u32 average_fps_counter = 0;
    
    Worker_group worker_group;
    Worker *main_worker;
    bool run_single_threaded = true;
    init_multithreading(&allocator, run_single_threaded, &worker_group, &main_worker);
    
    bool should_process_vertices = true;
    int num_vertices = 3;
    
    Model model = load_model_from_obj();
    Pipeline_input pipeline_input = get_pipeline_input(model.num_vertices, model.vertices);
    
    
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
        511, 0, 15,    1, 0,
        0, 0, 15,      0, 0,
        0, 511, 15,    0, 1,
        
        0, 511, 15,    0, 1,
        511, 511, 15,  1, 1,
        511, 0, 15,    1, 0
    };
    Vertex *temp_vertices = convert_positions_and_uvs_to_vertices(float_temp_vertices, 6);
    Pipeline_input temp_pipeline_input  = get_pipeline_input(6 , temp_vertices);
    
    while(!g_window_should_close) {
        //for (int i = 0; i < 1; ++i) {
        first_time = get_time();
        
        //stuff
        
        //tickfunc
#if 0
        Vertex_soa vertex_soa = {0};
        vertex_soa.vertex_array = (v4 *)allocate_frame(&allocator, num_vertices * sizeof(v4));
        vertex_soa.vertex_array[0] = (v4){511, 0, 15, 1};
        vertex_soa.vertex_array[1] = (v4){0, 0, 15, 1};
        vertex_soa.vertex_array[2] = (v4){0, 511, 15, 1};
        vertex_soa.vertex_array[3] = (v4){0, 511, 15, 1};
        vertex_soa.vertex_array[4] = (v4){511, 511, 15, 1};
        vertex_soa.vertex_array[5] = (v4){511, 0, 15, 1};
        
        vertex_soa.uv_array = (v2 *)allocate_frame(&allocator, num_vertices * sizeof(v2));
        vertex_soa.uv_array[0] = (v2){1 ,0};
        vertex_soa.uv_array[1] = (v2){0 ,0};
        vertex_soa.uv_array[2] = (v2){0, 1};
        vertex_soa.uv_array[3] = (v2){0 ,1};
        vertex_soa.uv_array[4] = (v2){1 ,1};
        vertex_soa.uv_array[5] = (v2){1, 0};
        
        vertex_soa.num_vertices = num_vertices;
#endif
        
        if (should_process_vertices) {
            
            //process_vertices(main_worker, &allocator, &pipeline_input, &scene, framebuffer.width, framebuffer.height);
        }
        
        rasterize(main_worker, &temp_pipeline_input, &texture, &framebuffer);
        
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
        
        reset_frame_memory(&allocator);
    }
    
    deinit_multithreading(&worker_group);
    
    //printf("median: %f\n", 1 / (median_fps / (double)max_frames));
    printf("lowest fps: %f, highest_fps: %f, average fps: %f\n", lowest_fps, highest_fps, 1.0f / (average_fps / average_fps_counter));
    assert(display != NULL);
    XCloseDisplay(display);
    display = NULL;
    
    return 0;
}
