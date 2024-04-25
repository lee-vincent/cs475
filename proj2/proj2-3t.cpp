#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

// Seed for random number
unsigned int seed = (unsigned int)time(NULL);

// Necessary constants
const float GRAIN_GROWS_PER_MONTH = 8.0;
const float ONE_DEER_EATS_PER_MONTH = 0.5;

const float AVG_PRECIP_PER_MONTH = 6.0; // average
const float AMP_PRECIP_PER_MONTH = 6.0; // plus or minus
const float RANDOM_PRECIP = 2.0;        // plus or minus noise

const float AVG_TEMP = 50.0;    // average
const float AMP_TEMP = 20.0;    // plus or minus
const float RANDOM_TEMP = 10.0; // plus or minus noise

const float MIDTEMP = 40.0;
const float MIDPRECIP = 10.0;

// System state global variables
int NowYear = 2024;
int NowMonth = 0;
float NowPrecip;
float NowTemp;
float NowHeight = 5.;
int NowNumDeer = 1;

// Barrier global variables
omp_lock_t Lock;
int NumInThreadTeam;
int NumAtBarrier;
int NumGone;

// Function prototypes
void InitBarrier(int n);
void WaitBarrier();
float SQR(float x);
float Ranf(unsigned int *seed, float low, float high);


// Random number generation function
float Ranf(unsigned int *seed, float low, float high)
{
    float r = (float)rand_r(seed); // 0 - RAND_MAX
    float t = r / (float)RAND_MAX; // 0. - 1.

    return low + t * (high - low);
}

// Function to calculate the square of a number
float SQR(float x)
{
    return x * x;
}

// Update Temperature and Precipitation
void updateTempAndPrecip()
{
    float ang = (30. * (float)NowMonth + 15.) * (M_PI / 180.);
    float temp = AVG_TEMP - AMP_TEMP * cos(ang);
    NowTemp = temp + Ranf(&seed, -RANDOM_TEMP, RANDOM_TEMP);
    float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin(ang);
    NowPrecip = precip + Ranf(&seed, -RANDOM_PRECIP, RANDOM_PRECIP);
    if (NowPrecip < 0.)
    {
        NowPrecip = 0.;
    }
}

// Barrier functions
void InitBarrier(int n)
{
    NumInThreadTeam = n;
    NumAtBarrier = 0;
    omp_init_lock(&Lock);
}

void WaitBarrier()
{
    omp_set_lock(&Lock);
    {
        NumAtBarrier++;
        if (NumAtBarrier == NumInThreadTeam)
        {
            NumGone = 0;
            NumAtBarrier = 0;
            while (NumGone != NumInThreadTeam - 1)
                ;
            omp_unset_lock(&Lock);
            return;
        }
    }
    omp_unset_lock(&Lock);

    while (NumAtBarrier != 0)
        ; // Wait for all threads to reach barrier

#pragma omp atomic
    NumGone++; // Increment the number of threads that have left the barrier
}

// Deer Simulation
void Deer()
{
    while (NowYear < 2030)
    {
        // Compute a temporary next-value for this quantity based on the current state of the simulation:
        int nextNumDeer = NowNumDeer;
        int carryingCapacity = (int)(NowHeight);

        if (nextNumDeer < carryingCapacity)
            nextNumDeer++;
        else if (nextNumDeer > carryingCapacity)
            nextNumDeer--;

        if (nextNumDeer < 0)
            nextNumDeer = 0;

        // DoneComputing barrier:
        WaitBarrier();

        NowNumDeer = nextNumDeer;

        // DoneAssigning barrier:
        WaitBarrier();

        // DonePrinting barrier:
        WaitBarrier();
    }
}

// Grain Growth Simulation
void Grain()
{
    while (NowYear < 2030)
    {
        // Compute a temporary next-value for this quantity based on the current state of the simulation:
        float tempFactor = exp(-SQR((NowTemp - MIDTEMP) / 10.));
        float precipFactor = exp(-SQR((NowPrecip - MIDPRECIP) / 10.));

        float nextHeight = NowHeight;
        nextHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
        nextHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;
        if (nextHeight < 0.)
            nextHeight = 0.;

        // DoneComputing barrier:
        WaitBarrier();

        NowHeight = nextHeight;

        // DoneAssigning barrier:
        WaitBarrier();

        // DonePrinting barrier:
        WaitBarrier();
    }
}

// Watcher Simulation
void Watcher()
{
    while (NowYear < 2030)
    {
        // DoneComputing barrier:
        WaitBarrier();

        // DoneAssigning barrier:
        WaitBarrier();

        // Print the current set of global state variables:
        printf("%d,%d,%.2f,%.2f,%d,%.2f\n",
               NowYear, NowMonth, NowTemp, NowPrecip, NowNumDeer, NowHeight);

        // Increment time:
        NowMonth++;
        if (NowMonth > 11)
        {
            NowMonth = 0;
            NowYear++;
        }

        // Compute new environmental parameters:
        updateTempAndPrecip();

        // DonePrinting barrier:
        WaitBarrier();
    }
}

int main(int argc, char *argv[])
{

    // Start the simulation with initial parameters
    omp_set_num_threads(3); // 3 threads for 3 functions
    InitBarrier(3);

    #pragma omp parallel sections
    {
        #pragma omp section
        {
            Deer();
        }

        #pragma omp section
        {
            Grain();
        }

        #pragma omp section
        {
            Watcher();
        }
    } // Implied barrier = all functions must return in order to proceed

    omp_destroy_lock(&Lock);

    return 0;
}