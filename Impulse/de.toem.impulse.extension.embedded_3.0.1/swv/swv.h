/*******************************************************************************
 * Copyright (c) 2012-2019 Thomas Haber - thomas haber
 * All rights reserved. This source code and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 *******************************************************************************/
#define USE_ITM 1

#define ITM_ENABLED(port) (USE_ITM && (ITM_ENA & (1<< port)) && (ITM_TCR))
#define ITM_STIM_U8(port)  (*(volatile unsigned char* )(0xE0000000|(port<<2))) /* STIM 0 Register/ 8 bits*/
#define ITM_STIM_U16(port) (*(volatile unsigned short*)(0xE0000000|(port<<2))) /* STIM 0 Register/ 16 bits*/
#define ITM_STIM_U32(port) (*(volatile unsigned int*  )(0xE0000000|(port<<2))) /* STIM 0 Register/ 32 bits*/

#define ITM_ENA            (*(volatile unsigned int*  )0xE0000E00) /* Trace Enable Ports Register*/
#define ITM_TPR            (*(volatile unsigned int*  )0xE0000E40) /* Trace Privilege Register*/
#define ITM_TCR            (*(volatile unsigned int*  )0xE0000E80) /* Trace control register*/
#define ITM_LSR            (*(volatile unsigned int*  )0xE0000FB0) /* ITM Lock Status Register*/
#define DHCSR 			   (*(volatile unsigned int*  )0xE000EDF0) /* Debug register*/
#define DEMCR 			   (*(volatile unsigned int*  )0xE000EDFC) /* Debug register*/
#define DBGMCUCR           (*(volatile unsigned int*  )0xE0042004) /* DBGMCU CR register*/
#define TPIU_ACPR          (*(volatile unsigned int*  )0xE0040010) /* Async Clock presacler register*/
#define TPIU_SPPR          (*(volatile unsigned int*  )0xE00400F0) /* Selected Pin Protocol Register*/
#define FFCR               (*(volatile unsigned int*  )0xE0040304) /* Formatter and flush Control Register*/
#define DWT_CTRL           (*(volatile unsigned int*  )0xE0001000) /* DWT Control Register*/

#define SEVERITY_ERROR 'e'
#define SEVERITY_WARNING 'w'
#define SEVERITY_INFO 'i'

void itm_write_i8(unsigned char port, signed char b);
void itm_write_i16(unsigned char port, short b);
void itm_write_i32(unsigned char port, long b);
void itm_write_i64(unsigned char port, long long b);
void itm_write_f32(unsigned char port, float f);
void itm_write_f64(unsigned char port, double f);
void itm_write_t0(unsigned char port, const char* text);
void itm_write_tn(unsigned char port, const char* text, char e);
void itm_write_log(unsigned char port, unsigned char severity, const char* source, const char* message);

