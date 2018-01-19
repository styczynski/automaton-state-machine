/** @file
*  Various utilities. (C99 standard)
*/
#ifndef __UTILS_H__
#define __UTILS_H__

#ifndef DEBUG
/**
* @def DEBUG
* Debug symbol definitions
*
* Use DEBUG 0 to disable debug macro DBG
* Use DEBUG 1 to enable debug macro DBG
*/
#define DEBUG 0
#endif // DEBUG

#ifndef DBG
/**
* @def DBG
* Macro used for debug purposes
* The code block after it will be executed
* only if DEBUG macro was set to 1
*
* Works as if clause e.g.
* @code
* DBG {
*   printf("Hello");
* }
* @endcode
*/
#define DBG if(DEBUG)
#endif // DBG

#endif // __UTILS_H__