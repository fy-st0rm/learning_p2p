// https://datatracker.ietf.org/doc/html/rfc5389#section-6

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define IPV4 0x01

// STUN message types
#define STUN_BINDING_REQUEST 0x0001
#define STUN_BINDING_RESPONSE 0x0101

// STUN attributes
#define STUN_ATTR_MAPPED_ADDRESS 0x0001

// STUN header structure
typedef struct {
	uint16_t type;
	uint16_t length;
	uint32_t magic_cookie;
	uint8_t transaction_id[12];
} __attribute__((packed)) stun_header_t;

// STUN MAPPED-ADDRESS attribute structure
typedef struct {
	uint16_t type;
	uint16_t length;
	uint8_t  unused;
	uint8_t  family;
	uint16_t port;
	uint32_t address;
} __attribute__((packed)) stun_mapped_address_t;

int main() {
	// Create a Socket
	int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (server_fd < 0) {
		perror("Failed to open socket");
		return 1;
	}

	// Prepare server address
	int port = getenv("PORT") ? atoi(getenv("PORT")) : 8080;

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	// Binding the socket
	if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("Failed to bind socket");
		return 1;
	}

	printf("Server binded on port: %d\n", port);

	while (1) {
		struct sockaddr_in client_addr;
		unsigned int addr_len = sizeof(server_addr);
		char buffer[1024];

		ssize_t received = recvfrom(server_fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &addr_len);
		if (received < 0) {
			perror("Failed to receive data");
			continue;
		}

		// Parsing the header
		stun_header_t* request = (stun_header_t*) buffer;
		if (ntohs(request->type) != STUN_BINDING_REQUEST) {
			fprintf(stderr, "Received a non-binding request.\n");
			continue;
		}

		printf("Received request from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		// Preparing response
		stun_header_t response = {0};
		response.type = htons(STUN_BINDING_RESPONSE);
		response.magic_cookie = htonl(0x2112A442);
		memcpy(response.transaction_id, request->transaction_id, 12);

		// Preparing mapped address
		stun_mapped_address_t mapped_address = {0};
		mapped_address.type = htons(STUN_ATTR_MAPPED_ADDRESS);
		mapped_address.length = htons(8);
		mapped_address.unused = htons(0);
		mapped_address.family = IPV4;
		mapped_address.port = client_addr.sin_port;
		mapped_address.address = client_addr.sin_addr.s_addr;

		// Setting up response length
		response.length = htons(sizeof(mapped_address));

		// Sending the response
		uint8_t response_buffer[sizeof(response) + sizeof(mapped_address)];
		memcpy(response_buffer, &response, sizeof(response));
		memcpy(response_buffer + sizeof(response), &mapped_address, sizeof(mapped_address));

		sendto(server_fd, response_buffer, sizeof(response_buffer), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
	}

	close(server_fd);
	
	return 0;
}
