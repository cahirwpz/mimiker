/*-
 * $FreeBSD$
 */

#ifndef	_KBIO_H_
#define	_KBIO_H_

#include <stdint.h>

/* get/set key map/accent map/function key strings */

#define NUM_KEYS	256		/* number of keys in table	*/
#define NUM_STATES	8		/* states per key		*/
#define ALTGR_OFFSET	128		/* offset for altlock keys	*/

#define NUM_FKEYS	96		/* max number of function keys	*/
#define MAXFK		16		/* max length of a function key str */

typedef struct {
    unsigned int map[NUM_STATES];
    unsigned char spcl;
    unsigned char flgs;
#define	FLAG_LOCK_O	0
#define	FLAG_LOCK_C	1
#define FLAG_LOCK_N	2
} keyent_t;

typedef struct {
    unsigned short n_keys;
    keyent_t key[NUM_KEYS];
} keymap_t;

/* defines for "special" keys (spcl bit set in keymap) */
#define NOP		0x00		/* nothing (dead key)		*/
#define LSH		0x02		/* left shift key		*/
#define RSH		0x03		/* right shift key		*/
#define CLK		0x04		/* caps lock key		*/
#define NLK		0x05		/* num lock key			*/
#define SLK		0x06		/* scroll lock key		*/
#define LALT		0x07		/* left alt key			*/
#define BTAB		0x08		/* backwards tab		*/
#define LCTR		0x09		/* left control key		*/
#define NEXT		0x0a		/* switch to next screen 	*/
#define F_SCR		0x0b		/* switch to first screen 	*/
#define L_SCR		0x1a		/* switch to last screen 	*/
#define F_FN		0x1b		/* first function key 		*/
#define L_FN		0x7a		/* last function key 		*/
/*			0x7b-0x7f	   reserved do not use !	*/
#define RCTR		0x80		/* right control key		*/
#define RALT		0x81		/* right alt (altgr) key	*/
#define ALK		0x82		/* alt lock key			*/
#define ASH		0x83		/* alt shift key		*/
#define META		0x84		/* meta key			*/
#define RBT		0x85		/* boot machine			*/
#define DBG		0x86		/* call debugger		*/
#define SUSP		0x87		/* suspend power (APM)		*/
#define SPSC		0x88		/* toggle splash/text screen	*/

#define F_ACC		DGRA		/* first accent key		*/
#define DGRA		0x89		/* grave			*/
#define DACU		0x8a		/* acute			*/
#define DCIR		0x8b		/* circumflex			*/
#define DTIL		0x8c		/* tilde			*/
#define DMAC		0x8d		/* macron			*/
#define DBRE		0x8e		/* breve			*/
#define DDOT		0x8f		/* dot				*/
#define DUML		0x90		/* umlaut/diaresis		*/
#define DDIA		0x90		/* diaresis			*/
#define DSLA		0x91		/* slash			*/
#define DRIN		0x92		/* ring				*/
#define DCED		0x93		/* cedilla			*/
#define DAPO		0x94		/* apostrophe			*/
#define DDAC		0x95		/* double acute			*/
#define DOGO		0x96		/* ogonek			*/
#define DCAR		0x97		/* caron			*/
#define L_ACC		DCAR		/* last accent key		*/

#define STBY		0x98		/* Go into standby mode (apm)   */
#define PREV		0x99		/* switch to previous screen 	*/
#define PNC		0x9a		/* force system panic */
#define LSHA		0x9b		/* left shift key / alt lock	*/
#define RSHA		0x9c		/* right shift key / alt lock	*/
#define LCTRA		0x9d		/* left ctrl key / alt lock	*/
#define RCTRA		0x9e		/* right ctrl key / alt lock	*/
#define LALTA		0x9f		/* left alt key / alt lock	*/
#define RALTA		0xa0		/* right alt key / alt lock	*/
#define HALT		0xa1		/* halt machine */
#define PDWN		0xa2		/* halt machine and power down */
#define PASTE		0xa3		/* paste from cut-paste buffer */

#define F(x)		((x)+F_FN-1)
#define	S(x)		((x)+F_SCR-1)
#define ACC(x)		((x)+F_ACC)

typedef struct {
	unsigned char str[MAXFK];
	unsigned char len;
} fkeytab_t;

/* flags set to the return value in the KD_XLATE mode */

#define	NOKEY		0x01000000	/* no key pressed marker 	*/
#define	FKEY		0x02000000	/* function key marker 		*/
#define	MKEY		0x04000000	/* meta key marker (prepend ESC)*/
#define	BKEY		0x08000000	/* backtab (ESC [ Z)		*/

#define	SPCLKEY		0x80000000	/* special key			*/
#define	RELKEY		0x40000000	/* key released			*/
#define	ERRKEY		0x20000000	/* error			*/

/*
 * The top byte is used to store the flags.  This means there are 24
 * bits left to store the actual character.  Because UTF-8 can encode
 * 2^21 different characters, this is good enough to get Unicode
 * working.
 */
#define KEYCHAR(c)	((c) & 0x00ffffff)
#define KEYFLAGS(c)	((c) & ~0x00ffffff)

#endif /* !_KBIO_H_ */
