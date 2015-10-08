#ifndef GLOBALS_H
#define GLOBALS_H

#endif // GLOBALS_H
#include "cconfig.h"

// CONFIGURATION
CConfig cfg;

// SPECTROMETER
double *Spectrum;
int ExposureTime;
int NumExposures;
int Pixels;
double Saturation;
string Model;
string SerialNumber;
int MaxIntensity;
int MinExposureTime = 10;
double LEDSaturation = -1;
int DarkInProgress = 0;


// GPS
double Latitude;
double Longitude;
double Altitude;
double Time = 0;
unsigned long Date;
double Speed;
double Course;
char WarnCode;
int Quality;
int Year;
int Month;
int Day;
int Hour;
int Minute;
double Second;
double LEDTime = 0;

// THREADING
pthread_t gpsthread;
pthread_t gpsledthread;
pthread_t gpslockledthread;
pthread_t specledthread;
pthread_t shutdownswitchthread;
pthread_t zipthread;
pthread_mutex_t gpslock;
int threaderr;
pthread_t specthread;
pthread_mutex_t speclock;

// OUTPUT
string OutputDir;
unsigned long CurrentFileNumber = 0;
string OutputPrefix = "spec";
string OutputSuffix = ".std";
unsigned long LastDarkFileNumber = -1;
int ReadyToZip = 0;
unsigned long ZipStartFileNumber = 0;

