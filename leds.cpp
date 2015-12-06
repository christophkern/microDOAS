#include <string>

void* start_gpsled(void *arg)
{
    while (1)
    {
        // blinks only if Time has changed since the last blink
        if (LEDTime != Time)
        {
            LEDTime = Time;
            string ledstr;
            ledstr = "echo doas | sudo -S python led810blink.py";
            system(ledstr.c_str());
        }
        usleep(10);
    }
    return(NULL);
}

void* start_gpslockled(void *arg)
{
    while (1)
    {
        if (WarnCode == 'A')
        {
            string ledstr;
            ledstr = "echo doas | sudo -S python led812blink.py";
            system(ledstr.c_str());
        }
        usleep(10);
    }
    return(NULL);
}

void* start_specled(void *arg)
{
    while (1)
    {
        string ledstr;
        if (DarkInProgress == 0)
        {
            if (LEDSaturation != Saturation)
            {
                LEDSaturation = Saturation;
                ledstr = "echo doas | sudo -S python led814blink.py";
                system(ledstr.c_str());
            }
        }
        else
        {
            if (DarkInProgress != 3)
            {
                ledstr = "echo doas | sudo -S python led814doubleblink.py";
                system(ledstr.c_str());
            }
        }
        usleep(10);
    }
    return(NULL);
}
