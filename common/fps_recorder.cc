//
// Created by sunkun01 on 2019-11-06.
//

#include <flutter/fml/logging.h>
#include "fps_recorder.h"

/**
 * 各线程繁忙情况，参见老方案
 */
namespace flutter {
    static const std::string kAvg = "ApplicationAverage";

    FpsRecorder::FpsRecorder() : is_drawn(false) {
        fps_data_[kAvg] = std::pair<size_t, size_t>(0, 0);
        ui_time_[kAvg] = std::pair<size_t, size_t>(0, 0);
        gpu_time_[kAvg] = std::pair<size_t, size_t>(0, 0);
    }

    FpsRecorder::~FpsRecorder() = default;

    void FpsRecorder::AddFrameCount(int count, const fml::TimeDelta &timeDelta) {
        draw_lock.lock();
        if (count > 0) {
            for (auto &it : fps_data_) {
                it.second.first += count;
            }
        }
        if (timeDelta == fml::TimeDelta::Zero()) {
            is_drawn = false;
        } else {
            for (auto &it : ui_time_) {
                it.second.first += 1;
                it.second.second += timeDelta.ToMilliseconds();
            }
        }
        draw_lock.unlock();
    }

    void FpsRecorder::AddDrawCount(const fml::TimeDelta &timeDelta) {
        draw_lock.lock();
//        FML_LOG(ERROR) << "=============AddDrawCount" << is_drawn;
        if (!is_drawn) {
            for (auto &it : fps_data_) {
                it.second.second += 1;
            }
            is_drawn = true;
        }
        for (auto &it : gpu_time_) {
            it.second.first += 1;
            it.second.second += timeDelta.ToMilliseconds();
        }
        draw_lock.unlock();
    }

    void FpsRecorder::StartRecordFps(const std::string &key) {
        draw_lock.lock();
        FML_LOG(ERROR) << "=============StartRecordFps:" << key;
        fps_data_[key] = std::pair<size_t, size_t>(0, 0);
        ui_time_[key] = std::pair<size_t, size_t>(0, 0);
        gpu_time_[key] = std::pair<size_t, size_t>(0, 0);
        draw_lock.unlock();
    }

    std::vector<double> FpsRecorder::ObtainFpsData(const std::string &key, bool stopRecord) {
        draw_lock.lock();
        std::vector<double> result;
        result.resize(3);
        auto it = fps_data_.find(key);
        if (it != fps_data_.end()) {
            auto fps_value = it->second;
            FML_LOG(ERROR) << "=============GetFps:" << fps_value.first << "===drawCount:" << fps_value.second << key;
            if (fps_value.first == 0) {
                result[0] = -1;
                result[1] = 0;
                result[2] = 0;
            } else {
                result[0] = (fps_value.second + (is_drawn ? 0 : 1)) * 60.0 / fps_value.first;
                result[1] = ui_time_[key].second * 1.0 / ui_time_[key].first;
                result[2] = gpu_time_[key].second * 1.0 / gpu_time_[key].first;
            }
            if (stopRecord) {
                fps_data_.erase(key);
                ui_time_.erase(key);
                gpu_time_.erase(key);
            }
        } else {
            result[0] = -2;
            result[1] = 0;
            result[2] = 0;
        }
        draw_lock.unlock();
        return result;
    }
}