/* Copyright (C) 2011  Nokia Corporation All Rights Reserved.
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

#import <AppKit/NSOpenGL.h>
#import <AppKit/NSView.h>
#import <Foundation/NSGeometry.h>
#import <Foundation/NSNotification.h>
#import "hcocoa.h"

HCocoa hcocoa;

@interface CocoaView : NSObject
{
@private
    NSView *mView;
    NSOpenGLContext *mContext;
}
- (id)initWithView:(NSView *)view;
- (void)activateInContext:(NSOpenGLContext *)context;
- (void)frameChanged:(NSNotification *)notification;
@end

@implementation CocoaView
- (id)initWithView:(NSView *)view
{
    if ((self = [super init]) != nil) {
        mView = [view retain];
        mContext = nil;
        [[NSNotificationCenter defaultCenter] addObserver:self
        selector:@selector(frameChanged:)
            name:NSViewGlobalFrameDidChangeNotification
            object:mView];
        [mView setPostsFrameChangedNotifications:YES];
        return self;
    }
    return nil;
}
- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    if (mContext != nil) {
        [mContext release];
        mContext = nil;
    }
    if (mView != nil) {
        [mView release];
        mView = nil;
    }
    [super dealloc];
}
- (void)finalize
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    if (mContext != nil) {
        [mContext release];
        mContext = nil;
    }
    if (mView != nil) {
        [mView release];
        mView = nil;
    }
    [super finalize];
}
- (void)activateInContext:(NSOpenGLContext *)context
{
    if (mContext != nil) {
        [mContext release];
    }
    mContext = [context retain];
    [mContext setView:mView];
}
- (void)frameChanged:(NSNotification *)notification
{
    if (mContext != nil && [mContext view] == mView) {
        [mContext update];
    }
}
@end

static void *CreateContext(CGLContextObj ctx)
{
    return [[[NSOpenGLContext alloc] initWithCGLContextObj:ctx] retain];
}

static void DestroyContext(void *nsctx)
{
    [(NSOpenGLContext *)nsctx release];
}

static void *CreateView(EGLSurface s, void *nsview, unsigned *width,
    unsigned *height)
{
    NSView *v = nsview;
    NSRect bounds = [v bounds];
    if(width)   *width = bounds.size.width;
    if(height) *height = bounds.size.height;
    return [[[CocoaView alloc] initWithView:v] retain];
}

static void DestroyView(void *cview)
{
    [[NSNotificationCenter defaultCenter] removeObserver:cview];
    [(CocoaView *)cview release];
}

static void SetView(void *nsctx, void *cview)
{
    [(CocoaView *)cview activateInContext:nsctx];
}

static void BindTexImageFromView(void *nsctx, void *cview)
{
    // TODO: check if NSOpenGLContext.createTexture would be suitable here
    Dprintf("not implemented yet\n");
}

int hcocoaLoad(void)
{
    hcocoa.CreateContext = &CreateContext;
    hcocoa.DestroyContext = &DestroyContext;
    hcocoa.CreateView = &CreateView;
    hcocoa.DestroyView = &DestroyView;
    hcocoa.SetView = &SetView;
    hcocoa.BindTexImageFromView = &BindTexImageFromView;
    return 1;
}