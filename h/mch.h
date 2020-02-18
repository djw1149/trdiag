/*  file: mch.h

    synopsis: this is the tools machine dependant header file, and
	      it's sole purpose in life is to keep byte swapping straight

    modification history:

      Jul-86 Chriss Stephens (chriss) at Carnegie Mellon University created

 */

#ifdef vax
#define LSBFIRST
#endif vax

#ifdef ibm032
#define MSBFIRST
#endif ibm032

#ifdef sun
#define MSBFIRST
#define INTISLONG
#endif sun

