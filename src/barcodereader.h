#pragma once

#include <ZXing/ReadBarcode.h>
#include <ZXing/TextUtfEncoding.h>


using namespace ZXing;

// Structure for coordonates for barcode positions
typedef struct point
{
    int x;
    int y;
} Point;

// Structure for barcode position chain list
typedef struct barcodePos
{
    int isValid;
    Point p1;
    Point p2;
    Point p3;
    Point p4;
    
    barcodePos * next;

} BarcodePos;

int readZxing(unsigned char *frame, int width, int height, int nbMaxBarcode, int compress, BarcodePos ** pos);
void drawLine(unsigned char *frame, Point x1, Point x2, int width, int height);
int set_type(const char * types);
char * get_positions();
char * getBarcodesInfo();
void clearBarcodeList();
