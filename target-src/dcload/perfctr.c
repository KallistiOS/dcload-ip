// ---- perfctr.c - SH7091 Performance Counter Module Code ----
//
// This file is part of the DreamHAL project, a hardware abstraction library
// primarily intended for use on the SH7091 found in hardware such as the SEGA
// Dreamcast game console.
//
// The performance counter module is hereby released into the public domain in
// the hope that it may prove useful. Now go profile some code and hit 60 fps! :)
//
// --Moopthehedgehog

// See perfctr.h for more of my notes and documentation on these counters.
#include "perfctr.h"

static unsigned char pmcr_enabled = 0;

//
// Initialize performance counters. It's just a clear -> enable.
// It's good practice to clear a counter before starting it for the first time.
//
// Also: Disabling and re-enabling the counters doesn't reset them; the clearing
// needs to happen while a counter is disabled to reset it.
//
// You can disable and re-enable with a different mode without explicitly
// clearing and have it keep going, continuing from where it left off.
//

void PMCR_Init(int which, unsigned short mode, unsigned char count_type) // Will do nothing if perfcounter is already running!
{
	// Don't do anything if being asked to enable an already-enabled counter
	if( (which == 1) && ((!pmcr_enabled) || (pmcr_enabled == 2)) )
	{
		// counter 1
		PMCR_Enable(1, mode, count_type, PMCR_RESET_COUNTER);
	}
	else if( (which == 2) && ((!pmcr_enabled) || (pmcr_enabled == 1)) )
	{
		// counter 2
		PMCR_Enable(2, mode, count_type, PMCR_RESET_COUNTER);
	}
	else if( (which == 3) && (!pmcr_enabled) )
	{
		// Both
		PMCR_Enable(3, mode, count_type, PMCR_RESET_COUNTER);
	}
}

// Enable "undocumented" performance counters (well, they were undocumented at one point. They're documented now!)
void PMCR_Enable(int which, unsigned short mode, unsigned char count_type, unsigned char reset_count) // Will do nothing if perfcounter is already running!
{
	// Don't do anything if count_type or reset_count are invalid
	if((count_type | reset_count) > 1)
	{
		return;
	}

	// Build config from parameters
	unsigned short pmcr_ctrl = PMCR_RUN_COUNTER | (reset_count << PMCR_RESET_COUNTER_SHIFT) | (count_type << PMCR_CLOCK_TYPE_SHIFT) | mode;

	// Don't do anything if being asked to enable an already-enabled counter
	if( (which == 1) && ((!pmcr_enabled) || (pmcr_enabled == 2)) )
	{
		// counter 1
		*((volatile unsigned short*)PMCR1_CTRL_REG) = pmcr_ctrl;

		pmcr_enabled += 1;
	}
	else if( (which == 2) && ((!pmcr_enabled) || (pmcr_enabled == 1)) )
	{
		// counter 2
		*((volatile unsigned short*)PMCR2_CTRL_REG) = pmcr_ctrl;

		pmcr_enabled += 2;
	}
	else if( (which == 3) && (!pmcr_enabled) )
	{
		// Both
		*((volatile unsigned short*)PMCR1_CTRL_REG) = pmcr_ctrl;
		*((volatile unsigned short*)PMCR2_CTRL_REG) = pmcr_ctrl;

		pmcr_enabled = 3;
	}
}

// For reference:
// #define PMCTR1H_REG 0xFF100004
// #define PMCTR1L_REG 0xFF100008

// #define PMCTR2H_REG 0xFF10000C
// #define PMCTR2L_REG 0xFF100010

static const unsigned int pmcr1_regh = PMCTR1H_REG;
static const unsigned int pmcr1_regl = PMCTR1L_REG;

static const unsigned int pmcr2_regh = PMCTR2H_REG;
static const unsigned int pmcr2_regl = PMCTR2L_REG;

// Sorry, can only read one counter at a time!
// out_array should be an array consisting of 2x unsigned ints.
void PMCR_Read(int which, volatile unsigned int *out_array)
{
 // if pmcr is not enabled, this function will just return 0

	// little endian (big endian would need to flip [0] and [1])

	// Note: These reads really do need to be done in assembly: unfortunately it
	// appears that using C causes GCC to insert a branch right smack in between
	// the high and low reads of perf counter 2 (with a nop, so it's literally
	// delaying the reads by several cycles!), which is totally insane. Doing it
	// the assembly way ensures that nothing ridiculous like that happens. It's
	// also portable between versions of GCC that do put the nonsensical branch in.
	//
	// One thing that would be nice is if SH4 had the movi20s instruction to make
	// absolute addresses in 3 cycles, but only the SH2A has that... :(
	if( (which == 1) && (pmcr_enabled & 0x1) )
	{
		// counter 1
//		out_array[1] = *((volatile unsigned int*)PMCTR1H_REG) & 0xffff;
//		out_array[0] = *((volatile unsigned int*)PMCTR1L_REG);
		asm volatile("mov.l %[reg1h],r1\n\t" // load counter address (high)
								 "mov.l %[reg1l],r2\n\t" // load counter address (low)
								 "mov.l @r1,r1\n\t" // read counter (high)
								 "mov.l @r2,r2\n\t" // read counter (low)
								 "extu.w r1,r1\n\t" // zero-extend high, aka high & 0xffff
								 "mov.l r1,%[outh]\n\t" // get data to memory
								 "mov.l r2,%[outl]\n\t" // get data to memory
		: [outh] "=m" (out_array[1]), [outl] "=m" (out_array[0])
		: [reg1h] "m" (pmcr1_regh), [reg1l] "m" (pmcr1_regl) // SH4 can't mov an immediate longword into a register...
		: "r1", "r2"
		);
	}
	else if( (which == 2) && (pmcr_enabled & 0x2) )
	{
		// counter 2
//		out_array[1] = *((volatile unsigned int*)PMCTR2H_REG) & 0xffff;
//		out_array[0] = *((volatile unsigned int*)PMCTR2L_REG);
		asm volatile("mov.l %[reg2h],r1\n\t" // load counter address (high)
								 "mov.l %[reg2l],r2\n\t" // load counter address (low)
								 "mov.l @r1,r1\n\t" // read counter (high)
								 "mov.l @r2,r2\n\t" // read counter (low)
								 "extu.w r1,r1\n\t" // zero-extend high, aka high & 0xffff
								 "mov.l r1,%[outh]\n\t" // get data to memory
								 "mov.l r2,%[outl]\n\t" // get data to memory
		: [outh] "=m" (out_array[1]), [outl] "=m" (out_array[0])
		: [reg2h] "m" (pmcr2_regh), [reg2l] "m" (pmcr2_regl) // SH4 can't mov an immediate longword into a register...
		: "r1", "r2"
		);
	}
	else if(!pmcr_enabled)
	{
		out_array[1] = 0;
		out_array[0] = 0;
	}
	else // Invalid
	{
		out_array[1] = 0xffff;
		out_array[0] = 0xffffffff;
	}
}

// Reset counter to 0 and start it again
// NOTE: It does not appear to be possible to clear a counter while it is running.
void PMCR_Restart(int which, unsigned short mode, unsigned char count_type)
{
	if( (which == 1) && (pmcr_enabled & 0x1) )
 	{
 		// counter 1
		PMCR_Stop(1);
		PMCR_Enable(1, mode, count_type, PMCR_RESET_COUNTER);
 	}
	else if( (which == 2) && (pmcr_enabled & 0x2) )
 	{
 		// counter 2
		PMCR_Stop(2);
		PMCR_Enable(2, mode, count_type, PMCR_RESET_COUNTER);
 	}
	else if( (which == 3) && (pmcr_enabled == 3) )
 	{
		// Both
		PMCR_Stop(3);
		PMCR_Enable(3, mode, count_type, PMCR_RESET_COUNTER);
 	}
}

// Clearing only works when the counter is disabled. Otherwise, stopping the
// counter via setting the 0x2000 bit holds the data in the data registers,
// whereas disabling without setting that bit reads back as all 0 (but doesn't
// clear the counters for next start). This function just stops a running
// counter and does nothing if the counter is already stopped or disabled, as
// clearing is handled by PMCR_Enable().
void PMCR_Stop(int which)
{
	if( (which == 1) && (pmcr_enabled & 0x1) )
	{
		// counter 1
		*((volatile unsigned short*)PMCR1_CTRL_REG) = PMCR_STOP_COUNTER;

		pmcr_enabled &= 0x2;
	}
	else if( (which == 2) && (pmcr_enabled & 0x2) )
	{
		// counter 2
		*((volatile unsigned short*)PMCR2_CTRL_REG) = PMCR_STOP_COUNTER;

		pmcr_enabled &= 0x1;
	}
	else if( (which == 3) && (pmcr_enabled == 3) )
	{
		// Both
		*((volatile unsigned short*)PMCR1_CTRL_REG) = PMCR_STOP_COUNTER;
		*((volatile unsigned short*)PMCR2_CTRL_REG) = PMCR_STOP_COUNTER;

		pmcr_enabled = 0;
	}
}

// Note that disabling does NOT clear the counter.
// It may appear that way because reading a disabled counter returns 0, but re-
// enabling without first clearing will simply continue where it left off.
void PMCR_Disable(int which)
{
	if(which == 1)
	{
		// counter 1
		*((volatile unsigned short*)PMCR1_CTRL_REG) = PMCR_DISABLE_COUNTER;

		pmcr_enabled &= 0x2;
	}
	else if(which == 2)
	{
		// counter 2
		*((volatile unsigned short*)PMCR2_CTRL_REG) = PMCR_DISABLE_COUNTER;

		pmcr_enabled &= 0x1;
	}
	else if(which == 3)
	{
		// Both
		*((volatile unsigned short*)PMCR1_CTRL_REG) = PMCR_DISABLE_COUNTER;
		*((volatile unsigned short*)PMCR2_CTRL_REG) = PMCR_DISABLE_COUNTER;

		pmcr_enabled = 0;
	}
}
