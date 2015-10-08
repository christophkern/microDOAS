#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

bool isNumeric(string line)
{
    char* p;
    int i = line.find_first_of(".");
    while (i != -1)
    {
        line = line.replace(i, 1, "0");
        i = line.find_first_of(".");
    }
    strtol(line.c_str(), &p, 10);
    return *p == 0;
}


bool fileExist (const std::string& name)
{
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }
}

bool dirExists (const char *path)
{
    struct stat info;
    if (stat(path, &info) != 0)
        return 0;
    else if(info.st_mode & S_IFDIR)
        return 1;
    else
        return 0;
}

static void set_non_blocking(int fd)
{
    int flags  = fcntl(fd, F_GETFL, 0 );
    flags |= O_NONBLOCK;
    flags = fcntl(fd, F_SETFL, flags);
}

double SpecMax(double *Array, int ArrayLength)
{
    int i;
    double Amax;
    Amax = -1;
    for (i=10; i<ArrayLength; i++)
    {
        if (Array[i] > Amax)
                Amax = Array[i];
    }
    return Amax;
}

int CreateOutputDir()
{
    string outdir, toutdir;
    int addnum = 0;
    char dateid[21];
    char addnumstr[4];
    outdir = "/media/doas/DATA/";
    pthread_mutex_lock(&gpslock);
    sprintf(dateid, "%04d-%02d-%02dT%02d-%02d-%02dZ", Year, Month, Day, Hour, Minute, int(round(Second)));
    pthread_mutex_unlock(&gpslock);
    outdir += dateid;
    //printf("dir to make: %s\n", outdir.c_str());
    if (dirExists(outdir.c_str()) == 1)
    {
        while (1)
        {
            sprintf(addnumstr, "%04d", addnum);
            toutdir = outdir + addnumstr;
            if (dirExists(toutdir.c_str()) == 0)
            {
                outdir = toutdir;
                break;
            }
            addnum++;
        }
    }
    mkdir(outdir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (dirExists(outdir.c_str()))
        printf("Successfully made output directory: %s\n", outdir.c_str());
    else
    {
        printf("Error occured while making output directory!\n");
        exit(1);
    }
    OutputDir = outdir + "/";
    return 0;
}

void* PopulateSystemTime(void *arg)
{
    time_t rawtime;
    struct tm * timeinfo;
    while (1)
    {
        time ( &rawtime );
        timeinfo = gmtime ( &rawtime );
        printf ( "NO GPS! Using System Time and Date %s", asctime (timeinfo) );
        pthread_mutex_lock(&gpslock);
        Year = timeinfo->tm_year + 1900;
        Month = timeinfo->tm_mon + 1;
        Day = timeinfo->tm_mday;
        Hour = timeinfo->tm_hour;
        Minute = timeinfo->tm_min;
        Second = double(timeinfo->tm_sec);
        pthread_mutex_unlock(&gpslock);
        sleep(1);
    }
    return NULL;
}

int WriteStdFile()
{
    string outfile;
    char numstr[6];
    int i, c;
    char latstring[20];
    char lonstring[20];
    char latstringshort[20];
    char lonstringshort[20];

    sprintf(latstring, "%15lf", Latitude);
    sprintf(lonstring, "%15lf", Longitude);
    c = 0;
    for (i=0; i<20; i++)
    {
        if (latstring[i] != ' ')
        {
            latstringshort[c] = latstring[i];
            c++;
        }
    }
    latstringshort[c] = '\0';
    c = 0;
    for (i=0; i<20; i++)
    {
        if (lonstring[i] != ' ')
        {
            lonstringshort[c] = lonstring[i];
            c++;
        }
    }
    lonstringshort[c] = '\0';

    FILE* stream;
    sprintf(numstr, "%06lu", CurrentFileNumber);
    if ((DarkInProgress == 0) || (DarkInProgress == 3))
    {
        outfile = OutputDir + OutputPrefix + numstr + OutputSuffix;
    }
    else
    {
        LastDarkFileNumber = CurrentFileNumber;
        if (DarkInProgress == 1)
        {
            outfile = OutputDir + "offset" + numstr + OutputSuffix;
        }
        if (DarkInProgress == 2)
        {
            outfile = OutputDir + "darkcurrent" + numstr + OutputSuffix;
        }
    }
    stream = fopen(outfile.c_str(), "w");
    fprintf(stream, "GDBGMNUP\n1\n");
    fprintf(stream, "%d\n", Pixels);
    for (i=0; i<Pixels; i++)
    {
        fprintf(stream, "%f\n", Spectrum[i]);
    }
    fprintf(stream, "spec%06lu\n\n\n", CurrentFileNumber);
    pthread_mutex_lock(&gpslock);
    fprintf(stream, "%02d/%02d/%04d\n", Month, Day, Year);
    fprintf(stream, "%02d:%02d:%02d\n", Hour, Minute, int(round(Second)));
    fprintf(stream, "%02d:%02d:%02d\n", Hour, Minute, int(round(Second)));
    fprintf(stream, "333\n0\n");
    fprintf(stream, "SCANS %d\n", NumExposures);
    fprintf(stream, "INT_TIME %d\n", ExposureTime);
    fprintf(stream, "SITE volcano\n");
    fprintf(stream, "LONGITUDE %s\n", lonstringshort);
    fprintf(stream, "LATITUDE %s\n", latstringshort);
    fprintf(stream, "NumScans = %d\n", NumExposures);
    fprintf(stream, "ExposureTime = %d\n", ExposureTime);
    fprintf(stream, "Altitude = %lf\n", Altitude);
    fprintf(stream, "SpectrumID = %lu\n", CurrentFileNumber);
    fprintf(stream, "SpectrometerType = %s\n", Model.c_str());
    fprintf(stream, "SpectrometerSerialNumber = %s\n", SerialNumber.c_str());
    fprintf(stream, "Device = %s\n", SerialNumber.c_str());
    fprintf(stream, "Latitude = %s\n", latstringshort);
    fprintf(stream, "Longitude = %s\n", lonstringshort);
    fprintf(stream, "Speed = %lf\n", Speed);
    fprintf(stream, "Course = %lf\n", Course);
    fprintf(stream, "GPSQuality = %d\n", Quality);
    fprintf(stream, "GPSWarnCode = %c\n", WarnCode);
    pthread_mutex_unlock(&gpslock);
    fprintf(stream, "Name = spec%06lu\n", CurrentFileNumber);
    fprintf(stream, "NChannels = %d\n", Pixels);
    fprintf(stream, "Filename = %s\n", outfile.c_str());
    fclose(stream);
    if (DarkInProgress == 0)
    {
        CurrentFileNumber++;
    }
    return 0;
}

void* start_CheckShutdownSwitch(void *arg)
{
    while (1)
    {
        string switchstr;
        switchstr = "echo doas | sudo -S python switch818.py";
        int switchstate;
        system(switchstr.c_str());
        FILE* stream;
        stream = fopen("switch818_state.dat", "r");
        fscanf(stream, "%d\n", &switchstate);
        fclose(stream);
        if (switchstate == 1)
        {
            switchstr = "echo doas | sudo -S poweroff";
            system(switchstr.c_str());
        }
        sleep(1);
    }
    return(NULL);
}

void* start_zip(void *arg)
{
    while (1)
    {
        if (ReadyToZip == 1)
        {
            ReadyToZip = 0;
            printf("Zipping raw spectra...\n");
            unsigned long lastfilenum;
            unsigned long zipfilenum;
            string zipfiles;
            int i;
            char numstr[6];
            string outputzipfile;
            string zipsuffix;
            string zipcmd;
            zipsuffix = ".zip";
            sprintf(numstr, "%06lu", ZipStartFileNumber);
            outputzipfile = OutputDir + OutputPrefix + numstr + zipsuffix;
            lastfilenum = ZipStartFileNumber + cfg.General.ZipInterval;
            zipfilenum = ZipStartFileNumber;
            while (zipfilenum < lastfilenum)
            {
                zipfiles = "";
                for (i=0;i<100;i++)
                {
                    sprintf(numstr, "%06lu", zipfilenum);
                    zipfiles = zipfiles + " " + OutputDir + OutputPrefix + numstr + OutputSuffix;
                    zipfilenum++;
                    if (zipfilenum == lastfilenum)
                    {
                        break;
                    }
                }
                zipcmd = "zip -j " + outputzipfile + " " + zipfiles;
                system(zipcmd.c_str());
            }                                    
            printf("Finished zipping spectra.\n");
            if (cfg.General.DeleteRawSpectraAfterZip == 1)
            {
                printf("Deleting raw spectra after zip...\n");
                zipfilenum = ZipStartFileNumber;
                while (zipfilenum < lastfilenum)
                {
                    zipfiles = "";
                    for (i=0;i<100;i++)
                    {
                        sprintf(numstr, "%06lu", zipfilenum);
                        zipfiles = zipfiles + " " + OutputDir + OutputPrefix + numstr + OutputSuffix;
                        zipfilenum++;
                        if (zipfilenum == lastfilenum)
                        {
                            break;
                        }
                    }
                    zipcmd = "rm " + zipfiles + " -f";
                    system(zipcmd.c_str());                    
                }
                printf("Finished deleting raw spectra after zip.\n");
            }
            ZipStartFileNumber += cfg.General.ZipInterval;
        }
        else
        {
            sleep(1);
        }
    }
    return (NULL);
}
