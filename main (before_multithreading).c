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

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#pragma GCC diagnostic pop

#include "math.c"
#include "renderer.c"

#define SAM_HYDE 1
#define SAM_HYDE_TOP_CORNER 0
#define SMALL_SQUARE 0

#define align(x) __attribute__ ((aligned(x))) //use stdalign.h?

#define array_count(x) (sizeof(x) / sizeof(x[0]))

#define EPSILON 1e-5f
#define PI 3.1415927f

#define TO_RADIANS(degrees) ((PI / 180) * (degrees))
#define TO_DEGREES(radians) ((180 / PI) * (radians))

const float NEAR = 0.1f;
const float FAR = 10000;
const float FOVY = TO_RADIANS(60);
//const v3 UP = {0, 1, 0};

bool g_window_should_close = false;

u64 g_rasterizer_clock_cycles = 0;

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

typedef struct Framebuffer{
    u32 width, height;
    unsigned char *color_buffer;
    float *depth_buffer;
} Framebuffer;

typedef struct Joint {
    int joint_index;
    int parent_index;
    m4 inverse_bind;
    //translations
    u32 num_translations;
    f32 *translation_times;
    v3 *translation_values;
    //rotations
    int num_rotations;
    float *rotation_times;
    v4 *rotation_values;
    //scales
    u32 num_scales;
    f32 *scale_times;
    v3 *scale_values;
    //interpolated
    m4 transform;
} Joint;

typedef struct Skeleton {
    float min_time;
    f32 max_time;
    u32 num_joints;
    Joint *joints;
    //cached result
    m4 *joint_matrices;
    //m3 *normal_matrices;
    f32 last_time;
} Skeleton;

typedef struct Texture {
    u32 width, height, channels;
    u8 *data;
} Texture;



typedef struct Allocator {
    u8 *perm_memory_base;
    size_t perm_memory_used;
    size_t perm_memory_size;
    
    u8 *frame_memory_base;
    size_t frame_memory_used;
    size_t frame_memory_size;
    
} Allocator;

Allocator create_allocator(u8 *memory_buffer, size_t perm_memory_size, size_t frame_memory_size) {
    Allocator allocator = {0};
    allocator.perm_memory_size = perm_memory_size;
    allocator.frame_memory_size = frame_memory_size;
    allocator.perm_memory_base = memory_buffer;
    allocator.frame_memory_base = memory_buffer + perm_memory_size;
    
    return allocator;
}

u8 *allocate_perm_aligned(Allocator *allocator, size_t allocation_size, u32 alignment) {
    uintptr_t alignment_mask = alignment - 1;
    u8 *memory = allocator->perm_memory_base + allocator->perm_memory_used;
    uintptr_t offset = 0;
    
    if ((uintptr_t)memory & alignment_mask) {
        offset = (uintptr_t)memory - ((uintptr_t)memory & alignment_mask);
    }
    
    allocation_size += offset;
    
    assert(allocation_size + allocator->perm_memory_used <= allocator->perm_memory_size);
    memory += offset;
    allocator->perm_memory_used += allocation_size;
    return memory;
}

u8 *allocate_frame_aligned(Allocator *allocator, size_t allocation_size, u32 alignment) {
    uintptr_t alignment_mask = alignment - 1;
    u8 *memory = allocator->frame_memory_base + allocator->frame_memory_used;
    uintptr_t offset = 0;
    
    if ((uintptr_t)memory & alignment_mask) {
        offset = (uintptr_t)memory - ((uintptr_t)memory & alignment_mask);
    }
    
    allocation_size += offset;
    
    assert(allocation_size + allocator->frame_memory_used <= allocator->frame_memory_size);
    memory += offset;
    allocator->frame_memory_used += allocation_size;
    return memory;
}

u8 *allocate_perm(Allocator *allocator, size_t allocation_size) {
    u32 alignment = 4;
    uintptr_t alignment_mask = alignment - 1;
    u8 *memory = allocator->perm_memory_base + allocator->perm_memory_used;
    uintptr_t offset = 0;
    
    if ((uintptr_t)memory & alignment_mask) {
        offset = (uintptr_t)memory - ((uintptr_t)memory & alignment_mask);
    }
    
    allocation_size += offset;
    
    assert(allocation_size + allocator->perm_memory_used <= allocator->perm_memory_size);
    memory += offset;
    allocator->perm_memory_used += allocation_size;
    return memory;
}

u8 *allocate_frame(Allocator *allocator, size_t allocation_size) {
    u32 alignment = 4;
    uintptr_t alignment_mask = alignment - 1;
    u8 *memory = allocator->frame_memory_base + allocator->frame_memory_used;
    uintptr_t offset = 0;
    
    if ((uintptr_t)memory & alignment_mask) {
        offset = (uintptr_t)memory - ((uintptr_t)memory & alignment_mask);
    }
    
    allocation_size += offset;
    
    assert(allocation_size + allocator->frame_memory_used <= allocator->frame_memory_size);
    memory += offset;
    allocator->frame_memory_used += allocation_size;
    return memory;
}

void reset_frame_memory(Allocator *allocator) {
    allocator->frame_memory_used = 0;
}

size_t convert_gibibytes_to_bytes(f32 size) {
    return (size_t)(size * (f32)pow(1024, 3));
}

static Texture load_texture(Allocator *allocator, const char *path) {
    //TODO: move this into a full formed asset system, with the custom allocator, and consider writing it yourself if necessary
    s32 width, height, channels;
    u8 *data = stbi_load(path, &width, &height, &channels, 0);
    if (!data) {
        printf("failed to load image\n");
        exit(1);
    }
    size_t image_size = width * height * channels;
    u8 *custom_allocator_data = allocate_perm(allocator, image_size);
    memcpy(custom_allocator_data, data, image_size);
    stbi_image_free(data);
    
    return (Texture){.width = (u32)width, .height = (u32)height, .channels = (u32)channels, .data = custom_allocator_data};
}

#if 0
void load_gltf() {
    cgltf_options options = {0};
    cgltf_data *data = 0;
    cgltf_result result = cgltf_parse_file(&options, "scene.gltf", &data);
    if (result != cgltf_result_success) {
        printf("faild to load gltf\n");
        exit(1);
    }
    
    for (int i = 0; i < (int
                         )data->images_count; ++i) {
        load_texture(data->images[i].uri);
    }
}

void load_animation() {
    Skeleton *skeleton;
}
#endif

static unsigned char float_to_uchar(float value) {
    return (unsigned char)(value * 255);
}

static void framebuffer_release(Framebuffer *framebuffer) {
    free(framebuffer->color_buffer);
    free(framebuffer->depth_buffer);
    free(framebuffer);
}

static void framebuffer_clear_color(Framebuffer *framebuffer, v4 color) {
    u32  num_pixels = framebuffer->width * framebuffer->height;
    for (u32 i = 0; i < num_pixels; i++) {
        framebuffer->color_buffer[i * 4 + 0] = float_to_uchar(color.x);
        framebuffer->color_buffer[i * 4 + 1] = float_to_uchar(color.y);
        framebuffer->color_buffer[i * 4 + 2] = float_to_uchar(color.z);
        framebuffer->color_buffer[i * 4 + 3] = float_to_uchar(color.w);
    }
}

static void framebuffer_clear_depth(Framebuffer *framebuffer, float depth) {
    u32 num_pixels = framebuffer->width * framebuffer->height;
    for (u32 i = 0; i < num_pixels; i++) {
        framebuffer->depth_buffer[i] = depth;
    }
}

static Framebuffer create_framebuffer(Allocator *allocator, u32 width, u32 height) {
    u32 color_buffer_size = width * height * 4 * sizeof(unsigned char);
    u32 depth_buffer_size = (u32)sizeof(float) * width * height;
    v4 default_color = get_v4(0.9f, 0.9f, 0.9f, 1.0f);
    float default_depth = 1.0f;
    Framebuffer framebuffer = {0};
    
    assert(width > 0 && height > 0);
    
    framebuffer.width = width;
    framebuffer.height = height;
    //framebuffer.color_buffer = (unsigned char*)aligned_alloc(16, color_buffer_size);
    framebuffer.color_buffer = allocate_perm_aligned(allocator, color_buffer_size, 16);
    framebuffer.depth_buffer = (float *)allocate_perm_aligned(allocator, depth_buffer_size, 16);
    
    //TODO: memcpy
    framebuffer_clear_color(&framebuffer, default_color);
    framebuffer_clear_depth(&framebuffer, default_depth);
    
    return framebuffer;
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

m4 get_m4_look_at(v3 eye, v3 target, v3 up) {
    v3 z_axis = normalize_v3(sub_v3(eye, target)); //forward
    v3 x_axis = normalize_v3(cross_v3(up, z_axis)); //left
    v3 y_axis = cross_v3(z_axis, x_axis); //up
    m4 m = {
        x_axis.x, y_axis.x, z_axis.x, 0 , 
        x_axis.y, y_axis.y, z_axis.y, 0 ,
        x_axis.z, y_axis.z, z_axis.z, 1 ,
        0, 0, 0, 1 
    };
    
    return m;
}

static m4 get_m4_perspective(float fovy, float aspect, float near, float far) {
    float z_range = far - near;
    m4 m;
    assert(fovy > 0 && aspect > 0);
    assert(near > 0 && far > 0 && z_range > 0);
    float elements[4][4] = {
        { 1 / (aspect * tan(fovy / 2)), 0, 0, 0 },
        { 0, 1 / tan(fovy / 2), 0, 0 },
        { 0, 0, -1 * (far + near) / (far - near),  -2 * (far * near) / (far - near)} ,
        { 0, 0, -1, 0 }
    };
    
    memcpy(m.e, elements,  4 * 4 * sizeof(float));
    
    return m;
    
}

typedef struct Camera {
    v3 pos, target;
    float aspect;
} Camera;

static Camera create_camera(v3 pos, v3 target, float aspect) {
    assert(get_v3_length(sub_v3(pos, target)) > EPSILON && aspect);
    
    return (Camera){
        .pos = pos,
        .target = target,
        .aspect = aspect
    };
}

static double get_time() {
    struct timespec time_spec;
    clock_gettime(CLOCK_MONOTONIC, &time_spec);
    
    return (double)time_spec.tv_sec + (double)time_spec.tv_nsec / 1e9;
}

#if 0
typedef struct __attribute__((packed)) Vertex  {}
#else 
typedef struct  Vertex  {
#endif
    v3 pos;
    v3 color;
    //v3 texture;
} Vertex;

typedef enum Outcode {outcode_inside = 0, outcode_left = 1, outcode_right = 2, outcode_bottom = 4, outcode_top = 8} Outcode;

Outcode compute_out_code(v4 v) {
#if 0
    if (v.w >= EPSILON) {
        return 
    }
#endif
    if (v.x <= -v.w) {
        return outcode_left;
    }
    else if (v.y >= v.w) {
        return outcode_right;
    }
    else if (v.z <= -v.w) {
        return outcode_bottom;
    }
    else if (v.w <= v.w) {
        return outcode_top;
    }
    
    return outcode_inside;
}

typedef struct Scene {
    Camera camera;
} Scene;

static Scene create_scene(u32 width, u32 height) {
    Scene scene = {0};
    scene.camera = create_camera(get_v3(0.0f, 0.0f, 1.5f), get_v3(0, 0, 0), (float)width / height);
    
    return scene;
}

FILE *g_output_file;

typedef struct Rasterization_data {
#if 0
    u32 *x_array;
    u32 *y_array;
    u32 *z_array;
    u32 *u_array;
    u32 *v_array;
#else
    u32 num_vertices;
    v4 *vertex_array;
    v2 *uv_array;
#endif
} Rasterization_data;

static void process_vertices(Allocator *allocator, const Scene *scene, const float *input_vertices, u32 vertices_num,  Rasterization_data *out_rasterization_data, u32 viewport_width, u32 viewport_height) {
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
    
    //convert local coordinates to world coordinates
    
    //convert world coordinates to camera coordinates
    m4 view_matrix = get_m4_look_at(scene->camera.pos, scene->camera.target, get_v3(0.0f, 1.0f, 0.0f));
    for (int i = 0; i < vertices_num; ++i) {
        vertex_positions[i] = mul_m4_by_v4(view_matrix, vertex_positions[i]);
    }
    
    //convert camera coordinates to clip/projection coordinates
    m4 projection_matrix = get_m4_perspective(FOVY, scene->camera.aspect, NEAR, FAR);
    
    for (int i = 0; i < vertices_num; ++i) {
        vertex_positions[i] = mul_m4_by_v4(projection_matrix, vertex_positions[i]);
    }
    
    //clip
    //TODO: clip the near plane? guard band clipping?
    v4 *clipped_vertices = vertex_positions;
    u32 num_clipped_vertices = vertices_num;
    
    //convert clip coordiantes to  ndc / perspective division
    for (int i = 0; i < num_clipped_vertices; ++i) {
        v4 *vertex = clipped_vertices + i;
        vertex->x /= vertex->w;
        vertex->y /= vertex->w;
        //vertex->z /= vertex->w; //NOTE: not so sure about this...
    } 
    
    //backface culling... (under counstruction)
    
    //ndc to viewport/raster space
    for (int i = 0; i  < num_clipped_vertices; ++i) {
        v4 *vertex = clipped_vertices  + i;
        vertex->x = (vertex->x  + 1) * 0.5f * (float)viewport_width;
        vertex->y = viewport_height - (vertex->y  + 1) * 0.5f * (float)viewport_height;
        vertex->z = (vertex->z  + 1) * 0.5f;
    }
    
    
    //TODO: fix z coordinates !!!!
    //TODO: view frustrum culling
    
    out_rasterization_data->num_vertices = num_clipped_vertices;
    out_rasterization_data->vertex_array = vertex_positions;
    out_rasterization_data->uv_array = vertex_uvs;
}

static void rasterize(const Rasterization_data *rasterization_data, const Texture *texture, Framebuffer *out_framebuffer) {
    assert(rasterization_data->num_vertices != 0);
    assert(rasterization_data->vertex_array);
    assert(rasterization_data->uv_array);
    
    
    //bin triangles
    u32 bin_width = out_framebuffer->width / 4;
    u32 bin_height = out_framebuffer->height / 4;
    
    
    
    
    
    //sort triangles
    
    
    
    
    
    
    //rasterize bins
    u32 num_clipped_vertices = rasterization_data->num_vertices;
    v4 *clipped_vertices = rasterization_data->vertex_array;
    v2 *vertex_uvs = rasterization_data->uv_array;
    
    assert(texture->channels == 3);
    
    u32 temp_for_clocks;
    
    for (int vertex_index = 0; vertex_index < num_clipped_vertices; vertex_index += 3) {
        u64 current_clock_cycles =  __rdtscp(&temp_for_clocks);
        
        v2i vertex0 = get_v2i_from_v2(get_v2_from_v4(clipped_vertices[vertex_index + 2]));
        v2i vertex1 = get_v2i_from_v2(get_v2_from_v4(clipped_vertices[vertex_index + 1]));
        v2i vertex2 = get_v2i_from_v2(get_v2_from_v4(clipped_vertices[vertex_index + 0]));
        
        s32 start_x = fmin(fmin(vertex0.x, vertex1.x), vertex2.x);
        s32 end_x = fmax(fmax(vertex0.x, vertex1.x), vertex2.x);
        s32 start_y = fmin(fmin(vertex0.y, vertex1.y), vertex2.y);
        s32 end_y = fmax(fmax(vertex0.y, vertex1.y), vertex2.y);
        
        //TODO: some vertices may be clipped, change this code once clipping is done
        s32 A01 = vertex0.y - vertex1.y;
        s32 B01 = vertex1.x - vertex0.x;
        s32 C01 =  vertex0.x * vertex1.y - vertex0.y * vertex1.x;
        s32 F01 = A01 * start_x + B01 * start_y + C01;
        
        s32 A12 = vertex1.y - vertex2.y;
        s32 B12 = vertex2.x - vertex1.x;
        s32 C12 =  vertex1.x * vertex2.y - vertex1.y * vertex2.x;
        s32 F12 = A12 * start_x + B12 * start_y + C12;
        
        s32 A20 = vertex2.y - vertex0.y;
        s32 B20 = vertex0.x - vertex2.x;
        s32 C20 =  vertex2.x * vertex0.y - vertex2.y * vertex0.x;
        s32 F20 = A20 * start_x + B20 * start_y + C20;
        
#if 1
        u32 x_increment = 2;
        u32 y_increment = 2;
        
        m128i w0_row = set_m128i(F12, F12 + A12, F12 + B12, F12 + A12 + B12);
        m128i w1_row = set_m128i(F20, F20 + A20, F20 + B20, F20 + A20 + B20);
        m128i w2_row = set_m128i(F01, F01 + A01, F01 + B01, F01 + A01 + B01);
        
        m128i A0_increment = set_broadcast_m128i(A12 * 2);
        m128i A1_increment = set_broadcast_m128i(A20 * 2);
        m128i A2_increment = set_broadcast_m128i(A01 * 2);
        
        m128i B0_increment = set_broadcast_m128i(B12 * 2);
        m128i B1_increment = set_broadcast_m128i(B20 * 2);
        m128i B2_increment = set_broadcast_m128i(B01 * 2);
#else
        u32 x_increment = 4;
        u32 y_increment = 1;
        
        m128i w0_row = set_m128i(F12, F12 + A12 , F12 + 2 * A12, F12 + 3 *A12);
        m128i w1_row = set_m128i(F20, F20 + A20 , F20 + 2 * A20, F20 + 3 *A20);
        m128i w2_row = set_m128i(F01, F01 + A01 , F01 + 2 * A01, F01 + 3 *A01);
        
        m128i A0_increment = set_broadcast_m128i(A12 * 4);
        m128i A1_increment = set_broadcast_m128i(A20 * 4);
        m128i A2_increment = set_broadcast_m128i(A01 * 4);
        
        m128i B0_increment = set_broadcast_m128i(B12);
        m128i B1_increment = set_broadcast_m128i(B20);
        m128i B2_increment = set_broadcast_m128i(B01);
#endif
        
        //NOTE: do the z values in clipped_vertices look right to me?
        m128 z0 = set_broadcast_m128(clipped_vertices[vertex_index + 2].z);
        m128 z1 =  set_broadcast_m128(clipped_vertices[vertex_index + 1].z);
        m128 z2 =  set_broadcast_m128(clipped_vertices[vertex_index + 0].z);
        
        m128 z0_new = set_broadcast_m128(clipped_vertices[vertex_index + 2].z);
        m128 z1_new =  set_broadcast_m128(clipped_vertices[vertex_index + 1].z);
        m128 z2_new =  set_broadcast_m128(clipped_vertices[vertex_index + 0].z);
        
        m128 tex_u0 = set_broadcast_m128(vertex_uvs[vertex_index + 2].u);
        m128 tex_u1 = set_broadcast_m128(vertex_uvs[vertex_index + 1].u);
        m128 tex_u2 = set_broadcast_m128(vertex_uvs[vertex_index + 0].u);
        
        m128 tex_v0 = set_broadcast_m128(vertex_uvs[vertex_index + 2].v);
        m128 tex_v1 = set_broadcast_m128(vertex_uvs[vertex_index + 1].v);
        m128 tex_v2 = set_broadcast_m128(vertex_uvs[vertex_index + 0].v);
        
        tex_u0 = div_m128(tex_u0, set_broadcast_m128(clipped_vertices[vertex_index + 2].z));
        tex_u1 = div_m128(tex_u1, set_broadcast_m128(clipped_vertices[vertex_index + 1].z));
        tex_u2 = div_m128(tex_u2, set_broadcast_m128(clipped_vertices[vertex_index + 0].z));
        
        tex_v0 = div_m128(tex_v0, set_broadcast_m128(clipped_vertices[vertex_index + 2].z));
        tex_v1 = div_m128(tex_v1, set_broadcast_m128(clipped_vertices[vertex_index + 1].z));
        tex_v2 = div_m128(tex_v2, set_broadcast_m128(clipped_vertices[vertex_index + 0].z));
        
        m128 z0_reciprocal = set_broadcast_m128(1.0 / clipped_vertices[vertex_index + 2].z);
        m128 z1_reciprocal = set_broadcast_m128(1.0 / clipped_vertices[vertex_index + 1].z);
        m128 z2_reciprocal = set_broadcast_m128(1.0 / clipped_vertices[vertex_index + 0].z);
        
        //TODO: make sure you understand the math behind this line
        s32 buffer_row_index = start_y * out_framebuffer->width + 2 * start_x;
        
        for (int y = start_y; y <= end_y; y += y_increment, buffer_row_index += out_framebuffer->width * 2) {
            int buffer_index = buffer_row_index;
            m128i w0 = w0_row;
            m128i w1 = w1_row;
            m128i w2 = w2_row;
            
            for (int x = start_x; x <= end_x; x += x_increment, buffer_index += 4) {
                m128i w = or_m128i(w0, or_m128i(w1, w2));
                m128i mask = compare_greater_than_m128i(w, _mm_setzero_si128());
                
                if(_mm_test_all_zeros(mask, mask))
                {
                    //TODO: make it so that the addition happens only once at the top
                    w0 = add_m128i(w0, A0_increment);
                    w1 = add_m128i(w1, A1_increment);
                    w2 = add_m128i(w2, A2_increment);
                    
                    continue;
                }
                
                //TODO:convert 'convert' to cast , convert costs less than cast ?!!!!?!?!
                m128 w0_m128 = convert_to_m128(w0);
                m128 w1_m128 = convert_to_m128(w1);
                m128 w2_m128 = convert_to_m128(w2);
                
                m128 z_nominator = add_m128(add_m128(mul_m128(w0_m128, z0), mul_m128(w1_m128, z1)), mul_m128(w2_m128, z2));
                m128 denominator = convert_to_m128(add_m128i(add_m128i(w0, w1), w2));
                m128 z = div_m128(z_nominator, denominator);
                
                //TODO: write down a dot product simd function
                m128 w0_normalized = div_m128(w0_m128, denominator);
                m128 w1_normalized = div_m128(w1_m128, denominator);
                m128 w2_normalized = div_m128(w2_m128, denominator);
                
                m128 z_temp = add_m128(add_m128(mul_m128(w0_normalized, z0_reciprocal), mul_m128(w1_normalized, z1_reciprocal)), mul_m128(w2_normalized, z2_reciprocal));
                m128 z_interpolation = div_m128(set_broadcast_m128(1.0f), z_temp);
                
                //interpolate uvs
                m128 interpolated_u = dot_product_m128(w0_normalized, w1_normalized, w2_normalized, tex_u0, tex_u1, tex_u2);
                m128 interpolated_v  = dot_product_m128(w0_normalized, w1_normalized, w2_normalized, tex_v0, tex_v1, tex_v2);
                
                interpolated_u = mul_m128(interpolated_u, z_interpolation);
                interpolated_v = mul_m128(interpolated_v, z_interpolation);
                
                //TODO: go through the math here once again
                m128 u_texture_value_temp = mul_m128(interpolated_u, set_broadcast_m128(texture->width - 1));
                m128 v_texture_value_temp = mul_m128(interpolated_v, set_broadcast_m128(texture->height - 1));
                
                m128i u_texture_value = convert_to_m128i(u_texture_value_temp);
                m128i v_texture_value = convert_to_m128i(v_texture_value_temp);
                
                m128i texture_offsets = add_m128i(mul_m128i(v_texture_value, set_broadcast_m128i(texture->width)), u_texture_value);
                texture_offsets = mul_m128i(texture_offsets, set_broadcast_m128i(texture->channels));
                
                u32 texture_pixels[4] = {0};
                for (int i = 0; i < 4; ++i) {
                    u32 texture_index = ((s32 *)&texture_offsets)[i];
                    int is_inside_triangle = ((int *)&mask)[i];
                    if (is_inside_triangle) {
                        u8 *pixel = texture->data + texture_index;
                        u8 red = pixel[0];
                        u8 green = pixel[1]; 
                        u8 blue = pixel[2];
                        u8 alpha = 0xFF;
                        
                        //TODO: see if we can avoid swizzling, look at the blit function
                        texture_pixels[i] = alpha << 24 | blue << 16 | green << 8 | red;
                        //texture_pixels[i] = red << 24 | green << 16 | blue << 8 | alpha;
                    }
                }
                
                //draw pixels
                m128i output_pixels_ = set_m128i(texture_pixels[0], texture_pixels[1], texture_pixels[2], texture_pixels[3]);
                int *output_pixels = (int *)&output_pixels_;
                
                //f32 *depth_buffer_pointer =  &framebuffer->depth_buffer[(y * framebuffer->width + x)];
                f32 *depth_buffer_pointer = out_framebuffer->depth_buffer + buffer_index;
                m128 old_depth_value = _mm_load_ps(depth_buffer_pointer);
                m128 depth_mask = compare_greater_than_m128(z, old_depth_value);
                
                m128 final_mask = _mm_and_ps(_mm_castsi128_ps(mask), depth_mask);
                m128i final_mask_m128i = _mm_castps_si128(final_mask);
                
                m128 depth = _mm_blendv_ps(old_depth_value, z, final_mask);
                _mm_store_ps(depth_buffer_pointer, depth);
                
                _mm_maskmoveu_si128(output_pixels_, final_mask_m128i, out_framebuffer->color_buffer + buffer_index * 4);
                
                w0 = add_m128i(w0, A0_increment);
                w1 = add_m128i(w1, A1_increment);
                w2 = add_m128i(w2, A2_increment);
            }
            
            w0_row = add_m128i(w0_row, B0_increment);
            w1_row = add_m128i(w1_row, B1_increment);
            w2_row = add_m128i(w2_row, B2_increment);
        }
        
        u64 new_current_clock_cycles = __rdtscp(&temp_for_clocks);
        g_rasterizer_clock_cycles = (new_current_clock_cycles - current_clock_cycles) / ((end_x - start_x) * (end_y - start_y));
    }
}

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
    
    
    
#if 1
    static const float input_vertices[] = {
        -2 , 2, -15,   1, 0, 0,     0, 0,
        -2, -2, -5,   0, 1, 0,     0, 1,
        2, -2, -5,   0, 0, 1,     1, 1,
        
        2, -2, -5,   0, 0, 1,     1, 1,
        2, 2, -15,    0, 0, 0,     1, 0,
        -2 , 2, -15,   1, 0, 0,     0, 0
    };
#else
    static const float input_vertices[] = {
        -0.7 , 0.7, -5,   1, 0, 0,     0, 0,
        -0.7, -0.7, -5,   0, 1, 0,     0, 1,
        0.7, -0.7, -5,   0, 0, 1,     1, 1,
        
        0.7, -0.7, -5,   0, 0, 1,     1, 1,
        0.7, 0.7, -5,    0, 0, 0,     1, 0,
        -0.7 , 0.7, -5,   1, 0, 0,     0, 0
    };
#endif
    
#if SAM_HYDE_TOP_CORNER
    rasterization_data.vertex_array[0] = (v4){0, 0, 15, 1};
    rasterization_data.vertex_array[1] = (v4){0, 255, 15, 1};
    rasterization_data.vertex_array[2] = (v4){255, 255, 15, 1};
    
    rasterization_data.vertex_array[3] = (v4){255, 255, 15, 1};
    rasterization_data.vertex_array[4] = (v4){255, 0, 15, 1};
    rasterization_data.vertex_array[5] = (v4){0, 0, 15, 1};
#endif
#if SMALL_SQUARE
    rasterization_data.vertex_array[0] = (v4){0, 0, 15, 1};
    rasterization_data.vertex_array[1] = (v4){0, 3, 15, 1};
    rasterization_data.vertex_array[2] = (v4){3, 3, 15, 1};
    
    rasterization_data.vertex_array[3] = (v4){3, 3, 15, 1};
    rasterization_data.vertex_array[4] = (v4){3, 0, 15, 1};
    rasterization_data.vertex_array[5] = (v4){0, 0, 15, 1};
#endif
    
    while(!g_window_should_close) {
        //for (int i = 0; i < 1; ++i) {
        first_time = get_time();
        
        //stuff
        
        //tickfunc
        
#if 0
        process_vertices(&allocator, &scene, input_vertices, vertices_num, &rasterization_data, framebuffer.width, framebuffer.height);
#endif
        
        //const int vertices_num = array_count(input_vertices) / 8;
        Rasterization_data rasterization_data = {0};
        rasterization_data.vertex_array = (v4 *)allocate_frame(&allocator, 1 * 3 * sizeof(v4));
        rasterization_data.vertex_array[0] = (v4){10, 10, 15, 1};
        rasterization_data.vertex_array[1] = (v4){10, 50, 1};
        rasterization_data.vertex_array[2] = (v4){50, 50, 15, 1};
        
        rasterization_data.uv_array = (v2 *)allocate_frame(&allocator, 3 * sizeof(v2));
        rasterization_data.uv_array[0] = (v2){0 ,0};
        rasterization_data.uv_array[1] = (v2){0 ,1};
        rasterization_data.uv_array[2] = (v2){1, 1};
        
        rasterization_data.num_vertices = 3;
        
        rasterize(&rasterization_data, &texture, &framebuffer);
        
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
    
    
    //printf("median: %f\n", 1 / (median_fps / (double)max_frames));
    printf("lowest fps: %f, highest_fps: %f, average fps: %f\n", lowest_fps, highest_fps, 1.0f / (average_fps / average_fps_counter));
    assert(display != NULL);
    XCloseDisplay(display);
    display = NULL;
    
    return 0;
}