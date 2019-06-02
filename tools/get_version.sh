#!/bin/bash
version=NULL
for i in `cat CMakeLists.txt`
do
	if [[ "$a" == "y" ]]
	then
		version=${i%\)}
		break
	else
		if [[ "$i" == "VERSION" ]]
		then
			a=y
		fi
	fi
done
if [[ "$version" == "NULL" ]]
then
	exit 1
fi
echo $version
