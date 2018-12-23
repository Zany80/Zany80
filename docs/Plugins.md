# Plugins

This is a simple document detailing how plugins work in Zany80 right now.

## Types

There are currently three types of plugins in Zany80: resource plugins, invoked
plugins, and perpetual plugins. They are all fairly simple.

### Perpetual Plugins

While resource plugins are, for good reason, mentioned first in the list, it is
important to first understand Perpetual Plugins. A perpetual plugin extends the
PerpetualPlugin class, which defines a set of virtual functions each plugin must
implement.

First, there is `frame`. This function is called every frame for every loaded
plugin. 

### Resource Plugins

Resource plugins shouldn't actually *do* anything, per sé, or at least, not on
their own. Instead, they provide useful resources that other plugins can use to
do work.

An example of a resource plugin is the CPU plugin. Like a perpetual plugin, the
CPU plugin is active every frame; however, it isn't controlled by itself.
Instead, a perpetual plugin such as the emulation layer requests a z80 object
from the CPU plugin, and tells the plugin what to do with it.

The key to resource plugins is that they don't have any set functions. Their
usage is defined by the plugins that use them. There is no way to check what
functions a resource plugin has; the only way to use a resource plugin is if you
already know what the plugin does.

## Internals

A plugin must, in addition to extending the relevant class, contain the
following in a `private:` block:

```
OryolClassDecl(SimpleShell);
OryolTypeDecl(SimpleShell, PerpetualPlugin);
```

Of course, SimpleShell and PerpetualPlugin should be replaced with the plugin's
name and the name of the superclass respectively.

What this does is generate custom information about the plugin that make it
possible for the host program to easily determine what type of plugin is being
loaded and treat it accordingly.

This is a form of Run-Time Type Information, but it is *not* the standard C++
RTTI. The standard C++ RTTI is actually *disabled* in Zany80. This uses Oryol's
custom, far more light-weight RTTI.

```Oryol's RTTI system is small and fast, there's one byte of static storage
added per RTTI-enabled class, and a RTTI check only involves a pointer
comparision.```

All plugins must additionally implement the `make` function, which should be
defined **outside of the class** in the global scope as follows:

```
extern "C" Ptr<Plugin>(*make())();
```

In case that looks confusing, don't worry; it's pretty simple to understand if
you understand function pointers.

It can be reduced to this:

```
typedef Ptr<Plugin>(*make_t)();
extern "C" make_t make();
```

In essence, it defines a function `make` that takes no arguments and returns a
function, which should itself take no arguments and return a smart pointer to a
Plugin object.

This function must return a function that instantiates the class. Thanks to
Oryol, this is exceptionally easy: ```
extern "C" Ptr<Plugin>(*make())() {
	return (Ptr<Plugin>(*)())(void*)SimpleShell::Creator();
}
```

The Creator() function is added by the optional OryolClassCreator macro. It
returns a lambda that returns a Ptr<TYPE>, where TYPE is the name of the class
in question. For instance, SimpleShell::Creator returns a function that returns
a Ptr<SimpleShell> when called. To make this function return the proper value,
we first convert it to a void pointer, then to a pointer to the correct function
type. The void pointer conversion is needed to surpress a warning caused by the
conversion.

From the Simple Shell source:

```
class SimpleShell : public PerpetualPlugin {
	OryolClassDecl(SimpleShell);
	OryolTypeDecl(SimpleShell, PerpetualPlugin);
	OryolClassCreator(SimpleShell);
```

An important thing to note is that these macros set the current member
visibility to `public` instead of the default `private`.
