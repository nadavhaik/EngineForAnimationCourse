//
// Created by nadavh on 2/22/23.
//

#include "PeriodicExecutor.h"

#include <utility>

PeriodicExecutor::PeriodicExecutor(int interval, std::function<void(void)> func): interval(interval),
func(std::move(func)), running(false) {}

void PeriodicExecutor::Start() {
    running = true;
    std::thread([this]()
                {
                    while (running) {
                        func();
                        std::this_thread::sleep_for(
                                std::chrono::milliseconds(interval));
                    }
                }).detach();
}

void PeriodicExecutor::Stop() {running = false;}
