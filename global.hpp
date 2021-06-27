#pragma once
#include <iostream>
#include <cmath>
#include <random>
#include <unordered_map>
#include <thread>

#undef M_PI
#define M_PI 3.141592653589793f

extern const float  EPSILON;
const float kInfinity = std::numeric_limits<float>::max();

inline std::unordered_map<size_t,std::random_device *> random_devices;
inline std::unordered_map<size_t, std::mt19937> rngs;

inline void init_random_device() {
    std::random_device *dev = new std::random_device();
    int hash = std::hash<std::thread::id>{}(std::this_thread::get_id());
    rngs[hash] = std::mt19937((*dev)());
    random_devices[hash] = dev;
}

inline void delete_random_device() {
    int hash = std::hash<std::thread::id>{}(std::this_thread::get_id());
    try {
        delete random_devices[hash];
    } catch(...) {}
}

inline float clamp(const float &lo, const float &hi, const float &v)
{ return std::max(lo, std::min(hi, v)); }

inline  bool solveQuadratic(const float &a, const float &b, const float &c, float &x0, float &x1)
{
    float discr = b * b - 4 * a * c;
    if (discr < 0) return false;
    else if (discr == 0) x0 = x1 = - 0.5 * b / a;
    else {
        float q = (b > 0) ?
                  -0.5 * (b + sqrt(discr)) :
                  -0.5 * (b - sqrt(discr));
        x0 = q / a;
        x1 = c / q;
    }
    if (x0 > x1) std::swap(x0, x1);
    return true;
}

inline float get_random_float()
{
    // std::random_device dev;
    // std::mt19937 rng(dev());
    int hash = std::hash<std::thread::id>{}(std::this_thread::get_id());
    std::uniform_real_distribution<float> dist(0.f, 1.f); // distribution in range [1, 6]

    return dist(rngs[hash]);
}

inline void UpdateProgress(float progress)
{
    int barWidth = 70;

    std::cout << "[";
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << " %\r";
    std::cout.flush();
};
