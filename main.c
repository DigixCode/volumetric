/*******************************************************************************
 * Copyright (C) 2017 Maxim Integrated Products, Inc., All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Maxim Integrated
 * Products, Inc. shall not be used except as stated in the Maxim Integrated
 * Products, Inc. Branding Policy.
 *
 * The mere transfer of this software does not imply any licenses
 * of trade secrets, proprietary technology, copyrights, patents,
 * trademarks, maskwork rights, or any other form of intellectual
 * property whatsoever. Maxim Integrated Products, Inc. retains all
 * ownership rights.
 *
 ******************************************************************************/

// This application example demonstrates how to measure water volume passing
// through the audiowell ultrasonic transducer flow body and conditionally deactivate
// an external residental (24VAC) water valve.  Two multi-position switches are
// read at start-up to determin the cut-off volume.  Volume (gallons) = 1000*S1+100*S2

#include "global.h"

#include "tmr_utils.h"
#include "board.h"
#include "transducer.h"

#include <tmr.h>

#define CUBIC_METERS_TO_GALLONS	264.172

static max3510x_results_t 	s_results;
static volatile uint32_t    s_results_count;
static volatile uint32_t	s_timeout_count;

void max3510x_int_isr(void * pv)
{
	// handle MAX35103 interrupts
	uint32_t ts = TMR32_GetCount(TIMESTAMP_TIMER);
	uint16_t status = max3510x_interrupt_status( &g_max35103 );

	if( (status & (MAX3510X_REG_INTERRUPT_STATUS_TOF|MAX3510X_REG_INTERRUPT_STATUS_TO)) == MAX3510X_REG_INTERRUPT_STATUS_TOF )
	{
		max3510x_read_results( &g_max35103, &s_results );
		s_results_count++;
	}
	else if( status )
	{
		s_timeout_count++;
	}
	GPIO_IntClr(&g_board_max3510x_int);
}

void SysTick_Handler(void)
{
	// systick provides the sampling timebase
    if(!s_results_count)
			max3510x_tof_diff(&g_max35103);
}

int main(void)
{
	double_t last_flow = 0.0, sum_flow, flow;

	board_init();
	GPIO_OutPut(&g_board_relay,0);
	
	uint32_t one_second = SystemCoreClock ;

	double_t shutoff = 100.0 * board_read_bcd_switches();

	SYS_SysTick_Config( (uint32_t)((float_t)SYS_SysTick_GetFreq() / 200.0F), 1);  // 50Hz sampling rate

	board_printf( "\033cMAX35103EVKIT2 VOLUMETRIC APPLICATION EXAMPLE\r\n");
	board_printf( "shut-off @ %dG\r\n", (uint32_t)shutoff );

	uint8_t c = 0;
	uint32_t ts, last_ts;
	last_ts = TMR32_GetCount(TIMESTAMP_TIMER);
	while( 1 )
	{
		ts = TMR32_GetCount(TIMESTAMP_TIMER);
		flow = 0;
		if( s_results_count )
		{
			// these checks can help flush out SPI issues.  Provided here as a refernce for new board designs.
			if( max3510x_validate_measurement( &s_results.up, transducer_hit_count() ) &&
				max3510x_validate_measurement( &s_results.down, transducer_hit_count() ) )
			{
				float_t t2_ideal = max3510x_ratio_to_float(s_results.up.t2_ideal);
				float_t t1_t2 = max3510x_ratio_to_float(s_results.up.t1_t2);
				if( t2_ideal >= 0.95f && t1_t2 > 0.50f )
				{
					// Use the ratios to validate measuremnets.  If air bubbles are in the flow body, attenuation
					// can cause the t1 wave to be missed.  Production applicaiotns can use more sophisticatd recovery
					// logic, but here we just duplicate the last valid sample.  This works for itermittant attenuation
					// due to the occastional bubble, but is insuffecient for applications that have large amounts
					// of undissolved gases present in the flow body.
					
					// For more information about attenuation due to undissolved gases, see Maxim application note 6357 "Dealing with Bubbles"
					
					double_t up = max3510x_fixed_to_double( &s_results.up.average );
					double_t down = max3510x_fixed_to_double( &s_results.down.average );
					flow = last_flow = transducer_flow( up, down );
				}
			}
			if( !flow )
			{
				flow = last_flow;
			}
			if( flow < 0 )
			{
				last_flow = 0;
				flow = 0;				// assume negative flow to be zero for the purpose of this application.
			}
			
			sum_flow += (flow * CUBIC_METERS_TO_GALLONS/50.0);	// scale flow to gallons based on 100Hz sampling rate
			s_results_count = 0;
			c++;
		}
		if(  shutoff <= sum_flow )
		{
			GPIO_OutToggle(&g_board_relay);
			board_printf("RELAY OFF\r\n");
			while(1);	// done for this cycle
		}
		else if(  ts - last_ts > one_second )
		{
			GPIO_OutToggle(&g_board_led);
			board_printf("%.2fG, %.2fGPM\r\n", sum_flow, last_flow * CUBIC_METERS_TO_GALLONS * 60.0	); // gallons per minute
			last_ts = ts;
		}
	}
}


