//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"


void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
        const Ray &ray,
        const std::vector<Object*> &objects,
        float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }


    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) {

    Intersection current = intersect(ray);
    if (current.happened) {
        Vector3f wo = normalize(-ray.direction);
        // Direct lighting calculation
        Intersection sample;
        float pdf;
        sampleLight(sample, pdf);
        
        Vector3f dl_vec = sample.coords - current.coords;
        
        Vector3f N = normalize(current.normal);
        Vector3f ws = normalize(dl_vec);
        float dl_distance = dl_vec.norm();
        Vector3f NN = normalize(sample.normal);

        Vector3f p = current.coords;

        auto dl_intersection = intersect(Ray(current.coords, ws));

        Vector3f l_dir(0,0,0);    
        if (dl_intersection.distance > dl_distance - 0.01) {
            l_dir = ((sample.emit * current.m->eval(wo, ws, N) * dotProduct(NN, 
                -ws) * dotProduct(N, ws))/ (dl_distance*dl_distance * pdf));
        }

        float random = (rand()%1000)/1000.0f;

        if (random > Scene::RussianRoulette) {
            return l_dir;
        }

        Vector3f wi = normalize(current.m->sample(wo, N));

        return l_dir + castRay(Ray(p,wi),depth+1) * 
            current.m->eval(wi, wo, N) * 
            dotProduct(N, wi)/(RussianRoulette * 
            current.m->pdf(wi, wo, N));
    }
    
    return Vector3f(0,0,0);
}