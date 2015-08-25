/*
 *  zipinfl.cpp
 *  zipzip_zip
 *
 *  Created by john on 05.02.09.
 *  Copyright 2009 YPA!! Co. All rights reserved.
 *
 */
#include <stdio.h> 
#include <string.h>
#include "zipdefl.h"

unsigned long xb=0,oxb;
int nb=0,onb;

#define ADDBTS(data,bts) add_bits(data,bts,buf,sz)

int save_bits()
{
  oxb=xb;
  onb=nb;
  return 0;
}

int load_bits()
{
  xb=oxb;
  nb=onb;
  return 0;
}

int add_bits(unsigned long data,int bts,char *buf,int *sz)
{
  unsigned long xd=data;
  if (bts==0)
  {
    if (nb==0) return 0;
    bts=8-nb;
	xd=0;
  }
  xd<<=nb;
  xb|=xd;
  nb+=bts;
  while(nb>=8)
  {
    buf[*sz]=xb & 0xFF;
	xb >>= 8;
	nb-=8;
	*sz+=1;
  }
  if (nb==0)
    xb=0;
  return 0;
}

#define TYPE_STORED 0
#define TYPE_FIXED 1
#define TYPE_DYNAMIC 2

int deflate_block(int type,char *buf,int *sz,bool last=false)
{
  ADDBTS((int)last,1);
  ADDBTS(type,2);
  if (type==TYPE_STORED)
     ADDBTS(0,0);
  return 0;
}

int deflate_store(char *data,unsigned int dsz,char *buf,int *sz)
{
  //int xdsz=(dsz & 0xFF) << 8 | (dsz >> 8);
  ADDBTS(dsz,16);
  ADDBTS((~dsz),16);
  for (int i=0;i<dsz;i++)
   ADDBTS(data[i],8);
  ADDBTS(0,0);
  return 0; 
}

int ROTB(int v,int sz)
{
  int x=0;
  for (int i=0;i<sz;i++)
  {
    x<<=1;
    x |= (v>>i) & 0x01;
  }
  return x;
}

#define RADDBTS(x,y) ADDBTS(ROTB(x,y),y)

int deflate_fixed_ch(char *data,unsigned int dsz,char *buf,int *sz)
{
  unsigned long dt;
  for (int i=0;i<dsz;i++)
  {
    unsigned char c=data[i];
	if (c<144)
	{
	  dt=0x18 + (c/2);
	  RADDBTS(dt,7);
	  ADDBTS(c%2,1);
	}else{
	 c-=144;
	 dt=0x64 + (c/4);
	 RADDBTS(dt,7);
	 RADDBTS(c%4,2);
	}
  }
  return 0;
}


int *cpcd;
int *cpex;
int *dist;
int *dex;

int make_fixed_tables()
{
  cpcd=new int[8+5*4];
  cpex=new int[8+5*4];
  int e=0;
  int cp=3;
  for (int i=0;i<8+5*4;i++)
  {
    if (i>7 && i%4==0)
	  e++;
	cpcd[i]=cp;
	cp+=(1<<e);
	cpex[i]=e;   
  }
  dist=new int[4+13*2];
  dex=new int[4+13*2];
  e=0;
  cp=1;
  for(int i=0;i<4+13*2;i++)
  {
    if (i>3 && i%2==0)
	  e++;
	dist[i]=cp;
	cp+=(1<<e);
	dex[i]=e;  
  }
  return 0;
}

int free_fixed_tables()
{
  delete[] cpcd;
  delete[] cpex;
  delete[] dist;
  delete[] dex;
  return 0;
}

int tbllook(int val,int *tbl,int* ex)
{
  int i=0;
  while(val>tbl[i]) i++;
  if (val==tbl[i]) return i;
  return i-1;
}

int deflate_fixed_cp(int len,int ofs,char *buf,int *sz)
{
    int i=tbllook(len,cpcd,cpex);
	RADDBTS(i+1,7);
	if (cpex[i]>0)
	  RADDBTS(len-cpcd[i],cpex[i]);
	i=tbllook(ofs,dist,dex);
	RADDBTS(i,5);
	if (dex[i]!=0)
	 ADDBTS(ofs-dist[i],dex[i]);
  return 0;
}

int deflate_fixed_end(char *buf,int *sz)
{
  ADDBTS(0,7);
  return 0;
}

int redef_buf(char **xbuf,int *xsz,char *buf,int *sz,bool save_hdr=true,bool fixend=true)
{
  deflate_block(TYPE_STORED,buf,sz);
  char tbuf[1024]={0};
  memcpy(tbuf,*xbuf,*xsz);
  int tsz=*xsz;
  char *p=buf+*sz-1;
  int hd=5;
  if (!save_hdr)
    hd=0;
  deflate_store(tbuf,tsz+hd,buf,sz);
  tsz+=hd;
  if (hd)
  {
  memcpy(buf+*sz-hd,p,hd);
  *xbuf=buf+*sz;
  *xsz=*sz;
  deflate_block(TYPE_FIXED,buf,sz);
  deflate_fixed_cp(tsz,tsz,buf,sz);
  if (fixend)
    deflate_fixed_end(buf,sz);
  *xsz=*sz-*xsz;
  }else{
    *xbuf=p;
	*xsz=tsz;
  }
  return 0;
}


int makeDeflate(char *header,int hsz,int hextra,char *footer,int fsz,char *buf, int *sz,int crc)
{
  make_fixed_tables();
  deflate_block(TYPE_STORED,buf,sz);
  deflate_store(header,hsz,buf,sz);
  char *mb=buf+*sz;
  int mbsz=*sz;
  deflate_block(TYPE_FIXED,buf,sz);
  deflate_fixed_ch(buf,5,buf,sz);
  deflate_fixed_cp(hsz,hsz+5,buf,sz);
  deflate_fixed_end(buf,sz);
  mbsz=*sz-mbsz+1;
  
  redef_buf(&mb,&mbsz,buf,sz,true,false);
//  redef_buf(&mb,&mbsz,buf,sz,true,false);
  int xsz=*sz;
  char *p=mb;//-mbsz;
  save_bits();
  mb=buf;
  int mbs2=*sz;
  deflate_fixed_cp(10,*sz-hextra-5-2,buf,sz);
  deflate_fixed_end(buf,sz);
  xsz=*sz-xsz+5+mbsz;
  load_bits();
  buf=mb;*sz=mbs2;
  deflate_fixed_cp(xsz,*sz-hextra-5-2,buf,sz);
  deflate_fixed_end(buf,sz);
  
  deflate_block(TYPE_STORED,buf,sz);
  char fb[1024]={0};
  memcpy(fb,footer,fsz);
  memcpy(fb+fsz,&crc,4);
  fsz+=4;
  deflate_store(fb,fsz+6,buf,sz);
  memcpy(buf+hextra+5,p,xsz);
  memcpy(header+hextra,p,xsz);
  p=buf+*sz-6;
  mbsz=*sz;
  save_bits();
  mb=buf;mbs2=*sz;
  deflate_block(TYPE_FIXED,buf,sz,true);
  deflate_fixed_cp(6,6,buf,sz);
  deflate_fixed_cp(fsz-4,12+fsz,buf,sz);
  deflate_fixed_end(buf,sz);
  ADDBTS(0,0);
  load_bits();
  mbsz=*sz-mbsz;
  buf=mb;*sz=mbs2;
  deflate_block(TYPE_FIXED,buf,sz,true);
  deflate_fixed_cp(mbsz,6,buf,sz);
  //fsz-=4; //crc
  deflate_fixed_cp(fsz-4,6+mbsz+fsz,buf,sz);
  deflate_fixed_end(buf,sz);
  ADDBTS(0,0);
  memcpy(p,buf+*sz-mbsz,mbsz);
  free_fixed_tables();
  return 0;
}
