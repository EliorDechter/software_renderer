#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <nmmintrin.h>
#include <immintrin.h>

typedef __m128 m128;
typedef __m128i m128i;

#define set_m128i(a0, a1, a2, a3) _mm_set_epi32(a3, a2, a1, a0)
//#define set_m128i(a0, a1, a2, a3) _mm_set_epi32(a0, a1, a2, a3)
#define mul_m128i(a, b) _mm_mullo_epi32(a, b)
#define mul_m128(a, b) _mm_mul_ps(a, b)
#define add_m128i(a, b) _mm_add_epi32(a, b)
#define add_m128(a, b) _mm_add_ps(a, b)
#define or_m128i(a, b) _mm_or_si128(a, b)
#define and_m128i(a, b) _mm_and_si128(a, b)
#define set_broadcast_m128i(a) _mm_set1_epi32(a)
#define blend_m128i(a, b, conditional) _mm_blendv_epi8(a, b, conditional)
#define move_m128i(a, b, conditional) _mm_maskmoveu_si128(a, b, conditional)
#define compare_less_than_m128i(a, b) _mm_cmplt_epi32(a, b)
#define compare_greater_than_m128i(a, b) _mm_cmpgt_epi32(a, b)
#define compare_equal_m128i(a, b) _mm_cmpeq_epi32(a, b)
#define convert_to_m128i(a) _mm_cvtps_epi32(a)
#define sub_m128i(a, b) _mm_sub_epi32(a, b)
#define cast_to_m128i(a) _mm_castps_si128(a)

#define set_broadcast_m128(a) _mm_set1_ps(a)
#define compare_greater_than_m128(a, b) _mm_cmpgt_ps(a, b)
#define convert_to_m128(a) _mm_cvtepi32_ps(a)
#define div_m128(a, b) _mm_div_ps(a, b)
#define load_m128(memory) _mm_load_ps(memory)
#define compare_less_than_or_equal_m128(a, b) _mm_cmple_ps(a, b)
#define and_m128(a, b) _mm_and_ps(a, b)
#define cast_to_m128(a) _mm_castsi128_ps(a)

//TODO: fma + avx2 instructions

typedef union v2i {
    struct { s32 x, y;  };
    struct { s32 u, v;  };
    s32 e[2];
} v2i;

typedef union v2 {
    struct { float x, y; };
    struct { float u, v; };
    struct { float c, r; };
    float e[2];
} v2;

typedef union v3 {
    struct { float x, y, z; };
    float e[3];
} v3;


typedef union  v4 {
    struct { float x, y, z, w; };
    struct { float r, g, b, a; };
    float e[4];
} v4;


typedef struct m4 {
    float e[4][4];
} m4;

v2 get_v2(float x, float y) {
    return (v2){.x =x, .y =y};
}

v4 get_v4(float x, float y, float z, float w) {
    return (v4){.x =x, .y =y, .z = z, .w = w};
}

v3 get_v3_from_v4(v4 vec) {
    v3 result = {
        .x = vec.x,
        .y = vec.y,
        .z = vec.z
    };
    
    return result;
}

v4 mul_m4_by_v4(m4 m, v4 vec) {
    v4 result = {0};
    
    for (int i = 0; i < 4; ++i) {
        result.e[i] += m.e[i][0] * vec.x;
        result.e[i] += m.e[i][1] * vec.y;
        result.e[i] += m.e[i][2] * vec.z;
        result.e[i] += m.e[i][3] * vec.w;
    }
    
    return result;
}

m4 mul_m4_by_m4(m4 a, m4 b) {
    m4 result = {0};
    
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            for (int k = 0; k < 4; ++k) {
                result.e[i][j] += a.e[i][k] * b.e[k][j];
            }
        }
    }
    
    return result;
}

v3 cross_v3(v3 vec1, v3 vec2) {
    v3 result = {
        .x = vec1.y * vec2.z - vec1.z * vec2.y,
        .y = vec1.z * vec2.x - vec1.x * vec2.z,
        .z = vec1.x * vec2.y - vec1.y * vec2.x
    };
    
    return result;
}

float get_v3_length(v3 vec) {
    float result = (float)sqrt(pow(vec.x, 2) + pow(vec.y, 2) + pow(vec.z, 2));
    return result;
}

v3 normalize_v3(v3 vec) {
    float length = get_v3_length(vec);
    v3 result = {
        .x = vec.x/length,
        .y = vec.y/length,
        .z = vec.z/length
    };
    
    return result;
}

m4 get_m4_identity() {
    m4 result =  {0};
    result.e[0][0] = result.e[1][1] = result.e[2][2] = result.e[3][3] = 1.0f;
    
    return result;
}

float lerp_float(float a, float b, float t) {
    return a + (b -a) * t;
}

v3 get_v3(float x, float y, float z) {
    return (v3){
        .x = x,
        .y = y,
        .z = z
    };
}

v3 add_v3(v3 a, v3 b) {
    return get_v3(a.x + b.x, a.y + b.y, a.z + b.z);
}

v3 sub_v3(v3 a, v3 b) {
    return get_v3(a.x - b.x, a.y - b.y, a.z - b.z);
}

v2 get_v2_from_v4(v4 v) {
    return get_v2(v.x, v.y);
}

v2 max_v2(v2 a, v2 b) {
    if (a.x > b.x) {
        return a;
    }
    if (a.y > b.y) {
        return a;
    }
    
    return b;
}

v2 min_v2(v2 a, v2 b) {
    if (a.x < b.x) {
        return a;
    }
    if (a.y < b.y) {
        return a;
    }
    
    return b;
}

v2 get_v2_from_v2i(v2i v) {
    return get_v2((float)v.x, (float)v.y);
}

v2i get_v2i(s32 x, s32 y) {
    return (v2i){.x =x, .y =y};
}

v2i get_v2i_from_v2(v2 v) {
    return get_v2i((s32)v.x, (s32)v.y);
}

v3 dot_v3(v3 a, v3 b) {
    return get_v3(a.x + b.x, a.y + b.y, a.z + b.z);
}

v3 hadamard_v3(v3 a, v3 b) {
    return get_v3(a.x * b.x, a.y * b.y, a.z * b.z);
}

m128i dot_product_m128i(m128i a0, m128i a1, m128i a2, m128i b0, m128i b1, m128i b2) {
    return add_m128i(add_m128i(mul_m128i(a0, b0), mul_m128i(a1, b1)), mul_m128i(a2, b2));
}

m128 dot_product_m128(m128 a0, m128 a1, m128 a2, m128 b0, m128 b1, m128 b2) {
    return add_m128(add_m128(mul_m128(a0, b0), mul_m128(a1, b1)), mul_m128(a2, b2));
}


s32 min_s32(s32 a, s32 b) {
    return a < b ? a : b;
}

s32 max_s32(s32 a, s32 b) {
    return a > b ? a : b;
}


u32 min_u32(u32 a, u32 b) {
    return a < b ? a : b;
}

u32 max_u32(u32 a, u32 b) {
    return a > b ? a : b;
}

f32 max_f32(f32 a, f32 b) {
    return a > b ? a : b;
}

v4 get_v4_from_v3(v3 v, f32 last_element) {
    return get_v4(v.e[0], v.e[1], v.e[2], last_element);
}

typedef struct Bounding_box {
    int x0, x1, y0, y1;
} Bounding_box;

Bounding_box get_2d_bounding_box_from_4_coords(int x0, int y0, int x1, int y1) {
    Bounding_box bounding_box = {
        .x0 = x0,
        .y0 = y0, 
        .x1 = x1,
        .y1 = y1
    };
    
    return bounding_box;
}

Bounding_box get_2d_bounding_box_from_3_v2(v2 a, v2 b, v2 c) {
    Bounding_box bounding_box;
    bounding_box.x0 = min_s32(min_s32(a.x, b.x), c.x);
    bounding_box.y0 = min_s32(min_s32(a.y, b.y), c.y);
    bounding_box.x1 = max_s32(max_s32(a.x, b.x), c.x);
    bounding_box.y1 = max_s32(max_s32(a.y, b.y), c.y);
    
    return bounding_box;
}

bool check_aabb_collision(Bounding_box box0, Bounding_box box1) {
    int width_sum = (box0.x1 - box0.x0) + (box1.x1  - box1.x0);
    int height_sum = (box0.y1 - box0.y0) + (box1.y1  - box1.y0);
    return (abs(box0.x0 - box1.x0) < width_sum) && (abs(box0.y0 - box1.y0) < height_sum);
}