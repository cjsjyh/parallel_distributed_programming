/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include "calculator.h"


void
cal_prog_1(char *host)
{
	CLIENT *clnt;
	custom_response  *result_1;
	custom_string  calculate_1_arg;

  /* Custom Code */
  char input[100];
  scanf("%s", input);
  calculate_1_arg.str.str_val = input;
  calculate_1_arg.str.str_len = strlen(calculate_1_arg.str.str_val);
  /* ----------- */

#ifndef	DEBUG
	clnt = clnt_create (host, CAL_PROG, CAL_VERS, "udp");
	if (clnt == NULL) {
		clnt_pcreateerror (host);
		exit (1);
	}
#endif	/* DEBUG */

	result_1 = calculate_1(&calculate_1_arg, clnt);
	if (result_1 == (custom_response *) NULL) {
		clnt_perror (clnt, "call failed");
	}
  if (result_1->is_valid == TRUE)
    printf("The answer is %d\n", result_1->result);
  else
    printf("Invalid operation\n");
#ifndef	DEBUG
	clnt_destroy (clnt);
#endif	 /* DEBUG */
}


int
main (int argc, char *argv[])
{
	char *host;

	if (argc < 2) {
		printf ("usage: %s server_host\n", argv[0]);
		exit (1);
	}
	host = argv[1];
	cal_prog_1 (host);
exit (0);
}
