/* Loading arbitrary modules using crypto api since v2.6.38
 *
 * - minipli
 */
#include <linux/if_alg.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifndef AF_ALG
#define AF_ALG 38
#endif


int main(int argc, char **argv) {
	struct sockaddr_alg sa_alg = {
		.salg_family = AF_ALG,
		.salg_type = "hash",
	};
	int sock;
	if (argc != 2) {
		printf("usage: %s MODULE_NAME\n", argv[0]);
		exit(1);
	}
	sock = socket(AF_ALG, SOCK_SEQPACKET, 0);
	if (sock < 0) {
		perror("socket(AF_ALG)");
		exit(1);
	}
	strncpy((char *) sa_alg.salg_name, argv[1], sizeof(sa_alg.salg_name));
	bind(sock, (struct sockaddr *) &sa_alg, sizeof(sa_alg));
	close(sock);

	return 0;
}
