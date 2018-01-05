/*	
 *	@author		P. Cooper
 * 
 *	mysys.h is a memory allocation routine to replace malloc courtesy of P. Cooper.
 *	Malloc uses memory allocated in the CPU for the USB and Network buffers, these memory pointers 
 *	can not be changed, so this code will not work if either of these features are used.
 *
 */
 
#ifndef __MSYS_H
#define __MSYS_H
extern void MSYS_Free( void *ptr );
extern void *MSYS_Alloc( unsigned size );
extern void MSYS_Init( void *heap, unsigned len );
extern void MSYS_Compact( void );
#endif
