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

/* /path-animation.c, created 2014-01-02 / */

#include "path-animation.h"

#pragma pack(push, 1)
typedef struct PathAnimationData
{
	GPath* subject;
	GPoint* _save;

	GPoint*from;
	GPoint*to;

	AnimationHandlers handlers;
	void* user_ctx;

	Layer* layer;
} PathAnimationData;
#pragma pack(pop)

#define with_path_data( anim, as_var, block... ) \
	if( (anim) != NULL ) { \
		PathAnimationData* as_var = (PathAnimationData*)animation_get_context( (anim) ); \
		block; \
	}


static void _on_animation_update( Animation* animation, uint32_t time_normalized );
static void _on_animation_started( Animation* animation, PathAnimationData* ctx );
static void _on_animation_stopped( Animation* animation, bool done, PathAnimationData* ctx );

static const struct AnimationImplementation _path_animation_impl = {
	.setup = NULL,
	.update = _on_animation_update,
	.teardown = NULL
};

/*
 * provide a way for users to supply their callbacks and context for .started/.stopped
 */
static const struct AnimationHandlers _path_animation_handlers = {
	.started = (AnimationStartedHandler)_on_animation_started,
	.stopped = (AnimationStoppedHandler)_on_animation_stopped
};

static void _on_animation_started( Animation* animation, PathAnimationData* ctx )
{
	if( ctx->handlers.started != NULL )
	{
		ctx->handlers.started( animation, ctx->user_ctx );
	}
}

static void _on_animation_stopped( Animation* animation, bool done, PathAnimationData* ctx )
{
	if( ctx->handlers.stopped != NULL )
	{
		ctx->handlers.stopped( animation, done, ctx->user_ctx );
	}
}

/*
 * the "real" workhorse - point update with quartic easing curve support..
 */
static void _on_animation_update( Animation* animation, uint32_t time_normalized )
{
#define ABS( x ) ( (x) < 0 ? -(x) : (x) )

	with_path_data( animation, data, {
		double  progress = (double)( time_normalized * 100.0 / ANIMATION_NORMALIZED_MAX );
		unsigned i;

		/* special attention for start / end cases */
		if( time_normalized == ANIMATION_NORMALIZED_MIN ||
		    time_normalized == ANIMATION_NORMALIZED_MAX )
		{
			memcpy( data->subject->points, 
			        ( time_normalized == ANIMATION_NORMALIZED_MIN ? data->from
			                                                      : data->to ),
			        sizeof( GPoint ) * data->subject->num_points );

			if( data->layer )
			{
				layer_mark_dirty( data->layer );
			}
			return;
		}

		for( i = 0; i < data->subject->num_points; ++i )
		{
			double t = progress;
			/* absolute distance */
			/* FIXME: this could be pre-calculated and stored */
			int16_t dist_x = ABS( data->from[i].x - data->to[i].x ),
			        dist_y = ABS( data->from[i].y - data->to[i].y );

			/* relative change */
			/* FIXME: probably also pre-calculatable.. */
			int16_t c_x = ( ( progress * dist_x / 100 ) * ( data->from[i].x > data->to[i].x ? -1 : 1 ) ),
			        c_y = ( ( progress * dist_y / 100 ) * ( data->from[i].y > data->to[i].y ? -1 : 1 ) );

			switch( animation->curve )
			{
				case AnimationCurveLinear:
				{
					data->subject->points[i].x = data->from[i].x + c_x;
					data->subject->points[i].y = data->from[i].y + c_y;
				}
				break;

				case AnimationCurveEaseIn:
				{
					/* quartic ease-in variation */
					t /= 100.0;
					data->subject->points[i].x = c_x * t * t * t * t + data->from[i].x;
					data->subject->points[i].y = c_y * t * t * t * t + data->from[i].y;
				}
				break;

				case AnimationCurveEaseOut:
				{
					/* quadric ease-out variation */
					t /= 100.0;
					--t;
					data->subject->points[i].x = -c_x * ( t * t * t * t - 1.0 ) + data->from[i].x;
					data->subject->points[i].y = -c_y * ( t * t * t * t - 1.0 ) + data->from[i].y;
				}
				break;

				case AnimationCurveEaseInOut:
				{
					/* quadric ease-in-out variation */
					t /= 50.0;
					if( t < 1.0 )
					{
						data->subject->points[i].x = c_x / 2.0 * t * t * t * t + data->from[i].x;
						data->subject->points[i].y = c_y / 2.0 * t * t * t * t + data->from[i].y;
					}
					else
					{
						t -= 2.0;
						data->subject->points[i].x = -c_x / 2.0 * ( t * t * t * t - 2.0 ) + data->from[i].x;
						data->subject->points[i].y = -c_y / 2.0 * ( t * t * t * t - 2.0 ) + data->from[i].y;
					}
				}
				break;
			}
		}

		if( data->layer != NULL )
		{
			layer_mark_dirty( data->layer );
		}
	} );
#undef ABS
}

PathAnimation* path_animation_create( GPath* path, const GPathInfo* from, const GPathInfo* to )
{
	Animation* anim = animation_create();

	/* sanity checks */
	if( path == NULL || to == NULL )
	{
		return NULL;
	}

	if( path->num_points != to->num_points ||
	    ( from != NULL && ( path->num_points != from->num_points ||
	                        path->num_points != to->num_points ) ) )
	{
		/* paths need to have an equal number of points.. sorry */
		return NULL;
	}

	if( anim != NULL )
	{
		PathAnimationData* data = (PathAnimationData*)malloc( sizeof( PathAnimationData ) );
		if( data != NULL )
		{
			data->subject = path;
			data->_save = path->points;
			data->from = ( from == NULL ? data->_save : from->points );
			data->to = to->points;

			data->handlers = (AnimationHandlers){ NULL, NULL };
			data->user_ctx = NULL;
			data->layer = NULL;

			/* create new points to work with */
			const size_t point_size = sizeof( GPoint ) * data->subject->num_points;

			data->subject->points = (GPoint*)malloc( point_size );
			if( data->subject->points != NULL )
			{
				memcpy( data->subject->points, data->from, point_size );

				animation_set_handlers( anim, _path_animation_handlers, data );
				animation_set_implementation( anim, &_path_animation_impl );

				return (PathAnimation*)anim;
			}
			/* else: restore subject and exit */
			path->points = data->_save;
			free( data );
		}
		/* else: no data - OOM? */
		animation_destroy( anim );
	}
	/* else: sorry, we failed */
	return NULL;
}

void path_animation_destroy( PathAnimation* animation )
{
	with_path_data( animation, data, {
		if( animation_is_scheduled( animation ) )
		{
			animation_unschedule( animation );
		}

		if( data != NULL )
		{
			if( data->subject != NULL )
			{
				free( data->subject->points );
				if( data->_save != NULL )
				{
					/* restore the pristine path we where created with */
					data->subject->points = data->_save;
				}
			}
		}
		free( data );
	} );

	animation_destroy( animation );
}

void path_animation_set_update_layer( PathAnimation* animation, Layer* layer )
{
	with_path_data( animation, data, { data->layer = layer; } );
}

void path_animation_set_handlers( PathAnimation* animation, AnimationHandlers handlers, void* context )
{
	with_path_data( animation, data, {
		if( animation_is_scheduled( animation ) )
		{
			animation_unschedule( animation );
		}

		data->handlers = handlers;
		data->user_ctx = context;
	} );
}

void path_animation_update_target( PathAnimation* animation, const GPathInfo* to )
{
	if( to == NULL )
	{
		return;
	}

	with_path_data( animation, data, {
		if( to->num_points != data->subject->num_points )
		{
			return;
		}

		if( animation_is_scheduled( animation ) )
		{
			animation_unschedule( animation );
		}

		data->from = data->to;
		data->to = to->points;
	} );
}

void* path_animation_get_context( PathAnimation* animation )
{
	with_path_data( animation, data, { return data->user_ctx; } );
	return NULL;
}
