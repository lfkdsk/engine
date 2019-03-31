// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/common/settings.h"

#include <sstream>

namespace blink {

Settings::Settings() = default;

Settings::Settings(const Settings& other) = default;

Settings::~Settings() = default;

std::string Settings::GetObservatoryHost() const {
  std::string observatory_host;
  if (!this->observatory_host.empty()) {
    observatory_host = this->observatory_host;
  } else {
    observatory_host = ipv6 ? "::1" : "127.0.0.1";
  }
  return observatory_host;
}

std::string Settings::ToString() const {
  std::stringstream stream;
  stream << "Settings: " << std::endl;
  stream << "vm_snapshot_data_path: " << vm_snapshot_data_path << std::endl;
  stream << "vm_snapshot_instr_path: " << vm_snapshot_instr_path << std::endl;
  stream << "isolate_snapshot_data_path: " << isolate_snapshot_data_path
         << std::endl;
  stream << "isolate_snapshot_instr_path: " << isolate_snapshot_instr_path
         << std::endl;
  stream << "application_library_path: " << application_library_path
         << std::endl;
  stream << "application_kernel_asset: " << application_kernel_asset
         << std::endl;
  stream << "application_kernel_list_asset: " << application_kernel_list_asset
         << std::endl;
  stream << "temp_directory_path: " << temp_directory_path << std::endl;
  stream << "dart_flags:" << std::endl;
  for (const auto& dart_flag : dart_flags) {
    stream << "    " << dart_flag << std::endl;
  }
  stream << "start_paused: " << start_paused << std::endl;
  stream << "trace_skia: " << trace_skia << std::endl;
  stream << "trace_startup: " << trace_startup << std::endl;
  stream << "endless_trace_buffer: " << endless_trace_buffer << std::endl;
  stream << "enable_dart_profiling: " << enable_dart_profiling << std::endl;
  stream << "disable_dart_asserts: " << disable_dart_asserts << std::endl;
  stream << "advisory_script_uri: " << advisory_script_uri << std::endl;
  stream << "disable_dart_asserts: " << disable_dart_asserts << std::endl;
  stream << "advisory_script_entrypoint: " << advisory_script_entrypoint << std::endl;
  stream << "observatory_host: " << observatory_host << std::endl;
  stream << "observatory_port: " << observatory_port << std::endl;
  stream << "ipv6: " << ipv6 << std::endl;
  stream << "use_test_fonts: " << use_test_fonts << std::endl;
  stream << "enable_software_rendering: " << enable_software_rendering
         << std::endl;
  stream << "skia_deterministic_rendering_on_cpu: " << skia_deterministic_rendering_on_cpu
         << std::endl;
  stream << "verbose_logging: " << verbose_logging << std::endl;
  stream << "log_tag: " << log_tag << std::endl;
  stream << "icu_data_path: " << icu_data_path << std::endl;
  stream << "assets_dir: " << assets_dir << std::endl;
  stream << "assets_path: " << assets_path << std::endl;
  stream << "flx_path: " << flx_path << std::endl;
  stream << "use_symbol_snapshot: " << use_symbol_snapshot << std::endl;
  return stream.str();
}

}  // namespace blink
