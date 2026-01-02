// GameBoy PPU emulation
#include "pch.h"

unsigned lcd_WYline;
uint8_t linebuffer[192]; // showed from 8-th byte
unsigned mainpal[64];  // 0-3 BG palette 4-7,8-11 - sprite palettes

void tilecache_init(void);

void ppu_init() 
{
	tilecache_init();
	sdl_win_init(160, 144);
	lcd_WYline=-1;
}

void ppu_shutdown()
{
	sdl_win_shutdown();
}

// **********************************************************************

typedef struct {
	uint8_t    y, x, n, a;
} SPR;

//static SPR *spr = (SPR *)hram;
#define spr ((SPR *)hram)
// one pointer less :) 

#define SPR_PRI     0x80
#define SPR_FLIPY   0x40
#define SPR_FLIPX   0x20
#define SPR_PAL     0x10

SPR used_spr[40];
/* We must be able to gather 40 sprites first, then sort,then leave 10
since sprites are as big as "int", there is no point in storing indexes
*/

unsigned num_sprites = 0;


//tilecache is 256-color based
// dir can take values: 1-left 2-right
#define DIR_NORMAL 0x20
#define DIR_XMIRROR 0x40


uint8_t tilecachedata[0x2000*4*2];
uint8_t tilecache[512];
uint32_t bitxlat_t[16],bitxlat2_t[16],bitxlatM_t[16],bitxlat2M_t[16];

void tilecache_init(void) {
	unsigned i,tmp;
	memset(tilecache,0,sizeof(tilecache));
	memset(tilecachedata,0,sizeof(tilecachedata));
	for(i=0;i<16;i++) {
		bitxlat_t[i] = tmp = ((i<<24)+(i<<15)+(i<<6)+(i>>3))&0x01010101; // Higher bits first
		bitxlat2_t[i] = tmp << 1;
		bitxlatM_t[i] = tmp = (i+(i<<7)+(i<<14)+(i<<21))&0x01010101;  // lower bits first
		bitxlat2M_t[i] = tmp << 1;
	}
}

static uint8_t* getcell(unsigned celln,unsigned dir) {
	uint8_t*dest=tilecachedata+((dir&0x40)<<9)+(celln<<6),*rp;
	uint32_t*wp;
	unsigned b0,b1;
	unsigned i;
	if(!(tilecache[celln]&dir)) {
		tilecache[celln]|=dir;
		wp = (uint32_t*)dest;
		rp = vram+(celln<<4);
		
		if(dir&DIR_XMIRROR) 
			for(i=0;i<8;i++) {
				wp[0]=bitxlatM_t[(b0=rp[0])&0xF]|bitxlat2M_t[(b1=rp[1])&0xF];
				wp[1]=bitxlatM_t[b0>>4]|bitxlat2M_t[b1>>4];
				wp+=2;
				rp+=2;
			}
		else
			for(i=0;i<8;i++) {
				wp[0]=bitxlat_t[(b0=rp[0])>>4]|bitxlat2_t[(b1=rp[1])>>4];
				wp[1]=bitxlat_t[b0&0xF]|bitxlat2_t[b1&0xF];
				wp+=2;
				rp+=2;
			}
	}
	return dest;
}


void ppu_enumsprites()
{
	unsigned h = ((R_LCDC & 4)<<1)+8;// ? (16) : (8); sprite height
	unsigned line = (unsigned)(R_LY)+16;
	unsigned i,j,ntosort;
	num_sprites = 0;

	
	if(R_LCDC & 2)
	{

		/* link sprite list */
		for(i=0; i<40; i++)
			if((line-(unsigned)spr[i].y)<h)
				used_spr[num_sprites++] = spr[i];

		if(num_sprites<2) return;
		ntosort=num_sprites-1;
		if(ntosort>10) ntosort = 10;
		for(i=0;i<ntosort;i++) {
			for(j=i+1;j<num_sprites;j++)
				if(used_spr[i].x>used_spr[j].x) {
					SPR tmp = used_spr[i];
					used_spr[i] = used_spr[j];
					used_spr[j] = tmp;
				}
		}
		if(num_sprites>10) num_sprites = 10;
	}
}

// **********************************************************************


void ppu_refreshline(void) {

	signed char *tilemapptr;
	uint8_t*writeptr,*tmpptr;
	unsigned tileofs,tilepage,tilemapx;
	unsigned X,Y,LY,WX,WY,i,j;
	//unsigned tmp0,tmp1;
	unsigned spriteh = ((R_LCDC & 4)<<1)+7;// sprite height
	unsigned sprtilemask=~((R_LCDC & 4)>>2);
	uint8_t spr_pal,tmp;

	//memset(tilecache,0,sizeof(tilecache));// TODO: SLOW!!! for debug only!!

	benchmark_gfx-=GetTimer();

	LY = R_LY;
	WX=167;

	/*Check for window*/
	if((R_LCDC & 0x20)  /*WIN enable*/
		&& ((WY=(unsigned)R_WY)<=LY)){
		lcd_WYline++;	// TODO: maybe incremented only if R_WX<167?
		if(R_WX<167 /*allowed position*/ )
			WX = R_WX;
	}

	tilepage = ~(((unsigned)R_LCDC&0x10)<<4);
	/*Show background*/
	if((R_LCDC & 1)/*BG enable*/ && (WX>7)/*not covered by WIN*/) {
		X = R_SCX;
		Y = LY + R_SCY;
		writeptr = linebuffer+8-(X&7);
		tilemapptr = (signed char *)(vram+0x1800+(((unsigned)R_LCDC & 8)<<7) // pointer to tile line
					 +((Y&(31*8))<<2));
		tilemapx = (X>>3)&31;
		tileofs = (Y&7)<<3;
		for(i=((X+WX)>>3)-(X>>3);i>0;i--) {
			tmpptr = getcell(
				(((signed int)tilemapptr[tilemapx] +256)&tilepage)
				,DIR_NORMAL)+tileofs;
			((unsigned *)writeptr)[0] = ((unsigned *)tmpptr)[0];
			((unsigned *)writeptr)[1] = ((unsigned *)tmpptr)[1]; // instead of memcpy
			tilemapx=(tilemapx+1)&31;
			writeptr+=8;
		}
	}

	//w_priority=(LCDC&2)
	/*Show window*/
	if(WX<167) {
		//X = 0;
		Y = lcd_WYline; // internal WIN counter used instead of LY-WY;
		writeptr = linebuffer+1+WX;
		tilemapptr = (signed char *)(vram+0x1800+(((unsigned)R_LCDC & 0x40)<<4) // pointer to tile line
					 +((Y&(31*8))<<2));
		tileofs = (Y&7)<<3;

		for(i=((166+8)-WX)>>3;i>0;i--) {
			tmpptr = getcell(
				(((signed int)*(tilemapptr++) +256)&tilepage)
				,DIR_NORMAL)+tileofs;
			((unsigned *)writeptr)[0] = ((unsigned *)tmpptr)[0];
			((unsigned *)writeptr)[1] = ((unsigned *)tmpptr)[1]; // instead of memcpy
			writeptr+=8;
		}

	}
	
	//num_sprites=0;
	for(i=0;i<num_sprites;i++) if((unsigned)((X=used_spr[i].x)-1) < (unsigned)167) {
		X=used_spr[i].x;
		Y=(LY+16-used_spr[i].y);
		if(used_spr[i].a&SPR_FLIPY) Y^=spriteh; // Y flip (bit 6)
		writeptr = linebuffer+X;
		tmpptr=getcell(((unsigned)used_spr[i].n&sprtilemask)+(Y>>3),
			(used_spr[i].a&SPR_FLIPX)+DIR_NORMAL)+((Y&7)<<3);
		spr_pal=((used_spr[i].a&0x10)>>2)+4; 
		if((used_spr[i].a & SPR_PRI)) {  // below everything
			for(j=0;j<8;j++) if(tmpptr[j]) {
				tmp=writeptr[j];
				if(!tmp) tmp = tmpptr[j]+spr_pal;
				writeptr[j]=tmp|0x80; // update pixel priority
			}
		} else {// above background, below previous sprites
			spr_pal|=0x80;
			for(j=0;j<8;j++)
				if(tmpptr[j] && ((signed char)writeptr[j]>=0))
					writeptr[j]=tmpptr[j]+spr_pal;
		}
	}
	lcd_refresh(LY);
	benchmark_gfx+=GetTimer();
}


void ppu_vsync()
{
	static int first = 1;
	static int frame = 1;
	char title[64];
	static unsigned oldtime[8][4],timepos=0;
	unsigned time,i,bm_g,bm_o,bm_s;
	lcd_WYline = -1; // reset internal WY line counter
	if(first)
	{
		TimerInit();
		Timer();
		first = 0;
	}

	time=GetTimer();
	if(lcd_fpslimit) while((time=GetTimer()) < 1000000/60) ;
	else while ((time = GetTimer()) < 1000000/1000);		// Limited to 1000 FPS
	Timer();
	oldtime[timepos][0]=time;
	oldtime[timepos][1]=benchmark_gfx;
	oldtime[timepos][2]=benchmark_sound;
	benchmark_gfx = benchmark_sound = 0;
	timepos=(timepos+1)&7;

	sdl_win_blit();
	sdl_win_update();

	/* Frames Per Second */
	frame++;

	if(!timepos)
	{
		time=bm_o=bm_s=bm_g=0;
		for(i=0;i<8;i++) {
				time+=oldtime[i][0];
				bm_g+=oldtime[i][1];
				bm_s+=oldtime[i][2];
		}
		time/=8;
		bm_g/=8;
		bm_s/=8;
		bm_o=time-bm_s-bm_g;
#if BENCHMARK
		if(time)
		    sprintf(title, "GameBoy - %15s [%u fps]O:%02dG:%02dS:%02d", cart.title,
			1000000/time,(bm_o*100)/time,(bm_g*100)/time,(bm_s*100)/time);
		else
#endif
			sprintf(title, "GameBoy - %s [%u fps]", cart.title, 1000000/time);
		sdl_win_update_title(title);
	}
}
