/*
MyCrobe-WASP - MCU-based externally-triggered auditory-stim presenter
   Copyright (C) 2011 Barkin Ilhan

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If no:t, see <https://www.gnu.org/licenses/>.

 Contact info:
 E-Mail:  barkin@unrlabs.org
 Website: http://icon.unrlabs.org/staff/barkin/
 Repo:    https://github.com/4e0n/
*/

// configuration bit settings, Fcy=80MHz, Fpb=80MHz
#pragma config POSCMOD=XT, FNOSC=PRIPLL
#pragma config FPLLIDIV=DIV_2, FPLLMUL=MUL_20, FPLLODIV=DIV_1
#pragma config FPBDIV=DIV_1, FWDTEN=OFF, CP=OFF, BWP=OFF

#include <plib.h>
#include <MMB.h>

#include <MDD File System/FSIO.h>
#include <LCD.h>
#include "CODECAudio.h"

#define  RIFF_DWORD  0x46464952UL
#define  WAVE_DWORD  0x45564157UL
#define  DATA_DWORD  0x61746164UL
#define  FMT_DWORD   0x20746d66UL
#define  WAV_DWORD   0x00564157UL

typedef struct { int ckID,ckSize,ckType; } chunk;

typedef struct { // format chunk
 unsigned short subtype,channels; // compression code, # of channels (1=mono,2=stereo)
 unsigned int srate,bps;          // sample rate in Hz, bytes per second
 unsigned short bpsample,bitpsample,extra; // bytes per sample (4=16bit stereo), bir per sample, extra format bytes
} WAVE_fmt;

void main() {
 SearchRec sr;
 unsigned char sa=ATTR_READ_ONLY|ATTR_ARCHIVE;

 char fn_str[10]; int wave_no;
// TRISB=TRISB|0xf;

 MMBInit();
 initLCD();
 putsLCD(" Mycrobe-WASP\n");
 putsLCD(" v0.1a\n");
 putsLCD(" (c)/GPL 2011 by\n");
 putsLCD(" Barkin Ilhan\n");
 MMBFadeIn(2000);

 while (!FSInit()) DelayMs(100);

 while(1) {
  //putsLCD( "Insert card...");

  wave_no=1;
  if (wave_no<10) sprintf(fn_str,"A00%d*.WAV",wave_no);
  else if (wave_no<100) sprintf(fn_str,"A0%d*.WAV",wave_no);
  else if (wave_no<1000) sprintf(fn_str,"A%d*.WAV",wave_no);

  if (FindFirst(fn_str,sa,&sr)) {
   clrLCD();
   putsLCD("File not found");
  } else do {
   clrLCD();
   putsLCD("Playing:");
   putsLCD(sr.filename);
   playWAV( sr.filename);
   if (MMBReadKey()) MMBGetKey(); // repeatedly play all songs

   wave_no++;
   if (wave_no<10) sprintf(fn_str,"A00%d*.WAV",wave_no);
   else if (wave_no<100) sprintf(fn_str,"A0%d*.WAV",wave_no);
   else if (wave_no<1000) sprintf(fn_str,"A%d*.WAV",wave_no);
  } while (!FindFirst(fn_str,sa,&sr));

  // ask to replace the card
  clrLCD();
  putsLCD("Trials finished!");

  // wait for SD card to be removed
  while(FSInit()) DelayMs(100); // !mount() )
  clrLCD();
 }
}

int playWAV(char *name) {
 chunk ck; WAVE_fmt wav; FSFILE *fp;
 short buffer[OUTBUF_SIZE];
 unsigned int lc,r,bpchan;
 char s[16];

 unsigned short trig,trig1,trig2; int delay;

 r=FALSE;

 // 1. open the file
 if ((fp=FSfopen(name,"r"))==NULL) return r; // failed to open

 // 2. verify it is a RIFF - WAVE formatted file
 FSfread((void*)&ck,sizeof(chunk),1,fp);
 if ((ck.ckID!=RIFF_DWORD)||(ck.ckType!=WAVE_DWORD)) goto Exit; // check that file type is correct

 // 3. look for the chunk containing the wave format data
 FSfread((void*)&ck,1,8,fp);
 if (ck.ckID!=FMT_DWORD) goto Exit;

 // 4. get the WAVE_FMT struct
 FSfread((void*)&wav,1,sizeof(WAVE_fmt),fp);
 ACfg.channels=wav.channels;
 bpchan=wav.bpsample/wav.channels;
 sprintf(s,"\nRate: %d",wav.srate); LCDPuts(s); // print some additional info
 sprintf(s,"\nChan: %d",wav.channels); LCDPuts(s);
 sprintf(s,"\nBits: %d",wav.bitpsample); LCDPuts(s);

 // 5. skip extra format bytes
 FSfseek(fp,ck.ckSize-sizeof(WAVE_fmt),SEEK_CUR);

 // 6. search for the "data" chunk
 while(1) { // read next chunk
  if (FSfread((void*)&ck,1,8,fp)!=8) goto Exit;
  if (ck.ckID!=DATA_DWORD) FSfseek(fp,ck.ckSize,SEEK_CUR); else break;
 }

 // 7. find the data size
 lc=ck.ckSize;

 // 8. reduce sample rate to sub of 44100Hz
 ACfg.rate=wav.srate;
 ACfg.skip=1;
 while (ACfg.rate<44100) {
  ACfg.rate<<=1; // multiply sample rate by two
  ACfg.skip<<=1; // multiply skip by two
 }
 ACfg.skip--; // 0,1,3,7...

 // 9. define a buffer to transfer data to CODEC
 ACfg.dbuf=buffer;

 // 10. configure codec/Player state machine and start
 initAudio();
 //setVolume(100);

// DelayMs(1000); // WAIT FOR TRIGGER
 trig=trig1=trig2=0;
 while (!trig) {
  trig1=PORTE&0x1;
  for(delay=0;delay<(GetSystemClock()/5/80000ul/10);delay++); // Approx. 100usec delay
  trig2=PORTE&0x1;
  trig=trig1&trig2;
 }

 startAudio(0);

 // 11. keep feeding the buffers in the playing loop as long as entire buffers can be filled
 while (lc>0) { // check user input to stop playback
  // if any button pressed
//  if (MMBReadKey()) { lc=0; break; } // playback completed

  // refill buffer
  r=FSfread((void*)buffer,1,OUTBUF_SIZE*bpchan,fp);
  lc -= r; // decrement byte count
  if (bpchan==1) { // extend samples to 16 bit signed
   unsigned char *p=(unsigned char*)&buffer[r/2-1]+1;
   int i;
   for( i=r-1;i>=0;i--) buffer[i]=((*p--)<<8)^0x8000;
  }
  ACfg.dsamples=r/bpchan;

  while (ACfg.dsamples >0); // wait for data to be consumed
  // <<put here additional tasks>>
 } // while wav data available

 // 16. stop playback
 haltAudio();

 // return with success
 r=TRUE;

Exit:
 // 17. close the file
 FSfclose(fp);

 // 18. return
 return r;

} // playWAV
