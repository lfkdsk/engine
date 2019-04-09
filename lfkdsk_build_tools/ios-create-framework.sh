#!/usr/bin/env bash

frameworkDir=../frameworks

function bundle() {
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

    if [[ ${CIRRUS_RELEASE} == "" ]]; then
       echo "Not a release. No need to deploy!"
       exit 0
    fi

    zip -rjq ${frameworkDir}/${prefix}-artifacts.zip ${targetDir}
    files=(
        ${frameworkDir}/$prefix-artifacts.zip
    )
    upload_to_assets files
}

function upload_to_assets() {
    files_to_upload=$1
    if [[ ${CIRRUS_RELEASE} == "" ]]; then
       echo "Not a release. No need to deploy!"
       exit 0
    fi

    if [[ ${github_token} != "" ]]; then
        GITHUB_TOKEN=${github_token}
    fi

    if [[ ${GITHUB_TOKEN} == "" ]]; then
      echo "Please provide GitHub access token via GITHUB_TOKEN environment variable!"
      exit 1
    fi

    file_content_type="application/octet-stream"
    for fpath in $files_to_upload
    do
      echo "Uploading $fpath..."
      name=android-$platform-$mode-$dynamic-$(basename "$fpath")
      url_to_upload="https://uploads.github.com/repos/$CIRRUS_REPO_FULL_NAME/releases/$CIRRUS_RELEASE/assets?name=$name"
      curl -X POST \
        --data-binary @$fpath \
        --header "Authorization: token $GITHUB_TOKEN" \
        --header "Content-Type: $file_content_type" \
        $url_to_upload
    done

    echo uploaded $name
}

bundle ios_fly_debug ios_fly_release FlyFlutter ios_fly_debug_sim
bundle ios_fly_profile_debug ios_flydynamic_dynamic_release FlyDynamicFlutter ios_flydynamic_debug_sim

bundle ios_fly_profile ios_fly_profile FlyFlutter
bundle ios_flydynamic_dynamic_profile ios_flydynamic_dynamic_profile FlyDynamicFlutter

bundle ios_fly_release ios_fly_release FlyFlutter
bundle ios_flydynamic_dynamic_release ios_flydynamic_dynamic_release FlyDynamicFlutter
