#include <cmath>
#include <fstream>

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
    // Snellの法則による屈折ベクトル
    double cosi = std::max(-1.0, std::min(1.0, I.x*N.x + I.y*N.y + I.z*N.z));
    double k = 1.0 - eta * eta * (1.0 - cosi * cosi);
    if (k < 0.0) return Vec3(0,0,0); // 全反射
    return I * eta + N * (eta * cosi - std::sqrt(k));
}

int main() {
    const int width = 400;
    const int height = 400;

    std::ofstream out("output.ppm");
    out << "P3\n" << width << " " << height << "\n255\n";

    // カメラ位置
    Vec3 cam(0, 0, 1);

    // 球の中心と半径
    Vec3 sphereCenter(0, 0, -1);
    double sphereR = 0.5;
    double ior = 1.5; // ガラスの屈折率

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            double u = (2.0 * x / width  - 1.0);
            double v = (2.0 * y / height - 1.0);

            Vec3 dir(u, -v, -1); // カメラの前方へ飛ばす
            dir = dir.normalized();

            // 光線-球交差
            Vec3 oc = cam - sphereCenter;
            double b = oc.x * dir.x + oc.y * dir.y + oc.z * dir.z;
            double c = oc.x*oc.x + oc.y*oc.y + oc.z*oc.z - sphereR*sphereR;
            double h = b*b - c;

            Vec3 color(0.2, 0.4, 1.0); // 背景色（ブルー）

            if (h > 0.0) {
                // 衝突点
                double t = -b - std::sqrt(h);
                Vec3 hit = cam + dir * t;

                // 表面法線
                Vec3 N = (hit - sphereCenter).normalized();

                // 屈折方向
                Vec3 r = refract(dir, N, 1.0 / ior);

                // 背景へ飛ばした色（屈折の効果）
                double k = std::max(0.0, r.y * 0.5 + 0.5);
                color = Vec3(1.0*k, 0.7*k, 0.5*k); // 簡易着色
            }

            int r = (int)(255 * std::max(0.0, std::min(1.0, color.x)));
            int g = (int)(255 * std::max(0.0, std::min(1.0, color.y)));
            int b2= (int)(255 * std::max(0.0, std::min(1.0, color.z)));
            out << r << " " << g << " " << b2 << "\n";
        }
    }

    out.close();
    return 0;
}
