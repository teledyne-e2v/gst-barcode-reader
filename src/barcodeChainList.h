#pragma once

#include "barcodereader.h"

void insertList(BarcodePos **l, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);

void freeList(BarcodePos **l);

void invalidateList(BarcodePos *l);

void copyList(BarcodePos * listSrc, BarcodePos ** listDest);