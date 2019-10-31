// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package io.flutter.embedding.engine.systemchannels;

import android.support.annotation.NonNull;

import java.util.HashMap;
import java.util.Map;

// BD ADD
import io.flutter.embedding.engine.FlutterJNI;
import io.flutter.embedding.engine.dart.DartExecutor;
import io.flutter.plugin.common.BasicMessageChannel;
import io.flutter.plugin.common.JSONMessageCodec;

/**
 * TODO(mattcarroll): fill in javadoc for SystemChannel.
 */
public class SystemChannel {

  @NonNull
  public final BasicMessageChannel<Object> channel;
  // BD ADD:START
  @NonNull
  private final DartExecutor dartExecutor;
  // END

  public SystemChannel(@NonNull DartExecutor dartExecutor) {
    // BD ADD
    this.dartExecutor = dartExecutor;
    this.channel = new BasicMessageChannel<>(dartExecutor, "flutter/system", JSONMessageCodec.INSTANCE);
  }

  public void sendMemoryPressureWarning() {
    Map<String, Object> message = new HashMap<>(1);
    message.put("type", "memoryPressure");
    // BD MOD
    //channel.send(message);
    channel.send(message, new BasicMessageChannel.Reply<Object>() {
      @Override
      public void reply(Object reply) {
        dartExecutor.getFlutterJNI().notifyLowMemory();
      }
    });
    // END
  }
}
