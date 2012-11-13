/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef _MARU_CAMERA_DARWIN_H_
#define _MARU_CAMERA_DARWIN_H_

#define MAKEFOURCC(a,b,c,d)\
    (((uint32_t)(a)<<0)|((uint32_t)(b)<<8)|((uint32_t)(c)<<16)|((uint32_t)(d)<<24))

/*      Pixel format         FOURCC                        depth  Description  */
#define V4L2_PIX_FMT_RGB555  MAKEFOURCC('R','G','B','O') /* 16  RGB-5-5-5     */
#define V4L2_PIX_FMT_RGB565  MAKEFOURCC('R','G','B','P') /* 16  RGB-5-6-5     */
#define V4L2_PIX_FMT_RGB555X MAKEFOURCC('R','G','B','Q') /* 16  RGB-5-5-5 BE  */
#define V4L2_PIX_FMT_RGB565X MAKEFOURCC('R','G','B','R') /* 16  RGB-5-6-5 BE  */
#define V4L2_PIX_FMT_BGR24   MAKEFOURCC('B','G','R','3') /* 24  BGR-8-8-8     */
#define V4L2_PIX_FMT_RGB24   MAKEFOURCC('R','G','B','3') /* 24  RGB-8-8-8     */
#define V4L2_PIX_FMT_BGR32   MAKEFOURCC('B','G','R','4') /* 32  BGR-8-8-8-8   */
#define V4L2_PIX_FMT_RGB32   MAKEFOURCC('R','G','B','4') /* 32  RGB-8-8-8-8   */
#define V4L2_PIX_FMT_YVU410  MAKEFOURCC('Y','V','U','9') /*  9  YVU 4:1:0     */
#define V4L2_PIX_FMT_YVU420  MAKEFOURCC('Y','V','1','2') /* 12  YVU 4:2:0     */
#define V4L2_PIX_FMT_YUYV    MAKEFOURCC('Y','U','Y','V') /* 16  YUV 4:2:2     */
#define V4L2_PIX_FMT_UYVY    MAKEFOURCC('U','Y','V','Y') /* 16  YUV 4:2:2     */
#define V4L2_PIX_FMT_YUV422P MAKEFOURCC('4','2','2','P') /* 16  YVU422 planar */
#define V4L2_PIX_FMT_YUV411P MAKEFOURCC('4','1','1','P') /* 16  YVU411 planar */
#define V4L2_PIX_FMT_Y41P    MAKEFOURCC('Y','4','1','P') /* 12  YUV 4:1:1     */
#define V4L2_PIX_FMT_YUV444  MAKEFOURCC('Y','4','4','4') /* 16  xxxxyyyy uuuuvvvv */
#define V4L2_PIX_FMT_YUV555  MAKEFOURCC('Y','U','V','O') /* 16  YUV-5-5-5     */
#define V4L2_PIX_FMT_YUV565  MAKEFOURCC('Y','U','V','P') /* 16  YUV-5-6-5     */
#define V4L2_PIX_FMT_YUV32   MAKEFOURCC('Y','U','V','4') /* 32  YUV-8-8-8-8   */
#define V4L2_PIX_FMT_YUV410  MAKEFOURCC('Y','U','V','9') /*  9  YUV 4:1:0     */
#define V4L2_PIX_FMT_YUV420  MAKEFOURCC('Y','U','1','2') /* 12  YUV 4:2:0     */
#define V4L2_PIX_FMT_YYUV    MAKEFOURCC('Y','Y','U','V') /* 16  YUV 4:2:2     */

void convert_frame(uint32_t pixel_format, int frame_width, int frame_height,
                   size_t frame_size, void* frame_pixels, void* video_buf);

#endif /* _MARU_CAMERA_DARWIN_H_ */
