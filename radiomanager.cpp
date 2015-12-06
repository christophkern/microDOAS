#include "radiomanager.h"

RadioManager::RadioManager(): HEADER(0xFAfaFAfa), HEADER_SIZE(sizeof(long))
{
    m_ttyPortName = "/dev/tty";
    m_ttyPortName += DEFAULT_TTY_PORT_NAME;
}

int RadioManager::setUpSerial()
{
    // open the port
    m_fd = open(m_ttyPortName.c_str(), O_RDWR | O_NOCTTY );

    if(m_fd < 0){
        // handle the error
        // handle the error
        // handle the error
        return OPEN_FAIL;
    }

    // check if valid serial port
    if(!isatty(m_fd)){
        return NOT_A_TTY;
    }

    // copy old config
    if(tcgetattr(m_fd, &m_oldConfig) < 0){
        return GET_CONFIG_FAIL;
    }

    m_config = m_oldConfig;

    // Input flags - turn off input processing
    m_config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |
                         INLCR | PARMRK | INPCK | ISTRIP | IXON);

    // Output flags - turn off output processing
    m_config.c_oflag = 0;

    // No line processing
    m_config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);

    // No character processing
    m_config.c_cflag &= ~(CSIZE | PARENB);
    m_config.c_cflag |= CS8;

    m_config.c_cc[VMIN] = 1;
    m_config.c_cc[VTIME] = 0;

    // baud rate
    if(cfsetspeed(&m_config, B57600)){
        return BAUD_FAILED;
    }

    // apply config to port
    if(tcsetattr(m_fd,TCSAFLUSH, &m_config) < 0){
        return CONFIG_APPLY_FAIL;
    }

    return 0;
}

int RadioManager::closeSerial()
{
    fsync(m_fd);
    // revert to old config settings
    int exitCode = 0;
    if(tcsetattr(m_fd,TCSAFLUSH, &m_oldConfig) < 0){
        exitCode |= CONFIG_APPLY_FAIL;
    }
    if(close(m_fd) < 0){
        exitCode |= CLOSE_FAIL;
    }

    return exitCode;
}

int RadioManager::send(byte * data, const ulong numBytes)
{
    //cout << "RadioManager::send( data: " << (void *) data << ", numBytes: " << numBytes << " )\n";
    int numSent = 0;
    numSent = write(m_fd,data,numBytes);
    fsync(m_fd);
    /*
    int bytesRemaining = numBytes;
    cout << "for loop: numBytes/MAX_BYTES+1 = " << numBytes/MAX_BYTES+1 << "\n";
    for(int i = 0; i < numBytes/MAX_BYTES+1; i++){
        int num = 0;
        int error = 0;
        if(bytesRemaining > MAX_BYTES){
            num = write(m_fd, data, MAX_BYTES);
            numSent += num;
            bytesRemaining -= MAX_BYTES;
            data += MAX_BYTES;
        } else{
            num = write(m_fd, data, bytesRemaining);
            numSent += num;
        }
        if(num < 0){
            error = errno;
            numSent -= num;
        }
        cout << "    " << "i: " << i << endl;
        cout << "    " << "bytesRemaining: " << bytesRemaining << endl;
        cout << "    " << "num: " << num << endl;
        cout << "    " << "numSent: " << numSent << endl;
        cout << "    " << "data: " << (void *) data << endl;
        if(num < 0){
            cout << "    " << "errno: " << error << " strerror: " << strerror(error) << endl;
        }
        cout << endl;
        fsync(m_fd);
    }
    */
    return numSent;
}

int RadioManager::sendCompressed(byte * data, const ulong numBytes)
{
    ulong sizeDataCompressed = (numBytes * 1.1) + 12;
    byte dataCompressed[sizeDataCompressed];
    int z_result = compress( dataCompressed, &sizeDataCompressed, data, numBytes);
    int numSent = -1;
    if(Z_OK == z_result){
        numSent = 0;
        m_crc.reset();
        m_crc.add(dataCompressed,sizeDataCompressed);
        byte hash[m_crc.HashBytes];
        m_crc.getHash(hash);
        //cout << m_crc.getHash() << endl;
        numSent += write(m_fd, &HEADER, HEADER_SIZE);
        numSent += write(m_fd, hash, m_crc.HashBytes);
        numSent += write(m_fd, dataCompressed, sizeDataCompressed);
        fsync(m_fd);
    }
    return numSent;
}
