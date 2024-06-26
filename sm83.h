#pragma once

#define BIGENDIAN

union Z80reg {
	struct {
#ifdef BIGENDIAN
		uint8_t l,h;
#else
		uint8_t h,l;
#endif
	};
	uint16_t hl;
	unsigned align; // So 1 Z80reg record will be always equal to CPU word size
};

extern union Z80reg r_af, r_bc, r_de, r_hl;
extern union Z80reg r_sp, r_pc;

extern unsigned HALT, IME;

#define R_AF r_af.hl
/*
 direct usage of AF register is not allowed in my CPU core,
 because i swapped A and F to speedup some calculations
*/

#define R_BC r_bc.hl
#define R_DE r_de.hl
#define R_HL r_hl.hl
#define R_SP r_sp.hl
#define R_PC r_pc.hl


#define R_A r_af.l
#define R_F r_af.h
#define R_B r_bc.h
#define R_C r_bc.l
#define R_D r_de.h
#define R_E r_de.l
#define R_H r_hl.h
#define R_L r_hl.l

/* flags */

#define SF_POS 7 
#define ZF_POS 6 
#define HF_POS 4
#define PF_POS 2
#define NF_POS 1
#define CF_POS 0


//#define SF (1<<SF_POS)
#define ZF (uint8_t)(1<<ZF_POS)
#define HF (uint8_t)(1<<HF_POS)
//#define PF (1<<PF_POS)
#define NF (uint8_t)(1<<NF_POS)
#define CF (uint8_t)(1<<CF_POS)

#define ZFh (uint16_t)(0x100<<ZF_POS)
#define HFh (uint16_t)(0x100<<HF_POS)
#define NFh (uint16_t)(0x100<<NF_POS)
#define CFh (uint16_t)(0x100<<CF_POS)

#define BFLAGS (uint8_t)0xAC
#define BFLAGSh (uint16_t)0xAC00
// eto maska dlya "skvoznyh" flagov) mozhet i ne nado ee uzat', tem ne menee

//#define FETCH() RD(R_PC++) disabled because of incompatibility

/* CPU interface */
void gbz80_init();
//int  gbz80_execute();
void gbz80_execute_until(unsigned long clk_nextevent); // Exact timing is not guarantied
void check4int(void); // Call interrupt if IE,request flags are set and not masked.

extern unsigned debug_canwrite,debug_written[2];
