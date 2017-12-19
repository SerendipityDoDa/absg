#ifndef SERIALPORT_H
#define SERIALPORT_H

#ifndef _MSC_VER

#if defined(__MINGW32__) || defined(__MINGW64__)
#include <windows.h>
typedef HANDLE SerialHandle;
#else
#ifdef __GNUC__
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/fcntl.h>
#include <errno.h>
typedef int SerialHandle;
#endif
#endif

class SerialPort
{
protected:
    SerialHandle m_serialHandle;
public:
    //__MINGW32__
    SerialPort(char *tPort,int tBaudrate,int tDatabits,char tParity, int tStopbits)
    {
        Open(tPort,tBaudrate,tDatabits,tParity,tStopbits);
    }
    virtual ~SerialPort()
    {
        Close();
    }

    unsigned int Open(char *tPort,int tBaudrate,int tDatabits,char tParity, int tStopbits)
    {
        unsigned int error=0;
#if defined(__MINGW32__) || defined(__MINGW64__)
        m_serialHandle = CreateFile(tPort,
                                    GENERIC_READ | GENERIC_WRITE,
                                    0,
                                    0,
                                    OPEN_EXISTING,
                                    0,
                                    0);
        DCB serialParams = {0};
        serialParams.DCBlength=sizeof(serialParams);
        serialParams.fBinary=TRUE;
        if (GetCommState(m_serialHandle, &serialParams))
        {
            serialParams.BaudRate=tBaudrate;
            serialParams.ByteSize=tDatabits;
            switch (tParity)
            {
            case 'E':
                serialParams.Parity=EVENPARITY;
                break;
            case 'M':
                serialParams.Parity=MARKPARITY;
                break;
            case 'O':
                serialParams.Parity=ODDPARITY;
                break;
            case 'S':
                serialParams.Parity=SPACEPARITY;
                break;
            case 'N':
            default:
                serialParams.Parity=NOPARITY;
                break;
            }
            switch (tStopbits)
            {
            case '1':
                serialParams.StopBits=ONE5STOPBITS;
                break;
            case '2':
                serialParams.StopBits=TWOSTOPBITS;
                break;
            case '0':
            default:
                serialParams.StopBits=ONESTOPBIT;
                break;
            }
            if(!SetCommState(m_serialHandle, &serialParams))
            {
                error=GetLastError();
            }
        }
        else error=GetLastError();
#else
#ifdef __GNUC__
        m_serialHandle=open(tPort,O_RDWR | O_NOCTTY | O_NDELAY);
        if (m_serialHandle>0)
        {
            struct termios termOptions;

            // Input flags - Turn off input processing
            // convert break to null byte, no CR to NL translation,
            // no NL to CR translation, don't mark parity errors or breaks
            // no input parity check, don't strip high bit off,
            // no XON/XOFF software flow control
            //
            termOptions.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | ISTRIP | IXON);

            //
            // Output flags - Turn off output processing
            // no CR to NL translation, no NL to CR-NL translation,
            // no NL to CR translation, no column 0 CR suppression,
            // no Ctrl-D suppression, no fill characters, no case mapping,
            // no local output processing
            //
            // termOptions.c_oflag &= ~(OCRNL | ONLCR | ONLRET | ONOCR | ONOEOT| OFILL | OLCUC | OPOST);
            termOptions.c_oflag = 0;

            //
            // No line processing:
            // echo off, echo newline off, canonical mode off,
            // extended input processing off, signal chars off
            //
            termOptions.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);

            switch (tDatabits)
            {
            case 5:
                termOptions.c_cflag |= CS5;
                break;
            case 6:
                termOptions.c_cflag |= CS6;
                break;
            case 7:
                termOptions.c_cflag |= CS7;
                break;
            default:
                termOptions.c_cflag |= CS8;
                break;
            }

            // set parity
            switch (tParity)
            {
            case 'E':
                termOptions.c_cflag |= PARENB;
                break;
            case 'O':
                termOptions.c_cflag |= (PARODD|PARENB);
                break;
            default:
                termOptions.c_cflag &= ~(PARODD|PARENB);
                break;
            }

            // set stop bits
            switch (tStopbits)
            {
            case 2:
                termOptions.c_cflag |= CSTOPB;
                break;
            default:
                termOptions.c_cflag &= ~CSTOPB;
                break;
            }

            // One input byte is enough to return from read()
            // Inter-character timer off
            //
            termOptions.c_cc[VMIN]  = 0;
            termOptions.c_cc[VTIME] = 50;

            // save settings to device
            tcsetattr(m_serialHandle, TCSAFLUSH, &termOptions);

            tcgetattr(m_serialHandle, &termOptions);
        }
        else error=errno;
#endif
#endif
        return error;
    }
    void Close()
    {
        if (m_serialHandle)
        {
#if defined(__MINGW32__) || defined(__MINGW64__)

            CloseHandle(m_serialHandle);
            m_serialHandle=NULL;

#else
#ifdef __GNUC__
            close(m_serialHandle);
            m_serialHandle=0;
#endif
#endif
        }
    }

    unsigned int Read(char *tBuffer,size_t tBufferSize,unsigned int &tBytesRead)
    {
        unsigned int error=0;
#if defined(__MINGW32__) || defined(__MINGW64__)
        if(!ReadFile(m_serialHandle, tBuffer, tBufferSize, (DWORD*)&tBytesRead, NULL))
        {
            error=GetLastError();
        }

#else
#ifdef __GNUC__
        tBytesRead=read(m_serialHandle,tBuffer,tBufferSize);
#endif
#endif
        return error;
    }
    unsigned int Write(char *tBuffer, unsigned int tBufferSize,unsigned int &tBytesWritten)
    {
        unsigned int error=0;
#if defined(__MINGW32__) || defined(__MINGW64__)
        if(!WriteFile(m_serialHandle, tBuffer, tBufferSize, (DWORD*)&tBytesWritten, NULL))
        {
            error=GetLastError();
        }
#else
#ifdef __GNUC__
        tBytesWritten=write(m_serialHandle, tBuffer, tBufferSize);
#endif
#endif
        return error;
    }
};
#endif

#endif
