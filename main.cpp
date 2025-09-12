#include <SDL2/SDL.h>
#include <cmath>
#include <vector>
#include <algorithm>

// Vector class
struct Vec3 {
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator + (const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator - (const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator * (float s) const { return Vec3(x * s, y * s, z * s); }
    Vec3 operator * (const Vec3& v) const { return Vec3(x * v.x, y * v.y, z * v.z); }
    Vec3 operator / (float s) const { return Vec3(x / s, y / s, z / s); }

    Vec3 normalize() const {
        float mag = std::sqrt(x*x + y*y + z*z);
        return Vec3(x / mag, y / mag, z / mag);
    }

    float dot(const Vec3& v) const { return x*v.x + y*v.y + z*v.z; }
};

// Ray class
struct Ray {
    Vec3 origin, direction;
    Ray(const Vec3& origin, const Vec3& direction) : origin(origin), direction(direction) {}
};

// Sphere class
struct Sphere {
    Vec3 center;
    float radius;
    Vec3 color;

    Sphere(const Vec3& center, float radius, const Vec3& color) : center(center), radius(radius), color(color) {}

    bool intersect(const Ray& ray, float& t) const {
        Vec3 oc = ray.origin - center;
        float a = ray.direction.dot(ray.direction);
        float b = 2.0f * oc.dot(ray.direction);
        float c = oc.dot(oc) - radius * radius;
        float discriminant = b*b - 4*a*c;
        if (discriminant < 0) return false;
        else {
            float t0 = (-b - std::sqrt(discriminant)) / (2.0f * a);
            float t1 = (-b + std::sqrt(discriminant)) / (2.0f * a);
            t = (t0 < t1) ? t0 : t1;
            if (t < 0) t = (t0 > t1) ? t0 : t1;
            return t >= 0;
        }
    }
};

// Trace the ray
Vec3 trace(const Ray& ray, const std::vector<Sphere>& spheres, const Vec3& lightPos, int depth = 0) {
    if (depth > 2) return Vec3(0.1f, 0.1f, 0.1f); // Limit recursion

    float nearest = 1e9;
    const Sphere* hitSphere = nullptr;

    for (const auto& sphere : spheres) {
        float t;
        if (sphere.intersect(ray, t)) {
            if (t < nearest) {
                nearest = t;
                hitSphere = &sphere;
            }
        }
    }

    if (hitSphere) {
        Vec3 hitPoint = ray.origin + ray.direction * nearest;
        Vec3 normal = (hitPoint - hitSphere->center).normalize();
        Vec3 lightDir = (lightPos - hitPoint).normalize();

        // Simple Lambertian shading
        float diff = std::max(normal.dot(lightDir), 0.0f);

        // Shadow check
        Ray shadowRay(hitPoint + normal * 0.001f, lightDir);
        bool inShadow = false;
        for (const auto& sphere : spheres) {
            float t;
            if (&sphere != hitSphere && sphere.intersect(shadowRay, t)) {
                inShadow = true;
                break;
            }
        }

        Vec3 baseColor = hitSphere->color * diff;
        if (inShadow) {
            baseColor = baseColor * 0.2f; // Darken in shadow
        }

        // Reflective contribution
        Vec3 reflectDir = ray.direction - normal * 2.0f * ray.direction.dot(normal);
        reflectDir = reflectDir.normalize();
        Ray reflectRay(hitPoint + normal * 0.001f, reflectDir);
        Vec3 reflectedColor = trace(reflectRay, spheres, lightPos, depth + 1);

        // Mix base color and reflected color
        float reflectivity = 0.5f; // You can change this per sphere later
        Vec3 finalColor = baseColor * (1.0f - reflectivity) + reflectedColor * reflectivity;
        return finalColor;
    }

    return Vec3(0.1f, 0.1f, 0.1f); // Background color
}


int main(int argc, char* argv[]) {
    const int width = 800;
    const int height = 600;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Simple Ray Tracer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    std::vector<Sphere> spheres = {
        Sphere(Vec3(-2.0f, 0.0f, -10.0f), 1.0f, Vec3(1.0f, 0.0f, 0.0f)),
        Sphere(Vec3(0.0f, 0.0f, -10.0f), 1.0f, Vec3(0.0f, 1.0f, 0.0f)),
        Sphere(Vec3(2.0f, 0.0f, -10.0f), 1.0f, Vec3(0.0f, 0.0f, 1.0f)),
        Sphere(Vec3(0.0f, -10004.0f, -10.0f), 10000.0f, Vec3(0.8f, 0.8f, 0.8f)) // Reflective floor
    };

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
        }

        // Animate the light position
        float time = SDL_GetTicks() / 1000.0f;
        float radius = 5.0f;
        Vec3 lightPos(radius * cos(time), 5.0f, radius * sin(time) - 10.0f); // Moving around spheres

        SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255); // Dark background
        SDL_RenderClear(renderer);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float u = (2.0f * (x + 0.5f) / width - 1.0f) * (width / (float)height);
                float v = (1.0f - 2.0f * (y + 0.5f) / height);

                Ray ray(Vec3(0.0f, 0.0f, 0.0f), Vec3(u, v, -1.0f).normalize());
                Vec3 color = trace(ray, spheres, lightPos);

                SDL_SetRenderDrawColor(renderer,
                    (Uint8)(std::min(color.x, 1.0f) * 255),
                    (Uint8)(std::min(color.y, 1.0f) * 255),
                    (Uint8)(std::min(color.z, 1.0f) * 255),
                    255);
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
