#!/bin/bash
if [[ "$TRAVIS" == "1" ]]
then
    git config --global user.name "Travis CI"
    git config --global user.email pleasantatk@gmail.com
    git config --global push.default simple
fi
pushd ..
if [[ ! -d zany80.github.io ]]
then
    git clone https://pixelherodev:$GITHUB_KEY@github.com/zany80/zany80.github.io || exit 1
fi
cd zany80.github.io
export BUILDS=Zany80
yes | ./update_zanyonline.sh
popd
