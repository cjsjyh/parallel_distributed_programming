/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <memory.h> /* for memset */
#include "calculator.h"

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

custom_response *
calculate_1(custom_string *argp, CLIENT *clnt)
{
	static custom_response clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, CALCULATE,
		(xdrproc_t) xdr_custom_string, (caddr_t) argp,
		(xdrproc_t) xdr_custom_response, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}
