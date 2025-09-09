/*
 * Copyright 2016-2018 Józef Kucia for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __VKD3D_TEST_UTILS_H
#define __VKD3D_TEST_UTILS_H

/* Hack for MinGW-w64 headers.
 *
 * We want to use WIDL C inline wrappers because some methods
 * in D3D12 interfaces return aggregate objects. Unfortunately,
 * WIDL C inline wrappers are broken when used with MinGW-w64
 * headers because FORCEINLINE expands to extern inline
 * which leads to the "multiple storage classes in declaration
 * specifiers" compiler error.
 */
#ifdef __MINGW32__
# include <_mingw.h>
# ifdef __MINGW64_VERSION_MAJOR
#  undef __forceinline
#  define __forceinline __inline__ __attribute__((__always_inline__,__gnu_inline__))
# endif
#endif

#define COBJMACROS
#define WIDL_C_INLINE_WRAPPERS
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "vkd3d_test.h"
#include "vkd3d_d3d12.h"
#include "vkd3d_d3dcompiler.h"
#include "vkd3d_shader.h"
#include "dxcompiler.h"

struct vec2
{
    float x, y;
};

struct vec4
{
    float x, y, z, w;
};

struct dvec2
{
    double x, y;
};

struct ivec4
{
    int x, y, z, w;
};

struct uvec4
{
    unsigned int x, y, z, w;
};

struct i64vec2
{
    int64_t x, y;
};

struct u64vec2
{
    uint64_t x, y;
};

struct resource_readback
{
    uint64_t width;
    unsigned int height;
    unsigned int depth;
    uint64_t row_pitch;
    void *data;
};

static inline uint32_t float_to_int(float f)
{
    union
    {
        uint32_t u;
        float f;
    } u;

    u.f = f;
    return u.u;
}

static inline bool vkd3d_array_reserve(void **elements, size_t *capacity, size_t element_count, size_t element_size)
{
    size_t new_capacity, max_capacity;
    void *new_elements;

    if (element_count <= *capacity)
        return true;

    max_capacity = ~(size_t)0 / element_size;
    if (max_capacity < element_count)
        return false;

    new_capacity = max(*capacity, 4);
    while (new_capacity < element_count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;

    if (new_capacity < element_count)
        new_capacity = element_count;

    if (!(new_elements = realloc(*elements, new_capacity * element_size)))
        return false;

    *elements = new_elements;
    *capacity = new_capacity;

    return true;
}

static bool compare_uint(unsigned int x, unsigned int y, unsigned int max_diff)
{
    unsigned int diff = x > y ? x - y : y - x;

    return diff <= max_diff;
}

static bool compare_uint64(uint64_t x, uint64_t y, uint64_t max_diff)
{
    uint64_t diff = x > y ? x - y : y - x;

    return diff <= max_diff;
}

static bool compare_color(DWORD c1, DWORD c2, BYTE max_diff)
{
    return compare_uint(c1 & 0xff, c2 & 0xff, max_diff)
           && compare_uint((c1 >> 8) & 0xff, (c2 >> 8) & 0xff, max_diff)
           && compare_uint((c1 >> 16) & 0xff, (c2 >> 16) & 0xff, max_diff)
           && compare_uint((c1 >> 24) & 0xff, (c2 >> 24) & 0xff, max_diff);
}

static bool compare_float(float f, float g, unsigned int ulps)
{
    int x, y;
    union
    {
        float f;
        int i;
    } u;

    u.f = f;
    x = u.i;
    u.f = g;
    y = u.i;

    if (x < 0)
        x = INT_MIN - x;
    if (y < 0)
        y = INT_MIN - y;

    return compare_uint(x, y, ulps);
}

static bool compare_double(double f, double g, unsigned int ulps)
{
    int64_t x, y;
    union
    {
        double f;
        int64_t i;
    } u;

    u.f = f;
    x = u.i;
    u.f = g;
    y = u.i;

    if (x < 0)
        x = INT64_MIN - x;
    if (y < 0)
        y = INT64_MIN - y;

    return compare_uint64(x, y, ulps);
}

static inline bool compare_uvec4(const struct uvec4 *v1, const struct uvec4 *v2)
{
    return v1->x == v2->x && v1->y == v2->y && v1->z == v2->z && v1->w == v2->w;
}

static inline bool compare_u64vec2(const struct u64vec2 *v1, const struct u64vec2 *v2)
{
    return compare_uint64(v1->x, v2->x, 0)
            && compare_uint64(v1->y, v2->y, 0);
}

static inline bool compare_vec(const struct vec4 *v1, const struct vec4 *v2,
        unsigned int ulps, unsigned component_count)
{
    if (component_count > 0 && !compare_float(v1->x, v2->x, ulps))
        return false;
    if (component_count > 1 && !compare_float(v1->y, v2->y, ulps))
        return false;
    if (component_count > 2 && !compare_float(v1->z, v2->z, ulps))
        return false;
    if (component_count > 3 && !compare_float(v1->w, v2->w, ulps))
        return false;
    return true;
}

static inline bool compare_vec4(const struct vec4 *v1, const struct vec4 *v2, unsigned int ulps)
{
    return compare_vec(v1, v2, ulps, 4);
}

static inline bool compare_dvec2(const struct dvec2 *v1, const struct dvec2 *v2, unsigned int ulps)
{
    return compare_double(v1->x, v2->x, ulps)
            && compare_double(v1->y, v2->y, ulps);
}

static inline void set_rect(RECT *rect, int left, int top, int right, int bottom)
{
    rect->left = left;
    rect->right = right;
    rect->top = top;
    rect->bottom = bottom;
}

static void *get_readback_data(const struct resource_readback *rb,
        unsigned int x, unsigned int y, unsigned int z, size_t element_size)
{
    unsigned int slice_pitch = rb->row_pitch * rb->height;
    return &((uint8_t *)rb->data)[slice_pitch * z + rb->row_pitch * y + x * element_size];
}

static float get_readback_float(const struct resource_readback *rb, unsigned int x, unsigned int y)
{
    return *(float *)get_readback_data(rb, x, y, 0, sizeof(float));
}

static double get_readback_double(const struct resource_readback *rb, unsigned int x, unsigned int y)
{
    return *(double *)get_readback_data(rb, x, y, 0, sizeof(double));
}

static unsigned int get_readback_uint(const struct resource_readback *rb, unsigned int x, unsigned int y, unsigned int z)
{
    return *(unsigned int*)get_readback_data(rb, x, y, z, sizeof(unsigned int));
}

static uint64_t get_readback_uint64(const struct resource_readback *rb, unsigned int x, unsigned int y)
{
    return *(uint64_t*)get_readback_data(rb, x, y, 0, sizeof(uint64_t));
}

static const struct vec4 *get_readback_vec4(const struct resource_readback *rb, unsigned int x, unsigned int y)
{
    return get_readback_data(rb, x, y, 0, sizeof(struct vec4));
}

static const struct dvec2 *get_readback_dvec2(const struct resource_readback *rb, unsigned int x, unsigned int y)
{
    return get_readback_data(rb, x, y, 0, sizeof(struct dvec2));
}

static const struct uvec4 *get_readback_uvec4(const struct resource_readback *rb, unsigned int x, unsigned int y)
{
    return get_readback_data(rb, x, y, 0, sizeof(struct uvec4));
}

static const struct u64vec2 *get_readback_u64vec2(const struct resource_readback *rb, unsigned int x, unsigned int y)
{
    return get_readback_data(rb, x, y, 0, sizeof(struct u64vec2));
}

#define check_readback_data_float(a, b, c, d) check_readback_data_float_(__FILE__, __LINE__, a, b, c, d)
static inline void check_readback_data_float_(const char *file, unsigned int line,
        const struct resource_readback *rb, const RECT *rect, float expected, unsigned int max_diff)
{
    RECT r = {0, 0, rb->width, rb->height};
    unsigned int x = 0, y;
    bool all_match = true;
    float got = 0;

    if (rect)
        r = *rect;

    for (y = r.top; y < r.bottom; ++y)
    {
        for (x = r.left; x < r.right; ++x)
        {
            got = get_readback_float(rb, x, y);
            if (!compare_float(got, expected, max_diff))
            {
                all_match = false;
                break;
            }
        }
        if (!all_match)
            break;
    }
    ok_(file, line)(all_match, "Got %.8e, expected %.8e at (%u, %u).\n", got, expected, x, y);
}

#define check_readback_data_double(a, b, c, d) check_readback_data_double_(__FILE__, __LINE__, a, b, c, d)
static inline void check_readback_data_double_(const char *file, unsigned int line,
        const struct resource_readback *rb, const RECT *rect, double expected, unsigned int max_diff)
{
    RECT r = {0, 0, rb->width, rb->height};
    unsigned int x = 0, y;
    bool all_match = true;
    double got = 0;

    if (rect)
        r = *rect;

    for (y = r.top; y < r.bottom; ++y)
    {
        for (x = r.left; x < r.right; ++x)
        {
            got = get_readback_double(rb, x, y);
            if (!compare_double(got, expected, max_diff))
            {
                all_match = false;
                break;
            }
        }
        if (!all_match)
            break;
    }
    ok_(file, line)(all_match, "Got %.15le, expected %.15le at (%u, %u).\n", got, expected, x, y);
}

#define check_readback_data_uint(a, b, c, d) check_readback_data_uint_(__FILE__, __LINE__, a, b, c, d)
static inline void check_readback_data_uint_(const char *file, unsigned int line,
        struct resource_readback *rb, const D3D12_BOX *box, unsigned int expected, unsigned int max_diff)
{
    D3D12_BOX b = {0, 0, 0, rb->width, rb->height, rb->depth};
    unsigned int x = 0, y = 0, z;
    bool all_match = true;
    unsigned int got = 0;

    if (box)
        b = *box;

    for (z = b.front; z < b.back; ++z)
    {
        for (y = b.top; y < b.bottom; ++y)
        {
            for (x = b.left; x < b.right; ++x)
            {
                got = get_readback_uint(rb, x, y, z);
                if (!compare_color(got, expected, max_diff))
                {
                    all_match = false;
                    break;
                }
            }
            if (!all_match)
                break;
        }
        if (!all_match)
            break;
    }
    ok_(file, line)(all_match, "Got 0x%08x, expected 0x%08x at (%u, %u, %u).\n", got, expected, x, y, z);
}

#define check_readback_data_uint64(a, b, c, d) check_readback_data_uint64_(__FILE__, __LINE__, a, b, c, d)
static inline void check_readback_data_uint64_(const char *file, unsigned int line,
        struct resource_readback *rb, const D3D12_BOX *box, uint64_t expected, unsigned int max_diff)
{
    D3D12_BOX b = {0, 0, 0, rb->width, rb->height, rb->depth};
    unsigned int x = 0, y = 0;
    bool all_match = true;
    uint64_t got = 0;

    if (box)
        b = *box;

    for (y = b.top; y < b.bottom; ++y)
    {
        for (x = b.left; x < b.right; ++x)
        {
            got = get_readback_uint64(rb, x, y);
            if (!compare_uint64(got, expected, max_diff))
            {
                all_match = false;
                break;
            }
        }
        if (!all_match)
            break;
    }
    ok_(file, line)(all_match, "Got 0x%016"PRIx64", expected 0x%016"PRIx64" at (%u, %u).\n", got, expected, x, y);
}

#define check_readback_data_vec2(a, b, c, d) check_readback_data_vec_(__FILE__, __LINE__, a, b, c, d, 2)
#define check_readback_data_vec4(a, b, c, d) check_readback_data_vec_(__FILE__, __LINE__, a, b, c, d, 4)
static inline void check_readback_data_vec_(const char *file, unsigned int line, const struct resource_readback *rb,
        const RECT *rect, const struct vec4 *expected, unsigned int max_diff, unsigned component_count)
{
    RECT r = {0, 0, rb->width, rb->height};
    unsigned int x = 0, y = 0;
    struct vec4 got = {0};
    bool all_match = true;

    if (rect)
        r = *rect;

    for (y = r.top; y < r.bottom; ++y)
    {
        for (x = r.left; x < r.right; ++x)
        {
            got = *get_readback_vec4(rb, x, y);
            if (!compare_vec(&got, expected, max_diff, component_count))
            {
                all_match = false;
                break;
            }
        }
        if (!all_match)
            break;
    }
    ok_(file, line)(all_match, "Got {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e} at (%u, %u).\n",
            got.x, got.y, got.z, got.w, expected->x, expected->y, expected->z, expected->w, x, y);
}

#define check_readback_data_dvec2(a, b, c, d) check_readback_data_dvec2_(__FILE__, __LINE__, a, b, c, d)
static inline void check_readback_data_dvec2_(const char *file, unsigned int line, const struct resource_readback *rb,
        const RECT *rect, const struct dvec2 *expected, unsigned int max_diff)
{
    RECT r = {0, 0, rb->width, rb->height};
    unsigned int x = 0, y = 0;
    struct dvec2 got = {0};
    bool all_match = true;

    if (rect)
        r = *rect;

    for (y = r.top; y < r.bottom; ++y)
    {
        for (x = r.left; x < r.right; ++x)
        {
            got = *get_readback_dvec2(rb, x, y);
            if (!compare_dvec2(&got, expected, max_diff))
            {
                all_match = false;
                break;
            }
        }
        if (!all_match)
            break;
    }
    ok_(file, line)(all_match, "Got {%.15e, %.15e}, expected {%.15e, %.15e} at (%u, %u).\n",
            got.x, got.y, expected->x, expected->y, x, y);
}

#define check_readback_data_ivec4(a, b, c) \
        check_readback_data_uvec4_(__FILE__, __LINE__, a, b, (const struct uvec4 *)(c))
#define check_readback_data_uvec4(a, b, c) check_readback_data_uvec4_(__FILE__, __LINE__, a, b, c)
static inline void check_readback_data_uvec4_(const char *file, unsigned int line,
        const struct resource_readback *rb, const RECT *rect, const struct uvec4 *expected)
{
    RECT r = {0, 0, rb->width, rb->height};
    unsigned int x = 0, y = 0;
    struct uvec4 got = {0};
    bool all_match = true;

    if (rect)
        r = *rect;

    for (y = r.top; y < r.bottom; ++y)
    {
        for (x = r.left; x < r.right; ++x)
        {
            got = *get_readback_uvec4(rb, x, y);
            if (!compare_uvec4(&got, expected))
            {
                all_match = false;
                break;
            }
        }
        if (!all_match)
            break;
    }
    ok_(file, line)(all_match,
            "Got {0x%08x, 0x%08x, 0x%08x, 0x%08x}, expected {0x%08x, 0x%08x, 0x%08x, 0x%08x} at (%u, %u).\n",
            got.x, got.y, got.z, got.w, expected->x, expected->y, expected->z, expected->w, x, y);
}

#define check_readback_data_i64vec2(a, b, c) \
        check_readback_data_u64vec2_(__FILE__, __LINE__, a, b, (const struct u64vec2 *)(c))
#define check_readback_data_u64vec2(a, b, c) check_readback_data_u64vec2_(__FILE__, __LINE__, a, b, c)
static inline void check_readback_data_u64vec2_(const char *file, unsigned int line,
        const struct resource_readback *rb, const RECT *rect, const struct u64vec2 *expected)
{
    RECT r = {0, 0, rb->width, rb->height};
    unsigned int x = 0, y = 0;
    struct u64vec2 got = {0};
    bool all_match = true;

    if (rect)
        r = *rect;

    for (y = r.top; y < r.bottom; ++y)
    {
        for (x = r.left; x < r.right; ++x)
        {
            got = *get_readback_u64vec2(rb, x, y);
            if (!compare_u64vec2(&got, expected))
            {
                all_match = false;
                break;
            }
        }
        if (!all_match)
            break;
    }
    ok_(file, line)(all_match, "Got {0x%016"PRIx64", 0x%016"PRIx64"}, expected {0x%016"PRIx64", 0x%016"PRIx64"}"
            " at (%u, %u).\n", got.x, got.y, expected->x, expected->y, x, y);
}

struct test_options
{
    bool use_warp_device;
    unsigned int adapter_idx;
    bool enable_debug_layer;
    bool enable_gpu_based_validation;
};

extern struct test_options test_options;

static inline void parse_args(int argc, char **argv)
{
    unsigned int i;

    for (i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--warp"))
            test_options.use_warp_device = true;
        else if (!strcmp(argv[i], "--adapter") && i + 1 < argc)
            test_options.adapter_idx = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--validate"))
            test_options.enable_debug_layer = true;
        else if (!strcmp(argv[i], "--gbv"))
            test_options.enable_gpu_based_validation = true;
    }
}

static inline HRESULT vkd3d_shader_code_from_dxc_blob(IDxcBlob *blob, struct vkd3d_shader_code *blob_out)
{
    size_t size;

    size = IDxcBlob_GetBufferSize(blob);
    if (!(blob_out->code = malloc(size)))
    {
        trace("Failed to allocate shader code.\n");
        return E_OUTOFMEMORY;
    }

    memcpy((void *)blob_out->code, IDxcBlob_GetBufferPointer(blob), size);
    blob_out->size = size;

    return S_OK;
}

static inline HRESULT dxc_compile(void *dxc_compiler, const WCHAR *profile, unsigned int compile_options,
        const WCHAR *entry_point, bool enable_16bit_types, const char *hlsl, struct vkd3d_shader_code *blob_out)
{
    DxcBuffer src_buf = {hlsl, strlen(hlsl), 65001};
    IDxcCompiler3 *compiler = dxc_compiler;
    IDxcBlobUtf8 *errors;
    IDxcResult *result;
    HRESULT compile_hr;
    size_t arg_count;
    IDxcBlob *blob;
    int hr;

    const WCHAR *args[] =
    {
        L"/T",
        profile,
        L"/Qstrip_reflect",
        L"/Qstrip_debug",
        L"/flegacy-macro-expansion",
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
    };

    memset(blob_out, 0, sizeof(*blob_out));

    arg_count = ARRAY_SIZE(args) - 6;
    if (!(compile_options & D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES))
        args[arg_count++] = L"/flegacy-resource-reservation";
    if (entry_point)
        args[arg_count++] = entry_point;
    if (compile_options & D3DCOMPILE_PACK_MATRIX_ROW_MAJOR)
        args[arg_count++] = L"/Zpr";
    if (compile_options & D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR)
        args[arg_count++] = L"/Zpc";
    if (compile_options & D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY)
        args[arg_count++] = L"/Gec";
    if (enable_16bit_types)
        args[arg_count++] = L"/enable-16bit-types";

    if (FAILED(hr = IDxcCompiler3_Compile(compiler, &src_buf, args,
            arg_count, NULL, &IID_IDxcResult, (void **)&result)))
        return hr;

    if (IDxcResult_HasOutput(result, DXC_OUT_ERRORS)
            && SUCCEEDED(hr = IDxcResult_GetOutput(result, DXC_OUT_ERRORS, &IID_IDxcBlobUtf8, (void **)&errors, NULL)))
    {
        if (IDxcBlobUtf8_GetStringLength(errors) && vkd3d_test_state.debug_level)
            trace("%s\n", (char *)IDxcBlobUtf8_GetStringPointer(errors));
        IDxcBlobUtf8_Release(errors);
    }

    if (FAILED(hr = IDxcResult_GetStatus(result, &compile_hr)) || FAILED((hr = compile_hr)))
    {
        if (hr == DXC_E_LLVM_CAST_ERROR)
            hr = E_FAIL;
        goto result_release;
    }

    if (FAILED(hr = IDxcResult_GetOutput(result, DXC_OUT_OBJECT, &IID_IDxcBlob, (void **)&blob, NULL)))
        goto result_release;

    IDxcResult_Release(result);

    hr = vkd3d_shader_code_from_dxc_blob(blob, blob_out);
    IDxcBlob_Release(blob);
    return hr;

result_release:
    IDxcResult_Release(result);
    return hr;
}

#if (defined(SONAME_LIBDXCOMPILER) || defined(VKD3D_CROSSTEST))
static inline IDxcCompiler3 *dxcompiler_create(void)
{
    DxcCreateInstanceProc create_instance;
    IDxcCompiler3 *compiler;
    const char *skip_dxc;
    void *dll;
    int hr;

    if ((skip_dxc = getenv("VKD3D_TEST_SKIP_DXC")) && strcmp(skip_dxc, "") != 0)
        return NULL;

# ifdef VKD3D_CROSSTEST
    dll = vkd3d_dlopen("dxcompiler.dll");
# else
    dll = vkd3d_dlopen(SONAME_LIBDXCOMPILER);
# endif
    ok(dll, "Failed to load dxcompiler library, %s.\n", vkd3d_dlerror());
    if (!dll)
        return NULL;

    create_instance = (DxcCreateInstanceProc)vkd3d_dlsym(dll, "DxcCreateInstance");
    ok(create_instance, "Failed to get DxcCreateInstance() pointer.\n");
    if (!create_instance)
        return NULL;

    hr = create_instance(&CLSID_DxcCompiler, &IID_IDxcCompiler3, (void **)&compiler);
    ok(SUCCEEDED(hr), "Failed to create instance, hr %#x.\n", hr);
    if (FAILED(hr))
        return NULL;

    return compiler;
}
#else
static inline IDxcCompiler3 *dxcompiler_create(void)
{
    return NULL;
}
#endif

static HRESULT d3d10_blob_from_vkd3d_shader_code(const struct vkd3d_shader_code *blob, ID3D10Blob **blob_out)
{
    ID3D10Blob *d3d_blob;
    int hr;

    if (FAILED(hr = D3DCreateBlob(blob->size, (ID3DBlob **)&d3d_blob)))
    {
        trace("Failed to create blob, hr %#x.\n", hr);
        return hr;
    }

    memcpy(ID3D10Blob_GetBufferPointer(d3d_blob), blob->code, blob->size);
    *blob_out = d3d_blob;

    return S_OK;
}

static inline HRESULT dxc_compiler_compile_shader(void *dxc_compiler,
        const char *profile, unsigned int compile_options, bool enable_16bit_types,
        bool alternate_ep, const char *hlsl, ID3D10Blob **blob_out)
{
    const WCHAR *entry_point = NULL;
    struct vkd3d_shader_code blob;
    WCHAR wprofile[7];
    HRESULT hr;

    *blob_out = NULL;

    if (alternate_ep)
    {
        if (*profile == 'h')
            entry_point = L"/Ehs_main";
        else if (*profile == 'd')
            entry_point = L"/Eds_main";
    }

    swprintf(wprofile, ARRAY_SIZE(wprofile), L"%hs", profile);
    if (FAILED(hr = dxc_compile(dxc_compiler, wprofile, compile_options,
            entry_point, enable_16bit_types, hlsl, &blob)))
        return hr;

    hr = d3d10_blob_from_vkd3d_shader_code(&blob, blob_out);
    free((void *)blob.code);

    return hr;
}

#endif
