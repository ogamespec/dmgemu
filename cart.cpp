#include "pch.h"

Cartridge cart;

#define Kbit *128
#define Mbit *(128*1024)

static SZstruct ROM_SIZES[] = {
{0,  256 Kbit},
{1,  512 Kbit},
{2,  1   Mbit},
{3,  2   Mbit},
{4,  4   Mbit},
{5,  8   Mbit},
{6,  16  Mbit},
{7,  32  Mbit},
{8,  64  Mbit},
{52, 9   Mbit},
{53, 10  Mbit},
{54, 12  Mbit},
{64, 20  Mbit},
{65, 24  Mbit},
{74, 36  Mbit},
{75, 40  Mbit},
{76, 48  Mbit},
{-1,0} };

static SZstruct RAM_SIZES[] = {
{0,0   Kbit},
{1,16  Kbit},
{2,64  Kbit},
{3,256 Kbit},
{4,1   Mbit},
{-1,0} };

rom_header_t* romhdr;

static void do_header_chk(int fire)
{
	uint8_t* buf = (uint8_t*)romhdr->title;
	int chk = 0;
	int i;

	for (i = 0; i < 25; i++)
		chk += buf[i];

	chk = 0x100 - (chk + 25);

	if (fire && romhdr->header_chk != (uint8_t)chk)
		sys_error("header checksum failed!");
}

/*static void do_global_chk(int fire)
{
	long i;
	register u_long chk = 0;

	for(i=0; i<rom_size; i++)
	{
		if(i != 0x14e && i != 0x14f)
			chk += game[i];
	}

	chk &= 0xffff;
	chk = (chk << 8) | (chk >> 8);

	if(fire && romhdr->global_chk != (uint16_t)chk)
		sys_error("global checksum failed!");
}*/


static int findsz(unsigned* sz, SZstruct* from, int what) {
	while (1) {
		if (from->n == -1) return 0;
		if (from->n == what) {
			*sz = from->sz;
			return -1;
		}
		from++;
	};
}

void check_ROM_header()
{
	if (cart.data == NULL)
	{
		static rom_header_t empty;
		romhdr = &empty;
		empty.type = 0;
		strcpy(cart.title, "NO CARD");
		return;
	}
	romhdr = (rom_header_t*)(cart.data + 0x100);
	printf("MBC type: %d, romsize: %d, ramsize: %d\n", romhdr->type, romhdr->romsize, romhdr->ramsize);
	if (!findsz(&(cart.rom_size), ROM_SIZES, romhdr->romsize))
		sys_error("Unknown ROM size");
	if (!findsz(&(cart.ram_size), RAM_SIZES, romhdr->ramsize))
		sys_error("Unknown RAM size");
	do_header_chk(0);
	memcpy(cart.title, romhdr->title, sizeof(romhdr->title));
	cart.title[sizeof(romhdr->title)] = '\0';
	//    do_global_chk(1);
}

/***********************************************************************
	MBC1 support
***********************************************************************/
static struct MBC_ {
	int bank1;	// MBC1
	int bank2;	// MBC1
	int mode;	// MBC1

	unsigned ramenabled;  // All MBCs
	unsigned clock_present; // MBC3
	unsigned clock_latched; // MBC3
} MBC;

void mem_w8_MBC1_RAMcontrol(unsigned addr, uint8_t n) {
	if ((n & 0xF) == 0xA) {
		if (!MBC.ramenabled) MAPRAM(mem_r8_RAMbank, mem_w8_RAMbank);
		MBC.ramenabled = 1;
	}
	else {
		if (MBC.ramenabled) MAPRAM(mem_r8_emptyRAM, mem_w8_NULL);
		MBC.ramenabled = 0;
	}
}
void mbc1_update_mapping()
{
	SETROM(0, MBC.mode ? (MBC.bank2 << 5) : 0);
	SETROM(1, (MBC.bank2 << 5) | MBC.bank1);
	SETRAM(MBC.mode ? MBC.bank2 : 0);
}
void mem_w8_MBC1_setBANK1(unsigned addr, uint8_t n) {
	MBC.bank1 = n & 0x1F; // 5 bits
	if (MBC.bank1 == 0) MBC.bank1 = 1;
	mbc1_update_mapping();
}
void mem_w8_MBC1_setBANK2(unsigned addr, uint8_t n) {
	MBC.bank2 = (unsigned)n & 3; // 2 bits
	mbc1_update_mapping();
}
void mem_w8_MBC1_mode(unsigned addr, uint8_t n) {
	MBC.mode = n & (unsigned)1;
	mbc1_update_mapping();
}

void InitMBC1_ROM(void) {
	MEMMAP_W(0x2000, 0x4000, mem_w8_MBC1_setBANK1);
	MEMMAP_W(0x4000, 0x6000, mem_w8_MBC1_setBANK2);
	MEMMAP_W(0x6000, 0x8000, mem_w8_MBC1_mode);
	MAPROM(mem_r8_ROMbank1);

	MBC.ramenabled = 0;
	MBC.bank1 = 1;
	MBC.bank2 = 0;
	MBC.mode = 0;
	mbc1_update_mapping();
}

void InitMBC1_RAM(void) {
	MEMMAP_W(0x0000, 0x2000, mem_w8_MBC1_RAMcontrol);
}

/***********************************************************************
	MBC2 support
***********************************************************************/


// TODO: I don't know exactly how to map those 512x4 bits properly.
void mem_w8_MBC2_set(unsigned addr, uint8_t n) {
	unsigned nn = n & 0xF;
	if (addr & 0x2100) {  // ROM control
		if (!nn) nn = 1;
		SETROM(1, nn);
	}
	else {		  // RAM control
		if (nn == 0xA) {
			if (!MBC.ramenabled) MAPRAM(mem_r8_RAMbank, mem_w8_RAMbank);
			MBC.ramenabled = 1;
		}
		else {
			if (MBC.ramenabled) MAPRAM(mem_r8_emptyROM, mem_w8_NULL);
			MBC.ramenabled = 0;
		}
	}
}

void InitMBC2(void) {
	MEMMAP_W(0x0000, 0x4000, mem_w8_MBC2_set);
	MAPROM(mem_r8_ROMbank1);
	MAPRAM(mem_r8_emptyROM, mem_w8_NULL);
	SETRAM(0);
	MBC.ramenabled = 0;
}

/***********************************************************************
	MBC3 support
***********************************************************************/
// TODO: no realtime clock support

void mem_w8_MBC3_setROM(unsigned addr, uint8_t n) {
	//register unsigned nn=n&0x7F; // 7 bits
	//if(!MBC.mode2) nn|=cart.rom[1].bank&~0x1F;  //
	if (!n) n = 1;
	SETROM(1, n);
}
void mem_w8_MBC3_setRAM_or_clock(unsigned addr, uint8_t n) {
	if (!(n & ~3)) { // set RAM bank
		if (cart.ram_nbanks) SETRAM(n);
	}
	else {	// set RT clock register

	}
}
void mem_w8_MBC3_clocklatch(unsigned addr, uint8_t n) {

}

void InitMBC3_ROM(void) {
	MEMMAP_W(0x2000, 0x4000, mem_w8_MBC3_setROM);
	MEMMAP_W(0x4000, 0x6000, mem_w8_MBC3_setRAM_or_clock);
	MEMMAP_W(0x6000, 0x8000, mem_w8_MBC3_clocklatch);
	MAPROM(mem_r8_ROMbank1);
	MBC.ramenabled = 0;
}

void InitMBC3_RAM(void) {
	InitMBC1_RAM();
}

/***********************************************************************
	MBC5 support
***********************************************************************/

void mem_w8_MBC5_setROM0(unsigned addr, uint8_t n) {
	unsigned nn = (unsigned)n | (cart.rom[1].bank & 0x100);
	SETROM(1, nn);
}
void mem_w8_MBC5_setROM1(unsigned addr, uint8_t n) {
	unsigned nn = ((unsigned)n << 8) | (cart.rom[1].bank & 0xFF);
	SETROM(1, nn);
}
void mem_w8_MBC5_setRAM(unsigned addr, uint8_t n) {
	SETRAM(n);
}

void InitMBC5_ROM(void) {
	MEMMAP_W(0x2000, 0x3000, mem_w8_MBC5_setROM0);
	MEMMAP_W(0x3000, 0x4000, mem_w8_MBC5_setROM1);
	MEMMAP_W(0x4000, 0x6000, mem_w8_MBC5_setRAM);
	//MEMMAP_W(0x6000,0x8000,mem_w8_MBC3_clocklatch);
	MAPROM(mem_r8_ROMbank1);
	MBC.ramenabled = 0;
}

void InitMBC5_RAM(void) {
	InitMBC1_RAM();
}