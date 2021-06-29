#include "Renderer.hpp"
#include "Scene.hpp"
#include "Triangle.hpp"
#include "Sphere.hpp"
#include "Vector.hpp"
#include "global.hpp"
#include <chrono>

// In the main function of the program, we create the scene (create objects and
// lights) as well as set the options for the render (image width and height,
// maximum recursion depth, field-of-view, etc.). We then call the render
// function().
int main(int argc, char** argv)
{

    // Change the definition here to change resolution
    Scene scene(784, 784);

    init_random_device();

    Material* red = new Material(DIFFUSE, Vector3f(0.0f));
    red->Kd = Vector3f(0.63f, 0.065f, 0.05f);
    red->Ks = Vector3f(0.63f, 0.065f, 0.05f);
    red->ior = 1.46;
    red->diffuseFactor = 1.0;
    red->roughness = 1.0;
    red->f0 = Vector3f(0.03, 0.03, 0.03);
    red->h_alpha = 1.0;
    red->metallic = 0.0f;

    Material* green = new Material(DIFFUSE, Vector3f(0.0f));
    green->Kd = Vector3f(0.14f, 0.45f, 0.091f);
    green->Ks = Vector3f(0.14f, 0.45f, 0.091f);
    green->ior = 1.46;
    green->diffuseFactor = 1.0;
    green->roughness = 1.0;
    green->f0 = Vector3f(0.03, 0.03, 0.03);
    green->h_alpha = 1.0;
    green->metallic = 0.0f;

    Material* white = new Material(DIFFUSE, Vector3f(0.0f));
    white->Kd = Vector3f(0.725f, 0.71f, 0.68f);
    white->Ks = Vector3f(0.725f, 0.71f, 0.68f);
    white->ior = 1.46;
    white->diffuseFactor = 1.0;
    white->roughness = 1.0;
    white->f0 = Vector3f(0.03, 0.03, 0.03);
    white->h_alpha = 1.0;
    white->metallic = 0.0f;

    Material* copper = new Material(MICROFACET, Vector3f(0.0f));
    copper->Kd = Vector3f(0.1914,0.125,0);
    copper->Ks = Vector3f(0.5977);
    copper->ior = 2.0f;
    copper->diffuseFactor = 0.;
    copper->roughness = 0.1;
    copper->f0 = Vector3f(0.95, 0.64, 0.54);
    copper->h_alpha = 0.2;
    copper->metallic = 0.8f;


    Material* mirror = new Material(MICROFACET, Vector3f(0.0f));
    mirror->Kd = Vector3f(0.1914,0.125,0);
    mirror->Ks = Vector3f(0.5977);
    mirror->ior = 2.0f;
    mirror->diffuseFactor = 0.;
    mirror->roughness = 0.1;
    mirror->f0 = Vector3f(1.00, 0.71, 0.29);
    mirror->h_alpha = 0.0;
    mirror->metallic = 0.8f;
    

    Material* light = new Material(DIFFUSE, 0.5*(8.0f * Vector3f(0.747f+0.058f, 0.747f+0.258f, 0.747f) + 15.6f * Vector3f(0.740f+0.287f,0.740f+0.160f,0.740f) + 18.4f *Vector3f(0.737f+0.642f,0.737f+0.159f,0.737f)));
    light->Kd = Vector3f(0.65f);
    light->Ks = Vector3f(0.65f);

    MeshTriangle floor("../models/cornellbox/floor.obj", white);
    MeshTriangle shortbox("../models/cornellbox/shortbox.obj", white);
    MeshTriangle tallbox("../models/cornellbox/tallbox.obj", mirror);
    // Sphere sphere = Sphere(Vector3f(300, 100, 300), 100, mirror);
    // MeshTriangle bunny("../models/bunny/bunny.obj", mirror, Vector3f(300, 0, 300), Vector3f(2000));
    MeshTriangle teapot("../models/teapot/teapot.obj", mirror, Vector3f(186, 166, 169), Vector3f(30));
    MeshTriangle left("../models/cornellbox/left.obj", red);
    MeshTriangle right("../models/cornellbox/right.obj", green);
    MeshTriangle light_("../models/cornellbox/light.obj", light);

    scene.Add(&floor);
    scene.Add(&shortbox);
    scene.Add(&tallbox);
    // scene.Add(&bunny);
    scene.Add(&teapot);
    // scene.Add(&sphere);
    scene.Add(&left);
    scene.Add(&right);
    scene.Add(&light_);

    scene.buildBVH();

    Renderer r;

    auto start = std::chrono::system_clock::now();
    r.Render(scene);
    auto stop = std::chrono::system_clock::now();

    std::cout << "Render complete: \n";
    std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::hours>(stop - start).count() << " hours\n";
    std::cout << "          : " << std::chrono::duration_cast<std::chrono::minutes>(stop - start).count() << " minutes\n";
    std::cout << "          : " << std::chrono::duration_cast<std::chrono::seconds>(stop - start).count() << " seconds\n";

    delete_random_device();
    return 0;
}