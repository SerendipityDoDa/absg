#include <stdio.h>
#include <unistd.h>
#include "serialport.h"


/*
Value 	Identified by 	Compression method 	Comments
0 	BI_RGB 	none 	Most common
1 	BI_RLE8 	RLE 8-bit/pixel 	Can be used only with 8-bit/pixel bitmaps
2 	BI_RLE4 	RLE 4-bit/pixel 	Can be used only with 4-bit/pixel bitmaps
3 	BI_BITFIELDS 	OS22XBITMAPHEADER: Huffman 1D 	BITMAPV2INFOHEADER: RGB bit field masks,
BITMAPV3INFOHEADER+: RGBA
4 	BI_JPEG 	OS22XBITMAPHEADER: RLE-24 	BITMAPV4INFOHEADER+: JPEG image for printing[12]
5 	BI_PNG 		BITMAPV4INFOHEADER+: PNG image for printing[12]
6 	BI_ALPHABITFIELDS 	RGBA bit field masks 	only Windows CE 5.0 with .NET 4.0 or later
11 	BI_CMYK 	none 	only Windows Metafile CMYK[3]
12 	BI_CMYKRLE8 	RLE-8 	only Windows Metafile CMYK
13 	BI_CMYKRLE4 	RLE-4 	only Windows Metafile CMYK
*/


unsigned char _crc_ibutton_update(unsigned char crc, unsigned char data)
{
    unsigned char i;

    crc = crc ^ data;
    for (i = 0; i < 8; i++)
    {
        if (crc & 0x01)
            crc = (crc >> 1) ^ 0x8C;
        else
            crc >>= 1;
    }

    return crc;
}


unsigned char crc8(const unsigned char *data,unsigned short len)
{
    unsigned char crc = 0;
    unsigned short idx;
    for (idx = 0; idx < len; idx++)
        crc = _crc_ibutton_update(crc, data[idx]);
    return crc; // must be 0
}


#pragma pack(1)
typedef struct
{
    char ID[2];                     // Must be 'BM'
    unsigned int    BMPFileSize;        // Size of this file
    unsigned int    Reserved;           // ???
    unsigned int    DataOfs;           // The offset, i.e. starting address, of the byte where the bitmap image data (pixel array) can be found.
    unsigned int    HeaderSize;         // Size of header
    int     Width, Height;      // Width and Height of image   uncompressed Windows bitmaps also can be stored from the top to bottom, when the Image Height value is negative.
    unsigned short biPlanes,           // the number of color planes (must be 1)
             Bits;               // Bits can be 1, 4, 8, or 24
    unsigned int    biCompression,      // the compression method being used. See the above table for a list of possible values
             biSizeImage;        // the image size. This is the size of the raw bitmap data; a dummy 0 can be given for BI_RGB bitmaps.
    int     biXPelsPerMeter,    // the horizontal resolution of the image. (pixel per meter, signed integer)
            biYPelsPerMeter;    // the vertical resolution of the image. (pixel per meter, signed integer)
    unsigned int    biClrUsed,          // the number of colors in the color palette, or 0 to default to 2^n
             biClrImportant;     // the number of important colors used, or 0 when every color is important; generally ignored
} BMP_Header;


typedef struct
{
    unsigned char r,g,b,reserved;
} ColorTableEntry;
#pragma pack()

/*{
    0b00110000,
    0b01000000,
    0b01000011,
    0b10000000,
    0b10000000,
    0b01000011,
    0b01000000,
    0b00110000,
}*/


void MakeBMP(char *path,unsigned char *data)
{
    BMP_Header tHeader=
    {
        {'B','M'},
        sizeof(BMP_Header)+(sizeof(ColorTableEntry)*2)+((128*64)/8),
        0,
        sizeof(BMP_Header)+(sizeof(ColorTableEntry)*2),
        40,//sizeof(BMP_Header)
        128,
        -64,
        1,
        1,
        0,
        0,      //((128*64)/8)
        4414,   //128/(2.9cm*0.01)
        4266,      //64/(1.5cm*0.01)
        0,
        0
    };
    //printf("tHeader.BMPFileSize=%u\n",tHeader.BMPFileSize);
    ColorTableEntry black= {0,0,0,0},
                    white= {255,255,255,0};
    unsigned char tBuffer[128*64];

    int idx,jdx,x=0,y=0,yofs=0;
    for (idx=0; idx<((128*64)/8); idx++)
    {
        for (jdx=0; jdx<8; jdx++)
        {
            tBuffer[((y+yofs)*128)+x]=(data[idx]>>jdx) & 1;
            y++;
            if (y>7)
            {
                y=0;
                x++;
                if (x>127)
                {
                    x=0;
                    yofs+=8;
                }
            }
        }
    }

    int ofs=0;
    jdx=0;
    unsigned char tByte=0;
    for (y=0; y<64; y++)
    {
        for (x=0; x<128; x++)
        {
            if (tBuffer[(y*128)+x])
            {
                tByte|=1;
            }
            if (jdx<7) tByte<<=1;
            jdx++;
            if (jdx==8)
            {
                data[ofs]=tByte;
                ofs++;
                tByte=0;
                jdx=0;
            }
        }
    }

    FILE *tFile=fopen(path,"wb");
    if (tFile)
    {
        fwrite(&tHeader,sizeof(tHeader),1,tFile);
        fwrite(&black,sizeof(black),1,tFile);
        fwrite(&white,sizeof(white),1,tFile);
        fwrite(data,((128*64/8)),1,tFile);
        fclose(tFile);
    }
}



int main(int argc,char *argv[])
{
    char *comPort=NULL,*backupPath=NULL,*restorePath=NULL,*screenShotPath=NULL;
    //Used in the parsing of command line arguments
    int topt;
    do
    {
        topt=getopt(argc,argv,"b:c:hr:s:");
        switch (topt)
        {
        case 'b':
            backupPath=optarg;
            break;
        case 'c':
            comPort=optarg;
            break;
        case '?':
        case 'h':
            printf("usage:\n%s [bchrs]\n\n\t-b <backup path> backup EEPROM to the specified path\n\t-c <comport> specify comport to use\n\t-h display usage information\n\t-r <restore path> restore EEPROM from specified path\n\t-s <screenshot path> specify path and filename prefix to capture screenshots to.  Files will be bmp extension.\n",argv[0]);
            return 1;
            break;
        case 'r':
            restorePath=optarg;
            break;
        case 's':
            screenShotPath=optarg;
            break;
        default:
            break;
        }
    }
    while (topt!=-1);

    if (comPort)
    {
        SerialPort tSerial(comPort,115200,8,'N',1);
        if (backupPath)
        {
            unsigned char readBuffer[0x400];
            unsigned int bytesRead=0;
            unsigned char tCRC;
            tSerial.Read((char*)&readBuffer,sizeof(readBuffer),bytesRead);
            if (bytesRead==sizeof(readBuffer))
            {
                FILE *tFile=fopen(backupPath,"wb");
                if (tFile)
                {
                    fwrite(&readBuffer,sizeof(readBuffer),1,tFile);
                    fclose(tFile);
                }
                else printf("Unable to open %s.\n",backupPath);
                tSerial.Read((char*)&tCRC,sizeof(tCRC),bytesRead);
                if (bytesRead==sizeof(tCRC))
                {
                    if (tCRC!=crc8((unsigned char*)&readBuffer,sizeof(readBuffer)))
                    {
                        printf("CRC mismatch, backup failed!\n");
                    }
                }
                else printf("Error reading CRC.\n");
            }
            else printf("Error reading EEPROM backup data.\n");
        }
        if (restorePath)
        {
            unsigned char writeBuffer[0x400];
            unsigned int bytesWritten=0;
            FILE *tFile=fopen(restorePath,"rb");
            if (tFile)
            {
                fread(&writeBuffer,sizeof(writeBuffer),1,tFile);
                fclose(tFile);
                unsigned char tCRC=crc8((unsigned char*)&writeBuffer,sizeof(writeBuffer));
                tSerial.Write((char*)&writeBuffer,sizeof(writeBuffer),bytesWritten);
                if (bytesWritten!=sizeof(writeBuffer))
                {
                    printf("Error writing EEPROM backup data.\n");
                }
                tSerial.Write((char*)&tCRC,sizeof(tCRC),bytesWritten);
                if (bytesWritten!=sizeof(tCRC))
                {
                    printf("Error writing CRC.\n");
                }
            }
            else printf("Unable to open %s.\n",restorePath);
        }
        if (screenShotPath)
        {
            //char sendbuffer[] = "test";
            unsigned char readbuffer[(128*64)/8];
            //write test
            //int bytesWritten = writeToSerialPort(h,sendbuffer,strlen(sendbuffer));
            //printf("%d Bytes were written\n",bytesWritten);
            //read something
            unsigned int bytesRead;
            int imageNum=0;
            char tstr[0x10000];
            do
            {
                tSerial.Read((char*)&readbuffer,sizeof(readbuffer),bytesRead);
                if (bytesRead==sizeof(readbuffer))
                {
                    sprintf(tstr,"%s%04d.bmp",screenShotPath,imageNum);
                    imageNum++;
                    MakeBMP(tstr,(unsigned char*)&readbuffer);
                }
            }
            while (bytesRead==sizeof(readbuffer));
        }
    }
    else printf("No com port specified.\n");
    return 0;
}
