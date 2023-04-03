/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2022 loic <<user@hostname.org>>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-barcodereader
 *
 * FIXME:Describe barcodereader here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! barcodereader ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>

#include "gstbarcodereaderplugin.h"
#include "barcodeChainList.h"

#include <iostream>
#include <fstream>
#include <string.h>

#include <sys/time.h>
#include <pthread.h>

GST_DEBUG_CATEGORY_STATIC(gst_bar_code_reader_debug);
#define GST_CAT_DEFAULT gst_bar_code_reader_debug

/* Filter signals and args */
enum
{
    /* FILL ME */
    LAST_SIGNAL
};

enum
{
    PROP_0,
    PROP_TYPE,
    PROP_NUMBER_BARCODE,
    PROP_FRAMING,
    PROP_STRPOS,
    PROP_STRCODE,
    PROP_COMPRESSED,
    PROP_CLEARBARCODE
};

typedef enum
{
    ACTIVE,
    INACTIVE,
    PENDING
} State;

volatile State state = PENDING;

int framing = 0;
int returnType = 0;

typedef struct frame
{
    unsigned char *frame;
    int width;
    int height;
    int nbMaxBarcode;
    int compressImg;
    BarcodePos **positions;
    GstBarCodeReader * filter;
} Frame;

unsigned char * dataFrame = NULL;

BarcodePos *pos = NULL;
BarcodePos *tmpBarcodePos = NULL;

Frame frame = {NULL, 0, 0, 0, 0, 0, NULL};

volatile gboolean pipelineRunning = TRUE;

pthread_t thread;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
                                                                   GST_PAD_SINK,
                                                                   GST_PAD_ALWAYS,
                                                                   GST_STATIC_CAPS("ANY"));

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE("src",
                                                                  GST_PAD_SRC,
                                                                  GST_PAD_ALWAYS,
                                                                  GST_STATIC_CAPS("ANY"));

#define gst_bar_code_reader_parent_class parent_class
G_DEFINE_TYPE(GstBarCodeReader, gst_bar_code_reader, GST_TYPE_ELEMENT)

static void gst_bar_code_reader_set_property(GObject *object, guint prop_id,
                                             const GValue *value, GParamSpec *pspec);
static void gst_bar_code_reader_get_property(GObject *object, guint prop_id,
                                             GValue *value, GParamSpec *pspec);

static GstFlowReturn gst_bar_code_reader_chain(GstPad *pad, GstObject *parent, GstBuffer *buf);

static void gst_barcodereader_finalize(GObject *object);

void *decode(void *);

/* GObject vmethod implementations */

/* initialize the barcodereader's class */

/* We initialize the different plugin parameters. We set their type, name, description, default value, min value, max value, rights...
 * Those information will be printer in the gst-inspect command.
 */
static void
gst_bar_code_reader_class_init(GstBarCodeReaderClass *klass)
{
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;

    gobject_class = (GObjectClass *)klass;
    gstelement_class = (GstElementClass *)klass;

    gobject_class->set_property = gst_bar_code_reader_set_property;
    gobject_class->get_property = gst_bar_code_reader_get_property;
    gobject_class->finalize = gst_barcodereader_finalize;

    // TODO : complete...
    /*g_object_class_install_property(gobject_class, PROP_BARCODE_TYPE,
                                    g_param_spec_());*/
            
    g_object_class_install_property(gobject_class, PROP_TYPE,
                                    g_param_spec_string("type", "Type",
                                                     "Specifie the different barcode type the plugin should decode. By default, Code128 and QRCode are set. Please, use the ',' separator. You can specifie multiple type amoung these types:\n\t\t\t- all : for all barcodes\n\t\t\t- 1D-Codes : for all the 1D barcodes\n\t\t\t- 2D-Codes : for all the 2D barcodes\n\t\t\t- Aztec\n\t\t\t- Codabar\n\t\t\t- Code39\n\t\t\t- Code93\n\t\t\t- Code128\n\t\t\t- DataBar\n\t\t\t- DataBarExpanded\n\t\t\t- DataMatrix\n\t\t\t- EAN-8\n\t\t\t- EAN-13\n\t\t\t- ITF\n\t\t\t- MaxiCode\n\t\t\t- PDF417\n\t\t\t- QRCode\n\t\t\t- UPC-A\n\t\t\t- UPC-E",
                                                     "Code128,QRCode", G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_NUMBER_BARCODE,
                                    g_param_spec_int("number_barcode", "Number_barcode",
                                                     "Set the maximum number of barcode it should found",
                                                     0, 25, 1, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_COMPRESSED,
                                    g_param_spec_boolean("compressed", "Compressed",
                                                         "Whether or not it should compressed the image for the detection and decoding. It reduce execution time but can find less barcode",
                                                         FALSE, G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_FRAMING,
                                    g_param_spec_boolean("framing", "Framing",
                                                         "Whether or not it should frame the barcodes detected",
                                                         TRUE, G_PARAM_READWRITE));
                                                         
    g_object_class_install_property(gobject_class, PROP_STRPOS,
                                    g_param_spec_string("strPos", "StrPos",
                                                         "Read-only property to get the coordinates of barcodes",
                                                         "", G_PARAM_READABLE));

    g_object_class_install_property(gobject_class, PROP_STRCODE,
                                    g_param_spec_string("strCode", "StrCode",
                                                         "Read-only property to get the decoding of barcodes",
                                                         "",G_PARAM_READABLE));

    g_object_class_install_property(gobject_class, PROP_CLEARBARCODE,
                                    g_param_spec_boolean("clearBarcode", "ClearBarcode",
                                                         "Set this property to any value to clear the logs",
                                                         FALSE, G_PARAM_WRITABLE));


    gst_element_class_set_details_simple(gstelement_class,
                                         "BarCodeReader",
                                         "FIXME:Generic",
                                         "Detect, Decode and frame barcode",
                                         "Esisar-PI2022 <<user@hostname.org>>");

    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&src_factory));
    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_bar_code_reader_init(GstBarCodeReader *filter)
{
    filter->sinkpad = gst_pad_new_from_static_template(&sink_factory, "sink");

    gst_pad_set_chain_function(filter->sinkpad,
                               GST_DEBUG_FUNCPTR(gst_bar_code_reader_chain));
    GST_PAD_SET_PROXY_CAPS(filter->sinkpad);
    gst_element_add_pad(GST_ELEMENT(filter), filter->sinkpad);

    filter->srcpad = gst_pad_new_from_static_template(&src_factory, "src");
    GST_PAD_SET_PROXY_CAPS(filter->srcpad);
    gst_element_add_pad(GST_ELEMENT(filter), filter->srcpad);

    filter->type = "Code128,QRCode";
    returnType = set_type(filter->type);

    filter->numberOfBarcode = 1;

    filter->compressed = FALSE;
    filter->framing = TRUE;
    
    filter->strPos = (char *) malloc(sizeof(char));
    filter->strPos = 0;
    filter->strCode = (char *) malloc(sizeof(char));
    filter->strCode = 0;

    int rc;
    if ((rc = pthread_create(&thread, NULL, decode, NULL)))
    {
        g_print("Error:unable to create thread, %d\n", rc);
        exit(-1);
    }
}

static void
gst_bar_code_reader_set_property(GObject *object, guint prop_id,
                                 const GValue *value, GParamSpec *pspec)
{
    GstBarCodeReader *filter = GST_BARCODEREADER(object);

    switch (prop_id)
    {
    case PROP_TYPE:
        filter->type = g_value_get_string(value);
        returnType = set_type(filter->type);
        break;
    case PROP_NUMBER_BARCODE:
        filter->numberOfBarcode = g_value_get_int(value);
        break;
    case PROP_COMPRESSED:
        filter->compressed = g_value_get_boolean(value);
        break;
    case PROP_FRAMING:
        filter->framing = g_value_get_boolean(value);
        break;
    case PROP_CLEARBARCODE:
        clearBarcodeList();
	    filter->strCode[0] = 0;
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_bar_code_reader_get_property(GObject *object, guint prop_id,
                                 GValue *value, GParamSpec *pspec)
{
    GstBarCodeReader *filter = GST_BARCODEREADER(object);

    switch (prop_id)
    {
    case PROP_TYPE:
        g_value_set_string(value, filter->type);
        break;
    case PROP_NUMBER_BARCODE:
        g_value_set_int(value, filter->numberOfBarcode);
        break;
    case PROP_COMPRESSED:
        g_value_set_boolean(value, filter->compressed);
        break;
    case PROP_FRAMING:
        g_value_set_boolean(value, filter->framing);
        break;
    case PROP_STRPOS:
        g_value_set_string(value, filter->strPos);
        break;
    case PROP_STRCODE:
        g_value_set_string(value, filter->strCode);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/*
 * Function to call the Zxing decoding functino and copy the chain list for all the barcode detected
 */
void *decode(void *)
{
    state = INACTIVE;

    while (pipelineRunning)
    {
        pthread_mutex_lock(&lock);

        pthread_cond_wait(&cond, &lock);

        if (state == ACTIVE)
        {
            // Call of the barcode reader function. We passed the data, some parameters and a structure to get the barcodes detected
            framing = readZxing(frame.frame, frame.width, frame.height, frame.nbMaxBarcode, frame.compressImg, frame.positions);

            // Get the position of all barcode. It is used for the application to frame the barcodes from the application
            char * tmp = get_positions();

            frame.filter->strPos = (char *) realloc(frame.filter->strPos, sizeof(char) * strlen(tmp)+1);

            // Copy the positions to frame the barcodes from application
            if(frame.filter->strPos!=NULL)
            {
                frame.filter->strPos = strcpy(frame.filter->strPos, tmp);
            }
            free(tmp);
            
            // Get the barcodes informations (type and data). It used for the application
            tmp = getBarcodesInfo();
            frame.filter->strCode = (char*)realloc(frame.filter->strCode, sizeof(char) * strlen(tmp) + 1);

            // Copy the informations of barcodes to print it from the application
            if (frame.filter->strCode != NULL)
            {
                frame.filter->strCode = strcpy(frame.filter->strCode, tmp);
            }
            free(tmp);

            // Copy the structure containing the detected barcodes to frame them from the plugin
            invalidateList(tmpBarcodePos);
            copyList(pos, &tmpBarcodePos);
            invalidateList(pos);

            state = INACTIVE;
        }

        pthread_mutex_unlock(&lock);
    }

    pthread_exit(NULL);
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_bar_code_reader_chain(GstPad *pad, GstObject *parent, GstBuffer *buf)
{
    GstBarCodeReader *filter = GST_BARCODEREADER(parent);

    GstMapInfo map;

    gst_buffer_map(buf, &map, GST_MAP_READ);

    GstCaps *caps = gst_pad_get_current_caps(pad);
    GstStructure *s = gst_caps_get_structure(caps, 0);

    gboolean res;
    gint width, height;
    // we need to get the final caps on the buffer to get the size
    res = gst_structure_get_int(s, "width", &width);
    res |= gst_structure_get_int(s, "height", &height);

    if (!res)
    {
        g_print("could not get snapshot dimension\n");
        exit(-1);
    }
    
    // Copy the dataframe for image processing. It allows to keep a stream video no jerky
    if (state == INACTIVE)
    {
        if (dataFrame == NULL)
        {
            dataFrame = (unsigned char *)malloc(sizeof(unsigned char) * width * height);
        }

        dataFrame = (unsigned char *)memcpy(dataFrame, map.data, width * height);

        if (dataFrame == NULL)
        {
            g_print("Unable to copy the frame's data\n");
            exit(-1);
        }
        else
        {
            frame.frame         = dataFrame;
            frame.width         = width;
            frame.height        = height;
            frame.nbMaxBarcode  = filter->numberOfBarcode;
            frame.compressImg   = filter->compressed;
            frame.positions     = &pos;
            frame.filter        = filter;

		if (returnType != -1)
		{
			state = ACTIVE;
            pthread_cond_signal(&cond);
		}
		else
		{
		    frame.filter->strPos = 0;
            // frame.filter->strCode = 0;
		}
		
        }
    }

    // Framing of detected barcodes from the plugin
    if (framing == 1 && filter->framing == TRUE)
    {
        BarcodePos *tmp = tmpBarcodePos;

        while (tmp != NULL && tmp->isValid)
        {
            drawLine(map.data, tmp->p1, tmp->p2, width, height);
            drawLine(map.data, tmp->p2, tmp->p3, width, height);
            drawLine(map.data, tmp->p3, tmp->p4, width, height);
            drawLine(map.data, tmp->p4, tmp->p1, width, height);

            tmp = tmp->next;
        }
    }

    gst_buffer_unmap(buf, &map);

    /* just push out the incoming buffer without touching it */
    return gst_pad_push(filter->srcpad, buf);
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
barcodereader_init(GstPlugin *barcodereader)
{
    /* debug category for fltering log messages
     *
     * exchange the string 'Template barcodereader' with your description
     */
    GST_DEBUG_CATEGORY_INIT(gst_bar_code_reader_debug, "barcodereader",
                            0, "Template barcodereader");

    return gst_element_register(barcodereader, "barcodereader", GST_RANK_NONE,
                                GST_TYPE_BARCODEREADER);
}

static void gst_barcodereader_finalize(GObject *object)
{
    pipelineRunning = FALSE;
    pthread_cond_signal(&cond);
    pthread_join(thread, NULL);

    freeList(&tmpBarcodePos);
    freeList(&pos);
    free(dataFrame);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstbarcodereader"
#endif

/* gstreamer looks for this structure to register barcodereaders
 *
 * exchange the string 'Template barcodereader' with your barcodereader description
 */

#ifdef HAVE_CONFIG_H

GST_PLUGIN_DEFINE(
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    barcodereader,
    "Template barcodereader",
    barcodereader_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/")
#else
GST_PLUGIN_DEFINE(
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    barcodereader,
    "Template barcodereader",
    barcodereader_init,
    "Unknown",
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/")
#endif
