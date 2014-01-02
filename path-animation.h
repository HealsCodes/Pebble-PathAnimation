/* Copyright (c) 2014, René Köcher <shirk@bitspin.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* /path-animation.h, created 2014-01-02 / */

#ifndef __PATH_ANIMATION_H
#define __PATH_ANIMATION_H

#include <pebble.h>

//! opaque Animation 'subclass'
typedef struct Animation PathAnimation;

//! Create a new PathAnimation
//! @param path A valid GPath which will should be animated
//! @param from A GPathInfo containing the path points
//! @param to A GPathInfo containing the path points
//! If from is NULL then the current points in path will be used.
//! All three parameters must share the same number of points.
//! The resulting PathAnimation must be destroyed using \ref path_animation destroy
//! *before* using gpath_destroy an path.
PathAnimation* path_animation_create( GPath* path, const GPathInfo* from, const GPathInfo* to );

//! Destroy a \ref PathAnimation created with path_animation create.
//! This must be called before destroying the animated \ref GPath object!
void path_animation_destroy( PathAnimation* animation );

//! Setup a layer for which \ref layer_mark_dirty() will be called on each animation step.
//! @param animation A \ref PropertyAnimation.
//! @param layer a valid layer object which will be linked to this animation or NULL to disable.
void path_animation_set_update_layer( PathAnimation* animation, Layer* layer );

//! Sets the callbacks for the animation.
//! This provides the same functionality as \ref animation_set_handlers but must
//! be used because \ref PathAnimation already hooks into the basic animation handlers.
//! @param animation A \ref PropertyAnimation.
//! @param handlers The callback handlers see \ref AnimationHandlers.
//! @param context A pointer to application specific data, that will be passed to the callbacks.
void path_animation_set_handlers( PathAnimation* animation, AnimationHandlers handlers, void* context );

//! Set a new target path to animate to
//! @param animation A \ref PropertyAnimation.
//! @param to A \ref GPathInfo that will become the new target for the animated path
//! If the animation is scheduled it will first be unscheduled.
//! The current implementation will also update the path to match the old target path
//! as the new starting point.
void path_animation_update_target( PathAnimation* animation, const GPathInfo* to );

//! Gets the application specific callback context pointer.
//! @param animation A \ref PropertyAnimation.
//! See \ref path_animation_set_handlers
void* path_animation_get_context( PathAnimation* animation );

#endif
