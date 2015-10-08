#ifndef CSPECTROMETER_H
#define CSPECTROMETER_H

#endif // CSPECTROMETER_H

using std::string;
using std::vector;


////////////////////////////////////////////////////////////////////////////////
// inner types
////////////////////////////////////////////////////////////////////////////////
struct Step
{
    unsigned long scanCount;
    unsigned long integrationTimeMicrosec;
    unsigned int  scansToAverage;
    unsigned long postScanSleepMicroseconds;
};


////////////////////////////////////////////////////////////////////////////////
// constants
////////////////////////////////////////////////////////////////////////////////

static const char *rcs_id __attribute__ ((unused)) =
    "$Header: http://gforge.oceanoptics.com/svn/seabreeze/releases/Release_2014_10_01-1730-3.0/sample-code/cpp/data-collection.cpp 1215 2014-08-07 21:52:36Z mzieg $";



int specIndex = 0;
int error = ERROR_SUCCESS;
unsigned iterations = 1;
vector<Step> steps;
string basefile = "data";
double *specbfr;
double *speccoaddbfr;
double *silentspec;
char model[16];
char modelshort[16];
int modellength;
char serialnumber[16];
char serialnumbershort[16];
int serialnumberlength;
int pixels;

int AcquireSpectrum(int exptime, int numexp);
int InitializeSpectrometer();
int SetMaxIntensity();
