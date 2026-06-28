/*******************************************************************************
 * Copyright (c) 2012-2019 Thomas Haber - thomas haber
 * All rights reserved. This source code and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 *******************************************************************************/
#include "swv.h"

void initSwv() {

	DBGMCUCR |= 0x00000027;
	DEMCR |= (1 << 24);
	ITM_LSR = 0xC5ACCE55;

	TPIU_SPPR = 0x00000002;     // Select NRZ mode
	TPIU_ACPR = 1;  			// Divider 1 means /2

	ITM_TPR = 0x00000000;
	DWT_CTRL = 0x400003FE;
	FFCR = 0x00000100;

	ITM_TCR = 0x1000F; 			// Enable ITM
	ITM_ENA = 0xffff; 			// Enable ITM stimulus port

}

void demo() {

	itm_write_t0(0, "Start Demo");
	wait(100);
	itm_write_t0(0, "Measure");

	int i = 0;
	float f = 0.0;
	double d = 0.0;
	unsigned char uc = 0;
	signed char sc = 0;
	unsigned short us = 0;
	signed short ss = 0;
	unsigned long ul = 0;
	signed long sl = 0;
	unsigned long long ull = 0;
	signed long long sll = 0;
	unsigned char logic = 0;

	while (1) {

		// float
		f = sin(i / 100.0) * 100.;
		d = cos(i / 50.0 + 1.0) * 50. + 20;
		itm_write_f32(1, f);
		itm_write_f64(2, d);

		wait(10);

		// text
		if (i % 500 == 0)
			itm_write_t0(0, "i%50 reached \nmore to come\n");

		// bytes
		if (i % 100 == 0) {
			uc = i / 20;
			sc += 1;
			itm_write_i8(3, uc);
			itm_write_i8(4, sc);
		}

		// short
		if (i % 125 == 0) {
			us = d + i;
			ss = d * 20;
			itm_write_i16(5, us);
			itm_write_i16(6, ss);
		}

		// long
		if (i % 175 == 0) {
			ul = i;
			sl = -i;
			itm_write_i32(7, ul);
			itm_write_i32(8, sl);
		}

		// long long
		if (i % 225 == 0) {
			ull += rand();
			sll = (((long long) d) << 32) + rand();
			itm_write_i64(9, ull);
			itm_write_i64(10, sll);
		}

		// logic
		if (i % 25 == 0) {
			logic = i & 1;
			itm_write_i8(11, logic);
		}

		// log
		if (i > 0 && i % 200 == 0)
			itm_write_log(12, SEVERITY_INFO, "Test", "i % 200 == 0");
		if (i > 0 && i % 350 == 0)
			itm_write_log(12, SEVERITY_ERROR, "Test", ">= 350");

		i++;
	}
}
