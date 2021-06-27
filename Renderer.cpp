//
// Created by goksu on 2/25/20.
//

#include <fstream>
#include "Scene.hpp"
#include "Renderer.hpp"
#include <atomic>


inline float deg2rad(const float& deg) { return deg * M_PI / 180.0; }

const float EPSILON = 0.00001;

// The main render function. This where we iterate over all pixels in the image,
// generate primary rays and cast these rays into the scene. The content of the
// framebuffer is saved to a file.
void Renderer::Render(Scene& scene)
{
    Vector3f framebuffer[scene.width * scene.height];

    float scale = tan(deg2rad(scene.fov * 0.5));
    float imageAspectRatio = scene.width / (float)scene.height;
    Vector3f eye_pos(278, 273, -800);
    int m = 0;

    // change the spp value to change sample ammount
    int spp = 32;
    std::cout << "SPP: " << spp << "\n";

    std::atomic<int> total_num;

    auto counter = std::thread([&]() {
        while(total_num != scene.height*scene.width) {
            usleep(1000);
            UpdateProgress(total_num / (float)(scene.height*scene.width));
        }
    });

    srand(1);

    // for (int i = 0; i < 10000; ++i) {
    //     int j = i;
    //     scene.t_pool.execThread([j](){
            
    //         usleep(100);
    //         std::cout << j << std::endl;
    //     });
    // }

    float width = scene.width;
    float height = scene.height;
    for (uint32_t j = 0; j < scene.height; ++j) {
        for (uint32_t i = 0; i < scene.width; ++i) {

            float x = (2 * (i + 0.5) / (float)scene.width - 1) *
                            imageAspectRatio * scale;
            float y = (1 - 2 * (j + 0.5) / (float)scene.height) * scale;

            scene.t_pool.execThread([eye_pos, x, y, 
                &framebuffer, m, spp, 
                &scene, &total_num]() {

                

                Vector3f dir = normalize(Vector3f(-x, y, 1));
                Vector3f mean = 0.0f;
                
                const Ray primaryRay = Ray(eye_pos, dir);
                
                Intersection current = scene.intersect(primaryRay);
                if (current.emit.norm() > EPSILON) {
                    mean = current.emit;
                } else {
                    for (int k = 0; k < spp; k++){
                        mean += scene.castRay(primaryRay, 0) / spp;  
                    }
                }
                framebuffer[m] = mean;
                ++total_num;
            });
            m++;
        }
    }

    counter.join();
    UpdateProgress(1.f);

    // save framebuffer to file
    FILE* fp = fopen("binary.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f));
        color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f));
        color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f));
        fwrite(color, 1, 3, fp);
    }
    fclose(fp);    
}
