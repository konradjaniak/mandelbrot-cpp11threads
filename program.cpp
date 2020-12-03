/*
    Compile: g++ program.cpp -o program -O3 -lpthread
    Use: ./program -PX (X is number of threads, ex: ./program -P2 means 3 threads)
    Threads range 1..6
*/

#include <chrono>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <thread>
#include <vector>

const int MIN_THREADS = 1;
const int MAX_THREADS = 6;
int numOfThreads = 1;

/* screen ( integer) coordinate */
const int SIZE = 5000;
const int iX_MAX = SIZE;
const int iY_MAX = SIZE;

const double CX_MIN = -2.5;
const double CX_MAX = 1.5;
const double CY_MIN = -2.0;
const double CY_MAX = 2.0;

const double PIXEL_WIDTH = (CX_MAX - CX_MIN) / iX_MAX;
const double PIXEL_HEIGHT = (CY_MAX - CY_MIN) / iY_MAX;

/* color component ( R or G or B) is coded from 0 to 255 */
/* it is 24 bit color RGB file */
const int MAX_COLOR_COMPONENT_VALUE = 255;

const int ITERATION_MAX = 250;
const char *filename = "result.ppm";

// bail-out value, radius of circle
const double ESCAPE_RADIUS = 2;
const double ER2 = ESCAPE_RADIUS * ESCAPE_RADIUS;

unsigned char color[SIZE][SIZE][3];

int getNumOfThreads(const char* arg);
void processData(int packetNumber, int columnsPerThread, int remainder);

int main(int argc, char ** argv)
{
    if (argc < 2)
    {
        std::cerr << "Nie podano rozmiaru tablicy\n";
        return 1;
    }

    int numOfThreads = getNumOfThreads(argv[1]);

    FILE * fp;
    fp = fopen(filename, "wb");

    // write ASCII header to the file
    fprintf(fp, "P6\n %s\n %d\n %d\n %d\n", "# ", iX_MAX, iY_MAX, MAX_COLOR_COMPONENT_VALUE);

    int columnsPerThread = (numOfThreads > 1) ? (SIZE / numOfThreads) : SIZE;
    int remainingColumns = SIZE % numOfThreads;
    int packetNumber = 0;

    std::vector<std::thread> threads;

    int ctr = 0;

    auto timeStart = std::chrono::steady_clock::now();

    if (numOfThreads > 1)
    {
        for (int i = 0; i < numOfThreads - 1; i++)
        {
            threads.push_back(std::thread(processData, packetNumber, columnsPerThread, 0));
            packetNumber++;
        }
    }
    
    processData(packetNumber, columnsPerThread, remainingColumns);

    if (numOfThreads > 1)
    {
        for (int i = 0; i < numOfThreads - 1; i++)
        {
            threads[i].join();
        }
    }

    auto timeEnd = std::chrono::steady_clock::now();
    std::chrono::duration<double> timeElapsed = timeEnd - timeStart;
    std::cout << "Czas wykonywania: " << timeElapsed.count() << " s\n";

    fwrite(color, 1, (3 * SIZE * SIZE), fp);

    fclose(fp);

    return 0;
}

int getNumOfThreads(const char* arg)
{
    std::string str = arg;

    if (str.find("-P") != std::string::npos)
    {
        std::string subs(arg, 2, str.size() - 2);
        int t = std::atoi(subs.c_str());
        if (t > 1 && t <= MAX_THREADS) return t;
        else if (t > MAX_THREADS) return MAX_THREADS;
    }

    return 1;
}

void processData(int packetNumber, int columnsPerThread, int remainder)
{
    int indexFirstElementOfPacket = packetNumber * columnsPerThread;
    int indexLastElementOfPacket = indexFirstElementOfPacket + columnsPerThread - 1 + remainder;

    int threadColor = (int)(indexFirstElementOfPacket * 1.0 / SIZE * MAX_COLOR_COMPONENT_VALUE);

    // compute and write image data bytes to the file
    for (int iY = indexFirstElementOfPacket; iY < indexLastElementOfPacket; iY++)
    {
        double Cy = CY_MIN + iY * PIXEL_HEIGHT;

        if (fabs(Cy) < PIXEL_HEIGHT / 2)
        {
            Cy = 0.0; // Main antenna
        }

        for (int iX = 0; iX < iX_MAX; iX++)
        {
            double Cx = CX_MIN + iX * PIXEL_WIDTH;

            // initial value of orbit = critical point Z = 0
            double Zx = 0.0;
            double Zy = 0.0;
            double Zx2 = Zx * Zx;
            double Zy2 = Zy * Zy;

            int Iteration;

            for (Iteration = 0; (Iteration < ITERATION_MAX) && ((Zx2 + Zy2) < ER2); Iteration++)
            {
                Zy = 2 * Zx * Zy + Cy;
                Zx = Zx2 - Zy2 + Cx;
                Zx2 = Zx * Zx;
                Zy2 = Zy * Zy;
            }

            // compute pixel color (24 bit = 3 bytes)
            if (Iteration == ITERATION_MAX)
            {
                // interior of Mandelbrot set = black
                color[iY][iX][0] = 0;
                color[iY][iX][1] = threadColor;
                color[iY][iX][2] = threadColor;
            }
            else
            {
                // exterior of Mandelbrot set = white
                color[iY][iX][0] = threadColor; // Red
                color[iY][iX][1] = threadColor; // Green
                color[iY][iX][2] = 0; // Blue
            }
        }
    }
}