Pebble-PathAnimation
====================

A GPath based implementation to be used in the animation framework on Pebble (SDK2.0+)

This "subclass" for the Pebble animation framework provides smooth transitions for GPath objects.
Basically one creates a GPath and then uses this framework to morph it betwenn any number of GPathInfo
layouts as long as they share the same number of path points.

A simple construction / destruction looks like this:

```C
#include "path-animation.h"

static GPath* path;
static PathAnimation* animation;

static const GPathInfo path_a { ... };
static const GPathInfo path_b { ... };

void on_window_loaded( Window* window )
{
  // create the path *first*
  path = gpath_create( &path_a );
  
  // then create and setup the PathAnimation
  animation = path_animation_create( path, NULL, &path_b );
  
  // this will call layer_mark_dirty() for the window layer on each animation frame
  path_animation_setup_update_layer( window_get_layer( window ) );
  
  // all other animation_* methods except for _destroy and _set_handlers are supported
  animation_schedule( animation );
}

void on_window_unloaded( Window* window )
{
  // destroy the PathAnimation *first*
  path_animation_destroy( animation );
  
  // then destroy the GPath object - other way around *WILL LEAK MEMORY*
  gpath_destroy( path );
}
```

See demo subdirectory for a working pebble sample (also in shown in the video link below).

I can't embed the [demo video](http://vimeo.com/83234539) here for obvious reasons but look for yourself..
