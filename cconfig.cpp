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
            // seems like allowing the code to proceed here could cause a crash
            // consider loading hard coded defaults or just returning 0;
            // It appears that any comments in the xml file will cause the program to just exit.
            // Any errors in the xml file could go undetected and cause unexpected behavior.
            // Just thought I'd mention it so you can fix that at some point if you feel the need.
            // - Adam Levy
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

    // radio transmit settings
    v = GetPropertyValue(&doc, "Radio", "SendEveryNthSTDFile");
    Radio.SendEveryNth = atoi(v.c_str());
    v = GetPropertyValue(&doc, "Radio", "NumSpecRegions");
    int numSpecRegions = atoi(v.c_str());
    for(int i = 0; i < numSpecRegions; i++){
        string index = std::to_string(i);
        string startTag = "StartSpec" + index;
        string endTag = "EndSpec" + index;
        v = GetPropertyValue(&doc, "Radio", startTag);
        Radio.start.push_back(atoi(v.c_str()));
        v = GetPropertyValue(&doc, "Radio", endTag);
        Radio.end.push_back(atoi(v.c_str()));
    }

    // check that spectrum regions are valid
    bool ok = true;
    for(int i = 0; i < numSpecRegions; i++){
        // valid values are 0-2047
        if( !( 0 <= Radio.start[i] && Radio.start[i] < 2048 ) ||
            !( 0 <= Radio.end[i] && Radio.end[i] < 2048 ) ){
            ok = false;
            break;
        }
        // for any given region the start must be less than or equal to the end
        if( !( Radio.start[i] <= Radio.end[i] ) ){
            ok = false;
            break;
        }
        if( i > 0 ){
            // the end of the previous region must be less than or equal to the start of this region
            if( !( Radio.end[i-1] <= Radio.start[i] ) ){
                ok = false;
                break;
            }
        }
    }

    // if the specified regions aren't okay then transmit all spectrum
    if(!ok){
        printf( "Spectrum regions in XML file are invalid. \nDefaulting to sending entire spectrum. \n");
        Radio.start.clear();
        Radio.end.clear();
        Radio.start.push_back(0);
        Radio.end.push_back(2047);
    }
    printf( "Transmitting the following spectra channels: \n");
    for( unsigned int i = 0; i < Radio.start.size(); i++ ){
        printf("\t %i - %i \n", Radio.start[i], Radio.end[i]);
    }


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





