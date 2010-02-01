/*
 * config.h -- configure various defines for tcsh
 *
 * All source files should #include this FIRST.
 *
 * This is the config file for Linux systems
 */

#ifndef _h_config
#define _h_config
/****************** System dependant compilation flags ****************/
/*
 * POSIX	This system supports IEEE Std 1003.1-1988 (POSIX).
 */
#define POSIX

/*
 * POSIXJOBS	This system supports the optional IEEE Std 1003.1-1988 (POSIX)
 *		job control facilities.
 */
#define POSIXJOBS

/*
 * POSIXSIGS	Use the POSIX signal facilities to emulate BSD signals.
 */
#undef POSIXSIGS

/*
 * VFORK	This machine has a vfork().  
 *		It used to be that for job control to work, this define
 *		was mandatory. This is not the case any more.
 *		If you think you still need it, but you don't have vfork, 
 *		define this anyway and then do #define vfork fork.  
 *		I do this anyway on a Sun because of yellow pages brain damage,
 *		[should not be needed under 4.1]
 *		and on the iris4d cause	SGI's fork is sufficiently "virtual" 
 *		that vfork isn't necessary.  (Besides, SGI's vfork is weird).
 *		Note that some machines eg. rs6000 have a vfork, but not
 *		with the berkeley semantics, so we cannot use it there either.
 */
#undef VFORK

/*
 * BSDJOBS	You have BSD-style job control (both process groups and
 *		a tty that deals correctly
 */
#define BSDJOBS

/*
 * BSDSIGS	You have 4.2-style signals, rather than USG style.
 *		Note: POSIX systems should not define this unless they
 *		have sigvec() and friends (ie: 4.3BSD-RENO, HP-UX).
 */
#undef BSDSIGS

/*
 * BSDTIMES	You have BSD-style process time stuff (like rusage)
 *		This may or may not be true.  For example, Apple Unix
 *		(OREO) has BSDJOBS and BSDSIGS but not BSDTIMES.
 */
#define BSDTIMES

/*
 * BSDLIMIT	You have BSD-style resource limit stuff (getrlimit/setrlimit)
 */
#define BSDLIMIT

/*
 * TERMIO	You have struct termio instead of struct sgttyb.
 * 		This is usually the case for SVID systems, where
 *		BSD uses sgttyb. POSIX systems should define this
 *		anyway, even though they use struct termios.
 */
#define TERMIO

/*
 * SYSVREL	Your machine is SYSV based (HPUX, A/UX)
 *		NOTE: don't do this if you are on a Pyramid -- tcsh is
 *		built in a BSD universe.
 *		Set SYSVREL to 1, 2, 3, or 4, depending the version of System V
 *		you are running. Or set it to 0 if you are not SYSV based
 *
 *		Note: Linux should work with any SYSVREL < 3.
 */
#define SYSVREL	0

/*
 * YPBUGS	Work around Sun YP bugs that cause expansion of ~username
 *		to send command output to /dev/null
 */
#undef YPBUGS

/*
 * Get the name space we want.
 *
 * The more recent defaults for gcc (e.g. on Red Hat 7.0)
 * also define _POSIX_C_SOURCE, which throws our code off.
 */

#define __STRICT_ANSI__
#define _BSD_SOURCE
#define _SVID_SOURCE
#define _POSIX_SOURCE
#define _XOPEN_SOURCE 500
#define _GNU_SOURCE

/*
 * Large file support from <features.h>
 */

#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

/****************** local defines *********************/
#ifndef _PATH_TCSHELL
#define _PATH_TCSHELL "/bin/tcsh"
#endif
#define ECHO_STYLE      BOTH_ECHO

#define NLS_CATALOGS

#endif /* _h_config */
