// BD ADD:
// Created by sunkun01 on 2019-11-06.
//

#include <flutter/fml/logging.h>
#include "fps_recorder.h"

namespace flutter {
    static const std::string kFpsKeyOfAppAvg = "ApplicationAverage";
    static const double kFrameCountOneSecond = 60.0;

    FpsRecorder::FpsRecorder() : is_drawn(false) {
        fps_data_[kFpsKeyOfAppAvg] = std::pair<size_t, size_t>(0, 0);
        ui_time_[kFpsKeyOfAppAvg] = std::pair<size_t, int64_t>(0, 0);
        gpu_time_[kFpsKeyOfAppAvg] = std::pair<size_t, int64_t>(0, 0);
    }

    FpsRecorder::~FpsRecorder() = default;

    /**
     * frame count++ when vsync arrive
     * add missing count and cost time when UI Thread finished
     * @param count: value is 1 when vsync arrive,value is missCount when UI Thread finished
     * @param timeDelta: has data when UI Thread finished
     */
    void FpsRecorder::AddFrameCount(int count, const fml::TimeDelta &timeDelta) {
        draw_lock.lock();
        if (count > 0) {
            for (auto &it : fps_data_) {
                it.second.first += count; // add FrameCount
            }
        }
        if (timeDelta == fml::TimeDelta::Zero()) {
            is_drawn = false;
        } else { // add CostTime
            for (auto &it : ui_time_) {
                it.second.first += 1;
                it.second.second += timeDelta.ToMicroseconds();
            }
        }
        draw_lock.unlock();
    }

    void FpsRecorder::AddDrawCount(const fml::TimeDelta &timeDelta) {
        draw_lock.lock();
        for (auto &it : fps_data_) {
            if (it.second.second >= it.second.first) { // left drawCount from last record,abandon this drawCount
                continue;
            }
            it.second.second += 1;
        }
        is_drawn = true;
        for (auto &it : gpu_time_) { // add costTime
            if (ui_time_[it.first].first == 0) { // has no frameCost,abandon this
                continue;
            }
            it.second.first += 1;
            it.second.second += timeDelta.ToMicroseconds();
        }
        draw_lock.unlock();
    }

    void FpsRecorder::StartRecordFps(const std::string &key) {
        draw_lock.lock();
        fps_data_[key] = std::make_pair(0, 0);
        ui_time_[key] = std::make_pair(0, 0);
        gpu_time_[key] = std::make_pair(0, 0);
        draw_lock.unlock();
    }

    /**
     *
     * @param key
     * @param stopRecord :true :Clear Data
     * @return
     */
    std::vector<double> FpsRecorder::ObtainFpsData(const std::string &key, bool stopRecord) {
        draw_lock.lock();
        std::vector<double> result(3, 0.0);
        auto it = fps_data_.find(key);
        if (it != fps_data_.end()) {
            auto fps_value = it->second;
            if (fps_value.first == 0) { // frame count is 0
                result[0] = -1;
            } else {
                result[0] =
                        (fps_value.second + (is_drawn ? 0 : 1)) * kFrameCountOneSecond / fps_value.first; // Fps value
                if (ui_time_[key].first != 0) {
                    result[1] = ui_time_[key].second / 1000.0 / ui_time_[key].first; // convert to millSeconds
                }
                if (gpu_time_[key].first != 0) {
                    result[2] = gpu_time_[key].second / 1000.0 / gpu_time_[key].first; // convert to millSeconds
                }
            }
            if (stopRecord) {
                fps_data_.erase(key);
                ui_time_.erase(key);
                gpu_time_.erase(key);
            }
        } else { // do not find the data for this key
            result[0] = -2;
        }
        draw_lock.unlock();
        return result;
    }
}