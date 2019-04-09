#!/usr/bin/env bash
# python2 flutter/tools/gn --goma --ios --runtime-mode debug --no-lto
# python2 flutter/tools/gn --goma --ios --runtime-mode debug --ios-cpu=arm --no-lto
# python2 flutter/tools/gn --goma --ios --runtime-mode debug --simulator --no-lto
# python2 flutter/tools/gn --goma --ios --runtime-mode profile
# python2 flutter/tools/gn --goma --ios --runtime-mode profile --ios-cpu=arm
# python2 flutter/tools/gn --goma --ios --runtime-mode release
# python2 flutter/tools/gn --goma --ios --runtime-mode release --ios-cpu=arm

# dynamic
# python2 flutter/tools/gn --goma --ios --runtime-mode profile --dynamic
# python2 flutter/tools/gn --goma --ios --runtime-mode profile --dynamic --ios-cpu=arm
# python2 flutter/tools/gn --goma --ios --runtime-mode release --dynamic
# python2 flutter/tools/gn --goma --ios --runtime-mode release --dynamic --ios-cpu=arm

ls -sail
ls .. --sail
ls $ENGINE_PATH -sail
ls $ENGINE_PATH/src -sail

python2 flutter/tools/gn --goma --ios --runtime-mode debug --no-lto

# Fly Serial

python2 flutter/tools/gn --goma --ios --runtime-mode profile --ios-framework-name=Fly
python2 flutter/tools/gn --goma --ios --runtime-mode profile --ios-cpu=arm --ios-framework-name=Fly
python2 flutter/tools/gn --goma --ios --runtime-mode release --ios-framework-name=Fly
python2 flutter/tools/gn --goma --ios --runtime-mode release --ios-cpu=arm --ios-framework-name=Fly
python2 flutter/tools/gn --goma --ios --runtime-mode debug --simulator --ios-framework-name=Fly

python2 flutter/tools/gn --goma --ios --runtime-mode profile --dynamic --ios-framework-name=FlyDynamic
python2 flutter/tools/gn --goma --ios --runtime-mode profile --dynamic --ios-cpu=arm --ios-framework-name=FlyDynamic
python2 flutter/tools/gn --goma --ios --runtime-mode release --dynamic --ios-framework-name=FlyDynamic
python2 flutter/tools/gn --goma --ios --runtime-mode release --dynamic --ios-cpu=arm --ios-framework-name=FlyDynamic
python2 flutter/tools/gn --goma --ios --runtime-mode debug --dynamic --simulator --ios-framework-name=FlyDynamic


