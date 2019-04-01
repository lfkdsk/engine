#!/usr/bin/env bash
cd ..

jcount=4
cacheDir=out/fly_android_cache
rm -rf $cacheDir
mkdir $cacheDir

platform=$1 # 'debug' 'profile' 'release'
if [[ ! $platform ]];
then
    platform=release
fi

mode=$2 # 'arm' 'x64' 'x86' 'arm64'
if [[ ! ${mode} ]];
then
    platform=arm
fi

dynamic=$3
if [[ ! ${dynamic} ]];
then 
    platform=normal
fi

if [[ ${mode} != 'debug' ]]; then
    if [ ${platform} = 'x64' -o $platform = 'x86' ]; then
        continue
    fi
fi

modeDir=android-$platform
# arm 不带后缀
if [ $platform = 'arm' ]; then
    platformPostFix=''
else
    platformPostFix=_${platform}
fi

# dynamic 只打非 debug
if [[ ${dynamic} = 'dynamic' ]]; then
    if [[ ${mode} = 'debug' ]]; then
        continue
    fi
    ./flutter/tools/gn --android --runtime-mode=$mode --android-cpu=$platform --dynamic
    androidDir=out/android_dynamic_${mode}${platformPostFix}
    modeDir=$modeDir-dynamic
else
    ./flutter/tools/gn --android --runtime-mode=$mode --android-cpu=$platform
    androidDir=out/android_${mode}${platformPostFix}
fi

# ninja build !!!
ninja -C $androidDir -j $jcount

if [[ $mode != 'debug' ]]; then
    modeDir=$modeDir-$mode
fi
rm -f $cacheDir/$modeDir
mkdir $cacheDir/$modeDir

function upload_to_assets() {
    files_to_upload=$1
    if [[ "$CIRRUS_RELEASE" == "" ]]; then
       echo "Not a release. No need to deploy!"
       exit 0
    fi

    if [[ "$GITHUB_TOKEN" == "" ]]; then
      echo "Please provide GitHub access token via GITHUB_TOKEN environment variable!"
      exit 1
    fi

    for fpath in $files_to_upload
    do
      echo "Uploading $fpath..."
      name=$platform-$mode-$dynamic-$(basename "$fpath")
      url_to_upload="https://uploads.github.com/repos/$CIRRUS_REPO_FULL_NAME/releases/$CIRRUS_RELEASE/assets?name=$name"
      curl -X POST \
        --data-binary @$fpath \
        --header "Authorization: token $GITHUB_TOKEN" \
        --header "Content-Type: $file_content_type" \
        $url_to_upload
    done

    echo uploaded $name
}

# 非 debug 还要带上 gen_snapshot
if [[ $mode != 'debug' ]]; then
    if [ -f "$androidDir/clang_x86/gen_snapshot" ];then
        zip -rjq $cacheDir/$modeDir/darwin-x64.zip $androidDir/clang_x86/gen_snapshot
    else
        zip -rjq $cacheDir/$modeDir/darwin-x64.zip $androidDir/clang_x64/gen_snapshot
    fi
    # node ./flutter/tt_build_tools/tosUpload.js $cacheDir/$modeDir/darwin-x64.zip flutter/framework/$tosDir/$modeDir/darwin-x64.zip
    files=(
        $cacheDir/$modeDir/darwin-x64.zip
    )
    upload_to_assets $files
fi

# x86和x64要带上 libflutter.so
if [[ $platform = 'x64' || $platform = 'x86' ]]; then
    zip -rjq $cacheDir/$modeDir/artifacts.zip $androidDir/flutter.jar $androidDir/lib.stripped/libflutter.so
else
    zip -rjq $cacheDir/$modeDir/artifacts.zip $androidDir/flutter.jar
fi
# node ./flutter/tt_build_tools/tosUpload.js $cacheDir/$modeDir/artifacts.zip flutter/framework/$tosDir/$modeDir/artifacts.zip
files=(
    $cacheDir/$modeDir/artifacts.zip
)
upload_to_assets files