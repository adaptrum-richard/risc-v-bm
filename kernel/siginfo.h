#ifndef __SIGINFO_H__
#define __SIGINFO_H__
/*
 * SIGSEGV si_codes
 */
#define SEGV_MAPERR	1	/* address not mapped to object */
#define SEGV_ACCERR	2	/* invalid permissions for mapped object */
#define SEGV_BNDERR	3	/* failed address bound checks */
#define SEGV_PKUERR	4	/* failed protection key checks */
#define SEGV_ACCADI	5	/* ADI not enabled for mapped object */
#define SEGV_ADIDERR	6	/* Disrupting MCD error */
#define SEGV_ADIPERR	7	/* Precise MCD exception */
#define SEGV_MTEAERR	8	/* Asynchronous ARM MTE error */
#define SEGV_MTESERR	9	/* Synchronous ARM MTE exception */
#define NSIGSEGV	9
#endif
