#!/usr/bin/env python3

MAJOR_VER=0
MINOR_VER=2
PATCH_VER=0
try:
    with open("build/bump", "r") as f:
        BUMP_VER = int(f.read())
except:
    BUMP_VER=0
BUMP_VER += 1
try:
    with open("build/bump", "w") as f:
        f.write(str(BUMP_VER))
except:
    print("Failed to save tweak version")
VERSION=str(MAJOR_VER) + '.' + str(MINOR_VER) + '.' + str(PATCH_VER)

import glob
import os
import argparse
from pathlib import Path
from string import Template
import itertools
import subprocess
from tempfile import NamedTemporaryFile
import multiprocessing

# should be fine to just hardcode these, IMO
BUILD_DIR = 'build'
BIN_DIR = BUILD_DIR
OBJ_DIR = os.path.join(BUILD_DIR, 'obj')
EXE_EXT = '.exe' if os.name == 'nt' else ''
# TODO: after the jam, split compiler and VM into two executables
# For now, we'll keep compiler source in 'src/compiler' and VM source in 'src/vm', with shared stuff in 'src/' directly.
BIN_NAME = 'Zany80' + EXE_EXT

DEFAULT_FLAGS = {'-Wall', '-pedantic', '-DSOKOL_GLCORE33' }
DEBUG_FLAGS = {'-g', '-Og', '-D_DEBUG'}
# pixelherodev's personal flag set :P I'm insane, I know.
PIXELS_DEVEL_FLAGS = { '-Werror', '-Wextra', '-Wno-error=reorder', '-Wno-error=pedantic', '-Wno-error=unused-parameter', '-Wno-error=missing-field-initializers', '-Wno-error=deprecated-declarations', '-pedantic', '-march=native', '-mtune=native', '-falign-functions=32' }
RELEASE_FLAGS = {'-O2'}

# these *do* need to be in order, so no sets!
BASE_LDFLAGS = [ '-lGL', '-lX11', '-ldl', '-lstdc++' ]
DEFAULT_LDFLAGS = ['-static', '-static-libgcc' ] + BASE_LDFLAGS

# parse arguments
parser = argparse.ArgumentParser(description='Builds Zany80.')
parser.add_argument('--pixel', dest='cflags', action='append_const',
                    const=PIXELS_DEVEL_FLAGS,
                    help='Enable additional development flags because you\'re pixelherodev and you make bad life choices')
parser.add_argument('--release', dest='cflags_mode', action='store_const',
                    const=RELEASE_FLAGS, default=DEBUG_FLAGS,
                    help='Enable release build')
parser.add_argument('-B', dest='force_rebuild', action='store_true',
                    help='Force rebuilding the project')
parser.add_argument('-r', dest='dynamic', action='store_true',
                    help='Build a dynamic executable')
parser.add_argument('-j', dest='num_threads', nargs='?',
                    type=int, default=max(multiprocessing.cpu_count() // 2, 1),
                    help='Number of threads to run `make` with')

args = parser.parse_args()
if '-O2' not in args.cflags_mode:
    VERSION = VERSION + '-' + str(BUMP_VER)
CFLAGS = DEFAULT_FLAGS | {'-std=c99'} | args.cflags_mode | (set(itertools.chain(*args.cflags)) if args.cflags else set()) | { '-DPROJECT_VERSION=' + VERSION }
CXXFLAGS = DEFAULT_FLAGS | {'-std=c++11'} | args.cflags_mode | (set(itertools.chain(*args.cflags)) if args.cflags else set()) | { '-DPROJECT_VERSION=' + VERSION }
LDFLAGS = DEFAULT_LDFLAGS
if args.dynamic:
    LDFLAGS = BASE_LDFLAGS

class Target:
    all = {}

    def __init__(self, name, artifacts):
        self.name = name
        self.artifacts = set(artifacts)
        self.sources = {}   # source => object mapping
        self.headers = set()
        self.includes = []
        self.dependencies = set()
        # target names must be unique
        assert name not in self.all
        self.all[name] = self

    def __enter__(self):
        return self
    def __exit__(self, exc_type, exc_value, traceback):
        pass

    @property
    def paths(self):
        return set(os.path.join(BIN_DIR, a) for a in self.artifacts)

    @staticmethod
    def obj_from_c(cname, skipn=0):
        return os.path.join(OBJ_DIR, *Path(cname).with_suffix('.o').parts[skipn:])

    def add_sources_glob(self, *paths, obj_skip=0):
        srcs = set(itertools.chain.from_iterable(glob.glob(path, recursive=True) for path in paths))
        objs = {src: self.obj_from_c(src, skipn=obj_skip) for src in srcs}
        self.sources.update(objs)

    def add_headers_glob(self, *paths):
        self.headers |= set(itertools.chain.from_iterable(glob.glob(path, recursive=True) for path in paths))

    def add_includes(self, *paths):
        for p in paths:
            if p not in self.includes:
                self.includes.append(p)

    def add_dependencies(self, *deps):
        self.dependencies |= {d.name if isinstance(d, Target) else d for d in deps}

    def create_obj_dirs(self):
        for dir in {os.path.dirname(obj) for obj in self.sources.values()}:
            os.makedirs(dir, exist_ok=True)

    def get_transitive_dependencies(self, tdeps=None):
        if tdeps is None:
            tdeps = {}
        for name in self.dependencies:
            if name in tdeps:
                continue
            dep = Target.all[name]
            tdeps[name] = dep
            dep.get_transitive_dependencies(tdeps)
        return tdeps

class SourceLibrary(Target):
    def __init__(self, name):
        super().__init__(name, set())

class Executable(Target):
    def __init__(self, name):
        super().__init__(name, {name + EXE_EXT})

with SourceLibrary('sokol') as sokol:
    sokol.add_sources_glob('lib/sokol/sokol_cpp.cpp')
    sokol.add_sources_glob('lib/sokol/sokol.c')
    sokol.add_headers_glob('lib/sokol/*.h')
    sokol.add_headers_glob('lib/sokol/util/*.h')
    
with SourceLibrary('cimgui') as cimgui:
    cimgui.add_sources_glob('lib/cimgui/*.cpp')
    cimgui.add_headers_glob('lib/cimgui/*.h')

with SourceLibrary('stb') as stb:
    stb.add_sources_glob('lib/stb/stb_ds.c')
    stb.add_headers_glob('lib/stb/stb_ds.h')

with SourceLibrary('TextEditor') as TextEditor:
    TextEditor.add_sources_glob('lib/TextEditor/TextEditor.cpp')
    TextEditor.add_headers_glob('lib/TextEditor/TextEditor.h')
    TextEditor.add_includes('lib/cimgui/')

with Executable('Zany80') as Zany80:
    Zany80.add_sources_glob('src/main.c', 'src/graphics.c', 'src/graphics_legacy.cpp', '-Werror')
    Zany80.add_dependencies('sokol', 'cimgui', 'stb', 'TextEditor')
    Zany80.add_includes('lib/')

# create object directories
for tgt in Target.all.values():
    tgt.create_obj_dirs()

t = []
# emit heading
t.append('''\
# AUTOGENERATED FILE; DO NOT MODIFY (use `build.py` instead)
CFLAGS+={CFLAGS}
CXXFLAGS+={CXXFLAGS}
LDFLAGS+={LDFLAGS}
INCLUDES+={includes_all}

.PHONY: default ./{BIN_DIR}/Zany80 all Zany80
default: Zany80
\t./{BIN_DIR}/Zany80

all: {targets_all}

HEADERS={headers_all}
{OBJ_DIR}/%.o: %.c $(HEADERS)
\t$(CC) $(CFLAGS) $(INCLUDES) $< -c -o $@
{OBJ_DIR}/%.o: %.cpp $(HEADERS)
\t$(CXX) $(CXXFLAGS) $(INCLUDES) $< -c -o $@
'''.format(
    CFLAGS=' '.join(sorted(CFLAGS)),
    CXXFLAGS=' '.join(sorted(CXXFLAGS)),
    LDFLAGS=' '.join(LDFLAGS),
    BIN_DIR=BIN_DIR,
    OBJ_DIR=OBJ_DIR,
    targets_all=' '.join(Target.all),
    # TODO: These should be per-target (but currently cannot be because of %.o: %.c rules)
    includes_all=' '.join(sorted({'-I' + inc for inc in itertools.chain.from_iterable(tgt.includes for tgt in Target.all.values())})),
    headers_all=' '.join(sorted(set(itertools.chain.from_iterable(tgt.headers for tgt in Target.all.values()))))
))

def emit_target(tgt, emitted=None):
    if emitted is None:
        emitted = set()
    if tgt.name in emitted:
        return
    emitted.add(tgt.name)

    dep_depends_objects = []
    dep_depends_headers = []
    # emit the target's own dependencies
    for dep in sorted(tgt.get_transitive_dependencies().values(), key=lambda v: v.name):
        emit_target(dep, emitted)
        dep_depends_objects.append('$({dep_name}_OBJECTS)'.format(dep_name=dep.name))
        dep_depends_headers.append('$({dep_name}_HEADERS)'.format(dep_name=dep.name))

    artifacts = sorted(tgt.paths)
    params = dict(
        t_name=tgt.name,
        t_artifacts=' '.join(artifacts),
        t_objects=' '.join(sorted(tgt.sources.values())),
        t_headers=' '.join(sorted(tgt.headers)),
        dep_depends_objects=''.join(' ' + dd for dd in dep_depends_objects),
        dep_depends_headers=''.join(' ' + dd for dd in dep_depends_headers),
    )
    t.append('''\

### TARGET: {t_name}
'''.format(**params))
    if artifacts:
        t.append('''\

{t_name}: {t_artifacts}
'''.format(**params))
    t.append('''\

{t_name}_OBJECTS = {t_objects}

{t_name}_HEADERS = {t_headers}

'''.format(**params))
    for a in artifacts:
        t.append('''\
{t_artifact}: $({t_name}_OBJECTS) {dep_depends_objects}
\t$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
'''.format(**params, t_artifact=a))

emitted = set()
for t_name in sorted(Target.all):
    emit_target(Target.all[t_name], emitted)

t = ''.join(t)
#print('-' * 10)
#if '-Werror' in CFLAGS:
#    print("Makefile generated.")
#else:
#    print(t.rstrip())
#print('-' * 10)

with open('Makefile', 'w') as f:
    f.write(t)

extra = []
if args.force_rebuild:
    extra.append('-B')
if args.num_threads is not ...:
    extra.append('-j%u' % args.num_threads if args.num_threads else '-j')
return_code = subprocess.call(['make'] + extra + ['-f', f.name, '--no-print-directory'])
exit(return_code)

