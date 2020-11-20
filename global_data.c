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


float g_cube_face_screen_space[] = {
    200, 0, -3.0f,  1.0f, 0.0f,
    0, 0, -0.9f,  0.0f, 0.0f,
    0,  200, -0.9f,  0.0f, 1.0f,
    0,  200, -0.9f,  0.0f, 1.0f,
    200,  200, -0.9f,  1.0f, 1.0f,
    200, 0, -3.0f,  1.0f, 0.0f,
    
};

float g_cube[] = {
    // back face
    -0.3f, -0.3f, -0.3f,  0.0f, 0.0f, // bottom-left
    0.3f, -0.3f, -0.3f,  1.0f, 0.0f, // bottom-right    
    0.3f,  0.3f, -0.3f,  1.0f, 1.0f, // top-right              
    0.3f,  0.3f, -0.3f,  1.0f, 1.0f, // top-right
    -0.3f,  0.3f, -0.3f,  0.0f, 1.0f, // top-left
    -0.3f, -0.3f, -0.3f,  0.0f, 0.0f, // bottom-left                
    // front face
    -0.3f, -0.3f,  0.3f,  0.0f, 0.0f, // bottom-left
    0.3f,  0.3f,  0.3f,  1.0f, 1.0f, // top-right
    0.3f, -0.3f,  0.3f,  1.0f, 0.0f, // bottom-right        
    0.3f,  0.3f,  0.3f,  1.0f, 1.0f, // top-right
    -0.3f, -0.3f,  0.3f,  0.0f, 0.0f, // bottom-left
    -0.3f,  0.3f,  0.3f,  0.0f, 1.0f, // top-left        
    // left face
    -0.3f,  0.3f,  0.3f,  1.0f, 0.0f, // top-right
    -0.3f, -0.3f, -0.3f,  0.0f, 1.0f, // bottom-left
    -0.3f,  0.3f, -0.3f,  1.0f, 1.0f, // top-left       
    -0.3f, -0.3f, -0.3f,  0.0f, 1.0f, // bottom-left
    -0.3f,  0.3f,  0.3f,  1.0f, 0.0f, // top-right
    -0.3f, -0.3f,  0.3f,  0.0f, 0.0f, // bottom-right
    // right face
    0.3f,  0.3f,  0.3f,  1.0f, 0.0f, // top-left
    0.3f,  0.3f, -0.3f,  1.0f, 1.0f, // top-right      
    0.3f, -0.3f, -0.3f,  0.0f, 1.0f, // bottom-right          
    0.3f, -0.3f, -0.3f,  0.0f, 1.0f, // bottom-right
    0.3f, -0.3f,  0.3f,  0.0f, 0.0f, // bottom-left
    0.3f,  0.3f,  0.3f,  1.0f, 0.0f, // top-left
    // bottom face          
    -0.3f, -0.3f, -0.3f,  0.0f, 1.0f, // top-right
    0.3f, -0.3f,  0.3f,  1.0f, 0.0f, // bottom-left
    0.3f, -0.3f, -0.3f,  1.0f, 1.0f, // top-left        
    0.3f, -0.3f,  0.3f,  1.0f, 0.0f, // bottom-left
    -0.3f, -0.3f, -0.3f,  0.0f, 1.0f, // top-right
    -0.3f, -0.3f,  0.3f,  0.0f, 0.0f, // bottom-right
    // top face
    -0.3f,  0.3f, -0.3f,  0.0f, 1.0f, // top-left
    0.3f,  0.3f, -0.3f,  1.0f, 1.0f, // top-right
    0.3f,  0.3f,  0.3f,  1.0f, 0.0f, // bottom-right                 
    0.3f,  0.3f,  0.3f,  1.0f, 0.0f, // bottom-right
    -0.3f,  0.3f,  0.3f,  0.0f, 0.0f, // bottom-left  
    -0.3f,  0.3f, -0.3f,  0.0f, 1.0f  // top-left              
};

float g_button_vertices[] = {
    20, 0, 0.3,    1, 0,
    0, 0, 0.3,      0, 0,
    0, 10, 0.3,    0, 1,
    
    0, 10, 0.3,    0, 1,
    20, 10, 0.3,  1, 1,
    20, 0, 0.3,     1, 0
};