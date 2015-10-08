#include <iostream>
#include "cconfig.h"
#include "tinyxml.h"
#include <string>

using namespace std;



int CConfig::ReadXML()
{
    TiXmlDocument doc;

    if (fileExist("/media/doas/DATA/microDOAS.xml"))
    {
        printf("Loading configuration from SD card\n");
        bool loadOkay = doc.LoadFile("/media/doas/DATA/microDOAS.xml");
        if (loadOkay == 0) printf("Failed to load microDOAS.xml");
    }
    else
    {
        if (fileExist("microDOAS.xml"))
        {
            printf("No configuration file found on SD card\n");
            printf("Loading configuration from local directory\n");
            bool loadOkay = doc.LoadFile("microDOAS.xml");
            if (loadOkay == 0) printf("Failed to load microDOAS.xml");
        }
        else
        {
            printf("ERROR: No configuration file found!");
        }
    }
    string v;
    v = GetPropertyValue(&doc, "Spectrometer", "MaxExposureTime");
    Spectrometer.MaxExposureTime = atoi(v.c_str());
    v = GetPropertyValue(&doc, "Spectrometer", "MinExposureTime");
    Spectrometer.MinExposureTime = atoi(v.c_str());
    v = GetPropertyValue(&doc, "Spectrometer", "TargetIntegrationTime");
    Spectrometer.TargetIntegrationTime = atoi(v.c_str());
    v = GetPropertyValue(&doc, "Spectrometer", "TargetSaturation");
    Spectrometer.TargetSaturation = atof(v.c_str());
    v = GetPropertyValue(&doc, "Spectrometer", "MaxSaturation");
    Spectrometer.MaxSaturation = atof(v.c_str());
    v = GetPropertyValue(&doc, "Spectrometer", "MinSaturation");
    Spectrometer.MinSaturation = atof(v.c_str());
    v = GetPropertyValue(&doc, "Spectrometer", "AutoDetectDark");
    Spectrometer.AutoDetectDark = atoi(v.c_str());
    v = GetPropertyValue(&doc, "Spectrometer", "DarkSaturation");
    Spectrometer.DarkSaturation = atof(v.c_str());
    v = GetPropertyValue(&doc, "Spectrometer", "OffsetExpTime");
    Spectrometer.OffsetExpTime = atoi(v.c_str());
    v = GetPropertyValue(&doc, "Spectrometer", "DarkCurrentExpTime");
    Spectrometer.DarkCurrentExpTime = atoi(v.c_str());
    v = GetPropertyValue(&doc, "GPS", "UseGPS");
    GPS.UseGPS = atoi(v.c_str());
    v = GetPropertyValue(&doc, "GPS", "WaitForLockOnStartup");
    GPS.WaitForLockOnStartup = atoi(v.c_str());
    v = GetPropertyValue(&doc, "GPS", "TTYPort");
    GPS.TTYPort = v.c_str();
    v = GetPropertyValue(&doc, "GPS", "BaudRate");
    GPS.BaudRate = atoi(v.c_str());
    v = GetPropertyValue(&doc, "General", "ZipSpectra");
    General.ZipSpectra = atoi(v.c_str());
    v = GetPropertyValue(&doc, "General", "DeleteRawSpectraAfterZip");
    General.DeleteRawSpectraAfterZip = atoi(v.c_str());
    v = GetPropertyValue(&doc, "General", "ZipInterval");
    General.ZipInterval = atoi(v.c_str());
    return 1;
}

string CConfig::GetPropertyValue(TiXmlDocument* doc, string header, string property)
{
    string returnv = "";
    string currentheader;
    string currentproperty;
    string currentvalue;
    bool gotit = 0;
    TiXmlNode* root = doc->RootElement();
    TiXmlNode* h = root->FirstChild();
    while (1)
    {
        currentheader = h->Value();
        TiXmlNode* p = h->FirstChild();
        while (1)
        {
             currentproperty = p->Value();
             TiXmlNode* value = p->FirstChild();
             currentvalue = value->Value();
             if ((currentheader.compare(header) == 0) & (currentproperty.compare(property) == 0))
             {
                 returnv = currentvalue;
                 gotit = 1;
                 break;
             }
             if (gotit == 1) break;
             if (p == h->LastChild())
             {
                 break;
             }
             else
             {
                p = p->NextSibling();
             }
        }
        if (gotit == 1) break;
        if (h == root->LastChild())
        {
                break;
        }
        else
        {
           h = h->NextSibling();
        }
    }
    if (gotit == 1) return returnv;
    else
    {
        printf("Could not find property %s in header %s in config file!\n", property.c_str(), header.c_str());
        return "";
    }
}





