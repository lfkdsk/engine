#!/usr/bin/env bash

frameworkDir=../frameworks

bundle() {
    targetDir=$1
    prefix=$2
    name=$3
    simDir=$4

    targetDir=$frameworkDir/$targetDir

    mkdir -p $targetDir
    cp -rf out/$prefix/$name.framework $targetDir/

    cp out/${prefix}/$name.framework/$name $targetDir/$name.arm64
    cp out/${prefix}_arm/$name.framework/$name $targetDir/$name.arm

    if [[ ! -z "$simDir" ]] && [[ -d "out/$simDir" ]]; then
        cp out/$simDir/$name.framework/$name $targetDir/$name.x86_x64
        lipo -create $targetDir/$name.arm64 $targetDir/$name.arm $targetDir/$name.x86_x64 \
            -output $targetDir/$name.framework/$name
    else
        lipo -create $targetDir/$name.arm64 $targetDir/$name.arm \
            -output $targetDir/$name.framework/$name
    fi
}

bundle ios_fly_debug ios_fly_release FlyFlutter ios_fly_debug_sim
bundle ios_fly_profile_debug ios_flydynamic_dynamic_release FlyDynamicFlutter ios_flydynamic_debug_sim

bundle ios_fly_profile ios_fly_profile FlyFlutter
bundle ios_flydynamic_dynamic_profile ios_flydynamic_dynamic_profile FlyDynamicFlutter

bundle ios_fly_release ios_fly_release FlyFlutter
bundle ios_flydynamic_dynamic_release ios_flydynamic_dynamic_release FlyDynamicFlutter
