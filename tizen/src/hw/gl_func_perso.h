/*
 *  Hand-implemented GL/GLX API
 * 
 *  Copyright (c) 2006,2007 Even Rouault
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

MAGIC_MACRO(_init32),
MAGIC_MACRO(_init64),
MAGIC_MACRO(_resize_surface),
MAGIC_MACRO(_render_surface),

/* When you add a glX call here, you HAVE TO update IS_GLX_CALL */
MAGIC_MACRO(glXChooseVisual),
MAGIC_MACRO(glXQueryExtensionsString),
MAGIC_MACRO(glXQueryServerString),
MAGIC_MACRO(glXCreateContext),
MAGIC_MACRO(glXCopyContext),
MAGIC_MACRO(glXDestroyContext),
MAGIC_MACRO(glXGetClientString),
MAGIC_MACRO(glXQueryVersion),
MAGIC_MACRO(glXMakeCurrent),
MAGIC_MACRO(glXGetConfig),
MAGIC_MACRO(glXGetConfig_extended),
MAGIC_MACRO(glXWaitGL),
MAGIC_MACRO(glXWaitX),
MAGIC_MACRO(glXGetFBConfigAttrib_extended),
MAGIC_MACRO(glXChooseFBConfig),
MAGIC_MACRO(glXGetFBConfigs),
MAGIC_MACRO(glXCreateNewContext),
MAGIC_MACRO(glXGetVisualFromFBConfig),
MAGIC_MACRO(glXGetFBConfigAttrib),
MAGIC_MACRO(glXQueryContext),
MAGIC_MACRO(glXQueryDrawable),
MAGIC_MACRO(glXUseXFont),
MAGIC_MACRO(glXIsDirect),
MAGIC_MACRO(glXGetProcAddress_fake),
MAGIC_MACRO(glXGetProcAddress_global_fake),
MAGIC_MACRO(glXSwapBuffers),
MAGIC_MACRO(glXQueryExtension),
MAGIC_MACRO(glXGetScreenDriver),
MAGIC_MACRO(glXGetDriverConfig),
MAGIC_MACRO(glXSwapIntervalSGI),

MAGIC_MACRO(glGetString),

MAGIC_MACRO(glShaderSourceARB_fake),
MAGIC_MACRO(glShaderSource_fake),
MAGIC_MACRO(glVertexPointer_fake),
MAGIC_MACRO(glNormalPointer_fake),
MAGIC_MACRO(glColorPointer_fake),
MAGIC_MACRO(glSecondaryColorPointer_fake),
MAGIC_MACRO(glIndexPointer_fake),
MAGIC_MACRO(glTexCoordPointer_fake),
MAGIC_MACRO(glEdgeFlagPointer_fake),
MAGIC_MACRO(glVertexAttribPointerARB_fake),
MAGIC_MACRO(glVertexAttribPointerNV_fake),
MAGIC_MACRO(glWeightPointerARB_fake),
MAGIC_MACRO(glMatrixIndexPointerARB_fake),
MAGIC_MACRO(glFogCoordPointer_fake),
MAGIC_MACRO(glVariantPointerEXT_fake),
MAGIC_MACRO(glInterleavedArrays_fake),
MAGIC_MACRO(glElementPointerATI_fake),
MAGIC_MACRO(glTuxRacerDrawElements_fake),
MAGIC_MACRO(glVertexAndNormalPointer_fake),
MAGIC_MACRO(glTexCoordPointer01_fake),
MAGIC_MACRO(glTexCoordPointer012_fake),
MAGIC_MACRO(glVertexNormalPointerInterlaced_fake),
MAGIC_MACRO(glVertexNormalColorPointerInterlaced_fake),
MAGIC_MACRO(glVertexColorTexCoord0PointerInterlaced_fake),
MAGIC_MACRO(glVertexNormalTexCoord0PointerInterlaced_fake),
MAGIC_MACRO(glVertexNormalTexCoord01PointerInterlaced_fake),
MAGIC_MACRO(glVertexNormalTexCoord012PointerInterlaced_fake),
MAGIC_MACRO(glVertexNormalColorTexCoord0PointerInterlaced_fake),
MAGIC_MACRO(glVertexNormalColorTexCoord01PointerInterlaced_fake),
MAGIC_MACRO(glVertexNormalColorTexCoord012PointerInterlaced_fake),
MAGIC_MACRO(glGenTextures_fake),
MAGIC_MACRO(glGenBuffersARB_fake),
MAGIC_MACRO(glGenLists_fake),
MAGIC_MACRO(_glDrawElements_buffer),
MAGIC_MACRO(_glDrawRangeElements_buffer),
MAGIC_MACRO(_glMultiDrawElements_buffer),
MAGIC_MACRO(_glVertexPointer_buffer),
MAGIC_MACRO(_glNormalPointer_buffer),
MAGIC_MACRO(_glColorPointer_buffer),
MAGIC_MACRO(_glSecondaryColorPointer_buffer),
MAGIC_MACRO(_glIndexPointer_buffer),
MAGIC_MACRO(_glTexCoordPointer_buffer),
MAGIC_MACRO(_glEdgeFlagPointer_buffer),
MAGIC_MACRO(_glVertexAttribPointerARB_buffer),
MAGIC_MACRO(_glWeightPointerARB_buffer),
MAGIC_MACRO(_glMatrixIndexPointerARB_buffer),
MAGIC_MACRO(_glFogCoordPointer_buffer),
MAGIC_MACRO(_glVariantPointerEXT_buffer),
MAGIC_MACRO(_glGetError_fake),
MAGIC_MACRO(_glReadPixels_pbo),
MAGIC_MACRO(_glDrawPixels_pbo),
MAGIC_MACRO(_glMapBufferARB_fake),
MAGIC_MACRO(_glSelectBuffer_fake),
MAGIC_MACRO(_glGetSelectBuffer_fake),
MAGIC_MACRO(_glFeedbackBuffer_fake),
MAGIC_MACRO(_glGetFeedbackBuffer_fake),
