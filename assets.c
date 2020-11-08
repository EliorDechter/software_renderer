#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobj_loader_c.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#if 0

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

#endif

static Texture load_texture(Allocator *allocator, const char *path) {
    //TODO: move this into a full formed asset system, with the custom allocator, and consider writing it yourself if necessary
    //consider baking it into a seperate asset file
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
static void load_gltf() {
    cgltf_options options = {0};
    cgltf_data *data = NULL;
    cgltf_result result = cgltf_parse_file(&options, "scene.gltf", &data);
    if (result != cgltf_result_success) {
        printf("faild to load gltf\n");
        exit(1);
    }
}

void load_animation() {
    Skeleton *skeleton;
}

typedef struct Mesh {
    int num_faces;
    vertex *vertices;
    v3 center;
} Mesh;

mesh *build_mesh(v3 *positions, v2 *tex_coords, v3 *normals, v4 *tangents, v4 *joints, v4 *weights, int *position_indices, int *tex_coords_indices, int *normal_indices) {
    v3 bbox_min = get_v3(+1e6, +1e6, +1e6);
    v3 bbox_min = get_v3(-1e6, -1e6, -1e6);
    int num_indices = 0; //TODO: init
    int num_faces = num_indices / 3;
    v3 *vertices;
    Mesh *mesh;
    int i;
    
    //TODO: assert some stuff
    
    vertices = (Vertex *)malloc(sizeof(Vertex) * num_indices);
    for (int i = 0; i < num_indices; ++i) {
        int position_index = position_indices[i];
        int tex_coord_index = 0;
        
        if (tangetsn) {
            int tangent_index = position_index;
            vertices[i].tangent = tangents
                
        }
    }
}
#endif

void read_obj_file(const char *filename, int is_mtl, const char *obj_filename, char **buf, size_t *len) {
    if (is_mtl) {
        *len = 0;
        *buf = NULL;
        return;
    }
    FILE *file = fopen(filename, "r");
    assert(file);
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0L, SEEK_SET);
    
    u8 *buffer = malloc(sizeof(u8) * file_size);
    fread(buffer, sizeof(u8), file_size, file);
    *buf = buffer;
    *len = file_size;
}

typedef struct Model  {
    Vertex *vertices;
    int num_vertices;
} Model;

Model load_model_from_obj() {
    tinyobj_attrib_t attrib;
    tinyobj_shape_t *shapes;
    size_t num_shapes;
    tinyobj_material_t *materials;
    size_t num_materials;
    const char *file_name = "viking_room.obj";
    int result = tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials, &num_materials, file_name, read_obj_file, TINYOBJ_FLAG_TRIANGULATE);
    //printf("%d\n", result);
    assert(result == TINYOBJ_SUCCESS);
    
    //TODO: identify whats causing heap top corruption when mallocing this
    Model model = {0};
    model.vertices = (Vertex *)allocate_perm(g_allocator, sizeof(Vertex) * 10000);
    
    for (int shape_index = 0; shape_index < num_shapes; ++shape_index) {
        for (int i = 0; i < attrib.num_faces; ++i) {
            int vertex_index = attrib.faces[i].v_idx;
            int texcoord_index = attrib.faces[i].vt_idx;
            int normal_index = attrib.faces[i].vn_idx;
            
            Vertex vertex;
            vertex.pos.e[0] = attrib.vertices[vertex_index * 3 + 0];
            vertex.pos.e[1] = attrib.vertices[vertex_index * 3 + 1];
            vertex.pos.e[2] = attrib.vertices[vertex_index * 3 + 2];
            
            vertex.uv.e[0] = attrib.texcoords[texcoord_index * 2  + 0];
            vertex.uv.e[1] = attrib.texcoords[texcoord_index * 2  + 1];
            
            assert(vertex.uv.e[0] >= 0 && vertex.uv.e[1] >= 0);
            
            vertex.normal.e[0] = attrib.normals[normal_index * 2 + 0];
            vertex.normal.e[1] = attrib.normals[normal_index * 2 + 1];
            
            model.vertices[model.num_vertices++] = vertex;
        }
    }
    
    for (int i = 0; i < model.num_vertices; ++i) {
        Vertex vertex = model.vertices[i];
        assert(vertex.uv.u >= 0 && vertex.uv.v >= 0);
    }
    
    return model;
}


