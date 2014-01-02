#include <pebble.h>
#include "path-animation.h"

static Window* window;
static TextLayer* text_layer;

static PathAnimation* animation;
static GPath* path;
static AnimationCurve curve = AnimationCurveLinear;

#define NUM_SHAPES 3

static unsigned shape_idx = 0;
static const GPathInfo shape_paths[NUM_SHAPES] = {
  /* simple square, (10 points to match the other paths, points are clockwise) */
  {
    .num_points = 10,
    .points = (GPoint[]) {
      { -40, -40 }, {   0, -40 }, {  40, -40 }, {  40, -33 },
      {  40,  33 }, {  40,  40 }, {   0,  40 }, { -40,  40 },
      { -40,  33 }, { -40, -33 }
    }
  },
  /* star shape */
  {
    .num_points = 10,
    .points = (GPoint[]) {
      { -12,  -11 }, {   0, -40 }, {  12, -11 }, {  38, -17 },
      {  17,   12 }, {  28,  39 }, {   0,  20 }, { -28,  39 },
      { -17,   12 }, { -38, -17 }
    }
  },
  /* dekagon shape */
  {
    .num_points = 10,
    .points = (GPoint[]) {
      { -19, -38 }, {  19, -38 }, {  34, -27 }, {  40,   0 },
      {  34,  27 }, {  19,  38 }, { -19,  38 }, { -34,  27 },
      { -40,   0 }, { -34, -27 }
    }
  }
};

static void _update_layer( Layer* layer, GContext* ctx )
{
  graphics_context_set_fill_color( ctx, GColorWhite );
  graphics_context_set_stroke_color( ctx, GColorBlack );
  graphics_fill_rect( ctx, layer_get_bounds( layer ), 0, 0 );

  gpath_draw_outline( ctx, path );
}

static void _on_animation_stopped( Animation* animation, bool done, void *ctx )
{
  if( done )
  {
    path_animation_update_target( animation, &shape_paths[shape_idx++] );
    animation_set_curve( animation, curve );
    animation_schedule( animation );

    if( shape_idx >= NUM_SHAPES )
    {
      shape_idx = 0;
    }
  }
}

static void select_click_handler( ClickRecognizerRef recognizer, void *ctx )
{
  if( curve == AnimationCurveEaseInOut )
  {
    text_layer_set_text( text_layer, "Linear" );
    curve = AnimationCurveLinear;
  }
  else
  {
    text_layer_set_text( text_layer, "EaseInOut" );
    curve = AnimationCurveEaseInOut;
  }
}

static void up_click_handler( ClickRecognizerRef recognizer, void *ctx )
{
  text_layer_set_text( text_layer, "EaseIn" );
  curve = AnimationCurveEaseIn;
}

static void down_click_handler( ClickRecognizerRef recognizer, void *ctx )
{
  text_layer_set_text( text_layer, "EaseOut" );
  curve = AnimationCurveEaseOut;
}

static void click_config_provider( void *context )
{
  window_single_click_subscribe( BUTTON_ID_SELECT, select_click_handler );
  window_single_click_subscribe( BUTTON_ID_UP, up_click_handler );
  window_single_click_subscribe( BUTTON_ID_DOWN, down_click_handler );
}

static void window_load( Window *window ) 
{
  Layer* window_layer = window_get_root_layer( window );
  GRect bounds = layer_get_bounds( window_layer );
  GRect label = { 
    .origin = { 0, 5 },
    .size = { bounds.size.w, 20 }
  };

  text_layer = text_layer_create( label );
  text_layer_set_text( text_layer, "Press a button" );
  text_layer_set_text_alignment( text_layer, GTextAlignmentCenter );

  layer_add_child( window_layer, text_layer_get_layer(text_layer) );
  layer_set_update_proc( window_layer, _update_layer );

  path = gpath_create( &shape_paths[NUM_SHAPES - 1] );
  gpath_move_to( path, (GPoint){ 72, 84 } );

  animation = path_animation_create( path, NULL, &shape_paths[shape_idx++] );
  path_animation_set_update_layer( animation, window_layer );
  path_animation_set_handlers( animation,
                               (AnimationHandlers){ 
                                            .started = NULL,
                                            .stopped = _on_animation_stopped
                               }, NULL );

  animation_set_curve( animation, AnimationCurveLinear );
  animation_set_duration( animation, 1000 );
  animation_set_delay( animation, 500 );
  animation_schedule( animation );
}

static void window_unload( Window *window )
{
  text_layer_destroy(text_layer);
  path_animation_destroy( animation );
  gpath_destroy( path );
}

static void init( void )
{
  window = window_create();

  window_set_click_config_provider( window, click_config_provider );
  window_set_window_handlers( window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  
  window_set_background_color( window, GColorWhite );

  const bool animated = true;
  window_stack_push( window, animated );
}

static void deinit( void )
{
  window_destroy( window );
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
