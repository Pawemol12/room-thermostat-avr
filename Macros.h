/*
 * Macros.h
 *
 * Created: 22.08.2020 13:23:41
 *  Author: Pawemol
 * https://www.avrfreaks.net/forum/tut-c-bit-manipulation-aka-programming-101
 */ 


#ifndef MACROS_H_
#define MACROS_H_

#define bit_get(p,m) ((p) & (m))
#define bit_set(p,m) ((p) |= (m))
#define bit_clear(p,m) ((p) &= ~(m))
#define bit_flip(p,m) ((p) ^= (m))
#define bit_write(c,p,m) (c ? bit_set(p,m) : bit_clear(p,m))
#define BIT(x) (0x01 << (x))
#define LONGBIT(x) ((unsigned long)0x00000001 << (x))



#endif /* MACROS_H_ */