#ifndef _MARSHALLER_H_
#define _MARSHALLER_H_

#include "includes.h"
#include "common.h"

int marshaller_init(void);
PyObject *marshal(em_common_t *, PyObject *);
PyObject *unmarshal(em_common_t *, PyObject *);
void marshaller_fini(void);

#endif /* _MARSHALLER_H_ */
