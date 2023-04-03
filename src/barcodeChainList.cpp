#include "barcodeChainList.h"

#include <stdio.h>

void insertList(BarcodePos **l, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
{
    if (*l != NULL)
    {
        if ((*l)->isValid)
        {
            insertList(&((*l)->next), x1, y1, x2, y2, x3, y3, x4, y4);
        }
        else
        {
            (*l)->p1.x = x1;
            (*l)->p1.y = y1;

            (*l)->p2.x = x2;
            (*l)->p2.y = y2;

            (*l)->p3.x = x3;
            (*l)->p3.y = y3;

            (*l)->p4.x = x4;
            (*l)->p4.y = y4;
        }
    }
    else
    {
        BarcodePos *newElt = (BarcodePos *)malloc(sizeof(BarcodePos));

        if (newElt != NULL)
        {
            newElt->p1.x = x1;
            newElt->p1.y = y1;

            newElt->p2.x = x2;
            newElt->p2.y = y2;

            newElt->p3.x = x3;
            newElt->p3.y = y3;

            newElt->p4.x = x4;
            newElt->p4.y = y4;

            newElt->next = *l;

            *l = newElt;
        }
        else
        {
            printf("Error: Unable to allocate memory\n");
        }
    }

    (*l)->isValid = 1;
}

void freeList(BarcodePos **l)
{
    if (*l != NULL)
    {
        BarcodePos *next = (*l)->next;
        free(*l);
        *l = NULL;

        freeList(&next);
    }
}

void invalidateList(BarcodePos *l)
{
    while (l != NULL)
    {
        l->isValid = 0;
        l = l->next;
    }
}

void copyList(BarcodePos *listSrc, BarcodePos **listDest)
{
    while (listSrc != NULL && listSrc->isValid)
    {
        insertList(listDest, listSrc->p1.x, listSrc->p1.y,
                   listSrc->p2.x, listSrc->p2.y,
                   listSrc->p3.x, listSrc->p3.y,
                   listSrc->p4.x, listSrc->p4.y);

        listSrc = listSrc->next;
    }
}
