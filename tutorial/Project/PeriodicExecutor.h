//
// Created by nadavh on 2/22/23.
//

#include <thread>
#include <functional>

#ifndef ENGINEREWORK_PERIODICTHREAD_H
#define ENGINEREWORK_PERIODICTHREAD_H



class PeriodicExecutor {
public:
    PeriodicExecutor(int interval, std::function<void(void)> func);
    ~PeriodicExecutor()=default;
    void Start();
    void Stop();
private:
    int interval;
    std::function<void(void)> func;
    bool running;
};


#endif //ENGINEREWORK_PERIODICTHREAD_H
