/* This file is machine generated, don't edit it !*/

/* Structure information for class: java/util/zip/Inflater */

typedef struct java_util_zip_Inflater {
   java_objectheader header;
   s4 mode;
   s4 readAdler;
   s4 neededBits;
   s4 repLength;
   s4 repDist;
   s4 uncomprLen;
   s4 isLastBlock;
   s4 totalOut;
   s4 totalIn;
   s4 nowrap;
   struct java_util_zip_StreamManipulator* input;
   struct java_util_zip_OutputWindow* outputWindow;
   struct java_util_zip_InflaterDynHeader* dynHeader;
   struct java_util_zip_InflaterHuffmanTree* litlenTree;
   struct java_util_zip_InflaterHuffmanTree* distTree;
   struct java_util_zip_Adler32* adler;
} java_util_zip_Inflater;

