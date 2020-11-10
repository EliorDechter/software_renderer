#if 0
rasterization_data.vertex_array[0] = (v4){0, 0, 15, 1};
rasterization_data.vertex_array[1] = (v4){0, 255, 15, 1};
rasterization_data.vertex_array[2] = (v4){255, 255, 15, 1};

rasterization_data.vertex_array[3] = (v4){255, 255, 15, 1};
rasterization_data.vertex_array[4] = (v4){255, 0, 15, 1};
rasterization_data.vertex_array[5] = (v4){0, 0, 15, 1};

rasterization_data.vertex_array[0] = (v4){0, 0, 15, 1};
rasterization_data.vertex_array[1] = (v4){0, 3, 15, 1};
rasterization_data.vertex_array[2] = (v4){3, 3, 15, 1};

rasterization_data.vertex_array[3] = (v4){3, 3, 15, 1};
rasterization_data.vertex_array[4] = (v4){3, 0, 15, 1};
rasterization_data.vertex_array[5] = (v4){0, 0, 15, 1};
#endif

float g_sam_hyde_top_corner[] = {
    0, 0, 15, 1,
    0, 255, 15, 1,
    255, 25, 15, 1,
    
    255, 255, 15, 1,
    255, 0, 15, 1,
    0, 0, 15, 1
};

float g_small_square[] = {
    0, 0, 15, 1,
    0, 3, 15, 1,
    3, 3, 15, 1,
    
    3, 3, 15, 1,
    3, 0, 15, 1,
    0, 0, 15, 1
};

float g_cube_face[] = {
    -0.3f, -0.3f, -0.3f,  0.0f, 0.0f,
    0.3f, -0.3f, -0.3f,  1.0f, 0.0f,
    0.3f,  0.3f, -0.3f,  1.0f, 1.0f,
    0.3f,  0.3f, -0.3f,  1.0f, 1.0f,
    -0.3f,  0.3f, -0.3f,  0.0f, 1.0f,
    -0.3f, -0.3f, -0.3f,  0.0f, 0.0f,
};

float g_cube[] = {
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
    0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
    
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
};