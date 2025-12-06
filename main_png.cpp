#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdio>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct Vec3 {
    double x, y, z;
    Vec3(double a=0, double b=0, double c=0): x(a), y(b), z(c) {}
    Vec3 operator+(const Vec3& v) const { return Vec3(x+v.x, y+v.y, z+v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x-v.x, y-v.y, z-v.z); }
    Vec3 operator*(double t) const { return Vec3(x*t, y*t, z*t); }
    Vec3 normalized() const {
        double s = std::sqrt(x*x + y*y + z*z);
        return Vec3(x/s, y/s, z/s);
    }
};

Vec3 refract(const Vec3& I, const Vec3& N, double eta) {
    double cosi = std::max(-1.0, std::min(1.0, I.x*N.x + I.y*N.y + I.z*N.z));
    double k = 1.0 - eta * eta * (1.0 - cosi * cosi);
    if (k < 0.0) return Vec3(0,0,0); // 全反射
    return I * eta + N * (eta * cosi - std::sqrt(k));
}

/* --------------------------
   PNG/JPG 環境マップ読み込み
--------------------------- */
unsigned char* envData = nullptr;
int envW=0, envH=0, envC=0;

void loadEnv(const char* filename) {
    envData = stbi_load(filename, &envW, &envH, &envC, 3);
    if (!envData) {
        printf("Failed to load environment map: %s\n", filename);
        exit(1);
    }
    printf("Environment map loaded: %s (%d x %d)\n", filename, envW, envH);
}

/* -------------------------------------
   レイ方向 d → 環境マップから色を得る
   （等距離射影 equirectangular）
-------------------------------------- */
Vec3 sampleEnv(const Vec3& d) {
    double offset = M_PI;  // 180° 横向きにずらす

    double theta = atan2(d.z, d.x) + offset;
    double phi   = acos(std::clamp(d.y, -1.0, 1.0));

    double u = (theta + M_PI) / (2.0 * M_PI);
    double v = phi / M_PI;

    u = u - floor(u);   // wrap（0〜1に戻す）

    int x = std::clamp(int(u * envW), 0, envW - 1);
    int y = std::clamp(int(v * envH), 0, envH - 1);

    int idx = (y * envW + x) * 3;

    return Vec3(
        envData[idx]   / 255.0,
        envData[idx+1] / 255.0,
        envData[idx+2] / 255.0
    );
}

/* --------------------------
   レンダリング本体
--------------------------- */
int main_png() {
    const int width = 800;
    const int height = 800;

    loadEnv("env.png");  // ★ 環境マップを読み込む（PNG/JPG OK）

    std::vector<unsigned char> image(width * height * 3);

    Vec3 cam(0, 0, 1);
    Vec3 sphereCenter(0, 0, -1);
    double sphereR = 0.9;
    double ior = 2.5;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            double fov = 70.0 * M_PI / 180.0;  // 50mm相当
            double aspect = (double)width / height;

            double px = ( (x + 0.5) / width  * 2.0 - 1.0 ) * tan(fov/2.0) * aspect;
            double py = ( (y + 0.5) / height * 2.0 - 1.0 ) * tan(fov/2.0);

            Vec3 dir(px, -py, -1); 
            dir = dir.normalized();

            
            Vec3 oc = cam - sphereCenter;
            double b = oc.x * dir.x + oc.y * dir.y + oc.z * dir.z;
            double c = oc.x*oc.x + oc.y*oc.y + oc.z*oc.z - sphereR*sphereR;
            double h = b*b - c;

            Vec3 color = sampleEnv(dir); // 背景（デフォルト）

            if (h > 0.0) {
                double t = -b - std::sqrt(h);
                Vec3 hit = cam + dir * t;
                Vec3 N = (hit - sphereCenter).normalized();

                Vec3 refr = refract(dir, N, 1.0 / ior).normalized();

                // ★ 屈折方向の色を環境マップから取得
                color = sampleEnv(refr);
            }

            int idx = (y * width + x) * 3;
            image[idx+0] = (unsigned char)(255 * std::clamp(color.x, 0.0, 1.0));
            image[idx+1] = (unsigned char)(255 * std::clamp(color.y, 0.0, 1.0));
            image[idx+2] = (unsigned char)(255 * std::clamp(color.z, 0.0, 1.0));
        }
    }

    stbi_write_png("output.png", width, height, 3, image.data(), width * 3);

    printf("Saved output.png\n");
    return 0;
}


/* --------------------------
   Windows 用エントリポイント
--------------------------- */
int main() {
    return main_png();
}
