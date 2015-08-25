#include <stdio.h>
#include <string.h>
#include "util.h"
#include "zipdefl.h"

#define TYPE_HRD_CRC	14
#define TYPE_HDR_CSZ	18
#define TYPE_HDR_RSZ	22
#define TYPE_FTR_CRC	16

int set_data(int val,int type,char *data)
{
  memcpy(data+type,&val,4);
  return 0;
}

int make_file_hdr(char *fl,char *extra,int esz,int meth,int crc,int rsz,int csz,char *buf,int *sz,bool cd=false)
{
  unsigned long x=0x04034b50;
  unsigned long none=0;
  if (cd)
    x=0x02014b50;
  memcpy(buf,&x,4);
  *sz=4;
  if (cd)
  {
    x=0x0317;
    memcpy(buf+*sz,&x,2); *sz+=2;
  }
  x=0x14;
  memcpy(buf+*sz,&x,2); *sz+=2;
  memcpy(buf+*sz,&none,2); *sz+=2;
  memcpy(buf+*sz,&meth,2); *sz+=2;
  memcpy(buf+*sz,&none,4); *sz+=4;
  memcpy(buf+*sz,&crc,4); *sz+=4;
  memcpy(buf+*sz,&csz,4); *sz+=4;
  memcpy(buf+*sz,&rsz,4); *sz+=4;
  x=strlen(fl);
  memcpy(buf+*sz,&x,2); *sz+=2;
  if (!extra) esz=0;
  memcpy(buf+*sz,&esz,2); *sz+=2;
  if (cd)
  {
    memcpy(buf+*sz,&none,4); *sz+=4;
	x=0x001;
	memcpy(buf+*sz,&x,2); *sz+=2;
	x=0x81A40000;
	memcpy(buf+*sz,&x,4); *sz+=4;
	memcpy(buf+*sz,&none,4); *sz+=4;
  }
  memcpy(buf+*sz,fl,strlen(fl)); *sz+=strlen(fl);
  if (extra && esz)
  {
    memcpy(buf+*sz,extra,esz); *sz+=esz;
  }
  return 0;
}

int make_header(char *fl,char *extra,int esz,int meth,int crc,int rsz,int csz,char *buf,int *sz)
{
  return make_file_hdr(fl,extra,esz,meth,crc,rsz,csz,buf,sz);
}

int make_footer(char *fl,char *extra,int esz,int meth,int crc,int rsz,int csz,int cd_ofs,char *comment,char *buf,int *sz)
{
  make_file_hdr(fl,extra,esz,meth,crc,rsz,csz,buf,sz,true);
  unsigned long x=0x06054b50;
  unsigned long none=0;
  int cdsz=*sz; 
  memcpy(buf+*sz,&x,4); *sz+=4;
  memcpy(buf+*sz,&none,4); *sz+=4;
  x=0x0001;
  memcpy(buf+*sz,&x,2); *sz+=2;
  memcpy(buf+*sz,&x,2); *sz+=2;
  memcpy(buf+*sz,&cdsz,4); *sz+=4;
  memcpy(buf+*sz,&cd_ofs,4); *sz+=4;
  x=0;
  if (comment)
	x=strlen(comment);
  memcpy(buf+*sz,&x,2); *sz+=2;
  if (x)
  {
    memcpy(buf+*sz,comment,x); *sz+=x;
  }
  return 0;
}


unsigned int normalizeCrc(char *h,int hsz,char *f,int fsz,char *buf,int bsz)
{
  char xbuf[4048];
  memcpy(xbuf,h,hsz);
  memcpy(xbuf+hsz,buf,bsz);
  memcpy(xbuf+hsz+bsz,f,fsz);
  int xsz=hsz+fsz+bsz;
  int ncrc=crc_back((unsigned char*)xbuf,xsz,0x107,0x66666666);
  printf("crcfix is %8.8X\n",ncrc);
  return ncrc;
}

int makezipzipzip(char *filename)
{
  FILE* f=fopen(filename,"w");
  if (!f)
  {
    printf("cant create file %s\n",filename);
    return 1;
  }
  char cbuf[4096]={0};
  char header[1024]={0};
  char footer[1024]={0};
  int hsz=0;
  int fsz=0;
  make_header("zipzip.zip","\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",13,8,0x66666666,0,0,header,&hsz);
  make_footer("zipzip.zip",NULL,0,8,0x66666666,0,0,0,
		"by Br. John Fn None AKA Ivan Palenov",
		footer,&fsz);
  int sz=0;
  makeDeflate(header,hsz,hsz-13,footer,fsz,cbuf,&sz);

  set_data(sz,TYPE_HDR_CSZ,header);
  set_data(sz+fsz+hsz,TYPE_HDR_RSZ,header);
  make_footer("zipzip.zip",NULL,0,8,0x66666666,sz+fsz+hsz,sz,sz+hsz,
		"by Br. John Fn None AKA Ivan Palenov",
		footer,&fsz);
  sz=0;
  makeDeflate(header,hsz,hsz-13,footer,fsz,cbuf,&sz,0x1FFFFFF1);  
  int crc=normalizeCrc(header,hsz,footer,fsz,cbuf,sz);
  sz=0;
  makeDeflate(header,hsz,hsz-13,footer,fsz,cbuf,&sz,crc);  

  fwrite(header,1,hsz,f);
  fwrite(cbuf,1,sz,f);
  fwrite(footer,1,fsz,f);
  fclose(f);
  return 0;
}

int main (int argc, char * const argv[]) {
    printf("zipzip_zip by Br. John fn None AKA Ivan Palenov Feb 2009 Ryazan.\n");
	makezipzipzip("../../test/zipzip.zip");
	return 0;
}
