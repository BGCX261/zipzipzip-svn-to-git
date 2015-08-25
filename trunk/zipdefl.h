/*
 *  zipinfl.h
 *  zipzip_zip
 *
 *  Created by john on 05.02.09.
 *  Copyright 2009 YPA!!! Co. All rights reserved.
 *
 */


int makeDeflate(char *header,int hsz,int hextra,char *footer,int fsz,char *buf, int *sz,int crc=0);

