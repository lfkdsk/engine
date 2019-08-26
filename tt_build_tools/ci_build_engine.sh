#!/usr/bin/env bash

# ninja
# brew install ninja

# ant
# brew install ant

# gclient
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git

export PATH=`pwd`/depot_tools:$PATH

mkdir engine
(
cat <<EOF
solutions = [
  {
    "managed": False,
    "name": "src/flutter",
    "url": "git@code.byted.org:tech_client/flutter_engine.git",
    "custom_deps": {},
    "deps_file": "DEPS",
    "safesync_url": "",
  },
]
EOF
) > engine/.gclient

cd engine
if [ -d "src/" ];then
	cd src
    git checkout master
	git reset --hard head
	git clean -fd
	git pull
	cd ..
fi

if [ ! -d "src/flutter" ];then
    gclient sync -D -f
fi

cd src
git fetch
git checkout -f origin/master

cd flutter
git fetch
git reset --hard origin/$BRANCH
git clean -fd

gclient sync -D -f

cd ..

if [ "$PLATFORM" != "none" ]; then

	./flutter/tools/gn
	ninja -C out/host_debug -j $JCOUNT

	./flutter/tools/gn --runtime-mode=release --dynamicart
	ninja -C out/host_release_dynamicart -j $JCOUNT

fi

cd flutter/tt_build_tools

if [ $PLATFORM == 'all' -o $PLATFORM == 'android' ]; then
	docompile=
else
	docompile=false
fi

bash android_build.sh $JCOUNT $MODE $docompile

if [ $PLATFORM == 'all' -o $PLATFORM == 'ios' ]; then
	docompile=
else
	docompile=false
fi

bash iOS_build.sh $JCOUNT $docompile

curl -F "uuid=default" -F "type=Native" -F "file=@../../out/android_release/libflutter.so" http://symbolicate.byted.org/android_upload
curl -F "uuid=default" -F "type=Native" -F "file=@../../out/android_release_arm64/libflutter.so" http://symbolicate.byted.org/android_upload
curl -F "uuid=default" -F "type=Native" -F "file=@../../out/android_release_dynamicart/libflutter.so" http://symbolicate.byted.org/android_upload
curl -F "uuid=default" -F "type=Native" -F "file=@../../out/android_release_arm64_dynamicart/libflutter.so" http://symbolicate.byted.org/android_upload