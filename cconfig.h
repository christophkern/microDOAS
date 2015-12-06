#ifndef CCONFIG_H
#define CCONFIG_H
#include <string>
#include "tinyxml.h"

using namespace std;

// the following structs hold the xml params
struct SSpectrometer
{
    int MaxExposureTime;
    int MinExposureTime;
    int TargetIntegrationTime;
    double TargetSaturation;
    double MaxSaturation;
    double MinSaturation;
    int AutoDetectDark;
    double DarkSaturation;
    int OffsetExpTime;
    int DarkCurrentExpTime;
};

struct SGPS
{
    int UseGPS;
    string TTYPort;
    int WaitForLockOnStartup;
    int BaudRate;
};

struct SGeneral
{
    int ZipSpectra;
    int DeleteRawSpectraAfterZip;
    int ZipInterval;
};

// reads xml file and stores config params
class CConfig
{
    public:
    SSpectrometer Spectrometer;
    SGPS GPS;
    SGeneral General;

    public:
    int ReadXML();

    private:
    bool isNumeric(string line);
    string GetPropertyValue(TiXmlDocument* doc, string header, string property);
};

bool fileExist (const std::string& name);
bool isNumeric(string line);

#endif // CCONFIG_H
