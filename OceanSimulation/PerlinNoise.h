#ifndef PERLIN_NOISE_H_
#define PERLIN_NOISE_H_

#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cassert>
#include <vector>
#include <string>

//using namespace std;


class PerlinNoise
{
public:
    int numX, numY, numOctaves, primeIndex;
    double persistence = 0.5;

    PerlinNoise(int sizeX = 512, int sizeY = 512, int octaves = 7, double persist = 0.5)
        : numX(sizeX), numY(sizeY)
        , numOctaves(octaves), persistence(persist)
        , primeIndex(0)
    {
        SavePPM("perlinnoise.ppm");
    }

private:

    double Noise(int i, int x, int y); 
    double SmoothedNoise(int i, int x, int y); 
    double Interpolate(double a, double b, double x); 
    double InterpolatedNoise(int i, double x, double y); 
    void SavePPM(std::string filename)
    {
            std::vector<double> value(numX * numY);
            double minValue = 1.0, maxValue = -1.0;
            for (int i = 0; i < numX * numY; i++)
            {
                double v = GetNoiseValue(i / numX, i % numX);
                if (v > maxValue) maxValue = v;
                if (v < minValue) minValue = v;
                value[i] = v;
            }
            assert(!filename.empty());
            FILE *f = fopen(filename.c_str(), "w");         // Write image to PPM file.
            fprintf(f, "P3\n%d %d\n%d\n", numX, numY, 255);
            for (int i = 0; i < numX * numY; i++)
            {
                double v = value[i];
                v = (v - minValue) / (maxValue - minValue);
                assert(v >= 0.0);
                int a = v * 255 > 255 ? 255 : v * 255;
                fprintf(f, "%d %d %d ", a, a, a);
            }
            fclose(f);
        }

public:
    double GetNoiseValue(double x, double y); 
};


#endif