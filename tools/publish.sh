#!/bin/bash
git config --global user.name "Travis CI"
git config --global user.email pleasantatk@gmail.com
git config --global push.default simple
pushd ..
git clone https://pixelherodev:$GITHUB_KEY@github.com/zany80/zany80.github.io || exit 1
cd zany80.github.io
export BUILDS=Zany80
yes | ./update_zanyonline.sh
popd
