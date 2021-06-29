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
    int spp = 1024;
    std::cout << "SPP: " << spp << "\n";

    std::atomic<int> total_num;

    auto counter = std::thread([&]() {
        while(total_num != scene.height*scene.width) {
            usleep(1000);
            UpdateProgress(total_num / (float)(scene.height*scene.width));
        }
    });

    srand(1);

    float width = scene.width;
    float height = scene.height;

    int NUM_OF_PRODUCERS = 8;
    std::thread producers[NUM_OF_PRODUCERS];

    auto producer_task = [&](int start, int end) {
        for (uint32_t j = start; j < end; ++j) {
            for (uint32_t i = 0; i < scene.width; ++i) {

                float x = (2 * (i + 0.5) / (float)scene.width - 1) *
                                imageAspectRatio * scale;
                float y = (1 - 2 * (j + 0.5) / (float)scene.height) * scale;

                Vector3f dir = normalize(Vector3f(-x, y, 1));
                
                const Ray primaryRay = Ray(eye_pos, dir);
                auto fb_ptr = framebuffer;

                scene.t_pool.produce([&scene, spp, primaryRay, 
                    fb_ptr, i, j, &total_num]() {
                    Vector3f mean = 0.0f;
                    
                    for (int k = 0; k < spp; k++){
                        mean += scene.castRay(primaryRay, 0) / spp;  
                    }
                    fb_ptr[j*scene.width+i] = mean;
                    ++total_num;
                });              
            }
        }
    };

    for (int i = 0; i < NUM_OF_PRODUCERS; ++i) {
        int start = (i)*(height/NUM_OF_PRODUCERS);
        int end = (i+1)*(height/NUM_OF_PRODUCERS);
        if (i == NUM_OF_PRODUCERS-1) end = height; 
        producers[i] = std::thread(producer_task, start, end);
    }

    for (int i = 0; i < NUM_OF_PRODUCERS; ++i) {
        producers[i].join();
    }

    counter.join();
    UpdateProgress(1.f);

    // save framebuffer to file
    FILE* fp = fopen("binary.ppm", "wb");
    // int count = 0;
    // int m_count = 0;
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        static unsigned char color[3];
        if (framebuffer[i].norm() > EPSILON) {
            // ++count;
            color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f));
            color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f));
            color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f));
        } else {
            color[0] = 0;
            color[1] = 0;
            color[2] = 0;
        }
        // if (color[0] > EPSILON || color[1] > EPSILON || color[2] > EPSILON) ++m_count;
        fwrite(color, 1, 3, fp);
    }
    // std::cout << count << " " <<m_count << std::endl;
    fclose(fp);    
}
