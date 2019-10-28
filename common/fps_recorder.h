//
// Created by sunkun01 on 2019-11-06.
//

#ifndef FLUTTER_FPS_RECORDER_H
#define FLUTTER_FPS_RECORDER_H

#include <vector>
#include <string>
#include <map>
#include <flutter/fml/time/time_delta.h>
#include "flutter/fml/synchronization/waitable_event.h"

namespace flutter {
    class FpsRecorder {
    public:
        static FpsRecorder *Current() {
            static FpsRecorder instance;
            return &instance;
        }

        void AddFrameCount(int count, const fml::TimeDelta &timeDelta = fml::TimeDelta::Zero());

        void AddDrawCount(const fml::TimeDelta &timeDelta);

        void StartRecordFps(const std::string &key);

        std::vector<double> ObtainFpsData(const std::string &key, bool stopRecord);

    private:
        FpsRecorder();

        ~FpsRecorder();

        bool is_drawn;
        std::mutex draw_lock;
        std::map<std::string, std::pair<size_t, size_t >> fps_data_;
        std::map<std::string, std::pair<size_t, size_t >> ui_time_;
        std::map<std::string, std::pair<size_t, size_t >> gpu_time_;
    };
}
#endif //FLUTTER_FPS_RECORDER_H
