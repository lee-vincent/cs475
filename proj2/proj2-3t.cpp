#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

// Seed for random number
// unsigned int seed = (unsigned int)time(NULL);
unsigned int seed = 0; // this is in the project notes

// Necessary constants
const float GRAIN_GROWS_PER_MONTH =     12.0;
const float ONE_DEER_EATS_PER_MONTH =   1.0;

const float AVG_PRECIP_PER_MONTH =      7.0; // average
const float AMP_PRECIP_PER_MONTH =      6.0; // plus or minus
const float RANDOM_PRECIP =             2.0; // plus or minus noise

const float AVG_TEMP =                  60.0; // average
const float AMP_TEMP =                  20.0; // plus or minus
const float RANDOM_TEMP =               10.0; // plus or minus noise

const float MIDTEMP =                   40.0;
const float MIDPRECIP =                 10.0;

// System state global variables
int     StartYear = 2024;
int     NowYear = 2024;     // 2024 - 2029
int     NowMonth = 0;       // 0 - 11
float   NowPrecip = 3.0;    // in of rain per month
float   NowTemp = 60.5;     // temperature Fahrenheit this month
float   NowHeight = 5.0;    // grain height in inches
int     NowNumDeer = 2;     // number of deer in the current population

// Barrier global variables
omp_lock_t Lock;
volatile int NumInThreadTeam; // number of threads you want to block at the barrier
volatile int NumAtBarrier;
volatile int NumGone;

// Re-entrant Random number generation function
float Ranf_r(unsigned int *seed, float low, float high)
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
void UpdateTempAndPrecip()
{
    float ang = (30. * (float)NowMonth + 15.) * (M_PI / 180.);
    float temp = AVG_TEMP - AMP_TEMP * cos(ang);
    NowTemp = temp + Ranf_r(&seed, -RANDOM_TEMP, RANDOM_TEMP);
    float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin(ang);
    NowPrecip = precip + Ranf_r(&seed, -RANDOM_PRECIP, RANDOM_PRECIP);
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
        if (NumAtBarrier == NumInThreadTeam) // release the waiting threads
        {
            NumGone = 0;
            NumAtBarrier = 0;
            // let all the other threads return before this one unlocks:
            while (NumGone != NumInThreadTeam - 1);
            omp_unset_lock(&Lock);
            return;
        }
    }
    omp_unset_lock(&Lock);

    while (NumAtBarrier != 0); // all threads wait here until the last one arrives...

    #pragma omp atomic // ... and sets NumAtBarrier to 0
        NumGone++;
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
        if (NowYear == 2024 && NowMonth ==0)
            printf("Month,Temp (C),Precip (cm),Deer,Height (cm)\n");
        int monthNum = (NowYear - 2024) * 12 + NowMonth;
        float nowTempCelsius = (5.0 / 9.0) * (NowTemp - 32.0);
        float nowPrecipCm = NowPrecip * 2.54;
        float nowHeightCm = NowHeight * 2.54;
        printf("%d,%.2f,%.2f,%d,%.2f\n",
               monthNum, nowTempCelsius, nowPrecipCm, NowNumDeer, nowHeightCm);
        // °C = (5. / 9.) * (°F - 32)
             // Increment time:
             NowMonth++;
        if (NowMonth > 11)
        {
            NowMonth = 0;
            NowYear++;
        }

        // Compute new environmental parameters:
        UpdateTempAndPrecip();

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