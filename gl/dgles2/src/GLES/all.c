/* Copyright (C) 2010  Samsung Electronics All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "hgl.h"

#define DOUBLE_TO_FIXED(a) ((GLfixed)((a)*65536.0))
#define FIXED_TO_DOUBLE(a) (((double)(a)) * (1.0/65536.0))
#define FIXED_TO_FLOAT(a)  (((float)(a)) * (1.0/65536.0))
// not really part of GLES1
#define GL_IMPLEMENTATION_COLOR_READ_TYPE GL_IMPLEMENTATION_COLOR_READ_TYPE_OES
#define GL_IMPLEMENTATION_COLOR_READ_FORMAT 0x8B9B
#define GL_SHADER_COMPILER 0x8DFA
#define GL_NUM_SHADER_BINARY_FORMATS 0x8DF9
#define GL_MAX_VERTEX_UNIFORM_VECTORS       0x8DFB
#define GL_MAX_VARYING_VECTORS              0x8DFC
#define GL_MAX_FRAGMENT_UNIFORM_VECTORS     0x8DFD

#include "common-gles.h"

GL_APICALL_BUILD void GL_APIENTRY glAlphaFunc (GLenum func, GLclampf ref)
{
  HGL_TLS.AlphaFunc(func, ref);
}

GL_APICALL_BUILD void GL_APIENTRY glClipPlanef (GLenum plane, const GLfloat *equation)
{
  GLdouble p[4] = {
      equation[0],
      equation[1],
      equation[2],
      equation[3]
  };

  HGL_TLS.ClipPlane(plane, p);
}

GL_APICALL_BUILD void GL_APIENTRY glColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
  HGL_TLS.Color4f(red, green, blue, alpha);
}

GL_APICALL_BUILD void GL_APIENTRY glFogf (GLenum pname, GLfloat param)
{
  HGL_TLS.Fogf(pname, param);
}

GL_APICALL_BUILD void GL_APIENTRY glFogfv (GLenum pname, const GLfloat *params)
{
  HGL_TLS.Fogfv(pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glFrustumf (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
  HGL_TLS.Frustum(left, right, bottom, top, zNear, zFar);
}

GL_APICALL_BUILD void GL_APIENTRY glGetClipPlanef (GLenum pname, GLfloat eqn[4])
{
  GLdouble ret[4] = {0};

  HGL_TLS.GetClipPlane(pname, ret);

  int i = 0;
  for(i = 0; i < 4; i ++)
    eqn[i] = (GLfloat) (ret[i]);
}

GL_APICALL_BUILD void GL_APIENTRY glGetLightfv (GLenum light, GLenum pname, GLfloat *params)
{
  HGL_TLS.GetLightfv(light, pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glGetMaterialfv (GLenum face, GLenum pname, GLfloat *params)
{
  HGL_TLS.GetMaterialfv(face, pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glGetTexEnvfv (GLenum env, GLenum pname, GLfloat *params)
{
  HGL_TLS.GetTexEnvfv(env, pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glLightModelf (GLenum pname, GLfloat param)
{
  HGL_TLS.LightModelf(pname, param);
}

GL_APICALL_BUILD void GL_APIENTRY glLightModelfv (GLenum pname, const GLfloat *params)
{
  HGL_TLS.LightModelfv(pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glLightf (GLenum light, GLenum pname, GLfloat param)
{
  HGL_TLS.Lightf(light, pname, param);
}

GL_APICALL_BUILD void GL_APIENTRY glLightfv (GLenum light, GLenum pname, const GLfloat *params)
{
  HGL_TLS.Lightfv(light, pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glLoadMatrixf (const GLfloat *m)
{
  HGL_TLS.LoadMatrixf(m);
}

GL_APICALL_BUILD void GL_APIENTRY glMaterialf (GLenum face, GLenum pname, GLfloat param)
{
  HGL_TLS.Materialf(face, pname, param);
}

GL_APICALL_BUILD void GL_APIENTRY glMaterialfv (GLenum face, GLenum pname, const GLfloat *params)
{
  HGL_TLS.Materialfv(face, pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glMultMatrixf (const GLfloat *m)
{
  HGL_TLS.MultMatrixf(m);
}

GL_APICALL_BUILD void GL_APIENTRY glMultiTexCoord4f (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
  HGL_TLS.MultiTexCoord4f(target, s, t, r, q);
}

GL_APICALL_BUILD void GL_APIENTRY glNormal3f (GLfloat nx, GLfloat ny, GLfloat nz)
{
  HGL_TLS.Normal3f(nx, ny, nz);
}

GL_APICALL_BUILD void GL_APIENTRY glOrthof (GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar)
{
  HGL_TLS.Ortho(left, right, bottom, top, zNear, zFar);
}

GL_APICALL_BUILD void GL_APIENTRY glPointParameterf (GLenum pname, GLfloat param)
{
  HGL_TLS.PointParameterf(pname, param);
}

GL_APICALL_BUILD void GL_APIENTRY glPointParameterfv (GLenum pname, const GLfloat *params)
{
  HGL_TLS.PointParameterfv(pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glPointSize (GLfloat size)
{
  HGL_TLS.PointSize(size);
}

GL_APICALL_BUILD void GL_APIENTRY glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
  HGL_TLS.Rotatef(angle, x, y, z);
}

GL_APICALL_BUILD void GL_APIENTRY glScalef (GLfloat x, GLfloat y, GLfloat z)
{
  HGL_TLS.Scalef(x, y, z);
}

GL_APICALL_BUILD void GL_APIENTRY glTexEnvf (GLenum target, GLenum pname, GLfloat param)
{
  HGL_TLS.TexEnvf(target, pname, param);
}

GL_APICALL_BUILD void GL_APIENTRY glTexEnvfv (GLenum target, GLenum pname, const GLfloat *params)
{
  HGL_TLS.TexEnvfv(target, pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glTranslatef (GLfloat x, GLfloat y, GLfloat z)
{
  HGL_TLS.Translatef(x, y, z);
}

GL_APICALL_BUILD void GL_APIENTRY glAlphaFuncx (GLenum func, GLclampx ref)
{
  HGL_TLS.AlphaFunc(func, FIXED_TO_FLOAT(ref));
}

GL_APICALL_BUILD void GL_APIENTRY glClearColorx (GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha)
{
  GLfloat r = FIXED_TO_FLOAT(red);
  GLfloat g = FIXED_TO_FLOAT(green);
  GLfloat b = FIXED_TO_FLOAT(blue);
  GLfloat a = FIXED_TO_FLOAT(alpha);

  HGL_TLS.ClearColor(r, g, b, a);
}

GL_APICALL_BUILD void GL_APIENTRY glClearDepthx (GLclampx depth)
{
  HGL_TLS.ClearDepth(FIXED_TO_FLOAT(depth));
}

GL_APICALL_BUILD void GL_APIENTRY glClientActiveTexture (GLenum texture)
{
  HGL_TLS.ClientActiveTexture(texture);
}

GL_APICALL_BUILD void GL_APIENTRY glClipPlanex (GLenum plane, const GLfixed *equation)
{
  GLdouble p[4] = {
      FIXED_TO_DOUBLE(equation[0]),
      FIXED_TO_DOUBLE(equation[1]),
      FIXED_TO_DOUBLE(equation[2]),
      FIXED_TO_DOUBLE(equation[3])
  };

  HGL_TLS.ClipPlane(plane, p);
}

GL_APICALL_BUILD void GL_APIENTRY glColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
  HGL_TLS.Color4ub(red, green, blue, alpha);
}

GL_APICALL_BUILD void GL_APIENTRY glColor4x (GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha)
{
  GLfloat r = FIXED_TO_FLOAT(red);
  GLfloat g = FIXED_TO_FLOAT(green);
  GLfloat b = FIXED_TO_FLOAT(blue);
  GLfloat a = FIXED_TO_FLOAT(alpha);

  HGL_TLS.Color4f(r, g, b, a);
}

GL_APICALL_BUILD void GL_APIENTRY glDepthRangex (GLclampx zNear, GLclampx zFar)
{
  HGL_TLS.DepthRange(FIXED_TO_FLOAT(zNear), FIXED_TO_FLOAT(zFar));
}

GL_APICALL_BUILD void GL_APIENTRY glFogx (GLenum pname, GLfixed param)
{
  HGL_TLS.Fogf(pname, FIXED_TO_FLOAT(param));
}

GL_APICALL_BUILD void GL_APIENTRY glFogxv (GLenum pname, const GLfixed *params)
{
  GLfloat fparams[] = {
      FIXED_TO_FLOAT(params[0]),
      FIXED_TO_FLOAT(params[1]),
      FIXED_TO_FLOAT(params[2]),
      FIXED_TO_FLOAT(params[3])
  };
  HGL_TLS.Fogfv(pname, fparams);
}

GL_APICALL_BUILD void GL_APIENTRY glFrustumx (GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar)
{
  GLfloat l = FIXED_TO_FLOAT(left);
  GLfloat r = FIXED_TO_FLOAT(right);
  GLfloat b = FIXED_TO_FLOAT(bottom);
  GLfloat t = FIXED_TO_FLOAT(top);
  GLfloat n = FIXED_TO_FLOAT(zNear);
  GLfloat f = FIXED_TO_FLOAT(zFar);

  HGL_TLS.Frustum(l, r, b, t, n, f);
}

GL_APICALL_BUILD void GL_APIENTRY glGetClipPlanex (GLenum pname, GLfixed eqn[4])
{
  GLdouble ret [4];
  HGL_TLS.GetClipPlane(pname, ret);

  int i = 0;
  for(i = 0; i < 4; i ++)
    eqn[i] = DOUBLE_TO_FIXED(ret[i]);
}

GL_APICALL_BUILD void GL_APIENTRY glGetFixedv (GLenum pname, GLfixed *params)
{
  GLdouble res[16] = {0};
  int size = 1;
  //TODO: drop these tests as qemu always gives as 16 sized params?
  //      alternatively find a way to share the size checking code, this is ugly

  switch (pname)
  {
//    case GL_ACCUM_RED_BITS:
//    case GL_ACCUM_GREEN_BITS:
//    case GL_ACCUM_BLUE_BITS:
//    case GL_ACCUM_ALPHA_BITS:
//    case GL_ALPHA_BIAS:
    case GL_ALPHA_BITS:
//    case GL_ALPHA_SCALE:
    case GL_ALPHA_TEST:
    case GL_ALPHA_TEST_FUNC:
    case GL_ALPHA_TEST_REF:
//    case GL_ATTRIB_STACK_DEPTH:
//    case GL_AUTO_NORMAL:
//    case GL_AUX_BUFFERS:
    case GL_BLEND:
    case GL_BLEND_DST:
    case GL_BLEND_SRC:
//    case GL_BLEND_SRC_RGB_EXT:
//    case GL_BLEND_DST_RGB_EXT:
//    case GL_BLEND_SRC_ALPHA_EXT:
//    case GL_BLEND_DST_ALPHA_EXT:
//    case GL_BLEND_EQUATION:
//    case GL_BLEND_EQUATION_ALPHA_EXT:
//    case GL_BLUE_BIAS:
    case GL_BLUE_BITS:
//    case GL_BLUE_SCALE:
//    case GL_CLIENT_ATTRIB_STACK_DEPTH:
    case GL_CLIP_PLANE0:
    case GL_CLIP_PLANE1:
    case GL_CLIP_PLANE2:
    case GL_CLIP_PLANE3:
    case GL_CLIP_PLANE4:
    case GL_CLIP_PLANE5:
    case GL_COLOR_MATERIAL:
//    case GL_COLOR_MATERIAL_FACE:
//    case GL_COLOR_MATERIAL_PARAMETER:
    case GL_CULL_FACE:
    case GL_CULL_FACE_MODE:
//    case GL_CURRENT_INDEX:
//    case GL_CURRENT_RASTER_DISTANCE:
//    case GL_CURRENT_RASTER_INDEX:
//    case GL_CURRENT_RASTER_POSITION_VALID:
//    case GL_DEPTH_BIAS:
    case GL_DEPTH_BITS:
    case GL_DEPTH_CLEAR_VALUE:
    case GL_DEPTH_FUNC:
//    case GL_DEPTH_SCALE:
    case GL_DEPTH_TEST:
    case GL_DEPTH_WRITEMASK:
    case GL_DITHER:
//    case GL_DOUBLEBUFFER:
//    case GL_DRAW_BUFFER:
//    case GL_EDGE_FLAG:
//    case GL_FEEDBACK_BUFFER_SIZE:
//    case GL_FEEDBACK_BUFFER_TYPE:
    case GL_FOG:
    case GL_FOG_DENSITY:
    case GL_FOG_END:
    case GL_FOG_HINT:
//    case GL_FOG_INDEX:
    case GL_FOG_MODE:
    case GL_FOG_START:
    case GL_FRONT_FACE:
//    case GL_GREEN_BIAS:
    case GL_GREEN_BITS:
//    case GL_GREEN_SCALE:
//    case GL_INDEX_BITS:
//    case GL_INDEX_CLEAR_VALUE:
//    case GL_INDEX_MODE:
//    case GL_INDEX_OFFSET:
//    case GL_INDEX_SHIFT:
//    case GL_INDEX_WRITEMASK:
    case GL_LIGHT0:
    case GL_LIGHT1:
    case GL_LIGHT2:
    case GL_LIGHT3:
    case GL_LIGHT4:
    case GL_LIGHT5:
    case GL_LIGHT6:
    case GL_LIGHT7:
    case GL_LIGHTING:
//    case GL_LIGHT_MODEL_COLOR_CONTROL:
//    case GL_LIGHT_MODEL_LOCAL_VIEWER:
    case GL_LIGHT_MODEL_TWO_SIDE:
    case GL_LINE_SMOOTH:
    case GL_LINE_SMOOTH_HINT:
//    case GL_LINE_STIPPLE:
//    case GL_LINE_STIPPLE_PATTERN:
//    case GL_LINE_STIPPLE_REPEAT:
    case GL_LINE_WIDTH:
//    case GL_LINE_WIDTH_GRANULARITY:
//    case GL_LIST_BASE:
//    case GL_LIST_INDEX:
//    case GL_LIST_MODE:
//    case GL_INDEX_LOGIC_OP:
    case GL_COLOR_LOGIC_OP:
    case GL_LOGIC_OP_MODE:
//    case GL_MAP1_COLOR_4:
//    case GL_MAP1_GRID_SEGMENTS:
//    case GL_MAP1_INDEX:
//    case GL_MAP1_NORMAL:
//    case GL_MAP1_TEXTURE_COORD_1:
//    case GL_MAP1_TEXTURE_COORD_2:
//    case GL_MAP1_TEXTURE_COORD_3:
//    case GL_MAP1_TEXTURE_COORD_4:
//    case GL_MAP1_VERTEX_3:
//    case GL_MAP1_VERTEX_4:
//    case GL_MAP2_COLOR_4:
//    case GL_MAP2_INDEX:
//    case GL_MAP2_NORMAL:
//    case GL_MAP2_TEXTURE_COORD_1:
//    case GL_MAP2_TEXTURE_COORD_2:
//    case GL_MAP2_TEXTURE_COORD_3:
//    case GL_MAP2_TEXTURE_COORD_4:
//    case GL_MAP2_VERTEX_3:
//    case GL_MAP2_VERTEX_4:
//    case GL_MAP_COLOR:
//    case GL_MAP_STENCIL:
    case GL_MATRIX_MODE:
//    case GL_MAX_ATTRIB_STACK_DEPTH:
//    case GL_MAX_CLIENT_ATTRIB_STACK_DEPTH:
    case GL_MAX_CLIP_PLANES:
//    case GL_MAX_ELEMENTS_VERTICES:
//    case GL_MAX_ELEMENTS_INDICES:
//    case GL_MAX_EVAL_ORDER:
    case GL_MAX_LIGHTS:
//    case GL_MAX_LIST_NESTING:
    case GL_MAX_MODELVIEW_STACK_DEPTH:
//    case GL_MAX_NAME_STACK_DEPTH:
//    case GL_MAX_PIXEL_MAP_TABLE:
    case GL_MAX_PROJECTION_STACK_DEPTH:
    case GL_MAX_TEXTURE_SIZE:
//    case GL_MAX_3D_TEXTURE_SIZE:
    case GL_MAX_TEXTURE_STACK_DEPTH:
    case GL_MODELVIEW_STACK_DEPTH:
//    case GL_NAME_STACK_DEPTH:
    case GL_NORMALIZE:
    case GL_PACK_ALIGNMENT:
//    case GL_PACK_LSB_FIRST:
//    case GL_PACK_ROW_LENGTH:
//    case GL_PACK_SKIP_PIXELS:
//    case GL_PACK_SKIP_ROWS:
//    case GL_PACK_SWAP_BYTES:
//    case GL_PACK_SKIP_IMAGES_EXT:
//    case GL_PACK_IMAGE_HEIGHT_EXT:
//    case GL_PACK_INVERT_MESA:
    case GL_PERSPECTIVE_CORRECTION_HINT:
//    case GL_PIXEL_MAP_A_TO_A_SIZE:
//    case GL_PIXEL_MAP_B_TO_B_SIZE:
//    case GL_PIXEL_MAP_G_TO_G_SIZE:
//    case GL_PIXEL_MAP_I_TO_A_SIZE:
//    case GL_PIXEL_MAP_I_TO_B_SIZE:
//    case GL_PIXEL_MAP_I_TO_G_SIZE:
//    case GL_PIXEL_MAP_I_TO_I_SIZE:
//    case GL_PIXEL_MAP_I_TO_R_SIZE:
//    case GL_PIXEL_MAP_R_TO_R_SIZE:
//    case GL_PIXEL_MAP_S_TO_S_SIZE:
    case GL_POINT_SIZE:
//    case GL_POINT_SIZE_GRANULARITY:
    case GL_POINT_SMOOTH:
    case GL_POINT_SMOOTH_HINT:
//    case GL_POINT_SIZE_MIN_EXT:
//    case GL_POINT_SIZE_MAX_EXT:
//    case GL_POINT_FADE_THRESHOLD_SIZE_EXT:
//    case GL_POLYGON_OFFSET_BIAS_EXT:
    case GL_POLYGON_OFFSET_FACTOR:
    case GL_POLYGON_OFFSET_UNITS:
//    case GL_POLYGON_OFFSET_POINT:
//    case GL_POLYGON_OFFSET_LINE:
    case GL_POLYGON_OFFSET_FILL:
//    case GL_POLYGON_SMOOTH:
//    case GL_POLYGON_SMOOTH_HINT:
//    case GL_POLYGON_STIPPLE:
    case GL_PROJECTION_STACK_DEPTH:
//    case GL_READ_BUFFER:
//    case GL_RED_BIAS:
    case GL_RED_BITS:
//    case GL_RED_SCALE:
//    case GL_RENDER_MODE:
    case GL_RESCALE_NORMAL:
//    case GL_RGBA_MODE:
    case GL_SCISSOR_TEST:
//    case GL_SELECTION_BUFFER_SIZE:
    case GL_SHADE_MODEL:
//    case GL_SHARED_TEXTURE_PALETTE_EXT:
    case GL_STENCIL_BITS:
    case GL_STENCIL_CLEAR_VALUE:
    case GL_STENCIL_FAIL:
    case GL_STENCIL_FUNC:
    case GL_STENCIL_PASS_DEPTH_FAIL:
    case GL_STENCIL_PASS_DEPTH_PASS:
    case GL_STENCIL_REF:
    case GL_STENCIL_TEST:
    case GL_STENCIL_VALUE_MASK:
    case GL_STENCIL_WRITEMASK:
//    case GL_STEREO:
    case GL_SUBPIXEL_BITS:
//    case GL_TEXTURE_1D:
    case GL_TEXTURE_2D:
//    case GL_TEXTURE_3D:
//    case GL_TEXTURE_1D_ARRAY_EXT:
//    case GL_TEXTURE_2D_ARRAY_EXT:
//    case GL_TEXTURE_BINDING_1D:
    case GL_TEXTURE_BINDING_2D:
//    case GL_TEXTURE_BINDING_3D:
//    case GL_TEXTURE_BINDING_1D_ARRAY_EXT:
//    case GL_TEXTURE_BINDING_2D_ARRAY_EXT:
//    case GL_TEXTURE_GEN_S:
//    case GL_TEXTURE_GEN_T:
//    case GL_TEXTURE_GEN_R:
//    case GL_TEXTURE_GEN_Q:
    case GL_TEXTURE_STACK_DEPTH:
    case GL_UNPACK_ALIGNMENT:
//    case GL_UNPACK_LSB_FIRST:
//    case GL_UNPACK_ROW_LENGTH:
//    case GL_UNPACK_SKIP_PIXELS:
//    case GL_UNPACK_SKIP_ROWS:
//    case GL_UNPACK_SWAP_BYTES:
//    case GL_UNPACK_SKIP_IMAGES_EXT:
//    case GL_UNPACK_IMAGE_HEIGHT_EXT:
//    case GL_UNPACK_CLIENT_STORAGE_APPLE:
//    case GL_ZOOM_X:
//    case GL_ZOOM_Y:
    case GL_VERTEX_ARRAY:
    case GL_VERTEX_ARRAY_SIZE:
    case GL_VERTEX_ARRAY_TYPE:
    case GL_VERTEX_ARRAY_STRIDE:
//    case GL_VERTEX_ARRAY_COUNT_EXT:
    case GL_NORMAL_ARRAY:
    case GL_NORMAL_ARRAY_TYPE:
    case GL_NORMAL_ARRAY_STRIDE:
//    case GL_NORMAL_ARRAY_COUNT_EXT:
    case GL_COLOR_ARRAY:
    case GL_COLOR_ARRAY_SIZE:
    case GL_COLOR_ARRAY_TYPE:
    case GL_COLOR_ARRAY_STRIDE:
//    case GL_COLOR_ARRAY_COUNT_EXT:
//    case GL_INDEX_ARRAY:
//    case GL_INDEX_ARRAY_TYPE:
//    case GL_INDEX_ARRAY_STRIDE:
//    case GL_INDEX_ARRAY_COUNT_EXT:
    case GL_TEXTURE_COORD_ARRAY:
    case GL_TEXTURE_COORD_ARRAY_SIZE:
    case GL_TEXTURE_COORD_ARRAY_TYPE:
    case GL_TEXTURE_COORD_ARRAY_STRIDE:
//    case GL_TEXTURE_COORD_ARRAY_COUNT_EXT:
//    case GL_EDGE_FLAG_ARRAY:
//    case GL_EDGE_FLAG_ARRAY_STRIDE:
//    case GL_EDGE_FLAG_ARRAY_COUNT_EXT:
    case GL_MAX_TEXTURE_UNITS:
    case GL_ACTIVE_TEXTURE:
    case GL_CLIENT_ACTIVE_TEXTURE:
//    case GL_TEXTURE_CUBE_MAP_ARB:
//    case GL_TEXTURE_BINDING_CUBE_MAP_ARB:
//    case GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB:
//    case GL_TEXTURE_COMPRESSION_HINT_ARB:
//    case GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB:
    case GL_COMPRESSED_TEXTURE_FORMATS:
//    case GL_ARRAY_ELEMENT_LOCK_FIRST_EXT:
//    case GL_ARRAY_ELEMENT_LOCK_COUNT_EXT:
//    case GL_COLOR_MATRIX_STACK_DEPTH_SGI:
//    case GL_MAX_COLOR_MATRIX_STACK_DEPTH_SGI:
//    case GL_POST_COLOR_MATRIX_RED_SCALE_SGI:
//    case GL_POST_COLOR_MATRIX_GREEN_SCALE_SGI:
//    case GL_POST_COLOR_MATRIX_BLUE_SCALE_SGI:
//    case GL_POST_COLOR_MATRIX_ALPHA_SCALE_SGI:
//    case GL_POST_COLOR_MATRIX_RED_BIAS_SGI:
//    case GL_POST_COLOR_MATRIX_GREEN_BIAS_SGI:
//    case GL_POST_COLOR_MATRIX_BLUE_BIAS_SGI:
//    case GL_POST_COLOR_MATRIX_ALPHA_BIAS_SGI:
//    case GL_CONVOLUTION_1D_EXT:
//    case GL_CONVOLUTION_2D_EXT:
//    case GL_SEPARABLE_2D_EXT:
//    case GL_POST_CONVOLUTION_RED_SCALE_EXT:
//    case GL_POST_CONVOLUTION_GREEN_SCALE_EXT:
//    case GL_POST_CONVOLUTION_BLUE_SCALE_EXT:
//    case GL_POST_CONVOLUTION_ALPHA_SCALE_EXT:
//    case GL_POST_CONVOLUTION_RED_BIAS_EXT:
//    case GL_POST_CONVOLUTION_GREEN_BIAS_EXT:
//    case GL_POST_CONVOLUTION_BLUE_BIAS_EXT:
//    case GL_POST_CONVOLUTION_ALPHA_BIAS_EXT:
//    case GL_HISTOGRAM:
//    case GL_MINMAX:
//    case GL_COLOR_TABLE_SGI:
//    case GL_POST_CONVOLUTION_COLOR_TABLE_SGI:
//    case GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI:
//    case GL_TEXTURE_COLOR_TABLE_SGI:
//    case GL_COLOR_SUM_EXT:
//    case GL_SECONDARY_COLOR_ARRAY_EXT:
//    case GL_SECONDARY_COLOR_ARRAY_TYPE_EXT:
//    case GL_SECONDARY_COLOR_ARRAY_STRIDE_EXT:
//    case GL_SECONDARY_COLOR_ARRAY_SIZE_EXT:
//    case GL_CURRENT_FOG_COORDINATE_EXT:
//    case GL_FOG_COORDINATE_ARRAY_EXT:
//    case GL_FOG_COORDINATE_ARRAY_TYPE_EXT:
//    case GL_FOG_COORDINATE_ARRAY_STRIDE_EXT:
//    case GL_FOG_COORDINATE_SOURCE_EXT:
//    case GL_MAX_TEXTURE_LOD_BIAS_EXT:
//    case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT:
    case GL_MULTISAMPLE:
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
    case GL_SAMPLE_ALPHA_TO_ONE:
    case GL_SAMPLE_COVERAGE:
    case GL_SAMPLE_COVERAGE_VALUE:
    case GL_SAMPLE_COVERAGE_INVERT:
    case GL_SAMPLE_BUFFERS:
    case GL_SAMPLES:
//    case GL_RASTER_POSITION_UNCLIPPED_IBM:
    case GL_POINT_SPRITE_OES:
//    case GL_POINT_SPRITE_R_MODE_NV:
//    case GL_POINT_SPRITE_COORD_ORIGIN:
//    case GL_GENERATE_MIPMAP_HINT_SGIS:
//    case GL_VERTEX_PROGRAM_BINDING_NV:
//    case GL_VERTEX_ATTRIB_ARRAY0_NV:
//    case GL_VERTEX_ATTRIB_ARRAY1_NV:
//    case GL_VERTEX_ATTRIB_ARRAY2_NV:
//    case GL_VERTEX_ATTRIB_ARRAY3_NV:
//    case GL_VERTEX_ATTRIB_ARRAY4_NV:
//    case GL_VERTEX_ATTRIB_ARRAY5_NV:
//    case GL_VERTEX_ATTRIB_ARRAY6_NV:
//    case GL_VERTEX_ATTRIB_ARRAY7_NV:
//    case GL_VERTEX_ATTRIB_ARRAY8_NV:
//    case GL_VERTEX_ATTRIB_ARRAY9_NV:
//    case GL_VERTEX_ATTRIB_ARRAY10_NV:
//    case GL_VERTEX_ATTRIB_ARRAY11_NV:
//    case GL_VERTEX_ATTRIB_ARRAY12_NV:
//    case GL_VERTEX_ATTRIB_ARRAY13_NV:
//    case GL_VERTEX_ATTRIB_ARRAY14_NV:
//    case GL_VERTEX_ATTRIB_ARRAY15_NV:
//    case GL_MAP1_VERTEX_ATTRIB0_4_NV:
//    case GL_MAP1_VERTEX_ATTRIB1_4_NV:
//    case GL_MAP1_VERTEX_ATTRIB2_4_NV:
//    case GL_MAP1_VERTEX_ATTRIB3_4_NV:
//    case GL_MAP1_VERTEX_ATTRIB4_4_NV:
//    case GL_MAP1_VERTEX_ATTRIB5_4_NV:
//    case GL_MAP1_VERTEX_ATTRIB6_4_NV:
//    case GL_MAP1_VERTEX_ATTRIB7_4_NV:
//    case GL_MAP1_VERTEX_ATTRIB8_4_NV:
//    case GL_MAP1_VERTEX_ATTRIB9_4_NV:
//    case GL_MAP1_VERTEX_ATTRIB10_4_NV:
//    case GL_MAP1_VERTEX_ATTRIB11_4_NV:
//    case GL_MAP1_VERTEX_ATTRIB12_4_NV:
//    case GL_MAP1_VERTEX_ATTRIB13_4_NV:
//    case GL_MAP1_VERTEX_ATTRIB14_4_NV:
//    case GL_MAP1_VERTEX_ATTRIB15_4_NV:
//    case GL_FRAGMENT_PROGRAM_NV:
//    case GL_FRAGMENT_PROGRAM_BINDING_NV:
//    case GL_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMETERS_NV:
//    case GL_TEXTURE_RECTANGLE_NV:
//    case GL_TEXTURE_BINDING_RECTANGLE_NV:
//    case GL_MAX_RECTANGLE_TEXTURE_SIZE_NV:
//    case GL_STENCIL_TEST_TWO_SIDE_EXT:
//    case GL_ACTIVE_STENCIL_FACE_EXT:
//    case GL_MAX_SHININESS_NV:
//    case GL_MAX_SPOT_EXPONENT_NV:
    case GL_ARRAY_BUFFER_BINDING:
    case GL_VERTEX_ARRAY_BUFFER_BINDING:
    case GL_NORMAL_ARRAY_BUFFER_BINDING:
    case GL_COLOR_ARRAY_BUFFER_BINDING:
//    case GL_INDEX_ARRAY_BUFFER_BINDING_ARB:
    case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING:
//    case GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB:
//    case GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB:
//    case GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB:
    case GL_ELEMENT_ARRAY_BUFFER_BINDING:
//    case GL_PIXEL_PACK_BUFFER_BINDING_EXT:
//    case GL_PIXEL_UNPACK_BUFFER_BINDING_EXT:
//    case GL_VERTEX_PROGRAM_ARB:
//    case GL_VERTEX_PROGRAM_POINT_SIZE_ARB:
//    case GL_VERTEX_PROGRAM_TWO_SIDE_ARB:
//    case GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB:
//    case GL_MAX_PROGRAM_MATRICES_ARB:
//    case GL_CURRENT_MATRIX_STACK_DEPTH_ARB:
//    case GL_MAX_VERTEX_ATTRIBS_ARB:
//    case GL_PROGRAM_ERROR_POSITION_ARB:
//    case GL_FRAGMENT_PROGRAM_ARB:
//    case GL_MAX_TEXTURE_COORDS_ARB:
//    case GL_MAX_TEXTURE_IMAGE_UNITS_ARB:
//    case GL_DEPTH_BOUNDS_TEST_EXT:
//    case GL_DEPTH_CLAMP:
//    case GL_MAX_DRAW_BUFFERS_ARB:
//    case GL_DRAW_BUFFER0_ARB:
//    case GL_DRAW_BUFFER1_ARB:
//    case GL_DRAW_BUFFER2_ARB:
//    case GL_DRAW_BUFFER3_ARB:
    case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
    case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
//    case GL_NUM_FRAGMENT_REGISTERS_ATI:
//    case GL_NUM_FRAGMENT_CONSTANTS_ATI:
//    case GL_NUM_PASSES_ATI:
//    case GL_NUM_INSTRUCTIONS_PER_PASS_ATI:
//    case GL_NUM_INSTRUCTIONS_TOTAL_ATI:
//    case GL_COLOR_ALPHA_PAIRING_ATI:
//    case GL_NUM_LOOPBACK_COMPONENTS_ATI:
//    case GL_NUM_INPUT_INTERPOLATOR_COMPONENTS_ATI:
//    case GL_STENCIL_BACK_FUNC:
//    case GL_STENCIL_BACK_VALUE_MASK:
//    case GL_STENCIL_BACK_WRITEMASK:
//    case GL_STENCIL_BACK_REF:
//    case GL_STENCIL_BACK_FAIL:
//    case GL_STENCIL_BACK_PASS_DEPTH_FAIL:
//    case GL_STENCIL_BACK_PASS_DEPTH_PASS:
//    case GL_FRAMEBUFFER_BINDING_EXT:
//    case GL_RENDERBUFFER_BINDING_EXT:
//    case GL_MAX_COLOR_ATTACHMENTS_EXT:
//    case GL_MAX_RENDERBUFFER_SIZE_EXT:
//    case GL_READ_FRAMEBUFFER_BINDING_EXT:
//    case GL_PROVOKING_VERTEX_EXT:
//    case GL_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION_EXT:
//    case GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB:
//    case GL_FRAGMENT_SHADER_DERIVATIVE_HINT_ARB:
//    case GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB:
//    case GL_MAX_VARYING_FLOATS_ARB:
//    case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB:
//    case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB:
//    case GL_CURRENT_PROGRAM:
//    case GL_MAX_SAMPLES:
//    case GL_VERTEX_ARRAY_BINDING_APPLE:
//    case GL_TEXTURE_CUBE_MAP_SEAMLESS:
//    case GL_MAX_SERVER_WAIT_TIMEOUT:
//    case GL_NUM_EXTENSIONS:
//    case GL_MAJOR_VERSION:
//    case GL_MINOR_VERSION:
      size = 1;
      break;
    case GL_DEPTH_RANGE:
//    case GL_LINE_WIDTH_RANGE:
    case GL_ALIASED_LINE_WIDTH_RANGE:
//    case GL_MAP1_GRID_DOMAIN:
//    case GL_MAP2_GRID_SEGMENTS:
    case GL_MAX_VIEWPORT_DIMS:
//    case GL_POINT_SIZE_RANGE:
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_SMOOTH_LINE_WIDTH_RANGE:
    case GL_SMOOTH_POINT_SIZE_RANGE:
//    case GL_POLYGON_MODE:
//    case GL_DEPTH_BOUNDS_EXT:
      size = 2;
      break;
    case GL_CURRENT_NORMAL:
//    case GL_DISTANCE_ATTENUATION_EXT:
      size = 3;
      break;
//    case GL_ACCUM_CLEAR_VALUE:
//    case GL_BLEND_COLOR_EXT:
    case GL_COLOR_CLEAR_VALUE:
    case GL_COLOR_WRITEMASK:
    case GL_CURRENT_COLOR:
//    case GL_CURRENT_RASTER_COLOR:
//    case GL_CURRENT_RASTER_POSITION:
//    case GL_CURRENT_RASTER_SECONDARY_COLOR:
//    case GL_CURRENT_RASTER_TEXTURE_COORDS:
    case GL_CURRENT_TEXTURE_COORDS:
    case GL_FOG_COLOR:
    case GL_LIGHT_MODEL_AMBIENT:
//    case GL_MAP2_GRID_DOMAIN:
    case GL_SCISSOR_BOX:
    case GL_VIEWPORT:
//    case GL_CURRENT_SECONDARY_COLOR_EXT:
      size = 4;
      break;
    case GL_MODELVIEW_MATRIX:
    case GL_PROJECTION_MATRIX:
    case GL_TEXTURE_MATRIX:
//    case GL_TRANSPOSE_COLOR_MATRIX_ARB:
//    case GL_TRANSPOSE_MODELVIEW_MATRIX_ARB:
//    case GL_TRANSPOSE_PROJECTION_MATRIX_ARB:
//    case GL_TRANSPOSE_TEXTURE_MATRIX_ARB:
//    case GL_COLOR_MATRIX_SGI:
//    case GL_CURRENT_MATRIX_ARB:
//    case GL_TRANSPOSE_CURRENT_MATRIX_ARB:
      size = 16;
      break;

    default:
      STUB("Unknown parameter name 0x%x!", pname);
      break;
  }

  int i = 0;
  HGL_TLS.GetDoublev(pname, res);
  for (i = 0; i < size; i ++)
    params[i] = DOUBLE_TO_FIXED(res[i]);
}

GL_APICALL_BUILD void GL_APIENTRY glGetLightxv (GLenum light, GLenum pname, GLfixed *params)
{
  GLfloat res[4] = {0};
  int size = 0;

  switch (pname)
  {
    case GL_SPOT_EXPONENT:
    case GL_SPOT_CUTOFF:
    case GL_CONSTANT_ATTENUATION:
    case GL_LINEAR_ATTENUATION:
    case GL_QUADRATIC_ATTENUATION:
      size = 1;
      break;

    case GL_SPOT_DIRECTION:
      size = 3;
      break;

    case GL_AMBIENT:
    case GL_DIFFUSE:
    case GL_SPECULAR:
    case GL_POSITION:
      size = 4;
      break;

    default:
      STUB("Unknown parameter name 0x%x!", pname);
      break;
  }

  int i = 0;
  HGL_TLS.GetLightfv(light, pname, res);
  for (i = 0; i < size; i ++)
    params[i] = DOUBLE_TO_FIXED(res[i]);
}

GL_APICALL_BUILD void GL_APIENTRY glGetMaterialxv (GLenum face, GLenum pname, GLfixed *params)
{
  GLfloat res[4] = {0};
  int size = 0;

  switch (pname)
  {
    case GL_SHININESS:
      size = 1;
      break;

//    case GL_COLOR_INDEXES:
      size = 3;
      break;

    case GL_AMBIENT:
    case GL_DIFFUSE:
    case GL_SPECULAR:
    case GL_EMISSION:
      size = 4;
      break;

    default:
      STUB("Unknown parameter name 0x%x!", pname);
      break;
  }

  int i = 0;
  HGL_TLS.Materialfv(face, pname, res);
  for (i = 0; i < size; i ++)
    params[i] = DOUBLE_TO_FIXED(res[i]);
}

GL_APICALL_BUILD void GL_APIENTRY glGetPointerv (GLenum pname, GLvoid **params)
{
  HGL_TLS.GetPointerv(pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glGetTexEnviv (GLenum env, GLenum pname, GLint *params)
{
  HGL_TLS.GetTexEnviv(env, pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glGetTexEnvxv (GLenum env, GLenum pname, GLfixed *params)
{
  GLfloat res[4] = {0};
  int size = 0;

  switch (pname)
  {
    case GL_SHININESS:
      size = 1;
      break;

//    case GL_COLOR_INDEXES:
      size = 3;
      break;

    case GL_AMBIENT:
    case GL_DIFFUSE:
    case GL_SPECULAR:
    case GL_EMISSION:
      size = 4;
      break;

    default:
      STUB("Unknown parameter name 0x%x!", pname);
      break;
  }

  int i = 0;
  HGL_TLS.GetTexEnvfv(env, pname, res);
  for (i = 0; i < size; i ++)
    params[i] = DOUBLE_TO_FIXED(res[i]);
}

GL_APICALL_BUILD void GL_APIENTRY glGetTexParameterxv (GLenum target, GLenum pname, GLfixed *params)
{
  GLfloat res[4] = {0};
  int size = 0;

  switch (pname)
  {
    case GL_TEXTURE_ENV_MODE:
//    case GL_TEXTURE_LOD_BIAS:
    case GL_COMBINE_RGB:
    case GL_COMBINE_ALPHA:
    case GL_SRC0_RGB:
    case GL_SRC1_RGB:
    case GL_SRC2_RGB:
    case GL_SRC0_ALPHA:
    case GL_SRC1_ALPHA:
    case GL_SRC2_ALPHA:
    case GL_OPERAND0_RGB:
    case GL_OPERAND1_RGB:
    case GL_OPERAND2_RGB:
    case GL_OPERAND0_ALPHA:
    case GL_OPERAND1_ALPHA:
    case GL_OPERAND2_ALPHA:
    case GL_RGB_SCALE:
    case GL_ALPHA_SCALE:
//    case GL_COORD_REPLACE:
      size = 1;
      break;

    case GL_TEXTURE_ENV_COLOR:
      size = 4;
      break;

    default:
      STUB("Unknown parameter name 0x%x!", pname);
      break;
  }

  int i = 0;
  HGL_TLS.GetTexParameterfv(target, pname, res);
  for (i = 0; i < size; i ++)
    params[i] = DOUBLE_TO_FIXED(res[i]);
}

GL_APICALL_BUILD void GL_APIENTRY glLightModelx (GLenum pname, GLfixed param)
{
  HGL_TLS.LightModelf(pname, FIXED_TO_FLOAT(param));
}

GL_APICALL_BUILD void GL_APIENTRY glLightModelxv (GLenum pname, const GLfixed *params)
{
  GLfloat p[4] = {0};
  int size = 0;

  switch (pname)
  {
//    case GL_LIGHT_MODEL_COLOR_CONTROL:
//    case GL_LIGHT_MODEL_LOCAL_VIEWER:
    case GL_LIGHT_MODEL_TWO_SIDE:
      size = 1;
      break;

    case GL_LIGHT_MODEL_AMBIENT:
      size = 4;
      break;

    default:
      STUB("Unknown parameter name 0x%x!", pname);
      break;
  }

  int i = 0;
  for (i = 0; i < size; i ++)
    p[i] = FIXED_TO_FLOAT(params[i]);

  HGL_TLS.LightModelfv(pname, p);
}

GL_APICALL_BUILD void GL_APIENTRY glLightx (GLenum light, GLenum pname, GLfixed param)
{
  HGL_TLS.Lightf(light, pname, FIXED_TO_FLOAT(param));
}

GL_APICALL_BUILD void GL_APIENTRY glLightxv (GLenum light, GLenum pname, const GLfixed *params)
{
  GLfloat p[4] = {0};
  int size = 0;

  switch (pname)
  {
    case GL_CONSTANT_ATTENUATION:
    case GL_LINEAR_ATTENUATION:
    case GL_QUADRATIC_ATTENUATION:
    case GL_SPOT_EXPONENT:
    case GL_SPOT_CUTOFF:
      size = 1;
      break;

    case GL_SPOT_DIRECTION:
      size = 3;
      break;


    case GL_AMBIENT:
    case GL_DIFFUSE:
    case GL_SPECULAR:
    case GL_POSITION:
      size = 4;
      break;

    default:
      STUB("Unknown parameter name 0x%x!", pname);
      break;
  }

  int i = 0;
  for (i = 0; i < size; i ++)
    p[i] = FIXED_TO_FLOAT(params[i]);

  HGL_TLS.Lightfv(light, pname, p);
}

GL_APICALL_BUILD void GL_APIENTRY glLineWidthx (GLfixed width)
{
  HGL_TLS.LineWidth(FIXED_TO_FLOAT(width));
}

GL_APICALL_BUILD void GL_APIENTRY glLoadIdentity (void)
{
  HGL_TLS.LoadIdentity();
}

GL_APICALL_BUILD void GL_APIENTRY glLoadMatrixx (const GLfixed *m)
{
  GLfloat p[16] = {0};
  for (int i = 0; i < 16; i ++)
    p[i] = FIXED_TO_FLOAT(m[i]);
  HGL_TLS.LoadMatrixf(p);
}

GL_APICALL_BUILD void GL_APIENTRY glLogicOp (GLenum opcode)
{
  HGL_TLS.LogicOp(opcode);
}

GL_APICALL_BUILD void GL_APIENTRY glMaterialx (GLenum face, GLenum pname, GLfixed param)
{
  HGL_TLS.Materialf(face, pname, FIXED_TO_FLOAT(param));
}

GL_APICALL_BUILD void GL_APIENTRY glMaterialxv (GLenum face, GLenum pname, const GLfixed *params)
{
  GLfloat p[4] = {0};
  int size = 0;

  switch (pname)
  {
    case GL_SHININESS:
      size = 1;
      break;

//    case GL_COLOR_INDEXES:
      size = 3;
      break;

    case GL_AMBIENT:
    case GL_DIFFUSE:
    case GL_SPECULAR:
    case GL_EMISSION:
    case GL_AMBIENT_AND_DIFFUSE:
      size = 4;
      break;

    default:
      STUB("Unknown parameter name 0x%x!", pname);
      break;
  }

  int i = 0;
  for (i = 0; i < size; i ++)
    p[i] = FIXED_TO_FLOAT(params[i]);

  HGL_TLS.Materialfv(face, pname, p);
}

GL_APICALL_BUILD void GL_APIENTRY glMatrixMode (GLenum mode)
{
  HGL_TLS.MatrixMode(mode);
}

GL_APICALL_BUILD void GL_APIENTRY glMultMatrixx (const GLfixed *m)
{
  GLdouble p[16] = {0};
  int i = 0;
  for (i = 0; i < 16; i ++)
    p[i] = FIXED_TO_DOUBLE(m[i]);
  HGL_TLS.MultMatrixd(p);
}

GL_APICALL_BUILD void GL_APIENTRY glMultiTexCoord4x (GLenum target, GLfixed s, GLfixed t, GLfixed r, GLfixed q)
{
  HGL_TLS.MultiTexCoord4d(target, FIXED_TO_DOUBLE(s), FIXED_TO_DOUBLE(t), FIXED_TO_DOUBLE(r), FIXED_TO_DOUBLE(q));
}

GL_APICALL_BUILD void GL_APIENTRY glNormal3x (GLfixed nx, GLfixed ny, GLfixed nz)
{
  HGL_TLS.Normal3f(FIXED_TO_FLOAT(nx), FIXED_TO_FLOAT(ny), FIXED_TO_FLOAT(nz));
}

GL_APICALL_BUILD void GL_APIENTRY glOrthox (GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar)
{
  GLfloat l = FIXED_TO_FLOAT(left);
  GLfloat r = FIXED_TO_FLOAT(right);
  GLfloat b = FIXED_TO_FLOAT(bottom);
  GLfloat t = FIXED_TO_FLOAT(top);
  GLfloat n = FIXED_TO_FLOAT(zNear);
  GLfloat f = FIXED_TO_FLOAT(zFar);
  HGL_TLS.Ortho(l, r, b, t, n, f);
}

GL_APICALL_BUILD void GL_APIENTRY glPointParameterx (GLenum pname, GLfixed param)
{
  HGL_TLS.PointParameterf(pname, FIXED_TO_FLOAT(param));
}

GL_APICALL_BUILD void GL_APIENTRY glPointParameterxv (GLenum pname, const GLfixed *params)
{
  GLfloat p[4] = {0};
  int size = 0;

  switch (pname)
  {
    case GL_POINT_SIZE_MIN:
    case GL_POINT_SIZE_MAX:
    case GL_POINT_FADE_THRESHOLD_SIZE:
//    case GL_POINT_SPRITE_COORD_ORIGIN:
      size = 1;
      break;

    case GL_POINT_DISTANCE_ATTENUATION:
      size = 3;
      break;

    default:
      STUB("Unknown parameter name 0x%x!", pname);
      break;
  }

  int i = 0;
  for (i = 0; i < size; i ++)
    p[i] = FIXED_TO_FLOAT(params[i]);

  HGL_TLS.PointParameterfv(pname, p);
}

GL_APICALL_BUILD void GL_APIENTRY glPointSizex (GLfixed size)
{
  HGL_TLS.PointSize(FIXED_TO_FLOAT(size));
}

GL_APICALL_BUILD void GL_APIENTRY glPolygonOffsetx (GLfixed factor, GLfixed units)
{
  HGL_TLS.PolygonOffset(FIXED_TO_FLOAT(factor), FIXED_TO_FLOAT(units));
}

GL_APICALL_BUILD void GL_APIENTRY glPopMatrix (void)
{
  HGL_TLS.PopMatrix();
}

GL_APICALL_BUILD void GL_APIENTRY glPushMatrix (void)
{
  HGL_TLS.PushMatrix();
}

GL_APICALL_BUILD void GL_APIENTRY glRotatex (GLfixed angle, GLfixed x, GLfixed y, GLfixed z)
{
  HGL_TLS.Rotatef(FIXED_TO_FLOAT(angle), FIXED_TO_FLOAT(x), FIXED_TO_FLOAT(y), FIXED_TO_FLOAT(z));
}

GL_APICALL_BUILD void GL_APIENTRY glSampleCoveragex (GLclampx value, GLboolean invert)
{
  HGL_TLS.SampleCoverage(FIXED_TO_FLOAT(value), invert);
}

GL_APICALL_BUILD void GL_APIENTRY glScalex (GLfixed x, GLfixed y, GLfixed z)
{
  HGL_TLS.Scalef(FIXED_TO_FLOAT(x), FIXED_TO_FLOAT(y), FIXED_TO_FLOAT(z));
}

GL_APICALL_BUILD void GL_APIENTRY glShadeModel (GLenum mode)
{
  HGL_TLS.ShadeModel(mode);
}

GL_APICALL_BUILD void GL_APIENTRY glTexEnvi (GLenum target, GLenum pname, GLint param)
{
  HGL_TLS.TexEnvi(target, pname, param);
}

GL_APICALL_BUILD void GL_APIENTRY glTexEnvx (GLenum target, GLenum pname, GLfixed param)
{
  HGL_TLS.TexEnvi(target, pname, param);
}

GL_APICALL_BUILD void GL_APIENTRY glTexEnviv (GLenum target, GLenum pname, const GLint *params)
{
  HGL_TLS.TexEnviv(target, pname, params);
}

GL_APICALL_BUILD void GL_APIENTRY glTexEnvxv (GLenum target, GLenum pname, const GLfixed *params)
{
  GLfloat p[4] = {0};
  int size = 0;

  switch (pname)
  {
    case GL_TEXTURE_ENV_MODE:
//    case GL_TEXTURE_LOD_BIAS:
      size = 1;
      break;

    case GL_TEXTURE_ENV_COLOR:
      size = 4;
      break;

    default:
      STUB("Unknown parameter name 0x%x!", pname);
      break;
  }

  int i = 0;
  for (i = 0; i < size; i ++)
    p[i] = FIXED_TO_FLOAT(params[i]);

  HGL_TLS.TexEnvfv(target, pname, p);
}

GL_APICALL_BUILD void GL_APIENTRY glTexParameterx (GLenum target, GLenum pname, GLfixed param)
{
  HGL_TLS.TexParameteri(target, pname, param);
}

GL_APICALL_BUILD void GL_APIENTRY glTexParameterxv (GLenum target, GLenum pname, const GLfixed *params)
{
  GLfloat p[4] = {0};
  int size = 0;

  switch (pname)
  {
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_MAG_FILTER:
//    case GL_TEXTURE_MIN_LOD:
//    case GL_TEXTURE_MAX_LOD:
//    case GL_TEXTURE_BASE_LEVEL:
//    case GL_TEXTURE_MAX_LEVEL:
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
//    case GL_TEXTURE_WRAP_R:
//    case GL_TEXTURE_PRIORITY:
//    case GL_TEXTURE_COMPARE_MODE:
//    case GL_TEXTURE_COMPARE_FUNC:
//    case GL_DEPTH_TEXTURE_MODE:
    case GL_GENERATE_MIPMAP:
      size = 1;
      break;

//    case GL_TEXTURE_BORDER_COLOR:
      size = 4;
      break;

    default:
      STUB("Unknown parameter name 0x%x!", pname);
      break;
  }

  int i = 0;
  for (i = 0; i < size; i ++)
    p[i] = FIXED_TO_FLOAT(params[i]);

  HGL_TLS.TexParameterfv(target, pname, p);
}

GL_APICALL_BUILD void GL_APIENTRY glTranslatex (GLfixed x, GLfixed y, GLfixed z)
{
  HGL_TLS.Translatef(FIXED_TO_FLOAT(x), FIXED_TO_FLOAT(y), FIXED_TO_FLOAT(z));
}
