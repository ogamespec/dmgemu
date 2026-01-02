/* GameBoy emu control */
#include "pch.h"

int skip_introm = 0;
unsigned benchmark_sound, benchmark_gfx;
int sound_enabled = 1;

/* run on emu start */
void gb_init()
{
	// log_init("gbemu.log");
	mem_init();
	sm83_init();
	pad_init();
	ppu_init();
	apu_init(44100);
	if (skip_introm) {
		R_BANK = 1;
		R_PC = 0x100;
	}
	__log("init OK.");
}

/* run on emu shutdown */
void gb_shutdown()
{
	mem_shutdown();
	pad_shutdown();
	apu_shutdown();
	ppu_shutdown();
	log_shutdown();
}

// **********************************************************************

uint32_t gb_clk;
uint32_t gb_eventclk; // timer before next possible interrupt/LCD mode change
uint32_t gb_timerclk; // time before next timer interrupt

unsigned lcd_int_on;

const char stat2LCDflg[8]={0x08,0x10,0x20,0x00, 0x48,0x50,0x60,0x40};


void check4LCDint(unsigned mode) { // Also called from mem.c!!
	//unsigned lcd_int_on_new=; // LYC is not processed here
	if(R_STAT&stat2LCDflg[mode]/* !lcd_int_on && */) {// check if interrupt is requested already
		R_IF|=INT_LCDSTAT;
		sm83_check4int(); // do int if possible
	}
	//lcd_int_on = lcd_int_on_new;
}
void check4LYC(void) {  // Also called from mem.c!!
	unsigned stnew = R_STAT&~4;
	if(R_LYC == R_LY) {
		stnew|=4;
		if(R_STAT < stnew)
			if(stnew&0x40) {// check if interrupt allowed
				R_IF|=INT_LCDSTAT;
				sm83_check4int(); // do int if possible
			}
	}
	R_STAT=stnew;
}	

#define STAT_MODE(n) \
	 R_STAT = (R_STAT&0xFC)|n; \
	 check4LCDint(n);

// **********************************************************************
uint32_t gb_divbase;
uint32_t gb_timbase;
/* these variables are added to current gb clock to obtain
	timer counter values in lower byte of result */
uint8_t gb_timshift;	// input clock shift       1048576/(4,16,64,256)

void gb_reload_tima(unsigned data) { // will only contain byte value
	gb_timbase = data-(int32_t)(gb_clk >> gb_timshift)-256;
	gb_timerclk = ((gb_clk>>gb_timshift)-gb_timbase)<<gb_timshift;
}

static void execute(uint32_t n) {
	gb_eventclk+=n; // timerclk = MAXULONG means that timer interrupt is off
	/*while(gb_eventclk>gb_timerclk) {
		sm83_execute_until(gb_timerclk);
		gb_reload_tima(R_TMA);
		R_IF|=INT_TIMER;	// request timer interrupt
		sm83_check4int();
	}*/
	sm83_execute_until(gb_eventclk);
}

// **********************************************************************

/* begin emulation */
void start()
{
	
	unsigned i,gb_old;
	//uint8_t lcd_status_prev,lcd_status:
	//lcd_status_prev=lcd_status=0;
	lcd_int_on=0;
	gb_eventclk = gb_clk = gb_divbase = 0;
	gb_timerclk = 0x7FFFFFFF;

	while(1) {
		gb_old = gb_clk;
		R_LY=0;
		for(i=144;i!=0;i--) {
			check4LYC();
			/* LCD during OAM-search (scan sprites) */

			/* LCD during H-Blank */
			//STAT_MODE(0);
			//gb_eventclk+=204;
			//sm83_execute_until(gb_eventclk);

			STAT_MODE(2);
			ppu_enumsprites();
			execute(20);
			/* LCD during data transfer (draw line) */
			STAT_MODE(3); /* no interrupt here !! */
			ppu_refreshline();
			execute(43);

			/* LCD during H-Blank */
			STAT_MODE(0);
			execute(51);

			R_LY++;
		}
		//R_LY = 143;
		/* LCD during V-Blank (10 "empty" lines) */
		//if(R_STAT & 0x10) // questionable
		R_IF|=INT_VBLANK; // Queue V-blank int
		sm83_check4int();
		STAT_MODE(1);
		ppu_vsync();
		for(i=10;i!=0;i--) {
			check4LYC();
			execute(114);
			R_LY++;
		}
		// smallest of all- eventclk
		if(gb_eventclk > (4<<28)) { // wrap _ALL_ counters
			gb_clk			-=(3<<28);
			gb_eventclk		-=(3<<28);
			apu_clk_inner[1] -=(3<<28);
			apu_clk_nextchange-=(3<<28);
			if(gb_timerclk<MAXULONG) gb_timerclk-=(3<<28);
		}
		apu_mix();
	}
}
