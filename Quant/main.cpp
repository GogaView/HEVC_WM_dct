

#include<math.h>
#include<iostream>
#include <vector>
#include <fstream>


#include "DCT.h"
#include "Quant.h"


#define LOG 0

using namespace std;

#define size2 288
#define size1 352
#define FRAME 100//260

int QP = 32;


typedef int LONG;
typedef unsigned char BYTE;
typedef uint32_t DWORD;
typedef unsigned short WORD;

// __attribute__((packed)) on non-Intel arch may cause some unexpected error, plz be informed.

typedef struct tagBITMAPFILEHEADER
{
    WORD    bfType; // 2  /* Magic identifier */
    DWORD   bfSize; // 4  /* File size in bytes */
    WORD    bfReserved1; // 2
    WORD    bfReserved2; // 2
    DWORD   bfOffBits; // 4 /* Offset to image data, bytes */
} __attribute__((packed)) BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
    DWORD    biSize; // 4 /* Header size in bytes */
    LONG     biWidth; // 4 /* Width of image */
    LONG     biHeight; // 4 /* Height of image */
    WORD     biPlanes; // 2 /* Number of colour planes */
    WORD     biBitCount; // 2 /* Bits per pixel */
    DWORD    biCompress; // 4 /* Compression type */
    DWORD    biSizeImage; // 4 /* Image size in bytes */
    LONG     biXPelsPerMeter; // 4
    LONG     biYPelsPerMeter; // 4 /* Pixels per meter */
    DWORD    biClrUsed; // 4 /* Number of colours */
    DWORD    biClrImportant; // 4 /* Important colours */
} __attribute__((packed)) BITMAPINFOHEADER;

/*
 typedef struct tagRGBQUAD
 {
 unsigned char    rgbBlue;
 unsigned char    rgbGreen;
 unsigned char    rgbRed;
 unsigned char    rgbReserved;
 } RGBQUAD;
 * for biBitCount is 16/24/32, it may be useless
 */

typedef struct tagRGBTRIPLE {
    BYTE    rgbtBlue;
    BYTE    rgbtGreen;
    BYTE    rgbtRed;
} RGBTRIPLE;

typedef struct
{
    BYTE    b;
    BYTE    g;
    BYTE    r;
} RGB_data; // RGB TYPE, plz also make sure the order


struct st_values
{
    int iFrame;
    int trSize;
    int TrW;
    int TrH;
    int trX;
    int trY;
    int offset;
    int isDST;
    int Value;
    
    int** pPred;
};

typedef int Int;
typedef unsigned int UInt;


static const int MAX_CU_SIZE =                                     64;
signed char  g_aucConvertToBit  [ MAX_CU_SIZE+1 ];


const Int g_invQuantScales[6] =
{
    40,45,51,57,64,72
};

void xPredIntraPlanar( int* pDSrc, int* rpDst, unsigned int width, unsigned int height )
{
    int srcStride = 2*width + 1;
    int dstStride = width;
    
    int* pSrc = pDSrc + srcStride + 1;
    
    
    
    
    Int leftColumn[MAX_CU_SIZE+1], topRow[MAX_CU_SIZE+1], bottomRow[MAX_CU_SIZE], rightColumn[MAX_CU_SIZE];
    UInt shift1Dhor = g_aucConvertToBit[ width ] + 2;
    UInt shift1Dver = g_aucConvertToBit[ height ] + 2;
    
    
    
    for(int i = 0; i!= height; i++)
    {
        for(int j = 0; j!= width; j++)
        {
            if(i == 0)
            {
                topRow[j] = pDSrc[i*width + j] ;
            }
            
            if(j == width-1)
            {
                leftColumn[i] = pDSrc[i*width + j] ;
            }
            std::cout << pDSrc[i*width + j] << " ";
        }
        std::cout << std::endl;
    }
    
    
    
    // Get left and above reference column and row
//    for(Int k=0;k<width+1;k++)
//    {
//        topRow[k] = pSrc[k-srcStride];
//    }
//    
//    for (Int k=0; k < height+1; k++)
//    {
//        leftColumn[k] = pSrc[k*srcStride-1];
//    }
//    
    // Prepare intermediate variables used in interpolation
    Int bottomLeft = leftColumn[height-1];
    Int topRight   = topRow[width-1];
    
    for(Int k=0;k<width;k++)
    {
        bottomRow[k]  = bottomLeft - topRow[k];
        topRow[k]     <<= shift1Dver;
    }
    
    for(Int k=0;k<height;k++)
    {
        rightColumn[k]  = topRight - leftColumn[k];
        leftColumn[k]   <<= shift1Dhor;
    }
    
    const UInt topRowShift = 0;
    
    // Generate prediction signal
    for (Int y=0;y<height;y++)
    {
        Int horPred = leftColumn[y] + width;
        for (Int x=0;x<width;x++)
        {
            horPred += rightColumn[y];
            topRow[x] += bottomRow[x];
            
            Int vertPred = ((topRow[x] + topRowShift)>>topRowShift);
            rpDst[y*dstStride+x] = ( horPred + vertPred ) >> (shift1Dhor+1);
        }
    }
}



std::vector<st_values> initConfig()
{
    std::vector<st_values> vect;
    
    std::ifstream file("/Users/macbook/Library/Developer/Xcode/DerivedData/HM_origin-abadnxruhqepfhdfonruobcflnzj/Build/Products/Debug/Pred.txt");
    
    char c[256];
//    file.getline(c,256);
    
    while(!file.eof())
    {
        file.getline(c,256);
        
        st_values val;
        sscanf(c,"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d", &val.iFrame, &val.trSize, &val.TrW, &val.TrH, &val.trX, &val.trY, &val.offset, &val.isDST, &val.Value);
        
        val.pPred = new int*[val.TrH];
        
        for(int i = 0; i != val.TrH; i++)
        {
            val.pPred[i] = new int[val.TrW];
            
            for(int j = 0; j != val.TrW; j++)
            {
                int c1;
                file >> c1;
                val.pPred[i][j] = c1;
            }
            
        }
        file.getline(c,256);
        file.getline(c,256);
        
        
        vect.push_back(val);
    }
    
    return vect;
}


int** _getRect(st_values stVal, int** bFrame)
{
    int** b = new int* [stVal.TrH];
    for (int i = 0; i < stVal.TrH; i++)
    {
        b[i] = new int [stVal.TrW];
    }
    
    
    for (int i = 0; i < stVal.TrH; i++)
        for (int j = 0; j < stVal.TrW; j++)
        {
            b[i][j] = bFrame[stVal.trY + i ][stVal.trX + j];
        }
#if LOG
    // print
    std::cout << "Rect: " << std::endl;
    for (int i = 0; i < stVal.TrH; i++)
    {
        for (int j = 0; j < stVal.TrW; j++)
        {
            std::cout << b[i][j]  << " ";
        }
        
        std::cout << std::endl;
    }
    std::cout << std::endl;
#endif
    
    return b;
}


int** _getPred(st_values stVal, int** bRect)
{
    int** b = new int* [stVal.TrH];
    for (int i = 0; i < stVal.TrH; i++)
    {
        b[i] = new int [stVal.TrW];
    }
    
    
    for (int i = 0; i < stVal.TrH; i++)
        for (int j = 0; j < stVal.TrW; j++)
        {
            b[i][j] = bRect[i][j] - stVal.pPred[i][j];
        }
#if LOG
    std::cout << "Pred: " << std::endl;
    // print
    for (int i = 0; i < stVal.TrH; i++)
    {
        for (int j = 0; j < stVal.TrW; j++)
        {
            std::cout << b[i][j]  << " ";
        }
        
        std::cout << std::endl;
    }
    std::cout << std::endl;
#endif
    
    return b;
}



int pp[16][16];


double pos;

double calc(int m, int p)
{
    double a = (double)3.14*p;
    a = (double)a*(2*m+1);
    a = (double)a/16;
    return a;
}

int current;

int* _dct1(st_values stVal, int** bFrame)
{
    int* block = new int[stVal.TrW * stVal.TrH];
    int* coeff = new int[stVal.TrW * stVal.TrH];
    
    
    for(int i = 0; i != stVal.TrH; i++)
        for(int j = 0; j != stVal.TrW; j++)
            block[i*stVal.TrW+j] = bFrame[i][j];
    
    TCoeff tmp[ 32 * 32 ];
    
    switch (stVal.TrW ) {
        case 4:
            if(stVal.isDST == 1)
                fastForwardDst(block, tmp, 1);
            else
                partialButterfly4 ( block, tmp, 1, stVal.TrH );
            break;
        case 8:
            partialButterfly8 ( block, tmp, 2, stVal.TrH );
            break;
        case 16:
            partialButterfly16 ( block, tmp, 3, stVal.TrH );
            break;
        case 32:
            partialButterfly32 ( block, tmp, 4, stVal.TrH );
            break;
    }
    
    
    switch (stVal.TrH ) {
        case 4:
            if(stVal.isDST == 1)
                fastForwardDst( tmp, coeff, 8 );
            else
                partialButterfly4 ( tmp, coeff, 8, stVal.TrW );
            break;
        case 8:
            partialButterfly8 ( tmp, coeff, 9, stVal.TrW );
            break;
        case 16:
            partialButterfly16 ( tmp, coeff, 10, stVal.TrW );
            break;
        case 32:
            partialButterfly32 ( tmp, coeff, 11, stVal.TrW );
            break;
    }
    
    int** out = new int*[stVal.TrH];
    
    for(int i = 0; i != stVal.TrH; i++)
    {
        out[i] = new int[stVal.TrW];
        
        for(int j = 0; j != stVal.TrW; j++)
            out[i][j] = coeff[i*stVal.TrW+j];
    }
    
#if LOG
    std::cout << "DCT: " << std::endl;
    // print
    for (int i = 0; i < stVal.TrH; i++)
    {
        for (int j = 0; j < stVal.TrW; j++)
        {
            std::cout << out[i][j]  << " ";
        }
        
        std::cout << std::endl;
    }
    std::cout << std::endl;
#endif
    
    return coeff;
    
    
}


double** _dct(st_values stVal, int** bFrame)
{
    double** DKP = new double* [stVal.TrH];
    for (int i = 0; i < stVal.TrH; i++)
    {
        DKP[i] = new double [stVal.TrW];
    }
    
    for (int p = 0; p < stVal.TrH; p++)
        for (int q = 0; q < stVal.TrW; q++)
        {
            double a = 0;
            double b = 0;
            
            
            if (p == 0)
                a = (double)sqrt((double)1/8);
            else
                a = (double)sqrt((double)2/8);
            if (q == 0)
                b = (double)sqrt((double)1/8);
            else
                b = (double)sqrt((double)2/8);
            
            DKP[p][q] = 0;
            for (int m = 0; m < stVal.TrH; m++)
                for (int n = 0; n < stVal.TrW; n++)
                {
                    int x = calc(m,p);
                    x = (double)cos(x);
                    int y = calc(n,q);
                    y = (double)cos(y);
                    DKP[p][q] = DKP[p][q] + x*y*bFrame[m][n];
                }
            DKP[p][q] = (double)DKP[p][q]*a*b;
        }

    return DKP;
}



void _init()
{
    Int i, c;
    
    // g_aucConvertToBit[ x ]: log2(x/4), if x=4 -> 0, x=8 -> 1, x=16 -> 2, ...
    ::memset( g_aucConvertToBit,   -1, sizeof( g_aucConvertToBit ) );
    c=0;
    for ( i=4; i<=MAX_CU_SIZE; i*=2 )
    {
        g_aucConvertToBit[ i ] = c;
        c++;
    }
}

int* _getIntPtr(st_values stVal, int** bFrame)
{
    int* Mass = new int [stVal.TrH * stVal.TrW];
    
    for (int i = 0; i < stVal.TrH; i++)
        for (int j = 0; j < stVal.TrW; j++)
        {
            Mass[i*stVal.TrH+j] = bFrame[i][j];
        }
    
    
    
    return Mass;
}

double* _getIntPtr(st_values stVal, double** bFrame)
{
    double* Mass = new double [stVal.TrH * stVal.TrW];
    
    for (int i = 0; i < stVal.TrH; i++)
        for (int j = 0; j < stVal.TrW; j++)
        {
            Mass[i*stVal.TrH+j] = bFrame[i][j];
        }
    
    
    
    return Mass;
}

int* _quant(st_values stVal, int* dct)
{
    int inputMinimum = -32768;
    int inputMaximum = 32767;
    
    int* Quant = new int[stVal.TrW * stVal.TrH];
    
    int iTransformShift = 15 - 8 - stVal.trSize;
    int iQBits = 14 + QP/6 + iTransformShift;             /// this is iQBits
//    iQBits = 23;
    int iDefQuantCoeff = g_quantScales[QP%6];
    
    for( Int n = 0; n < stVal.TrW * stVal.TrH; n++ )
    {
        int iTmpVal = abs(dct[n]) * iDefQuantCoeff;
        
        int lLevelDouble = std::min<Int64>(iTmpVal, std::numeric_limits<int>::max() - (int(1) << (iQBits - 1)));
        
        typedef unsigned int UInt;
        
        UInt uiMaxAbsLevel  = std::min<UInt>(UInt(inputMaximum), UInt((lLevelDouble + (int(1) << (iQBits - 1))) >> iQBits));
        Quant[n] = uiMaxAbsLevel;
    }

#if LOG
    std::cout << "Quant:" << std::endl;
    for (int i = 0; i < stVal.TrH; i++)
    {
        for (int j = 0; j < stVal.TrW; j++)
        {
            std::cout << Quant[i*stVal.TrH+j] << " " ;
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
#endif
    
    return Quant;
    
}


int _getValue(int* vMas, st_values val,int iInd)
{
    int index = (iInd*val.TrW)+iInd;
    return vMas[index];
}

int _getValue(int* vMas, st_values val, int iX, int iY)
{
    int index = (iY*val.TrW)+iX;
    return vMas[index];
}

bool _isCorrect(int* vMas, st_values val, int iX, int iY)
{
    auto masValue = _getValue(vMas, val, iX, iY);
    
    if(masValue % 4 == 0 || masValue % 4 == 1)
    {
        return (val.Value == 0);
    }
    else // if(masValue % 4 == 2 || masValue % 4 == 3)
    {
        return (val.Value == 1);
    }
}

struct st_valRes
{
    int iFrame;
    int bRes;
};

int main ()
{
    FILE* f2;
    
    f2 = fopen("/Users/macbook/Library/Developer/Xcode/DerivedData/HM_origin-abadnxruhqepfhdfonruobcflnzj/Build/Products/Debug/bus_new.yuv","rb");
//    f2 = fopen("/Users/macbook/Library/Developer/Xcode/DerivedData/HM_origin-abadnxruhqepfhdfonruobcflnzj/Build/Products/Debug/bus.yuv","rb");
    
    _init();
    auto vectPositions = initConfig();
    
    int** b = new int* [size1];
    for (int i = 0; i < size1; i++)
    {
        b[i] = new int [size2];
    }

    current = 1;
    
    
    unsigned char luma1 = '\0';
    unsigned char chroma;
    unsigned long long l = 0;
    
    
    auto itBit = vectPositions.begin();
    
    
    int i_00 = 0;
    int i_10 = 0;
    int i_01 = 0;
    int i_11 = 0;
    int i_20 = 0;
    int i_02 = 0;
    int i_22 = 0;
    
    
    int det_0 = 0;
    int det_1 = 0;
    int det_2 = 0;
    int det_3 = 0;
    int det_4 = 0;
    int det_5 = 0;
    int det_more = 0;
    
    std::vector<int> vRes;
    int iCount = 0;
    
    
    // read all data
    for (int fr = 0; (fr < FRAME); fr++)
    {
        l = 0;
        
        // Y
        for (int i = 0; i < size2; i++)
            for (int j = 0; j < size1; j++)
            {
                fread(&luma1,  1, 1, f2);
                b[i][j] = luma1;
                l++;
            }
        
        // U
        for (int i = 0; i < size2/2; i++)
            for (int j = 0; j < size1/2; j++)
            {
                fread(&luma1,  1, 1, f2);
                l++;
            }
        
        // V
        for (int i = 0; i < size2/2; i++)
            for (int j = 0; j < size1/2; j++)
            {
                fread(&luma1,  1, 1, f2);
                l++;
            }
        
        
//        std::ofstream frame("/Users/macbook/Library/Developer/Xcode/DerivedData/HM_origin-abadnxruhqepfhdfonruobcflnzj/Build/Products/Debug/Frame");
//        for (int i = 0; i < size2; i++)
//        {
//            for (int j = 0; j < size1; j++)
//            {
//                frame << b[i][j] << " " ;
//            }
//            
//            frame << std::endl;
//        }
        
        int iClose = 1;
        
        
        while(itBit != vectPositions.end() && itBit->iFrame == fr)
        {
            if( (itBit->trX < iClose && itBit->trY < iClose ) ||
                (itBit->trX + itBit->trSize > size1 - iClose && itBit->trY < iClose ) ||
                (itBit->trX < iClose && itBit->trY + itBit->trSize > size2 - iClose ) ||
                (itBit->trX + itBit->trSize > size1 - iClose && itBit->trY + itBit->trSize > size2 - iClose ) )
            {
//                itBit++;
//                continue;
            }
            
            
            auto Rect = _getRect(*itBit, b);
            auto Pred = _getPred(*itBit, Rect);
            auto DKP = _dct1(*itBit, Pred);
            auto Quant = _quant(*itBit, DKP);
            
            
            int iValidCount = 0;
            auto val = _getValue(Quant, *itBit, 2, 2);
//            auto det =abs(itBit->Value) - abs(val);
//            if(det == 0)
//                det_0++;
//            else if(det == 1)
//                det_1++;
//            else if(det == 2)
//                det_2++;
//            else if(det == 3)
//                det_3++;
//            else if(det == 4)
//                det_4++;
//            else if(det == 5)
//                det_5++;
//            else if(det > 5)
//                det_more++;
            
            iValidCount += _isCorrect(Quant, *itBit, 2, 2);
            
            
//            iValidCount += ( _getValue(Quant, *itBit, 0, 2) % 2 == 0);
//            iValidCount += ( _getValue(Quant, *itBit, 2, 2) % 4 == 0) || ( _getValue(Quant, *itBit, 2, 2) % 4 == 1);
//            iValidCount += ( _getValue(Quant, *itBit, 2, 0) % 2 == 0);
//            iValidCount += ( _getValue(Quant, *itBit, 0, 1) % 2 == 0);
//            iValidCount += ( _getValue(Quant, *itBit, 1, 0) % 2 == 0);
//            iValidCount += ( _getValue(Quant, *itBit, 0, 0) % 2 == 0);
//            iValidCount += ( _getValue(Quant, *itBit, 2, 2) % 3 == 0);
            
//            i_02 += ( _getValue(Quant, *itBit, 0, 2) % 2 == 0);
            i_11 += ( _getValue(Quant, *itBit, 2, 2) % 4 == 0) || ( _getValue(Quant, *itBit, 2, 2) % 4 == 1);
//            i_20 += ( _getValue(Quant, *itBit, 2, 0) % 2 == 0);
//            i_01 += ( _getValue(Quant, *itBit, 0, 1) % 2 == 0);
//            i_10 += ( _getValue(Quant, *itBit, 1, 0) % 2 == 0);
//            i_00 += ( _getValue(Quant, *itBit, 0, 0) % 2 == 0);
//            i_22 += ( _getValue(Quant, *itBit, 2, 2) % 3 == 0);
            
            iCount++;
            std::cout << /*std::distance(vectPositions.begin(), itBit)*/ iCount <<  "\t" << iValidCount <<  "\t";
//            if( _getValue(Quant, *itBit, 1) % 2 == 0)
//            if( ( _getValue(Quant, *itBit, 0, 1) % 2 == 0) && ( _getValue(Quant, *itBit, 1, 0) % 2 == 0) )
            if(iValidCount >= 1)
                std::cout << "true" << std::endl;
            else
                std::cout << "false" << std::endl;
            
            
            vRes.push_back(iValidCount >= 1);
            
            itBit++;
        }
        
        int ii = 0;
    
    }
    
//    
//    while(vRes.size() >= 3)
//    {
//        int iRes = 0;
//        int iCount = 3;
//        
//        for(int i = 0; i != iCount; i++)
//        {
//            int i1 = rand()%vRes.size();
//            iRes += vRes[i1];
//            vRes.erase(vRes.begin() + i1);
//        }
//        
//            if(iRes >= 2)
//                std::cout << "true" <<std::endl;
//            else
//                std::cout << "false" <<std::endl;
//
//
//    }
//    
    
    
    fclose(f2);
    
    return 1;
}