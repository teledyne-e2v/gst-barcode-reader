#include "barcodereader.h"

#include <iostream>
#include <stdio.h>
#include <sys/time.h>
#include <ZXing/ZXVersion.h>
#include <vector>
#include <iterator>
#include <stdexcept>
#include <cstdlib>

#include "barcodeChainList.h"

using namespace ZXing;

int setNbBarcode = 1;
int lengthList = 0;
BarcodeFormats value = BarcodeFormat::None;
BarcodePos *tmpPos = NULL;
std::vector<std::string> barcodeTypeVect{};

/*
 * Get all the coordinates of detected barcodes. It return a string of coordinates
 * Format : "p1x,p1y;p2x,p2y;p3x,p3y;p4x,p4y/"
 */
char *get_positions()
{
    char *stringBarcodePos = NULL;
    char coordinateX[5];
    char coordinateY[5];

    stringBarcodePos = (char *)realloc(stringBarcodePos, (lengthList * (9 * 4 + 5 * 4 + 2)));
    BarcodePos *tmp = tmpPos;
    stringBarcodePos[0] = 0;

    coordinateX[0] = 0;
    coordinateY[0] = 0;

    while (tmp != NULL && tmp->isValid)
    {
        sprintf(coordinateX, "%d", tmp->p1.x);
        sprintf(coordinateY, "%d", tmp->p1.y);

        strcat(stringBarcodePos, coordinateX);
        strcat(stringBarcodePos, ",");
        strcat(stringBarcodePos, coordinateY);
        strcat(stringBarcodePos, ";");

        sprintf(coordinateX, "%d", tmp->p2.x);
        sprintf(coordinateY, "%d", tmp->p2.y);

        strcat(stringBarcodePos, coordinateX);
        strcat(stringBarcodePos, ",");
        strcat(stringBarcodePos, coordinateY);
        strcat(stringBarcodePos, ";");

        sprintf(coordinateX, "%d", tmp->p3.x);
        sprintf(coordinateY, "%d", tmp->p3.y);

        strcat(stringBarcodePos, coordinateX);
        strcat(stringBarcodePos, ",");
        strcat(stringBarcodePos, coordinateY);
        strcat(stringBarcodePos, ";");

        sprintf(coordinateX, "%d", tmp->p4.x);
        sprintf(coordinateY, "%d", tmp->p4.y);

        strcat(stringBarcodePos, coordinateX);
        strcat(stringBarcodePos, ",");
        strcat(stringBarcodePos, coordinateY);
        strcat(stringBarcodePos, "/");

        tmp = tmp->next;
    }

    strcat(stringBarcodePos, "\0");

    return stringBarcodePos;
}

/*
 * Image compression
 */
unsigned char *compressY800(unsigned char *data, int width, int height)
{
    unsigned char *dataCompressed = (unsigned char *)malloc(sizeof(unsigned char) * width * height / 4);
    int count = 0;

    int tmpWidth = width;
    int tmpHeight = height;

    if (width % 2 != 0)
        tmpWidth--;
    if (height % 2 != 0)
        tmpHeight--;

    for (int i = 0; i < tmpHeight; i += 2)
    {
        for (int j = 0; j < tmpWidth; j += 2)
        {
            dataCompressed[count] = (data[(i * width) + j] + data[((i + 1) * width) + j] + data[((i + 1) * width) + j + 1] + data[(i * width) + j + 1]) / 4;
            count++;
        }
    }

    return dataCompressed;
}

/*
 * Set barcode type for detecting and decoding
 */
int set_type(const char *types)
{
    int returnType = -1;

    if (std::string(types) == "all")
    {
        value = BarcodeFormatsFromString("1D-Codes,2D-Codes");
        returnType = 1;
    }
    else if (std::string(types) == "None")
        ;
    else
    {
        value = BarcodeFormatsFromString(types);
        returnType = 2;
    }

    return returnType;
}

/*
 * Drawing a gray shade color of pixel
 * Y value for NV121 format
 */
void drawPixelY(unsigned char *frame, int x, int y, int width, int height)
{
    int pixelDim = 7;

    // Some drawing delimitation
    if (x + pixelDim >= height || x - pixelDim <= 0 || y + pixelDim >= width || y - pixelDim <= 0)
    {
        return;
    }

    int start = -pixelDim / 2;
    int end = -start;

    // Some drawing delimitation
    if ((x - start) < 0)
        start = 0;

    for (int i = start; i < end; i++)
    {
        for (int j = start; j < end; j++)
        {
            frame[(x + i) * width + y + j] = 0x50;
        }
    }
}

/*
 * Drawing square of pixels
 */
void drawPixel(unsigned char *frame, int x, int y, int width, int height)
{
    int pixelDim = 5;

    drawPixelY(frame, x, y, width, height);

    if (x + pixelDim >= height || x - pixelDim <= 0 || y + pixelDim >= width || y - pixelDim <= 0)
    {
        return;
    }

    x /= 2;
    unsigned char *frameTmp = frame + (width * height);

    int start = -pixelDim / 2;
    int end = -start;

    // Some drawing delimitation
    if ((x - start) < 0)
        start = 0;
    if ((y + end) > width)
        end = 0;

    // Some drawing delimitation
    if ((x + start) * width + y + start % 2 != 0)
    {
        start -= 1;
        end -= 1;
    }

    // Adding some color for NV12 image format for UV value
    for (int i = start; i < end; i++)
    {
        for (int j = start; j < end; j += 2)
        {
            if ((y + j) % 2 == 0)
            {
                frameTmp[(x + i) * width + y + j - 1] = 0x64;
                frameTmp[(x + i) * width + y + j] = 0xd4;
            }
            else
            {
                frameTmp[(x + i) * width + y + j] = 0x64;
                frameTmp[(x + i) * width + y + j + 1] = 0xd4;
            }
        }
    }
}

/*
 * Drawing line function
 */
void drawLine(unsigned char *frame, Point x1, Point x2, int width, int height)
{
    int dx, dy;

    if ((dx = x2.x - x1.x) != 0)
    {
        if (dx > 0)
        { // si dx > 0 alors
            dy = x2.y - x1.y;
            if (dy != 0)
            { //     si (dy ← y2 - y1) ≠ 0 alors

                if (dy > 0)
                { // si dy > 0 alors
                    // vecteur oblique dans le 1er cadran

                    if (dx >= dy)
                    { // si dx ≥ dy alors
                        // vecteur diagonal ou oblique proche de l’horizontale, dans le 1er octant
                        int e = dx; // déclarer entier e ;
                        // dx ← (e ← dx) × 2 ; dy ← dy × 2 ;  // e est positif
                        dx = 2 * dx;
                        dy = 2 * dy;
                        while (true) // boucle sans fin  // déplacements horizontaux
                        {
                            drawPixel(frame, x1.x, x1.y, width, height);
                            x1.x++;
                            if (x1.x == x2.x)
                            {
                                break; // interrompre boucle si (x1 ← x1 + 1) = x2 ;
                            }
                            e = e - dy;
                            if (e < 0)
                            {               // si (e ← e - dy) < 0 alors
                                x1.y++;     // y1 ← y1 + 1 ;  // déplacement diagonal
                                e = e + dx; // e ← e + dx ;
                                // fin si ;
                            }

                        } // fin boucle ;
                    }
                    else
                    { // sinon
                        // vecteur oblique proche de la verticale, dans le 2d octant
                        int e = dy; // déclarer entier e ;
                        dy *= 2;    // dy ← (e ← dy) × 2 ; dx ← dx × 2 ;  // e est positif
                        dx *= 2;
                        while (true)
                        { // boucle sans fin  // déplacements verticaux
                            drawPixel(frame, x1.x, x1.y, width, height);
                            x1.y++;
                            if (x1.y == x2.y)
                            { // interrompre boucle si (y1 ← y1 + 1) = y2 ;
                                break;
                            }
                            e = e - dx;
                            if (e < 0) // si (e ← e - dx) < 0 alors
                            {
                                x1.x++;     // x1 ← x1 + 1 ;  // déplacement diagonal
                                e = e + dy; // e ← e + dy ;
                            }               // fin si ;
                        }                   // fin boucle ;
                    }
                } // fin si ;
                else
                { // dy < 0 (et dx > 0)
                    // vecteur oblique dans le 4e cadran
                    if (dx >= -dy)
                    { // si dx ≥ - dy alors
                        // vecteur diagonal ou oblique proche de l’horizontale, dans le 8e octant
                        int e = dx;  //              déclarer entier e;
                        dx *= 2;     // dx ← (e ← dx) × 2;
                        dy = dy * 2; // dy ← dy × 2;    // e est positif
                        while (true)
                        { // boucle sans fin // déplacements horizontaux
                            drawPixel(frame, x1.x, x1.y, width, height);
                            x1.x++;
                            if (x1.x == x2.x)
                            {
                                break; // interrompre boucle si(x1 ← x1 + 1) = x2;
                            }
                            e = e + dy;
                            if (e < 0)
                            {                    // si(e ← e + dy) < 0 alors
                                x1.y = x1.y - 1; // y1 ← y1 - 1; // déplacement diagonal
                                e = e + dx;      // e ← e + dx;
                            }                    // fin si;

                        } // fin boucle;
                    }
                    else
                    {                // sinon // vecteur oblique proche de la verticale, dans le 7e octant
                        int e = dy;  //    déclarer entier e;
                        dy = dy * 2; // dy ← (e ← dy) × 2;
                        dx = dx * 2; // dx ← dx × 2;    // e est négatif
                        while (true)
                        { // boucle sans fin // déplacements verticaux
                            drawPixel(frame, x1.x, x1.y, width, height);
                            x1.y--;
                            if (x1.y == x2.y)
                            {
                                break; // interrompre boucle si(y1 ← y1 - 1) = y2;
                            }
                            e = e + dx;
                            if (e > 0)
                            {               // si(e ← e + dx) > 0 alors
                                x1.x++;     //             x1 ← x1 +
                                            // 1; // déplacement diagonal
                                e = e + dy; // e ← e + dy;
                            }               // fin si;
                        }                   // fin boucle;
                    }                       // fin si;

                } // fin si;
            }
            else
            { // sinon // dy = 0 (et dx > 0)

                // vecteur horizontal vers la droite
                while (x1.x != x2.x)
                {
                    drawPixel(frame, x1.x, x1.y, width, height);
                    x1.x++;
                }
                // répéter

                // jusqu’à ce que(x1 ← x1 + 1) = x2;

                // fin si;
            }
        }
        else if (dx < 0) // dx < 0
        {
            dy = x2.y - x1.y;
            if (dy != 0)
            { // si(dy ← y2 - y1) ≠ 0 alors
                if (dy > 0)
                { // si dy > 0 alors
                    // vecteur oblique dans le 2d cadran

                    if (-dx >= dy)
                    { // si - dx ≥ dy alors
                        // vecteur diagonal ou oblique proche de l’horizontale, dans le 4e octant
                        int e; // déclarer entier e;
                        e = dx;
                        dx = dx * 2; // dx ← (e ← dx) × 2;
                        dy = dy * 2; // dy ← dy × 2;    // e est négatif
                        while (true)
                        { // boucle sans fin // déplacements horizontaux
                            drawPixel(frame, x1.x, x1.y, width, height);
                            x1.x--;

                            if (x1.x == x2.x)
                            {
                                break; // interrompre boucle si(x1 ← x1 - 1) = x2;
                            }
                            e = e + dy;
                            if (e >= 0)
                            {
                                // si(e ← e + dy) ≥ 0 alors
                                x1.y++; // y1 ← y1 +
                                // 1; // déplacement diagonal
                                e = e + dx; // e ← e + dx;
                            }
                            // fin si;
                        } // fin boucle;
                    }
                    else
                    {

                        // sinon
                        //  vecteur oblique proche de la verticale, dans le 3e octant
                        int e; // déclarer entier e;
                        e = dy;
                        dy = dy * 2; // ← (e ← dy) × 2;
                        dx *= 2;     // dx ← dx × 2;    // e est positif
                        while (true)
                        { // boucle sans fin // déplacements verticaux
                            drawPixel(frame, x1.x, x1.y, width, height);
                            if ((x1.y = x1.y + 1) == x2.y)
                            {
                                break;
                            } // interrompre boucle si(y1 ← y1 + 1) = y2;
                            if ((e = e + dx) <= 0)
                            { // si(e ← e + dx) ≤ 0 alors
                                // x1 ← x1 -
                                x1.x--;     // 1; // déplacement diagonal
                                e = e + dy; // e ← e + dy;
                            }               // fin si;
                        }                   // fin boucle;
                    }                       // fin si;
                }
                else
                { // sinon // dy < 0 (et dx < 0)
                  //  vecteur oblique dans le 3e cadran
                    if (dx <= dy)
                    {           // si dx ≤ dy alors
                                // vecteur diagonal ou oblique proche de l’horizontale, dans le 5e octant
                        int e;  //           déclarer entier e;
                        e = dx; // dx ← (e ← dx) × 2;
                        dx *= 2;
                        dy *= 2; // dy ← dy × 2;    // e est négatif
                        while (true)
                        { // boucle sans fin // déplacements horizontaux
                            drawPixel(frame, x1.x, x1.y, width, height);
                            x1.x--;
                            if (x1.x == x2.x)
                            {
                                break;
                            } // interrompre boucle si(x1 ← x1 - 1) = x2;
                            if ((e = e - dy) >= 0)
                            { // si(e ← e - dy) ≥ 0 alors
                                // y1 ← y1 -
                                x1.y--;     // 1; // déplacement diagonal
                                e = e + dx; // e ← e + dx;
                            }               // fin si;
                        }                   // fin boucle;
                    }
                    else
                    {           // sinon // vecteur oblique proche de la verticale, dans le 6e octant
                        int e;  // déclarer entier e;
                        e = dy; // dy ← (e ← dy) × 2;
                        dy = dy * 2;
                        dx *= 2; // dx ← dx × 2;    // e est négatif
                        while (true)
                        { // boucle sans fin // déplacements verticaux
                            drawPixel(frame, x1.x, x1.y, width, height);
                            if ((x1.y = x1.y - 1) == x2.y)
                            {
                                break;
                            }
                            // interrompre boucle si(y1 ← y1 - 1) = y2;

                            if ((e = e - dx) >= 0)
                            {           // si(e ← e - dx) ≥ 0 alors
                                x1.x--; // x1 ← x1 -
                                // 1; // déplacement diagonal
                                e = e + dy; // e ← e + dy;si dx ≤ dy alors
                            }
                        }
                    }
                }
            }
            else
            { // sinon // dy = 0 (et dx < 0)

                // vecteur horizontal vers la gauche
                while ((x1.x = x1.x - 1) == x2.x)
                { // répéter
                    drawPixel(frame, x1.x, x1.y, width, height);
                } // jusqu’à ce que(x1 ← x1 - 1) = x2;

            } // fin si;
        }
    }
    else
    { // sinon  // dx = 0
        if ((dy = x2.y - x1.y) != 0)
        { // si(dy ← y2 - y1) ≠ 0 alors
            if (dy > 0)
            { // si dy > 0 alors
                // vecteur vertical croissant
                while ((x1.y = x1.y + 1) != x2.y)
                { // répéter
                    drawPixel(frame, x1.x, x1.y, width, height);
                } // jusqu’à ce que(y1 ← y1 + 1) = y2;
            }
            else
            { // sinon // dy < 0 (et dx = 0)

                // vecteur vertical décroissant
                while ((x1.y = x1.y - 1) == x2.y)
                { // répéter
                    drawPixel(frame, x1.x, x1.y, width, height);
                } // jusqu’à ce que(y1 ← y1 - 1) = y2;
            }
        }

        // fin si;
        // fin si;
    }
    // fin si ;
    //  le pixel final (x2, y2) n’est pas tracé.
    // fin procédure;
}

/*
 * Detecting and decoding function
 */
int readZxing(unsigned char *frame, int width, int height, int nbMaxBarcode, int compress, BarcodePos **pos)
{
	if(height<0 || width<0 || frame == NULL)
{
	return 0;
}
    unsigned char *tmp_frame = frame;

    // If compression is activated
    if (compress == 1)
    {
        tmp_frame = compressY800(frame, width, height);

        width /= 2;
        height /= 2;
    }

    if (tmp_frame == NULL)
    {
        return 0;
    }

    ImageView imgView(tmp_frame, width, height, ImageFormat::Lum);



    DecodeHints hints;
    Results results;

    struct timeval start, end;
    double elapsed = 0.0f;

    // Set max number of barcode if Zxing version is greater than 1.2.0
#if ZXING_VERSION_MAJOR >= 1 && ZXING_VERSION_MINOR > 2 && ZXING_VERSION_PATCH >= 0
    hints.setMaxNumberOfSymbols(nbMaxBarcode);
#endif

    setNbBarcode = nbMaxBarcode;
    lengthList = 0;

    // Set types format it should decode
    hints.setFormats(value);
	
try
    {
        gettimeofday(&start, NULL);
        results = ReadBarcodes(imgView, hints);
    gettimeofday(&end, NULL);
    }
    catch ( ... )
    {
        // Handle exception 
        std::cout << "Error during barcode decoding: " << std::endl;
        return 1;
    }



    elapsed +=
        ((end.tv_sec * 1000000 + end.tv_usec) -
         (start.tv_sec * 1000000 + start.tv_usec)) /
        1000.0f;

    if (compress == 1)
        free(tmp_frame);

    if (!results.empty())
    {

        //std::cout << "Done, took " << elapsed << "ms\n"
        //          << std::endl;

        for (auto &&result : results)
        {
            // Getting type and data of detected barcodes
            std::string sFormat = ToString(result.format());
            std::string sData = TextUtfEncoding::ToUtf8(result.text());

            std::string sType = sFormat + "\n" + sData + "\n\n";

            char *tmpFormat = &sFormat[0];
            char *tmpData = &sData[0];

            // Insertion of barcode information in a vector if it's a new one
            if (std::find(barcodeTypeVect.begin(), barcodeTypeVect.end(), sType) == std::end(barcodeTypeVect))
            {
                barcodeTypeVect.insert(barcodeTypeVect.begin(), sType);
            }

            // We memorize at most 50 different barcodes. The oldest barcode is erased
            if (barcodeTypeVect.size() > 50)
            {
                barcodeTypeVect.pop_back();
            }

            std::cout << "Format: " << sFormat << "\n"
                      << "Data:   " << sData << "\n"
                      << "Orientation: " << result.orientation() << std::endl;

            Position p = result.position();

            // Insert the barcode position in a chain list to frame them from the plugin
            if (compress == 1)
            {
                insertList(pos, p.topLeft().y * 2, p.topLeft().x * 2,
                           p.topRight().y * 2, p.topRight().x * 2,
                           p.bottomRight().y * 2, p.bottomRight().x * 2,
                           p.bottomLeft().y * 2, p.bottomLeft().x * 2);
            }
            else
            {
                insertList(pos, p.topLeft().y, p.topLeft().x,
                           p.topRight().y, p.topRight().x,
                           p.bottomRight().y, p.bottomRight().x,
                           p.bottomLeft().y, p.bottomLeft().x);
            }
            lengthList++;
        }
    }

    tmpPos = *pos;

    return !results.empty();
}

/*
 * Clear the barcode list
 */
void clearBarcodeList() 
{
    barcodeTypeVect.clear();
}

/*
 * Get all the type and data from the barcode detected. It return a string of barcode type and data.
 */
char * getBarcodesInfo()
{
    size_t nb = barcodeTypeVect.size();
    size_t len = nb * 21 * 8002;
    char *str = (char *)malloc(sizeof(char) * len);
    str = (char *)memset(str, '\0', len);

    if (nb > 0)
    {
        std::string tmp;

        for (auto itr = barcodeTypeVect.begin(); itr != barcodeTypeVect.end(); itr++)
        {
            tmp = *itr;
            strcat(str, &tmp[0]);
        }
    }

    return str;
}
