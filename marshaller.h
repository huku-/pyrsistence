#ifndef _MARSHALLER_H_
#define _MARSHALLER_H_

#include "includes.h"

int marshaller_init(void);
PyObject *marshal(PyObject *);
PyObject *unmarshal(PyObject *);
void marshaller_fini(void);

#endif /* _MARSHALLER_H_ */
