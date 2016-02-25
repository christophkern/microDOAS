/***********************************************************************//**
 * @file    data-collection.cpp
 * @date    Feb 3, 2014
 * @author  Ocean Optics, Inc
 *
 * This is a command-line utility to perform bulk data-collection via SeaBreeze.
 *
 * Invocation and arguments: see usage()
 *
 * To-Do:
 *
 * - more complex "step" syntax, including support for EDC, NLC, etc:
 *
 *     --step 'cnt=10,integ=100ms,avg=3,delay=50ms,edc=on,nlc=on,boxcar=5'
 *
 * - or, move to an external file specifying steps (possibly read at STDIN):
 *
 *   <steps>
 *     <step label="laser" count="1"  integ="10ms"  averaging="1" delay="0ms"  edc="on" nlc="on" boxcar="0" />
 *     <step label="raman" count="10" integ="100ms" averaging="3" delay="50ms" edc="on" nlc="on" boxcar="5" />
 *   </steps>
 *
 * LICENSE:
 *
 * SeaBreeze Copyright (C) 2014, Ocean Optics Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "globals.h"
#include "api/SeaBreezeWrapper.h"
#include "api/seabreezeapi/SeaBreezeAPIConstants.h"
#include "common/Log.h"
#include "util.h"

#include <string>
#include <vector>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "tinyxml.h"
#include "tinystr.h"
#include "tinyxml.cpp"
#include "tinystr.cpp"
#include "tinyxmlparser.cpp"
#include "tinyxmlerror.cpp"
#include "cconfig.h"
#include "cconfig.cpp"
#include "gpsparser.cpp"
#include "toolbox.cpp"
#include "cspectrometer.cpp"
#include "radiomanager.cpp"
//#include "BBBiolib.h"
#include "leds.cpp"
#include "crc32.cpp"


#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

using std::string;
using std::vector;

#define MAX_EEPROM_LENGTH   15
#define MAX_DEVICES         32
#define MAX_PIXELS        2048


////////////////////////////////////////////////////////////////////////////////
// globals
////////////////////////////////////////////////////////////////////////////////




int main()
{
    cout << "\n\n\n********************************" << endl;
    cout << "********************************" << endl;
    cout << "**********  microDOAS  *********" << endl;
    cout << "********************************" << endl;
    cout << "********************************" << endl;
    cout << "version 2.0 from September 2015" << endl;
    cout << "Christoph Kern (ckern@usgs.gov)\n" << endl;

    // read the config file
    cfg.ReadXML();

    // start the GPS or System Timer
    if (pthread_mutex_init(&gpslock, NULL) != 0)
    {
        printf("Error initializing gps/system timer mutex.\n");
        return 1;
    }
    if (cfg.GPS.UseGPS == 1)
    {
        printf("Booting GPS...\n");        
        threaderr = pthread_create(&gpsthread, NULL, &start_gps, NULL);
        if (threaderr != 0)
        {
            printf("Error initializing gps thread : %s\n", strerror(threaderr));
            return 1;
        }
    }
    else
    {
        printf("NOT USING A GPS, as per config file. Are you sure?\n");
        printf("Using the system time for data acquisition.\n");
        threaderr = pthread_create(&gpsthread, NULL, &PopulateSystemTime, NULL);
        if (threaderr != 0)
        {
            printf("Error initializing system timer thread : %s\n", strerror(threaderr));
            return 1;
        }
        sleep(2);
    }

    if (cfg.GPS.UseGPS == 1)
    {
        // start GPSLED thread
        threaderr = pthread_create(&gpsledthread, NULL, &start_gpsled, NULL);
        if (threaderr != 0)
        {
            printf("Error initializing gps led thread : %s\n", strerror(threaderr));
            return 1;
        }

        // start GPSLOCKLED thread
        threaderr = pthread_create(&gpslockledthread, NULL, &start_gpslockled, NULL);
        if (threaderr != 0)
        {
            printf("Error initializing gps lock led thread : %s\n", strerror(threaderr));
            return 1;
        }
    }

    // start SHUTDOWNSWITCH thread
    threaderr = pthread_create(&shutdownswitchthread, NULL, &start_CheckShutdownSwitch, NULL);
    if (threaderr != 0)
    {
        printf("Error initializing shutdown switch thread : %s\n", strerror(threaderr));
        return 1;
    }

    if (cfg.General.ZipSpectra == 1)
    {
        // start ZIP thread
        threaderr = pthread_create(&zipthread, NULL, &start_zip, NULL);
        if (threaderr != 0)
        {
            printf("Error initializing zip thread : %s\n", strerror(threaderr));
            return 1;
        }
    }

    if (cfg.GPS.WaitForLockOnStartup == 0)
    {
        sleep(5);
    }
    else
    {
        printf("Waiting for GPS lock...");
        while (1)
        {
            if (WarnCode == 'A')
            {
                printf("Got GPS lock!\n");
                break;
            }
            sleep(2);
        }
    }

    if (Date != 0)
    {
        while (1)
        {
            printf("Waiting for good date and time info...\n");
            if ((Year > 2014) & (Year < 2050))
            {
                char datestrbfr[15];
                string datestrstr;
                sprintf(datestrbfr, "%02d%02d%02d%02d%04d.%02d", Month, Day, Hour, Minute, Year, int(round(Second)));
                datestrstr = "echo doas | sudo -S date ";
                datestrstr += datestrbfr;
                //datestrstr += "\"";
                printf("Updating system time from GPS: %s\n", datestrstr.c_str());
                system(datestrstr.c_str());
                break;
            }
            else
            {
                sleep(2);
            }
        }

    }

    CreateOutputDir();

    // set up the radio
    int serialErrorCode = radio.open_serial();
    string error = "Radio/serial error: ";
    switch(serialErrorCode){
        case OPEN_SUCCESS:
            cout << "Radio/serial: open success\n";
            break;
        case OPEN_FAIL:
            cout << error << serialErrorCode << "failed to open port\n";
            break;
        case NOT_A_TTY:
            cout << error << serialErrorCode << "not a tty port\n";
            break;
        case GET_CONFIG_FAIL:
            cout << error << serialErrorCode << "failed to copy previous config settings\n";
            break;
        case BAUD_FAILED:
            cout << error << serialErrorCode << "failed to apply baud rate\n";
            break;
        case CONFIG_APPLY_FAIL:
            cout << error << serialErrorCode << " failed to apply config settings\n";
            break;
        default:
            cout << error << serialErrorCode << "unknown error\n";
    }

    // start acquiring spectra
    printf("Starting spectral acquisition...\n");
    if (pthread_mutex_init(&speclock, NULL) != 0)
    {
        printf("Error initializing spec mutex.\n");
        return 1;
    }
    threaderr = pthread_create(&specthread, NULL, &start_spec, NULL);
    if (threaderr != 0)
    {
        printf("Error initializing spec thread : %s\n", strerror(threaderr));
        return 1;
    }

    // start SPECLED thread
    threaderr = pthread_create(&specledthread, NULL, &start_specled, NULL);
    if (threaderr != 0)
    {
        printf("Error initializing spectrometer led thread : %s\n", strerror(threaderr));
        return 1;
    }


    int userin = fileno(stdin);
    //int charcount;
    char userinbuf[10];
    char quitbuf[10];
    int charsread;

    set_non_blocking(userin);
    while (strcmp(quitbuf, "quit") != 0)
    {
        charsread = read(userin, userinbuf, sizeof userinbuf);
        if (charsread > 0)
        {
            strncpy(quitbuf, userinbuf, 10);
            quitbuf[4] = '\0';
            printf("USER INPUT: %s\n", userinbuf);
        }     
        sleep(1);
        //printf("Time:[%f]\nDate:[%lu]\nLatitude:[%f]\nLongitude:[%f]\nAltitude:[%f]\nCourse:[%f]\nSpeed:[%f]\nQuality:[%d]\nWarningCode:[%c]\n", Time, Date, Latitude, Longitude, Altitude, Course, Speed, Quality, WarnCode);
    }

    printf("Exiting microDOAS. Have a nice day.\n");
    return 0;
}
