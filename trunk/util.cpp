/*
 *  util.cpp
 *  zipzip_zip
 *
 *  Created by john on 05.02.09.
 *  Copyright 2009 YPA!!! Co. All rights reserved.
 *
 */

#include <string.h>
#include "util.h"

#define CRCPOLY 0xEDB88320
#define CRCINV 0x5B358FD3
#define INITXOR 0xFFFFFFFF
#define FINALXOR 0xFFFFFFFF
typedef unsigned int uint32;

void make_crc_table(uint32 *table){
	uint32 c;
	int n,k;
	for(n=0;n<256;n++){
		c=n;
		for(k=0;k<8;k++){
			if((c&1)!=0){
				c=CRCPOLY^(c>>1);
			}else{
			c=c>>1;
			}
		} 
		table[n]=c;
	} 
} 

int crc32_tabledriven(unsigned char  *buffer,
int length,
uint32 *crc_table)
{
int i;
uint32 crcreg=INITXOR;
for(i=0;i<length;++i){
crcreg=(crcreg>>8)^crc_table[((crcreg^buffer[i])&0xFF)];
}
return crcreg^FINALXOR;
}


int crc32(unsigned char* buf,int sz)
{
  uint32 tbl[256]={0};
  make_crc_table((uint32*)&tbl);
  return crc32_tabledriven(buf,sz,(uint32*)&tbl);
}

void  make_crc_rev_table(uint32 *table) { 
	uint32 c;
	int n,k;
	for (n=0;n<256;n++) { 
		c = n<<3*8; 
		for ( k=0;k<8;k++) { 
			if (( c & 0x80000000)!= 0) { 
				c = (( c^ CRCPOLY) <<1 ) | 1 ; 
			} else { 
				c <<= 1 ; 
			} 
		} 
		table[n] = c ; 
	} 
} 

int fix_crc_pos( unsigned char* buffer , 
int length, 
uint32 tcrcreg , 
int fix_pos, 
uint32* crc_table, 
uint32* crc_revtable ) 
{ 
	int i;
	fix_pos=((fix_pos%length)+length)%length;
	uint32 crcreg = INITXOR; 
	for (i=0;i<fix_pos;++i) { 
		crcreg=(crcreg>>8)^crc_table[((crcreg^buffer[i])&0xFF)]; 
	}
	for (i=0;i<4;++i) 
		buffer[fix_pos+i]=(crcreg>>i*8)&0xFF; 
	tcrcreg^=FINALXOR; 
	for (i=length-1;i>=fix_pos;--i) { 
		tcrcreg=(tcrcreg<<8)^crc_revtable[tcrcreg>>3*8]^buffer[i]; 
	}
	for(i=0;i<4;++i) 
		buffer[fix_pos+i]=(tcrcreg>>i*8)&0xFF;
	int res=0;
	memcpy(&res,buffer+fix_pos,4);
	return res;
} 



int crc_back(unsigned char *p,int len,int pos,int initial)
{
  uint32 tbl[256]={0};
  uint32 rtbl[256]={0};
  make_crc_table((uint32*)&tbl);
  make_crc_rev_table((uint32*)&rtbl);
  return fix_crc_pos(p,len,initial,pos,(uint32*)&tbl,(uint32*)&rtbl);
}
