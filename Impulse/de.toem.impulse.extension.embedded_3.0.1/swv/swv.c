/*******************************************************************************
 * Copyright (c) 2012-2019 Thomas Haber - thomas haber
 * All rights reserved. This source code and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 *******************************************************************************/
#include"swv.h"

void itm_write_i8(unsigned char port, signed char b) {
	if (ITM_ENABLED(port)) {
		while (ITM_STIM_U8(port) == 0)
			;
		ITM_STIM_U8 (port) = b;
	}
}

void itm_write_i16(unsigned char port, short b) {
	if (ITM_ENABLED(port)) {
		while (ITM_STIM_U8(port) == 0)
			;
		ITM_STIM_U16 (port) = b;
	}
}

void itm_write_i32(unsigned char port, long b) {
	if (ITM_ENABLED(port)) {
		while (ITM_STIM_U8(port) == 0)
			;
		ITM_STIM_U32 (port) = b;
	}
}

void itm_write_i64(unsigned char port, long long b) {
	if (ITM_ENABLED(port)) {
		while (ITM_STIM_U8(port) == 0)
			;
		ITM_STIM_U32 (port) = ((long*) &b)[0];
		while (ITM_STIM_U8(port) == 0)
			;
		ITM_STIM_U32 (port) = ((long*) &b)[1];
	}
}

void itm_write_f32(unsigned char port, float f) {
	if (ITM_ENABLED(port)) {
		while (ITM_STIM_U8(port) == 0)
			;
		ITM_STIM_U32 (port) = *(long*) &f;
	}
}

void itm_write_f64(unsigned char port, double f) {
	if (ITM_ENABLED(port)) {
		while (ITM_STIM_U8(port) == 0)
			;
		ITM_STIM_U32 (port) = ((long*) &f)[0];
		while (ITM_STIM_U8(port) == 0)
			;
		ITM_STIM_U32 (port) = ((long*) &f)[1];
	}
}

void itm_write_t0(unsigned char port, const char* text) {
	if (ITM_ENABLED(port)) {
		while (*text != 0) {
			while ((ITM_STIM_U8(port)) == 0)
				;
			ITM_STIM_U8 (port) = *text++;
		}
		while (ITM_STIM_U8(port) == 0)
			;
		ITM_STIM_U8 (port) = 0;
	}
}

void itm_write_tn(unsigned char port, const char* text, char e) {
	if (ITM_ENABLED(port)) {
		while (*text != 0) {
			while ((ITM_STIM_U8(port)) == 0)
				;
			ITM_STIM_U8 (port) = *text++;
		}
		while (ITM_STIM_U8(port) == 0)
			;
		ITM_STIM_U8 (port) = e;
	}
}

void itm_write_log(unsigned char port, unsigned char severity, const char* source, const char* message) {
	if (ITM_ENABLED(port)) {
		itm_write_i8(port, severity);
		itm_write_i8(port, (unsigned char) 10);
		itm_write_tn(port, source, 10);
		itm_write_t0(port, message);
	}
}
