# Repositories

SIMPLE doesn't come with any applications. Instead, it allows the use of
user-defined repositories using Git.

## Manifest

The manifest is an XML file, appropriately named manifest.xml. It must be placed
at the root of the git repository.

## Why XML? XML Sucks!

Yeah. It does. However, it's more than tolerable, it's well-known, and most
importantly, it already works and I'm not willing to frustrate myself on
unnecessary and annoying tasks like switching the format.

Plus, I already wrote a nice API layer for XML usage (and expose that to all
plugins for their own usage e.g. with data files), and I'm not doing that for
any other format. It was annoying the first time.

### But $FORMATJESUS is better!!! You don't want to burn for using XML do you?

If you want to make a big deal out of technical decisions and don't want to put
in the effort to change it, you're not welcome here. If you want to write a
SIMPLE YAML/JSON/etc API then by all means go right ahead, but my time is
limited and my time for hobby projects even more so.

On that vein, the only real rule for the community is that you have to be
constructive. I doubt this will ever matter since I'm the only one who's likely
to ever use this project, so I'm not going to bother moving it out of this file.
It flows from the above.

### I've written a parser for $FORMATJESUS! Notice me senpai!

Firstly, never say that again. Ever.

Okay, there's a simple checklist for getting it merged:

* If this is the first non-XML format, make an abstraction layer that can
interface to both XML files (for legacy support) and the new format and expose
the same information regardless of which format is actually being used (redirect
based on file extension, perhaps?)
	* **Do not** under any circumstances touch the legacy parsers. They should
remain XML-only, as we shouldn't encourage people to use the legacy formats and
I don't want you to waste your time doing that.
	* Next, rewire the current format parser to use this new API.
	* Update this documentation
	* Give me time to review it
	* Feel good about yourself! (Seriously though, this is mandatory)
* If someone else has already contributed a second format, and done the work
mentioned above (creating an abstraction layer), just integrate your format with
that abstraction layer, give me time to review, and you're done!

### Format

The format is very much subject to change, but backwards compatibility is
guaranteed. The root of the manifest is a PluginRepository element, with a
*mandatory* Format attribute. The format attribute specifies the format of the
repository's manifest. If it is missing, SIMPLE will reject the repository as
unsupported.

Old formats will likely be supported indefinitely with the only allowed patches
being bug fixes / security improvements. They are all supported via parsers in
legacy.c, and I won't bother deleting them unless it ever grows to be
unmanagable, but that shouldn't happen as it'll basically never need to be
updated.

As a special exception, all alpha- formats are parsed by a single function. This
is because the differences between the alpha- formats are minute, and there's no
point in separating the code into multiple copies when most of it is the same.

### alpha- formats

There are two formats currently, alpha-1 and alpha-2.

### alpha-1 and alpha-2

alpha-1 and alpha-2 share a lot in common, so the commonalities will be shown
together. For differences, see the next sections.

The PluginRepository root contains a number of Plugin and DataFile elements.
Both Plugin and DataFile elements must have a name attribute.

#### Plugins

Plugins must contain at least one Version element. Version elements have an id
attribute (which contains the version as a string) and an ABI attribute. The ABI
attribute specifies the ABI of the actual plugin file.

Version elements must contain at least one Platform element. Platform elements
consist of an id attribute which specifies the platform (currently, only Linux
and Windows are supported). The id is case-sensitive. The platform element's
value is the path to the plugin relative to the repository root. The alpha-3
revision will likely split the Linux platform into Linux-glibc and Linux-musl,
and *require* both to be present if either is. Future revisions may also require
that all supported platforms be present, so that a plugin cannot support just
Windows or just Linux, and must support all platforms in general use.

Note that all copies of a plugin version must adhere to the same ABI regardless
of platform. It is impossible to specify that v1.0.0-Linux uses ABI X while
v1.0.0-Windows uses ABI Y.

An example of a SIMPLE repository containing a single plugin with both Linux and
Windows support follows with the manifest specified in alpha-2 format follows
(changing the 2 to 1 would result in a valid alpha-1 repo though, as the
differences lie only in DataFile treatment):

```xml
<PluginRepository format="alpha-2">
	<Plugin name="Hello, World!">
		<Version id="1.0.0" ABI="1">
			<Platform id="Windows">HelloWorld/1.0.0/Windows.dll</Platform>
			<Platform id="Linux">HelloWorld/1.0.0/Linux.so</Platform>
		</Version>
	</Plugin>
</PluginRepository>
```

It's worth noting that the directory structure shown in that example is not
mandatory (and indeed isn't even used by the official repos at present).

#### Data files

Data files are slightly different between alpha-1 and alpha-2.

##### Alpha-1

In alpha-1, data files are not versioned. They consist only of the name
attribute and a relative path.

```xml
<DataFile name="test file">data/test_file.bin</DataFile>
```

##### Alpha-2

In alpha-2, data files are versioned just as plugins are.

```xml
<DataFile name="test file">
	<Version id="1.0.0">data/test_file.bin</DataFile>
</DataFile>
```

#### Conclusion

That's it! That's the entirety of the alpha-2 SIMPLE format! There are some
major overhauls planned for alpha-3 though, such as plugin depedencies and
interactions (not quite *dependencies*, but "slots" that can be filled by other
plugins based on user-provided configuration, to be decided in the UI by the
user when the repository is installed).
