#include "cspectrometer.h"


static const struct option opts[] =
{
    { "basefile",   required_argument, NULL, 0 },
    { "index",      required_argument, NULL, 0 },
    { "iterations", required_argument, NULL, 0 },
    { "log-level",  required_argument, NULL, 0 },
    { "step",       required_argument, NULL, 0 },
    { NULL,         no_argument,       NULL, 0 }
};

void usage() {
    puts("data-collection (C) 2014, Ocean Optics Inc\n"
         "\n"
         "Usage:\n"
         "    $ ./data-collection [--index specIndex] [--step cnt:integ:avg]+ [--iterations iter]\n"
         "                        [--basefile foo] [--log-level foo]\n"
         "\n"
         "Where:\n"
         "\n"
         "--index      takes the integral index of an enumerated Ocean Optics spectrometer (0-127)\n"
         "             (default: 0, the first or only connected spectrometer found)\n"
         "--step       zero or more colon-delimited tuples (e.g. '10:100:3'), where the first value\n"
         "             is number of acquisitions to take, the second value is the integration time in\n"
         "             microseconds, and the third value is the number of scans to average (no default);\n"
         "             if a 4th tuple is present, it will represent a post-scan sleep in microseconds\n"
         "             (scanCount may be zero for sleep-only steps)\n"
         "--iterations how many times to run the complete sequence of steps (default: 1) (0 to run forever)\n"
         "--basefile   prefix of output filename (i.e. '/path/to/data').  Will be automatically be\n"
         "             suffixed with '-iter-00000-step-00-acq-00000.csv' for each recorded acquisition.\n"
         "--log-level  one of TRACE, DEBUG, INFO, WARN, ERROR\n"
    );

    exit(1);
}

bool hasError(const char *label)
{
    if (error != ERROR_SUCCESS)
    {
        char msg[80];
        seabreeze_get_error_string(error, msg, sizeof(msg));
        seabreeze_log_error("error calling %s: %s\n", label, msg);
    }
    return error != 0;
}

void parseArgs(int argc, char **argv)
{
    int longIndex = 0;

    // iterate over cmd-line arguments
    while (1)
    {
        // read next option
        int opt = getopt_long(argc, argv, "", opts, &longIndex);

        // no more options
        if (opt == -1)
            break;

        // was this a valid argument?
        if (opt == 0)
        {
            string option(opts[longIndex].name);

            if (option == "index")
                specIndex = atoi(optarg);
            else if (option == "iterations")
                iterations = atoi(optarg);
            else if (option == "basefile")
                basefile = optarg;
            else if (option == "log-level")
                seabreeze_log_set_level_string(optarg);
            else if (option == "step")
            {
                vector<string> tuples = OOI::Util::split(string(optarg), ':');

                Step step;
                step.scanCount                 = strtol(tuples[0].c_str(), NULL, 10);
                step.integrationTimeMicrosec   = strtol(tuples[1].c_str(), NULL, 10);
                step.scansToAverage            = strtol(tuples[2].c_str(), NULL, 10);
                step.postScanSleepMicroseconds = (tuples.size() > 3)
                                               ? strtol(tuples[3].c_str(), NULL, 10)
                                               : 0;
                steps.push_back(step);
            }
            else
                usage();
        }
        else
            usage();
    }
}

int maintest(int argc, char **argv)
{
    seabreeze_log_set_level_string("info");

    ////////////////////////////////////////////////////////////////////////////
    // parse & validate arguments
    ////////////////////////////////////////////////////////////////////////////

    seabreeze_log_debug("processing cmd-line args");
    parseArgs(argc, argv);

    if (steps.size() == 0)
    {
        printf("ERROR: no steps provided\n");
        usage();
    }

    if (iterations > 0)
        seabreeze_log_info("The following step%s will be executed %lu time%s:",
            steps.size() > 1 ? "s" : "", iterations, iterations > 1 ? "s" : "");
    else
        seabreeze_log_info("The following step%s will be executed indefinitely:",
            steps.size() > 1 ? "s" : "");

    for (unsigned i = 0; i < steps.size(); i++)
    {
        seabreeze_log_info("Step #%u", i + 1);
        seabreeze_log_info("    scan count       = %lu", steps[i].scanCount);
        seabreeze_log_info("    integration time = %lu microsec", steps[i].integrationTimeMicrosec);
        seabreeze_log_info("    scans to average = %lu", steps[i].scansToAverage);
        seabreeze_log_info("    post-scan sleep  = %lu microsec", steps[i].postScanSleepMicroseconds);
        seabreeze_log_info("");
    }

    ////////////////////////////////////////////////////////////////////////////
    // initialize SeaBreeze
    ////////////////////////////////////////////////////////////////////////////

    // give SeaBreeze time to fully instantiate
    seabreeze_log_debug("initializing SeaBreeze");
    sleep(1);

    seabreeze_open_spectrometer(specIndex, &error);
    if (hasError("open_spectrometer"))
        exit(1);

    unsigned pixels = seabreeze_get_formatted_spectrum_length(specIndex, &error);
    if (hasError("get_formatted_spectrum_length"))
        exit(1);

    double wavelengths[pixels];
    seabreeze_get_wavelengths(specIndex, &error, wavelengths, sizeof(wavelengths));
    if (hasError("get_wavelengths"))
        exit(1);

    ////////////////////////////////////////////////////////////////////////////
    // perform data collection
    ////////////////////////////////////////////////////////////////////////////

    // iterate over iterations
    unsigned iteration = 0;
    bool done = false;

    while (!done)
    {
        seabreeze_log_debug("processing iteration #%u", iteration + 1);

        // iterate over steps
        for (unsigned stepCount = 0; stepCount < steps.size(); stepCount++)
        {
            seabreeze_log_debug("processing step #%d", stepCount + 1);
            Step& step = steps[stepCount];

            // handle sleep-only steps
            if (step.scanCount == 0)
            {
                seabreeze_log_info("sleeping %lu microseconds", step.postScanSleepMicroseconds);
                usleep(step.postScanSleepMicroseconds);
                continue;
            }

            // apply settings for this step
            seabreeze_set_integration_time_microsec(specIndex, &error, step.integrationTimeMicrosec);
            if (hasError("set_integration_time_microsec"))
                exit(1);

            // iterate over scans
            for (unsigned scanCount = 0; scanCount < step.scanCount; scanCount++)
            {
                // process acquisition
                seabreeze_log_debug("collecting scan %d of %lu", scanCount + 1, step.scanCount);
                double spectrum[pixels];
                seabreeze_get_formatted_spectrum(specIndex, &error, spectrum, sizeof(spectrum));
                if (hasError("get_formatted_spectrum"))
                    exit(1);

                // perform multi-scan averaging (used to be in SeaBreeze, now application code)
                if (step.scansToAverage > 1)
                {
                    double tmp[pixels];
                    for (unsigned acqCount = 1; acqCount < step.scansToAverage; acqCount++)
                    {
                        seabreeze_get_formatted_spectrum(specIndex, &error, tmp, sizeof(tmp));
                        if (hasError("get_formatted_spectrum"))
                            exit(1);
                        for (unsigned pixel = 0; pixel < pixels; pixel++)
                            spectrum[pixel] += tmp[pixel];
                    }
                    for (unsigned pixel = 0; pixel < pixels; pixel++)
                        spectrum[pixel] /= step.scansToAverage;
                }

                // save averaged acquisition to file
                char filename[256];
                snprintf(filename, sizeof(filename), "%s-iter-%05u-step-%02u-acq-%05u.csv",
                    basefile.c_str(), iteration + 1, stepCount + 1, scanCount + 1);
                seabreeze_log_info("saving %s", filename);
                FILE *f = fopen(filename, "w");
                if (f != NULL)
                {
                    for (unsigned pixel = 0; pixel < pixels; pixel++)
                        fprintf(f, "%.2lf,%.2lf\n", wavelengths[pixel], spectrum[pixel]);
                    fclose(f);
                }
                else
                {
                    printf("ERROR: can't write %s\n", filename);
                    exit(1);
                }

                // perform post-scan delay
                if (step.postScanSleepMicroseconds > 0)
                {
                    seabreeze_log_debug("sleeping %lu microseconds", step.postScanSleepMicroseconds);
                    usleep(step.postScanSleepMicroseconds);
                }
            }
        }

        iteration++;
        if (iterations > 0 && iteration >= iterations)
            done = true;
    }

    ////////////////////////////////////////////////////////////////////////////
    // shutdown
    ////////////////////////////////////////////////////////////////////////////

    seabreeze_close_spectrometer(specIndex, &error);

    return 0;
}

void* start_spec_test(void *arg)
{
    seabreeze_log_set_level_string("info");
    seabreeze_open_spectrometer(specIndex, &error);
    if (hasError("open_spectrometer"))
        exit(1);
    printf("Opened spectrometer\n");
    unsigned pixels = seabreeze_get_formatted_spectrum_length(specIndex, &error);
    if (hasError("get_formatted_spectrum_length"))
        exit(1);
    printf("Spectrometer has %d channels\n", pixels);
    double spectrum[pixels];
    while(1)
    {
        //double wavelengths[pixels];
        //seabreeze_get_wavelengths(specIndex, &error, wavelengths, sizeof(wavelengths));
        //if (hasError("get_wavelengths"))
        //    exit(1);

        // apply settings for this step
        seabreeze_set_integration_time_microsec(specIndex, &error, 1000000);
        if (hasError("set_integration_time_microsec"))
            exit(1);

        // iterate over scans
        for (unsigned scanCount = 0; scanCount < 1; scanCount++)
        {
            // process acquisition
            seabreeze_log_debug("collecting scan %d of %lu", scanCount + 1, 1);
            seabreeze_get_formatted_spectrum(specIndex, &error, spectrum, sizeof(spectrum));
            if (hasError("get_formatted_spectrum"))
                exit(1);
        }

        char filename[256];
        snprintf(filename, sizeof(filename), "testout.txt");
        //snprintf(filename, sizeof(filename), "%s-iter-%05u-step-%02u-acq-%05u.csv",
        //    basefile.c_str(), iteration + 1, stepCount + 1, scanCount + 1);
        seabreeze_log_info("saving %s", filename);
        FILE *f = fopen(filename, "w");
        if (f != NULL)
        {
            for (unsigned pixel = 0; pixel < pixels; pixel++)
                fprintf(f, "%.2lf\n", spectrum[pixel]);
            fclose(f);
        }
        else
        {
            printf("ERROR: can't write %s\n", filename);
            exit(1);
        }
        // perform post-scan delay
        seabreeze_log_debug("sleeping %lu microseconds", 10);
        usleep(10);
    }
    return NULL;
}

int InitializeSpectrometer()
{
    printf("Initializing Spectrometer:\n");
    int i;
    seabreeze_log_set_level_string("info");
    seabreeze_open_spectrometer(specIndex, &error);
    if (hasError("open_spectrometer"))
        exit(1);
    printf("Opened spectrometer\n");
    modellength = seabreeze_get_model(specIndex, &error, model, 16);
    if (hasError("get_model"))
        exit(1);
    for (i=0;i<16;i++)
    {
        modelshort[i] = model[i];
    }
    modelshort[modellength] = '\0';
    printf("Spectrometer model is %s\n", modelshort);
    SetMaxIntensity();
    serialnumberlength = seabreeze_get_serial_number(specIndex, &error, serialnumber, 16);
    if (hasError("get_serial_number"))
        exit(1);
    for (i=0;i<16;i++)
    {
        serialnumbershort[i] = serialnumber[i];
    }
    serialnumbershort[serialnumberlength] = '\0';
    printf("Spectrometer serial number is %s\n", serialnumbershort);
    pixels = seabreeze_get_formatted_spectrum_length(specIndex, &error);
    if (hasError("get_formatted_spectrum_length"))
        exit(1);
    printf("Spectrometer has %d channels\n", pixels);
    specbfr = (double *) malloc(pixels * sizeof(double));
    speccoaddbfr = (double *) malloc(pixels * sizeof(double));
    silentspec = (double *) malloc(pixels * sizeof(double));
    seabreeze_set_integration_time_microsec(specIndex, &error, 20000);
    if (hasError("set_integration_time_microsec"))
        exit(1);
    seabreeze_get_formatted_spectrum(specIndex, &error, specbfr, pixels * sizeof(double));
        if (hasError("get_formatted_spectrum"))
            exit(1);
    printf("Acquiring test spectrum.\n");
    seabreeze_get_formatted_spectrum(specIndex, &error, specbfr, pixels * sizeof(double));
        if (hasError("get_formatted_spectrum"))
            exit(1);
    pthread_mutex_lock(&speclock);
    Spectrum = (double *) malloc(pixels * sizeof(double));
    for (i=0;i<pixels;i++)
    {
        Spectrum[i] = specbfr[i];
    }
    Pixels = pixels;
    ExposureTime = 20;
    NumExposures = 1;    
    Saturation = SpecMax(specbfr, Pixels) / MaxIntensity;
    Model = modelshort;
    SerialNumber = serialnumbershort;
    pthread_mutex_unlock(&speclock);
    usleep(10);
    printf("Spectrometer successfully initialized.\n");
    return 0;
}


int SilentAcquireSpectrum(int exptime, int numexp)
{
    int i;
    int exposure;
    for (i=0; i<Pixels; i++)
    {
        speccoaddbfr[i] = 0;
    }
    seabreeze_set_integration_time_microsec(specIndex, &error, exptime * 1000);
    if (hasError("set_integration_time_microsec"))
        exit(1);
    seabreeze_get_formatted_spectrum(specIndex, &error, specbfr, Pixels * sizeof(double));
    if (hasError("get_formatted_spectrum"))
        exit(1);
    for (exposure=0; exposure<numexp; exposure++)
    {
        seabreeze_get_formatted_spectrum(specIndex, &error, specbfr, Pixels * sizeof(double));
        if (hasError("get_formatted_spectrum"))
            exit(1);
        for (i=0; i<Pixels; i++)
        {
            speccoaddbfr[i] += specbfr[i];
        }
    }
    usleep(10);
    for (i=0; i<Pixels; i++)
    {
        silentspec[i] = speccoaddbfr[i];
    }
    // this is because pixels 0 and 1 appear to always have the same (wierd values):
    silentspec[0] = silentspec[2];
    silentspec[1] = silentspec[2];
    return 0;
}

int AcquireSpectrum(int exptime, int numexp)
{
    int i;
    int exposure;

    for (i=0; i<Pixels; i++)
    {
        speccoaddbfr[i] = 0;
    }

    if (exptime != ExposureTime)
    {
        seabreeze_set_integration_time_microsec(specIndex, &error, exptime * 1000);
        if (hasError("set_integration_time_microsec"))
            exit(1);
        seabreeze_get_formatted_spectrum(specIndex, &error, specbfr, Pixels * sizeof(double));
        if (hasError("get_formatted_spectrum")){
            cout << "line 425" << endl;
            exit(1);
        }
    }

    for (exposure=0; exposure<numexp; exposure++)
    {
        seabreeze_get_formatted_spectrum(specIndex, &error, specbfr, Pixels * sizeof(double));
        if (hasError("get_formatted_spectrum")){
            cout << "line 434" << endl;
            exit(1);
        }
        for (i=0; i<Pixels; i++)
        {
            speccoaddbfr[i] += specbfr[i];
        }
    }

    pthread_mutex_lock(&speclock);
    pthread_mutex_lock(&gpslock);
    for (i=0;i<pixels;i++)
    {
        Spectrum[i] = speccoaddbfr[i];
    }
    // this is because pixels 0 and 1 appear to always have the same (weird values):
    Spectrum[0] = Spectrum[2];
    Spectrum[1] = Spectrum[2];
    ExposureTime = exptime;
    NumExposures = numexp;
    Saturation = SpecMax(Spectrum, Pixels) / (MaxIntensity * NumExposures);
    printf("Acquired spectrum: ExpTime [%d] | NumExp [%d] | Saturation [%.2f]\n", ExposureTime, NumExposures, Saturation);
    WriteStdFile();
    static int sendEveryNth = cfg.Radio.SendEveryNth; // static since it only needs to load once
    if(sendEveryNth > 0){
        if( 0 == CurrentFileNumber%sendEveryNth ){
            int bytesSent = transmitRadioData();
            printf("Sent spectrum %ld in %d bytes \n", (CurrentFileNumber-1), bytesSent);
            exit(0);
        }
    }
    pthread_mutex_unlock(&gpslock);
    pthread_mutex_unlock(&speclock);
    printf("Saved spectrum %ld.\n", (CurrentFileNumber - 1));
    if (cfg.General.ZipSpectra == 1)
    {
        double divresult;
        divresult = double(CurrentFileNumber) / double(cfg.General.ZipInterval);
        if (divresult == round(divresult))
        {
            ReadyToZip = 1;
        }
    }
    return 0;
}


int GetAutoExpTime(bool BaseOnExistingSpec)
{    
    double silentmax;
    double silentsat;
    int testexptime;
    int newexptime;
    if (BaseOnExistingSpec == 0)
    {
        testexptime = 20;
        SilentAcquireSpectrum(testexptime, 1);
        silentmax = SpecMax(silentspec, Pixels);
        silentsat = silentmax / MaxIntensity;
    }
    else
    {
        silentsat = Saturation;
        testexptime = ExposureTime;
    }
    while(1)
    {
        if (silentsat > cfg.Spectrometer.MaxSaturation)
        {
            newexptime = round(testexptime * 0.66);
            if (newexptime < cfg.Spectrometer.MinExposureTime)
            {
                newexptime = cfg.Spectrometer.MinExposureTime;
                break;
            }
        }
        else
        {
            if (silentsat < cfg.Spectrometer.MinSaturation)
            {                
                newexptime = round(double(testexptime) * double(cfg.Spectrometer.TargetSaturation) / silentsat);                
                if (newexptime > cfg.Spectrometer.MaxExposureTime)
                {
                    newexptime = cfg.Spectrometer.MaxExposureTime;
                    break;
                }
            }
            else
            {
                newexptime = testexptime;
                break;
            }
        }
        testexptime = newexptime;        
        SilentAcquireSpectrum(testexptime, 1);
        silentmax = SpecMax(silentspec, Pixels);
        silentsat = silentmax / MaxIntensity;
    }
    return newexptime;
}


int SetMaxIntensity()
{
    MaxIntensity = -1;
    if (strcmp(modelshort, "USB2000") == 0)
        MaxIntensity = 4095;
    if (strcmp(modelshort, "USB2000PLUS") == 0)
        MaxIntensity = 65535;
    if (strcmp(modelshort, "QE65000") == 0)
        MaxIntensity = 65535;
    if (strcmp(modelshort, "USB4000") == 0)
        MaxIntensity = 65535;
    if (strcmp(modelshort, "MAYA2000") == 0)
        MaxIntensity = 65535;
    if (strcmp(modelshort, "MAYA2000PRO") == 0)
        MaxIntensity = 65535;
    if (strcmp(modelshort, "HR2000") == 0)
        MaxIntensity = 4095;
    if (strcmp(modelshort, "HR2000PLUS") == 0)
        MaxIntensity = 65535;
    if (strcmp(modelshort, "HR4000") == 0)
        MaxIntensity = 65535;
    if (MaxIntensity == -1)
    {
        printf("ERROR: UNRECOGNIZED SPECTROMETER TYPE.\n");
        exit(1);
    }
    return 0;
}

void* start_spec(void *arg)
{
    int currentexptime;
    int currentnumexp;
    InitializeSpectrometer();
    while(1)
    {        
        currentexptime = GetAutoExpTime(1);
        currentnumexp = ceil(double(cfg.Spectrometer.TargetIntegrationTime) / double(currentexptime));
        AcquireSpectrum(currentexptime, currentnumexp);
        if ((currentexptime == cfg.Spectrometer.MaxExposureTime) && (Saturation < cfg.Spectrometer.DarkSaturation) && (cfg.Spectrometer.AutoDetectDark == 1) && (CurrentFileNumber != LastDarkFileNumber))
        {
            cout << "Dark conditions detected. Recording dark references:" << endl;
            DarkInProgress = 1;
            cout << "Acquiring Offset spectrum:" << endl;
            AcquireSpectrum(5, 1000);
            DarkInProgress = 2;
            cout << "Acquiring Dark Current spectrum:" << endl;
            AcquireSpectrum(20000, 1);
            DarkInProgress = 3;
            while(1)
            {
                cout << "Finished recording dark. Please open the shutter." << endl;
                currentexptime = GetAutoExpTime(0);
                AcquireSpectrum(currentexptime, currentnumexp);
                if (Saturation > cfg.Spectrometer.DarkSaturation)
                {
                    break;
                }
            }
            DarkInProgress = 0;
        }
    }
    return NULL;
}

