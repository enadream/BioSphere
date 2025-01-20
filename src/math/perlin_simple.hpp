#include <iostream>
#include <cmath>
#include <vector>

class PerlinNoise {
private:
    std::vector<int> p;

    int fastfloor(double x) {
        return x > 0 ? (int)x : (int)x - 1;
    }

    double fade(double t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    double lerp(double t, double a, double b) {
        return a + t * (b - a);
    }

    double grad(int hash, double x, double y, double z) {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

public:
    PerlinNoise(int seed = 0) {
        p.resize(512);
        for (int i = 0; i < 256; ++i) {
            p[i] = i;
        }
        std::srand(seed);
        for (int i = 255; i > 0; --i) {
            int j = std::rand() % (i + 1);
            std::swap(p[i], p[j]);
        }
        for (int i = 0; i < 256; ++i) {
            p[256 + i] = p[i];
        }
    }

    double noise(double x, double y, double z = 0) {
        int X = fastfloor(x) & 255;
        int Y = fastfloor(y) & 255;
        int Z = fastfloor(z) & 255;
        x -= fastfloor(x);
        y -= fastfloor(y);
        z -= fastfloor(z);
        double u = fade(x);
        double v = fade(y);
        double w = fade(z);
        int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
        int B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;
        return lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z)),
                               lerp(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))),
                       lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1), grad(p[BA + 1], x - 1, y, z - 1)),
                               lerp(u, grad(p[AB + 1], x, y - 1, z - 1), grad(p[BB + 1], x - 1, y - 1, z - 1))));
    }
};