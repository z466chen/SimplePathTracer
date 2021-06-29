//
// Created by Göksu Güvendiren on 2019-05-14.
//

#pragma once

#include <vector>
#include "Vector.hpp"
#include "Object.hpp"
#include "Light.hpp"
#include "AreaLight.hpp"
#include "BVH.hpp"
#include "Ray.hpp"

#include <semaphore.h>
#include <future>
#include <mutex>
#include <unistd.h>
#include <thread>
#include <random>
#include <queue>

inline int rec_dis(int a, int b, int b_size) {
    return (a >= b)? (a - b):(a + b_size - b);
}

class Scene
{
public:
    // setting up options

    class ThreadPool {
    public:
        const static int THREAD_NUM = 32;

        const static int WINDOW_SIZE = 1024;

        const static int BUFFER_SIZE = 2048;

        
        // std::mutex p_locks[THREAD_NUM];
        std::thread consumers [THREAD_NUM];
        std::function<void(void)> waitQueue[BUFFER_SIZE];
        int consumer_id = 0;
        int producer_id = 0;

        std::condition_variable consumer_cv, producer_cv;
        
        // std::mutex p_lock,c_lock;
        std::mutex q_lock;
        // int nthread_waits = 0;
        std::atomic<bool> terminate = false;

        ThreadPool() {

            for (int i = 0; i < THREAD_NUM; ++i) {
                int j = i;
                consumers[i] = std::thread([&](){
                    init_random_device();
                    while(!terminate) {
                        std::function<void(void)> target;
                        {
                            std::unique_lock<std::mutex> lock(q_lock);

                            consumer_cv.wait(lock, [&](){
                                int dis = rec_dis(producer_id, consumer_id, BUFFER_SIZE);
                                return ((dis > 0) && (dis <= WINDOW_SIZE)) || (terminate);
                            });
                            if(terminate) break;
                            target = std::move(waitQueue[consumer_id]);
                            int dis = rec_dis(producer_id, consumer_id, BUFFER_SIZE);
                            if (dis == WINDOW_SIZE) {
                                producer_cv.notify_all();
                            }
                            consumer_id = (consumer_id + 1)%BUFFER_SIZE;
                        }
                        target();
                    }
                    delete_random_device();
                });
            }
        }

        void produce(std::function<void(void)> &&task) {
            std::unique_lock<std::mutex> lock(q_lock);
            producer_cv.wait(lock, [&](){
                int dis = rec_dis(producer_id, consumer_id, BUFFER_SIZE);
                return ((dis >= 0) && (dis < WINDOW_SIZE)) || (terminate);
            });     
            if(terminate) return;    
            waitQueue[producer_id] = std::move(task);
            int dis = rec_dis(producer_id, consumer_id, BUFFER_SIZE);
            if (dis == 0) {
                consumer_cv.notify_all(); 
            }
            producer_id = (producer_id+1)%BUFFER_SIZE;
        }

        ~ThreadPool() {
            terminate = true;
            {
                std::unique_lock<std::mutex> lock(q_lock);
                consumer_cv.notify_all();
                producer_cv.notify_all();
            }
            for (auto & t: consumers) {
                if (t.joinable()) {
                    t.join();
                }
            }          
        }
    } t_pool;

    int width = 1280;
    int height = 960;
    double fov = 40;
    Vector3f backgroundColor = Vector3f(0.235294, 0.67451, 0.843137);
    int maxDepth = 1;
    float RussianRoulette = 0.8;

    Scene(int w, int h) : width(w), height(h) {
        // sem_init(&thread_limiter, 1, THREAD_NUM);
    }

    ~Scene() {
        // sem_destroy(&thread_limiter);
    }

    void Add(Object *object) { objects.push_back(object); }
    void Add(std::unique_ptr<Light> light) { lights.push_back(std::move(light)); }

    const std::vector<Object*>& get_objects() const { return objects; }
    const std::vector<std::unique_ptr<Light> >&  get_lights() const { return lights; }
    Intersection intersect(const Ray& ray) const;
    BVHAccel *bvh;
    void buildBVH();
    
    Vector3f castRay(const Ray &ray, int depth);

    void sampleLight(Intersection &pos, float &pdf) const;
    bool trace(const Ray &ray, const std::vector<Object*> &objects, float &tNear, uint32_t &index, Object **hitObject);
    std::tuple<Vector3f, Vector3f> HandleAreaLight(const AreaLight &light, const Vector3f &hitPoint, const Vector3f &N,
                                                   const Vector3f &shadowPointOrig,
                                                   const std::vector<Object *> &objects, uint32_t &index,
                                                   const Vector3f &dir, float specularExponent);

    // creating the scene (adding objects and lights)
    std::vector<Object* > objects;
    std::vector<std::unique_ptr<Light> > lights;

    // Compute reflection direction
    Vector3f reflect(const Vector3f &I, const Vector3f &N) const
    {
        return I - 2 * dotProduct(I, N) * N;
    }



// Compute refraction direction using Snell's law
//
// We need to handle with care the two possible situations:
//
//    - When the ray is inside the object
//
//    - When the ray is outside.
//
// If the ray is outside, you need to make cosi positive cosi = -N.I
//
// If the ray is inside, you need to invert the refractive indices and negate the normal N
    Vector3f refract(const Vector3f &I, const Vector3f &N, const float &ior) const
    {
        float cosi = clamp(-1, 1, dotProduct(I, N));
        float etai = 1, etat = ior;
        Vector3f n = N;
        if (cosi < 0) { cosi = -cosi; } else { std::swap(etai, etat); n= -N; }
        float eta = etai / etat;
        float k = 1 - eta * eta * (1 - cosi * cosi);
        return k < 0 ? 0 : eta * I + (eta * cosi - sqrtf(k)) * n;
    }



    // Compute Fresnel equation
//
// \param I is the incident view direction
//
// \param N is the normal at the intersection point
//
// \param ior is the material refractive index
//
// \param[out] kr is the amount of light reflected
    void fresnel(const Vector3f &I, const Vector3f &N, const float &ior, float &kr) const
    {
        float cosi = clamp(-1, 1, dotProduct(I, N));
        float etai = 1, etat = ior;
        if (cosi > 0) {  std::swap(etai, etat); }
        // Compute sini using Snell's law
        float sint = etai / etat * sqrtf(std::max(0.f, 1 - cosi * cosi));
        // Total internal reflection
        if (sint >= 1) {
            kr = 1;
        }
        else {
            float cost = sqrtf(std::max(0.f, 1 - sint * sint));
            cosi = fabsf(cosi);
            float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
            float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
            kr = (Rs * Rs + Rp * Rp) / 2;
        }
        // As a consequence of the conservation of energy, transmittance is given by:
        // kt = 1 - kr;
    }
};