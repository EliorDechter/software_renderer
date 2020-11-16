#ifndef PR_RENDERER
#define PR_RENDERER

u64 g_rasterizer_clock_cycles = 0;

typedef struct Texture {
    u32 width, height, channels;
    u8 *data;
} Texture;

typedef struct Framebuffer{
    u32 width, height;
    unsigned char *color_buffer;
    float *depth_buffer;
} Framebuffer;

const float NEAR = 0.1f;
const float FAR = 100;
const float FOVY = TO_RADIANS(60);

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


v4 g_default_framebuffer_color;
int g_default_framebuffer_depth;

static Framebuffer create_framebuffer(Allocator *allocator, u32 width, u32 height) {
    u32 color_buffer_size = width * height * 4 * sizeof(unsigned char);
    u32 depth_buffer_size = (u32)sizeof(float) * width * height;
    //v4 default_color = get_v4(0.9f, 0.9f, 0.9f, 1.0f);
    //float default_depth = 1.0f; 
    Framebuffer framebuffer = {0};
    
    assert(width > 0 && height > 0);
    
    framebuffer.width = width;
    framebuffer.height = height;
    //framebuffer.color_buffer = (unsigned char*)aligned_alloc(16, color_buffer_size);
    framebuffer.color_buffer = allocate_perm_aligned(allocator, color_buffer_size, 16);
    framebuffer.depth_buffer = (float *)allocate_perm_aligned(allocator, depth_buffer_size, 16);
    
    //TODO: memcpy
    framebuffer_clear_color(&framebuffer, g_default_framebuffer_color);
    framebuffer_clear_depth(&framebuffer, g_default_framebuffer_depth);
    
    return framebuffer;
}

m4 get_m4_look_at(v3 eye, v3 target, v3 up_dir) {
    v3 forward = normalize_v3(sub_v3(eye, target)); 
    v3 left = normalize_v3(cross_v3(up_dir, forward)); 
    v3 up = cross_v3(forward, left); 
    
    m4 m = {
        left.x, up.x, forward.x, 0 , 
        left.y, up.y, forward.y, 0 ,
        left.z, up.z, forward.z, 0 ,
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

Camera *g_camera;

static Camera create_camera(v3 pos, v3 target, float aspect) {
    assert(get_v3_length(sub_v3(pos, target)) > EPSILON && aspect);
    
    return (Camera){
        .pos = pos,
        .target = target,
        .aspect = aspect
    };
}

void move_camera(v3 amount) {
    g_camera->pos =  add_v3(g_camera->pos, amount);
}

//NOTE: packing gives an improvement of 2x ?!
typedef struct __attribute__((packed)) Vertex  {
    //typedef struct Vertex {
    v3 pos;
    v2 uv;
    v2 normal;
    //v3 texture;
} Vertex;

void change_object_size(Vertex *vertices, u32 num_vertices, int amount) {
    for (u32 i = 0; i < num_vertices; ++i) {
        vertices[i].pos.x *= amount;
        vertices[i].pos.y *= amount;
    }
}

typedef enum Outcode {outcode_inside = 0, outcode_left = 1, outcode_right = 2, outcode_bottom = 4, outcode_top = 8} Outcode;

static Outcode compute_out_code(v4 v) {
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
    scene.camera = create_camera(get_v3(0.0f, 0.0f, 0.1f), get_v3(0, 0, 0), (float)width / height);
    
    return scene;
}

typedef struct Vertex_shader_data {
    //v3 pos;
    m4 mvp;
} Vertex_shader_data;

typedef struct Pipeline_data {
    enum { vertex_processing_input, rasterization_input } input_type;
    u32 num_vertices;
    v4 *positions;
    v2 *uvs;
    v2 *normals;
} Pipeline_data;

typedef struct Vertex_buffer {
    Vertex *vertices;
    u32 num_vertices;
} Vertex_buffer;

Vertex_buffer convert_static_positions_and_uvs_to_vertices(float *float_vertices, u32 num_vertices) {
    Vertex *vertices = (Vertex *)allocate_perm(g_allocator, sizeof(Vertex) * num_vertices);
    for (int i = 0; i < num_vertices; ++i) {
        vertices[i].pos = get_v3(float_vertices[5 * i + 0], float_vertices[5 * i + 1], float_vertices[5 * i + 2]);
        vertices[i].uv = get_v2(float_vertices[5 * i + 3], float_vertices[5 * i + 4]);
        vertices[i].normal = get_v2(0, 0);
    }
    
    Vertex_buffer vertex_buffer = {.vertices = vertices, .num_vertices = num_vertices};
    return vertex_buffer;
}

Pipeline_data get_pipeline_data(const Vertex_buffer *vertex_buffer) {
    Vertex *vertices = vertex_buffer->vertices;
    u32 num_vertices = vertex_buffer->num_vertices;
    v4 *positions = (v4 *)allocate_frame(g_allocator, num_vertices * sizeof(v4));
    v2 *uvs = (v2 *)allocate_frame(g_allocator, num_vertices * sizeof(v2));
    v2 *normals = (v2 *)allocate_frame(g_allocator, num_vertices * sizeof(v2));
    
    for (int i = 0; i < num_vertices; ++i) {
        assert(vertices[i].uv.u >= 0 && vertices[i].uv.v >= 0); 
    }
    
    for (int i = 0; i < num_vertices; ++i) {
        positions[i] = get_v4_from_v3(vertices[i].pos, 1.0f);
        uvs[i] = vertices[i].uv;
        assert(vertices[i].uv.u >= 0 && vertices[i].uv.v >= 0);
        normals[i] = vertices[i].normal;
    }
    
    Pipeline_data pipeline_data = {0};
    pipeline_data.positions = positions;
    pipeline_data.uvs = uvs;
    pipeline_data.normals = normals;
    pipeline_data.num_vertices = num_vertices;
    
    return pipeline_data;
}

void vertex_shader_function(int index, void *array, void *data) {
    //TODO: investigate alternatives for void *
    //consider making a generic shader system
    Vertex_shader_data *vertex_shader_data = (Vertex_shader_data *)data;
    v4 *vertex = (v4 *)array + index;
    *vertex = mul_m4_by_v4(vertex_shader_data->mvp, *vertex);
}

static void process_vertices(Worker *main_worker, Allocator *allocator, Pipeline_data *pipeline_data, const Scene *scene, u32 viewport_width, u32 viewport_height) {
    v4 *vertex_positions = pipeline_data->positions;
    v2 *vertex_uvs = pipeline_data->uvs;
    u32 vertices_num = pipeline_data->num_vertices;
    u32 num_vertices = vertices_num; //TODOl: srlsly?
    
    for (int i = 0; i < num_vertices; ++i) {
        //TODO: assert anti-clockwise order
    }
    
    m4 model_matrix = get_m4_identity();
    m4 view_matrix = get_m4_look_at(scene->camera.pos, scene->camera.target, get_v3(0.0f, 1.0f, 0.0f));
    m4 projection_matrix = get_m4_perspective(FOVY, scene->camera.aspect, NEAR, FAR);
    m4 mvp = get_m4_identity();
    //mul_m4_by_m4(mul_m4_by_m4(model_matrix, view_matrix), projection_matrix);
    Vertex_shader_data vertex_shader_data = {0};
    vertex_shader_data.mvp = mvp;
    
#if 0
    parallel_for(main_worker, 0, vertices_num, 1, vertex_positions, vertices_num, &vertex_shader_data, vertex_shader_function);
#else
#if 1
    for (int i = 0; i < vertices_num; ++i) {
        v4 result = mul_m4_by_v4(view_matrix, vertex_positions[i]);
        vertex_positions[i] = result;
    }
#endif
    for (int i = 0; i < vertices_num; ++i) {
        vertex_positions[i] = mul_m4_by_v4(projection_matrix, vertex_positions[i]);
    }
#endif
    
    //clip
    //TODO: clip the near plane? guard band clipping?
    v4 *clipped_vertices = vertex_positions;
    u32 num_clipped_vertices = vertices_num;
    
    //convert clip coordiantes to  ndc / perspective division
    for (int i = 0; i < num_clipped_vertices; ++i) {
        v4 *vertex = clipped_vertices + i;
        vertex->x /= vertex->w;
        vertex->y /= vertex->w;
        vertex->z /= vertex->w; //NOTE: not so sure about this...
    } 
    
    //backface culling... (under counstruction)
    
    //ndc to viewport/raster space
    for (int i = 0; i  < num_clipped_vertices; ++i) {
        v4 *vertex = clipped_vertices  + i;
        vertex->x = (vertex->x  + 1) * 0.5f * (float)viewport_width;
        vertex->y = viewport_height - (vertex->y  + 1) * 0.5f * (float)viewport_height;
        vertex->z = (vertex->z  + 1) * 0.5f;
    }
    
    //TODO: make better TODO's
    //TODO: fix z coordinates !!!!
    //TODO: view frustrum culling
    pipeline_data->num_vertices = num_clipped_vertices;
    pipeline_data->positions = clipped_vertices;
    pipeline_data->uvs = vertex_uvs; //this will break the moment u clip
}

typedef struct Triangle {
    v4 positions[3];
    v2 uvs[3];
} Triangle;

typedef struct Bin {
    Triangle *triangles;
    volatile u32 num_triangles;
    Bounding_box bounding_box;
} Bin;

typedef struct Binner_data {
    Triangle *triangles;
    int num_triangles;
} Binner_data;

#define MAX_NUM_TRIANGLES_PER_BIN 40 //WARNING: this is a bug

static void bin_triangles(int index, void *array, void *data) {
    //TODO: use the abrash algorithm
    Binner_data *binner_data = (Binner_data *)data;
    Bin *bin = (Bin *)array + index;
    
    for (int i = 0; i < binner_data->num_triangles; ++i) {
        Triangle *triangle = &binner_data->triangles[i];
        Bounding_box triangle_bounding_box = get_2d_bounding_box_from_3_v2(get_v2_from_v4(triangle->positions[0]), get_v2_from_v4(triangle->positions[1]), get_v2_from_v4(triangle->positions[2]));
        if (check_aabb_collision(triangle_bounding_box, bin->bounding_box)) {
            assert(bin->num_triangles < MAX_NUM_TRIANGLES_PER_BIN);
            bin->triangles[bin->num_triangles++] = *triangle;
        }
    }
    
#if 0
    //TODO: remove lock
    //TODO: Im leaving thiss dead code for later. Investigate how to atomically insert to an array and incrmenet its value.
    Triangle triangle = triangles[triangle_index];
    v2 bin_coord[3];
    for (int vertex_index = 0; vertex_index < 3; ++vertex_index) {
        int bin_r = (int)(triangle.vertices[vertex_index].x / binner_data->tile_width);
        int bin_c = (int)(triangle.vertices[vertex_index].y / binner_data->tile_height);
        bin_coord[vertex_index] = get_v2(bin_c, bin_r);
    }
    
    Bounding_box bounding_box = get_2d_bounding_box(bin_coord[0], bin_coord[1], bin_coord[2]);
    
    for (int c = min_v2.c; c < max_v2.c; ++c) {
        for (int r = min_v2.r; r < max_v2.r; ++r) {
            Bin *bin = &bin[r + c * binner_data->num_bins_in_row];
            lock_mutex(mutex);
            {
                bin->triangles[bin->num_triangles] = triangle;
                bin->num_triangles++;
            }
            unlock_mutex(mutex);
        }
    }
#endif
}

int bin_sort_function(const void *a, const void *b) {
    Bin *bin_a = (Bin *)a;
    Bin *bin_b = (Bin *)b;
    if (bin_a->num_triangles <= bin_b->num_triangles)
        return 1;
    return 0;
}

static void sort_bins(Bin *bins, int num_bins) {
    //TODO: replace this eventually?
    qsort(bins, num_bins, sizeof(Bin), bin_sort_function);
}

typedef struct Bin_rasterization_data  {
    //Vertex_soa *rasterization_data;
    Texture *texture;
    Framebuffer *framebuffer;
    Binner_data *binner_data;
} Bin_rasterization_data;

static void rasterize_bins(int index, void *array,  void *data) {
    Bin_rasterization_data *bin_rasterization_data = (Bin_rasterization_data *)data;
    Bin *bins = (Bin *)array;
    u32 bin_index = index;
    
    Bin bin = bins[bin_index];
    u32 num_triangles = bin.num_triangles;
    Triangle *triangles = bin.triangles;
    u32 bin_begin_x = bin.bounding_box.x0;
    u32 bin_end_x = bin.bounding_box.x1;
    u32 bin_begin_y = bin.bounding_box.y0;
    u32 bin_end_y = bin.bounding_box.y1;
    Texture *texture = bin_rasterization_data->texture;
    Framebuffer *framebuffer = bin_rasterization_data->framebuffer;
    
    assert(texture->channels == 3);
    
    u32 num_vertices = num_triangles * 3;
    v4 *vertices = (v4 *)allocate_frame(g_allocator, num_vertices * sizeof(v4));
    v2 *uvs = (v2 *)allocate_frame(g_allocator, num_vertices * sizeof(v2));
    for (int i = 0; i < num_triangles; ++i) {
        for (int j = 0; j < 3; ++j) {
            int triangle_index = i * 3 + j;
            //assert(triangles[i].positions[j].z < 1);
            vertices[triangle_index] = triangles[i].positions[j];
            
            assert(triangles[i].uvs[j].u == 0 || triangles[i].uvs[j].u == 1);
            assert(triangles[i].uvs[j].v == 0 || triangles[i].uvs[j].v == 1);
            uvs[triangle_index] = triangles[i].uvs[j];
        }
    }
    
    u32 temp_for_clocks;
    for (int vertex_index = 0; vertex_index < num_vertices; vertex_index += 3) {
        u64 current_clock_cycles =  __rdtscp(&temp_for_clocks);
        
        v2i vertex0 = get_v2i_from_v2(get_v2_from_v4(vertices[vertex_index + 2]));
        v2i vertex1 = get_v2i_from_v2(get_v2_from_v4(vertices[vertex_index + 1]));
        v2i vertex2 = get_v2i_from_v2(get_v2_from_v4(vertices[vertex_index + 0]));
        
#if 0
        //TODO: fix
        s32 begin_x = fmax(bin_begin_x, fmin(fmin(vertex0.x, vertex1.x), vertex2.x));
        s32 end_x = fmin(bin_end_x, fmax(fmax(vertex0.x, vertex1.x), vertex2.x));
        s32 begin_y = fmax(bin_begin_y, fmin(fmin(vertex0.y, vertex1.y), vertex2.y));
        s32 end_y = fmin(bin_end_y, fmax(fmax(vertex0.y, vertex1.y), vertex2.y));
#else
        s32 begin_x = bin_begin_x;
        s32 end_x = bin_end_x;
        s32 begin_y = bin_begin_y;
        s32 end_y = bin_end_y;
#endif
        
        assert(begin_x < end_x);
        assert(begin_y < end_y);
        
        //TODO: some vertices may be clipped, change this code once clipping is done
        s32 A01 = vertex0.y - vertex1.y;
        s32 B01 = vertex1.x - vertex0.x;
        s32 C01 =  vertex0.x * vertex1.y - vertex0.y * vertex1.x;
        s32 F01 = A01 * begin_x + B01 * begin_y + C01;
        
        s32 A12 = vertex1.y - vertex2.y;
        s32 B12 = vertex2.x - vertex1.x;
        s32 C12 =  vertex1.x * vertex2.y - vertex1.y * vertex2.x;
        s32 F12 = A12 * begin_x + B12 * begin_y + C12;
        
        s32 A20 = vertex2.y - vertex0.y;
        s32 B20 = vertex0.x - vertex2.x;
        s32 C20 =  vertex2.x * vertex0.y - vertex2.y * vertex0.x;
        s32 F20 = A20 * begin_x + B20 * begin_y + C20;
        
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
        m128 z0 = set_broadcast_m128(vertices[vertex_index + 2].z);
        m128 z1 =  set_broadcast_m128(vertices[vertex_index + 1].z);
        m128 z2 =  set_broadcast_m128(vertices[vertex_index + 0].z);
#if 0
        m128 z0_new = set_broadcast_m128(vertices[vertex_index + 2].z);
        m128 z1_new =  set_broadcast_m128(vertices[vertex_index + 1].z);
        m128 z2_new =  set_broadcast_m128(vertices[vertex_index + 0].z);
#endif
        
        m128 tex_u0 = set_broadcast_m128(uvs[vertex_index + 2].u);
        m128 tex_u1 = set_broadcast_m128(uvs[vertex_index + 1].u);
        m128 tex_u2 = set_broadcast_m128(uvs[vertex_index + 0].u);
        
        m128 tex_v0 = set_broadcast_m128(uvs[vertex_index + 2].v);
        m128 tex_v1 = set_broadcast_m128(uvs[vertex_index + 1].v);
        m128 tex_v2 = set_broadcast_m128(uvs[vertex_index + 0].v);
        
        tex_u0 = div_m128(tex_u0, set_broadcast_m128(vertices[vertex_index + 2].z));
        tex_u1 = div_m128(tex_u1, set_broadcast_m128(vertices[vertex_index + 1].z));
        tex_u2 = div_m128(tex_u2, set_broadcast_m128(vertices[vertex_index + 0].z));
        
        tex_v0 = div_m128(tex_v0, set_broadcast_m128(vertices[vertex_index + 2].z));
        tex_v1 = div_m128(tex_v1, set_broadcast_m128(vertices[vertex_index + 1].z));
        tex_v2 = div_m128(tex_v2, set_broadcast_m128(vertices[vertex_index + 0].z));
        
        m128 z0_reciprocal = set_broadcast_m128(1.0 / vertices[vertex_index + 2].z);
        m128 z1_reciprocal = set_broadcast_m128(1.0 / vertices[vertex_index + 1].z);
        m128 z2_reciprocal = set_broadcast_m128(1.0 / vertices[vertex_index + 0].z);
        
        //TODO: make sure you understand the math behind this line
        s32 buffer_row_index = begin_y * framebuffer->width + 2 * begin_x;
        
        for (int y = begin_y; y <= end_y; y += y_increment, buffer_row_index += framebuffer->width * 2) {
            int buffer_index = buffer_row_index;
            m128i w0 = w0_row;
            m128i w1 = w1_row;
            m128i w2 = w2_row;
            
            for (int x = begin_x; x <= end_x; x += x_increment, buffer_index += 4) {
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
                f32 *depth_buffer_pointer = framebuffer->depth_buffer + buffer_index;
                m128 old_depth_value = _mm_load_ps(depth_buffer_pointer);
                m128 depth_mask = compare_less_than_m128(z, old_depth_value);
                
                m128 final_mask = _mm_and_ps(_mm_castsi128_ps(mask), depth_mask);
                m128i final_mask_m128i = _mm_castps_si128(final_mask);
                
                m128 depth = _mm_blendv_ps(old_depth_value, z, final_mask);
                _mm_store_ps(depth_buffer_pointer, depth);
                
                _mm_maskmoveu_si128(output_pixels_, final_mask_m128i, framebuffer->color_buffer + buffer_index * 4);
                
                w0 = add_m128i(w0, A0_increment);
                w1 = add_m128i(w1, A1_increment);
                w2 = add_m128i(w2, A2_increment);
            }
            
            w0_row = add_m128i(w0_row, B0_increment);
            w1_row = add_m128i(w1_row, B1_increment);
            w2_row = add_m128i(w2_row, B2_increment);
        }
        
        u64 new_current_clock_cycles = __rdtscp(&temp_for_clocks);
        g_rasterizer_clock_cycles = (new_current_clock_cycles - current_clock_cycles) / ((end_x - begin_x) * (end_y - begin_y));
    }
}


static void rasterize(Worker *main_worker, Pipeline_data *pipeline_data, Texture *texture, Framebuffer *out_framebuffer) {
    assert(pipeline_data->num_vertices != 0);
    assert(pipeline_data->positions);
    assert(pipeline_data->uvs);
    
    
    
    //bin triangles
    u32 num_triangles = pipeline_data->num_vertices / 3;
    Triangle *triangles = (Triangle *)allocate_frame(g_allocator, sizeof(Triangle) * num_triangles);
    for (int i = 0; i < num_triangles; ++i) {
        Triangle *triangle = triangles + i;
        for (int j = 0; j < 3; ++j) {
            int index = i * 3 + j;
            triangle->positions[j] = pipeline_data->positions[index];
            triangle->uvs[j] = pipeline_data->uvs[index];
        }
    }
    
    u32 num_bins_in_row =  4;
    u32 num_bins_in_column = 4;
    u32 tile_width = out_framebuffer->width / num_bins_in_row;
    u32 tile_height = out_framebuffer->height / num_bins_in_column;
    int num_bins = num_bins_in_row * num_bins_in_column;
    Bin *bins = (Bin *)allocate_frame(g_allocator, num_bins *  sizeof(Bin));
    for (int y = 0; y  < num_bins_in_column; ++y) {
        for (int x = 0; x < num_bins_in_row; ++x) {
            //TODO: should it be 512 or 511?
            Bin *bin = &bins[x + y * num_bins_in_row];
            bin->bounding_box.x0 = x * tile_width;
            bin->bounding_box.x1 = bin->bounding_box.x0 + tile_width;
            bin->bounding_box.y0 = y * tile_width;
            bin->bounding_box.y1 = bin->bounding_box.y0 + tile_height;
            bin->triangles = (Triangle *)allocate_frame(g_allocator, sizeof(Triangle) * MAX_NUM_TRIANGLES_PER_BIN);
            bin->num_triangles = 0;
        }
    }
    
    Binner_data binner_data = {
        triangles = triangles,
        num_triangles = num_triangles
    };
    
    parallel_for(main_worker, 0, num_bins, 1, bins, num_bins, &binner_data, bin_triangles);
    
    //sort triangles
    //sort_bins(bins, num_bins);
    
    //rasterize bins
    Bin_rasterization_data bin_rasterization_data = {0};
    bin_rasterization_data.binner_data = &binner_data;
    bin_rasterization_data.texture = texture;
    bin_rasterization_data.framebuffer = out_framebuffer;
    
    //TODO: test what happens when end_index < array_count
    for (int bin_index = 0; bin_index < num_bins; ++bin_index) {
        int num_triangles = bins[bin_index].num_triangles;
        for (int triangle_index = 0; triangle_index < num_triangles; ++triangle_index) {
            Triangle triangle = bins[bin_index].triangles[triangle_index];
            for (int uv_index = 0; uv_index < 3; ++uv_index) {
                assert(triangle.uvs[uv_index].u == 0 || triangle.uvs[uv_index].u == 1);
                assert(triangle.uvs[uv_index].v == 0 || triangle.uvs[uv_index].v == 1);
            }
        }
    }
    
    parallel_for(main_worker, 0, num_bins, 1, bins, num_bins, &bin_rasterization_data, rasterize_bins);
    
}

#endif
