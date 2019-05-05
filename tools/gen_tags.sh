#!/bin/bash
# Make it possible to run both in and out of the tools folder
if [[ ! -f fips.yml ]]
then
	cd ..
fi
headers=""
flags="-w -DGLM_FORCE_RADIANS=1 -DORYOL_GL_USE_GETATTRIBLOCATION=0 -DORYOL_LINUX=1 -DORYOL_OPENGL=1 -DORYOL_OPENGL_CORE_PROFILE=1 -DORYOL_POSIX=1 -DORYOL_SAMPLE_URL=\"http://floooh.github.com/oryol/data/\" -DORYOL_USE_LIBCURL=1 -fPIC -std=c++11 -pthread -fno-strict-aliasing -Wno-expansion-to-defined -Wno-multichar -Wall -Wextra -Wno-unused-parameter -Wno-unknown-pragmas -Wno-ignored-qualifiers -fno-exceptions -fno-rtti -O3 -ftree-vectorize -ffast-math -DNDEBUG   -std=gnu++11"
echo Add in all headers in /usr/include and /usr/local/include as well?
read
if [[ "$REPLY" = "y" ]]
then
	for a in `find /usr/include -type d ! -path "/usr/include/bits" ! -path "/usr/include/bsd*"` `find /usr/local/include -type d`
	do
		flags="$flags -I$a"
	done
elif [[ "$REPLY" = "local_only" ]]
then
	for a in `find /usr/local/include -type d`
	do
		flags="$flags -I$a"
	done
fi

for a in `find -name "*.h" ! -path "*fips-sdks/*" ! -path "*private*" ! -path "*examples*" ! -path "*glfw/deps*" ! -path "*glfw/src*" ! -name "*internal*" ! -path "*glm/test*"` `find -name "*.hpp" ! -path "*fips-sdks/*" ! -path "*private*" ! -path "*examples*" ! -path "*glfw/deps*" ! -path "*glfw/src*" ! -path "*glm/test*"`
do
	headers="$headers $a"
done
for a in `find -type d ! -path "*fips-sdks/*" ! -path "*private*" ! -path "*examples*" ! -path "*glfw/deps*" ! -path "*glfw/src*" ! -path "*glm/test*"`
do
	flags="$flags -I$a"
done
#for a in `cat .fips-settings.yml`; do config=$a; done
#config="${config%\}}"
echo Fetching libraries...
./fips fetch
CFLAGS="$flags" geany -g zany80_deps.cpp.tags.1 $headers
CFLAGS="$flags" geany -g zany80_deps.cpp.tags.2 -P $headers
if [[ "$REPLY" = "y" ]]
then
	CFLAGS="$flags" find /usr/include -type f -name "*.hpp" -or -name "*.h" -exec bash -c 'echo Appending {}...;geany -g zany80_deps.cpp.tags.3 {};cat zany80_deps.cpp.tags.{2,3} >> zany80_deps.cpp.tags.4;mv zany80_deps.cpp.tags.{4,2}' \;
fi
if [[ "$REPLY" = "y" || "$REPLY" = "local_only" ]]
then
	CFLAGS="$flags" find /usr/local/include -type f -name "*.hpp" -or -name "*.h" -exec bash -c 'echo Appending {}...;geany -g zany80_deps.cpp.tags.3 {};cat zany80_deps.cpp.tags.{2,3} >> zany80_deps.cpp.tags.4;mv zany80_deps.cpp.tags.{4,2}' \;
	rm zany80_deps.cpp.tags.3
fi
cat zany80_deps.cpp.tags.{1..2} | uniq > zany80_deps.cpp.tags
rm zany80_deps.cpp.tags.{1..2}
