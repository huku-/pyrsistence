#ifndef _INCLUDES_H_
#define _INCLUDES_H_

#include <Python.h>
#include <structmember.h>
#include <marshal.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <Windows.h>
#include <Strsafe.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

#include <fcntl.h>
#include <errno.h>

#endif /* _INCLUDES_H_ */

