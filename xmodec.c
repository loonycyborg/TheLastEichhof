/* Allegro port Copyright (C) 2014 Gavin Smith. */
/*-------------------------------------------------------*/
/*                                                       */
/*       C part of the VGA undocumented 320x240          */
/*       graphics pack.                                  */
/*                                                       */
/*          [c] copyright by Alpha-Helix 1992.           */
/*              written by Dany Schoch                   */
/*                                                       */
/*-------------------------------------------------------*/



#include <sys/stat.h>
#include <fcntl.h>

#include "xmode.h"


#define selectplane(x)		/*outport(SC_INDEX, (x << 8) + MAP_MASK)*/
#define selectmask(x)		/*outport(GC_INDEX, (x << 8) + BIT_MASK)*/


// Graphic window.

int   windowx0, windowy0;       	// Upper left corber.
int   windowx1, windowy1;               // Lower right corner.


BITMAP *pages[2];
BITMAP *full_pages[2];

/* Initialize s with the data read from the file.*/
void create_sprstrc (struct sprstrc **sp, unsigned char *data)
{
  struct sprstrc *s = malloc (sizeof (*s));
  s->xs   = data[0] + (data[1] << 8);
  s->ys   = data[2] + (data[3] << 8);
  s->maxn = data[4] + (data[5] << 8);
  s->data = data + 6;
  *sp = s;
}
#if 0
void load_sprstrc_from_file (struct sprstrc **s, void *database, char *file)
{
  struct sprstrc *new = malloc (sizeof (struct sprstrc));
  char *ptr = loadfile(database, file);
  create_sprstrc (new, ptr);
  *s = new;
}
#endif

/*------------------------------------------------------
Function: defstarfield

Description: Defines a STARFIELD, but doesn't animate it
	     yet. WARNING: (x, y) coordinates in
	     '*star' will get destroyed if used.
------------------------------------------------------*/

void defstarfield(int n, starstrc_ptr star)
{
   int   i;

   _sfield.n = n;
   _sfield.star = star;
   _sfield.go = FALSE;

/* Before I change this, I'll see how the stars are actually animated to
   see if it's needed (I assume it's something like star.x += s.speed). */
/*
   for (i = 0; i < n; i++) {
      star[i].x = star[i].y*BYTESPERLINE + star[i].x/4;
      star[i].speed *= BYTESPERLINE;
   }
*/

   _sfield.active = TRUE;

}


void killstarfield(void)
{

   _sfield.active = FALSE;

}



/*------------------------------------------------------
Function: defsprite

Description:
   Prepares a sprite to be used by the low level sprite routines.
   On successful completion 'defsprite' returns a sprite HANDLE.
   Don't lose this number or you will lose access to the sprite
   (and memory will be lost!).
   If no free entry could be found, 'defsprite' returns -1.

   args: *sprite -> pointer to sprite structure
	 align   -> 4: only x coords as a multiple of 4 are supported.
		    2: even x coords are supported.
------------------------------------------------------*/

int defsprite(void *sprite, unsigned flags)
{

  int            i, a, n, x, y, p;
   
  struct sprstrc *spr;
  struct lowspr  *s;
  int            align;
  int            xs, ys, maxn, size;
  int            plane;

  char *data;

  spr = (struct sprstrc *)sprite;

  /* SPR_ALIGN = 0x07. Following wil be somewhere between 4 and -3, although
     presumably between 4 and 0. */
  align = 4 - (flags & SPR_ALIGN);

  for (i = 0; i < MAXSPRITES; i++) {
     if (!_sprite[i].active) {		// Look for a free sprite slot.
        s = &_sprite[i];
        xs = spr->xs; ys = spr->ys; maxn = spr->maxn;
        s->xs = xs; s->ys = ys;
        s->maxn = maxn*2;
        if (flags & SPR_DOUBLE) s->nadd = 1; else s->nadd = 2;
        //xs = (xs+3+align) & 0xfffc;
        size = (xs/4) * ys;
        s->xsalign = xs;
        s->picsize = size;
        s->seqsize = size*maxn*(align/2);
        s->fullsize = size*maxn*(align/2 + 1);

        s->bmp = malloc (maxn * sizeof (BITMAP *));
        s->bmp_silhouette = malloc (maxn * sizeof (BITMAP *));

        //for (a = 0; a <= align; a+=2) {
           data = spr->data;
           for (n = 0; n < maxn; n++) {
             if (   !(s->bmp[n]            = create_bitmap(xs, ys))
                 || !(s->bmp_silhouette[n] = create_bitmap(xs, ys))) {
                exit(1);
             }
             for (y = 0; y < ys; y++) {
                for (x = 0; x < xs; x+=1) {
                  putpixel (s->bmp[n], x, y, data[x]);
                  if (data[x])
                    /* FIXME: Find an unused palette entry */
                    putpixel (s->bmp_silhouette[n], x, y, FLASH_COLOUR);
                    //putpixel (s->bmp_silhouette[n], x, y, data[x]);
                  else
                    putpixel (s->bmp_silhouette[n], x, y, 0);
                }
                data += spr->xs;
             }
            }
        //}

        s->active = TRUE;
        return i;
     }
  }

  return -1;

}


void killsprite(int sprite)
{
   char *scr1;
   char *scr2;
   int  i;

   if (_sprite[sprite].active) {
      _sprite[sprite].active = FALSE;

      for (i = 0; i < _sprite[sprite].maxn / 2; i++) {
        destroy_bitmap (_sprite[sprite].bmp[i]); 
        destroy_bitmap (_sprite[sprite].bmp_silhouette[i]); 
      }

   }
}


void killallsprites(void)
{
   int   i;

   for (i = 0; i < MAXSPRITES; i++) {
      if (_sprite[i].active) {
	 free(_sprite[i].mask);
	 _sprite[i].active = FALSE;
      }
   }
}


void setpage(int p)
{
   page = p;
}


void initxmode(void)
{
   int   i;

   setxmode();
   setstandardpalette();
   windowx0 = XMIN; windowy0 = YMIN;
   windowx1 = XMAX; windowy1 = YMAX;
   for (i = 0; i < MAXSPRITES; i++) {
      _sprite[i].active = FALSE;
   }
   killstarfield();
   killallobjects();
}

void shutxmode(void)
{

   killallobjects();
   killallsprites();
   killstarfield();

}

// VGA color and palette functions.

static PALETTE standardpal = {
{0x00,0x00,0x00},
{0x00,0x00,0x2A},
{0x00,0x2A,0x00},
{0x00,0x2A,0x2A},
{0x2A,0x00,0x00},
{0x2A,0x00,0x2A},
{0x2A,0x15,0x00},
{0x2A,0x2A,0x2A},
{0x15,0x15,0x15},
{0x15,0x15,0x3F},
{0x15,0x3F,0x15},
{0x15,0x3F,0x3F},
{0x3F,0x15,0x15},
{0x3F,0x15,0x3F},
{0x3F,0x3F,0x15},
{0x3F,0x3F,0x3F},
{0x3B,0x3B,0x3B},
{0x37,0x37,0x37},
{0x34,0x34,0x34},
{0x30,0x30,0x30},
{0x2D,0x2D,0x2D},
{0x2A,0x2A,0x2A},
{0x26,0x26,0x26},
{0x23,0x23,0x23},
{0x1F,0x1F,0x1F},
{0x1C,0x1C,0x1C},
{0x19,0x19,0x19},
{0x15,0x15,0x15},
{0x12,0x12,0x12},
{0x0E,0x0E,0x0E},
{0x0B,0x0B,0x0B},
{0x08,0x08,0x08},
{0x3F,0x00,0x00},
{0x3B,0x00,0x00},
{0x38,0x00,0x00},
{0x35,0x00,0x00},
{0x32,0x00,0x00},
{0x2F,0x00,0x00},
{0x2C,0x00,0x00},
{0x29,0x00,0x00},
{0x26,0x00,0x00},
{0x22,0x00,0x00},
{0x1F,0x00,0x00},
{0x1C,0x00,0x00},
{0x19,0x00,0x00},
{0x16,0x00,0x00},
{0x13,0x00,0x00},
{0x10,0x00,0x00},
{0x3F,0x36,0x36},
{0x3F,0x2E,0x2E},
{0x3F,0x27,0x27},
{0x3F,0x1F,0x1F},
{0x3F,0x17,0x17},
{0x3F,0x10,0x10},
{0x3F,0x08,0x08},
{0x3F,0x00,0x00},
{0x3F,0x2A,0x17},
{0x3F,0x26,0x10},
{0x3F,0x22,0x08},
{0x3F,0x1E,0x00},
{0x39,0x1B,0x00},
{0x33,0x18,0x00},
{0x2D,0x15,0x00},
{0x27,0x13,0x00},
{0x3F,0x3F,0x36},
{0x3F,0x3F,0x2E},
{0x3F,0x3F,0x27},
{0x3F,0x3F,0x1F},
{0x3F,0x3E,0x17},
{0x3F,0x3D,0x10},
{0x3F,0x3D,0x08},
{0x3F,0x3D,0x00},
{0x39,0x36,0x00},
{0x33,0x31,0x00},
{0x2D,0x2B,0x00},
{0x27,0x27,0x00},
{0x21,0x21,0x00},
{0x1C,0x1B,0x00},
{0x16,0x15,0x00},
{0x10,0x10,0x00},
{0x34,0x3F,0x17},
{0x31,0x3F,0x10},
{0x2D,0x3F,0x08},
{0x28,0x3F,0x00},
{0x24,0x39,0x00},
{0x20,0x33,0x00},
{0x1D,0x2D,0x00},
{0x18,0x27,0x00},
{0x36,0x3F,0x36},
{0x2F,0x3F,0x2E},
{0x27,0x3F,0x27},
{0x20,0x3F,0x1F},
{0x18,0x3F,0x17},
{0x10,0x3F,0x10},
{0x08,0x3F,0x08},
{0x00,0x3F,0x00},
{0x00,0x3F,0x00},
{0x00,0x3B,0x00},
{0x00,0x38,0x00},
{0x00,0x35,0x00},
{0x01,0x32,0x00},
{0x01,0x2F,0x00},
{0x01,0x2C,0x00},
{0x01,0x29,0x00},
{0x01,0x26,0x00},
{0x01,0x22,0x00},
{0x01,0x1F,0x00},
{0x01,0x1C,0x00},
{0x01,0x19,0x00},
{0x01,0x16,0x00},
{0x01,0x13,0x00},
{0x01,0x10,0x00},
{0x36,0x3F,0x3F},
{0x2E,0x3F,0x3F},
{0x27,0x3F,0x3F},
{0x1F,0x3F,0x3E},
{0x17,0x3F,0x3F},
{0x10,0x3F,0x3F},
{0x08,0x3F,0x3F},
{0x00,0x3F,0x3F},
{0x00,0x39,0x39},
{0x00,0x33,0x33},
{0x00,0x2D,0x2D},
{0x00,0x27,0x27},
{0x00,0x21,0x21},
{0x00,0x1C,0x1C},
{0x00,0x16,0x16},
{0x00,0x10,0x10},
{0x17,0x2F,0x3F},
{0x10,0x2C,0x3F},
{0x08,0x2A,0x3F},
{0x00,0x27,0x3F},
{0x00,0x23,0x39},
{0x00,0x1F,0x33},
{0x00,0x1B,0x2D},
{0x00,0x17,0x27},
{0x36,0x36,0x3F},
{0x2E,0x2F,0x3F},
{0x27,0x27,0x3F},
{0x1F,0x20,0x3F},
{0x17,0x18,0x3F},
{0x10,0x10,0x3F},
{0x08,0x09,0x3F},
{0x00,0x01,0x3F},
{0x00,0x00,0x3F},
{0x00,0x00,0x3B},
{0x00,0x00,0x38},
{0x00,0x00,0x35},
{0x00,0x00,0x32},
{0x00,0x00,0x2F},
{0x00,0x00,0x2C},
{0x00,0x00,0x29},
{0x00,0x00,0x26},
{0x00,0x00,0x22},
{0x00,0x00,0x1F},
{0x00,0x00,0x1C},
{0x00,0x00,0x19},
{0x00,0x00,0x16},
{0x00,0x00,0x13},
{0x00,0x00,0x10},
{0x3C,0x36,0x3F},
{0x39,0x2E,0x3F},
{0x36,0x27,0x3F},
{0x34,0x1F,0x3F},
{0x32,0x17,0x3F},
{0x2F,0x10,0x3F},
{0x2D,0x08,0x3F},
{0x2A,0x00,0x3F},
{0x26,0x00,0x39},
{0x20,0x00,0x33},
{0x1D,0x00,0x2D},
{0x18,0x00,0x27},
{0x14,0x00,0x21},
{0x11,0x00,0x1C},
{0x0D,0x00,0x16},
{0x0A,0x00,0x10},
{0x3F,0x36,0x3F},
{0x3F,0x2E,0x3F},
{0x3F,0x27,0x3F},
{0x3F,0x1F,0x3F},
{0x3F,0x17,0x3F},
{0x3F,0x10,0x3F},
{0x3F,0x08,0x3F},
{0x3F,0x00,0x3F},
{0x38,0x00,0x39},
{0x32,0x00,0x33},
{0x2D,0x00,0x2D},
{0x27,0x00,0x27},
{0x21,0x00,0x21},
{0x1B,0x00,0x1C},
{0x16,0x00,0x16},
{0x10,0x00,0x10},
{0x3F,0x3A,0x37},
{0x3F,0x38,0x34},
{0x3F,0x36,0x31},
{0x3F,0x35,0x2F},
{0x3F,0x33,0x2C},
{0x3F,0x31,0x29},
{0x3F,0x2F,0x27},
{0x3F,0x2E,0x24},
{0x3F,0x2C,0x20},
{0x3F,0x29,0x1C},
{0x3F,0x27,0x18},
{0x3C,0x25,0x17},
{0x3A,0x23,0x16},
{0x37,0x22,0x15},
{0x34,0x20,0x14},
{0x32,0x1F,0x13},
{0x2F,0x1E,0x12},
{0x2D,0x1C,0x11},
{0x2A,0x1A,0x10},
{0x28,0x19,0x0F},
{0x27,0x18,0x0E},
{0x24,0x17,0x0D},
{0x22,0x16,0x0C},
{0x20,0x14,0x0B},
{0x1D,0x13,0x0A},
{0x1B,0x12,0x09},
{0x17,0x10,0x08},
{0x15,0x0F,0x07},
{0x12,0x0E,0x06},
{0x10,0x0C,0x06},
{0x0E,0x0B,0x05},
{0x0A,0x08,0x03},
{0x00,0x00,0x00},
{0x00,0x00,0x00},
{0x00,0x00,0x00},
{0x00,0x00,0x00},
{0x00,0x00,0x00},
{0x00,0x00,0x00},
{0x00,0x00,0x00},
{0x00,0x00,0x00},
{0x31,0x0A,0x0A},
{0x31,0x13,0x0A},
{0x31,0x1D,0x0A},
{0x31,0x27,0x0A},
{0x31,0x31,0x0A},
{0x27,0x31,0x0A},
{0x1D,0x31,0x0A},
{0x13,0x31,0x0A},
{0x0A,0x31,0x0C},
{0x0A,0x31,0x17},
{0x0A,0x31,0x22},
{0x0A,0x31,0x2D},
{0x0A,0x2A,0x31},
{0x0A,0x1F,0x31},
{0x0A,0x14,0x31},
{0x0B,0x0A,0x31},
{0x16,0x0A,0x31},
{0x21,0x0A,0x31},
{0x2C,0x0A,0x31},
{0x31,0x0A,0x2B},
{0x31,0x0A,0x20},
{0x31,0x0A,0x15},
{0x31,0x0A,0x0A},
{0x3F,0x3F,0x3F}
};

void setpalette (PALETTE ptr)
{
   set_palette (ptr);
}

void copycolor (int c1, int c2)
{
   RGB rgb;

   rgb.r = palette[c2].r;
   rgb.g = palette[c2].g;
   rgb.b = palette[c2].b;

   //vsync();
   //set_color (c, &palette[c]);
   set_color (c1, &rgb);
}

/* Set one color of the palette */
void setcolor(int c, int r, int g, int b)
{
   RGB rgb;

   rgb.r = r;
   rgb.g = g;
   rgb.b = b;

   set_color (c, &rgb);
}

void setstandardpalette(void)
{
   memcpy(palette, standardpal, sizeof(PALETTE));
   setpalette(palette);
}

void setvanillapalette(int c)
{
  int  i;

  PALETTE tmp_palette;
  for (i = 0; i < 256; i++) {
    RGB tmp;
    tmp.r = tmp.g = tmp.b = i;
    tmp_palette[i] = tmp;
  }
  setpalette (tmp_palette);
}


int cyclepalette(int c1, int c2, int pos)
{
   int   i;
   int   n;

   PALETTE cycled_palette;

   memmove (cycled_palette, palette, sizeof (PALETTE));
   if ((n = --pos + c1) < c1) pos = c2 - c1;
   for (i = c2; i >= c1; i--) {
      if (n < c1) n = c2;
      cycled_palette[i] = palette[n];
      n--;
   }

   set_palette (cycled_palette);

   return pos;
}


void glowto(int r0, int g0, int b0)
{
   int  i, j;
   int  r, g, b;

   for (i = 0; i < 63; i+=3) {
      for (j = 0; j < 256; j++) {
	 r = (r0*i) / 63;
	 g = (g0*i) / 63;
	 b = (b0*i) / 63;
	 setcolor(j, r, g, b);
      }
      retrace();
   }
}


void clearscreen(void)
{
   clear_bitmap (full_pages[0]);
   clear_bitmap (full_pages[1]);
   showpage(page);
}

void clearregion(int y, int n)
{

#if 0
   selectplane(0x0f);
   selectmask(0xff);
   _fmemset(MK_FP(base, y * BYTESPERLINE), backgrndcolor, n * BYTESPERLINE);
   _fmemset(MK_FP(base, PAGESIZE + y * BYTESPERLINE), backgrndcolor, n * BYTESPERLINE);
#endif
}

void showpcx256(char *pic, int line)
{
   int      row, col;
   int      height;
   char     data, count;
   unsigned i;
   char     *pos;

   uintptr_t write_address;

   height = ((short int *)pic)[5] + 1;	// Get picture height.
   pos = pic + 128;		// Skip Header.
   
   bmp_select(full_pages[page]);
   col = 0;
   row = line;
   if (row >= 0 && row < full_pages[0]->h)
     write_address = bmp_write_line(full_pages[page], row);

// Decode picture.
   while (row < line + height) {
      count = *pos; pos++;
      if ((count & 0xc0) == 0xc0) {
	 count = count & 0x3f;
	 data = *pos; pos++;
      } else {
	 data = count; count = 1;
      }

      for (i = 0; i < count; i++) {
         if (row >= 0 && row < full_pages[0]->h)
           bmp_write8 (write_address + col, data);
         col++;
         if (col >= full_pages[0]->w) {
           col = 0;
           row++;
           if (row >= 0 && row < full_pages[0]->h)
             write_address = bmp_write_line(full_pages[page], row);
         }
      }
   }

   bmp_unwrite_line (full_pages[page]);
// Load palette.
   pos++;

   for (i = 0; i < 255; i++, pos += 3) {
     RGB rgb;

     rgb.r = (unsigned char) pos[0] >> 2;
     rgb.g = (unsigned char) pos[1] >> 2;
     rgb.b = (unsigned char) pos[2] >> 2;
      
     palette[i] = rgb;
   }

   /* Don't set the palette immediately - we may want to glow in from
      black. */
   //set_palette(palette);
}


