#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080

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

void generate_transaction_id(uint8_t *transaction_id) {
	for (int i = 0; i < 12; i++) {
		transaction_id[i] = rand() % 256;
	}
}

int main() {
	srand(time(NULL));

	// Create a Socket
	int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (server_fd < 0) {
		perror("Failed to open socket");
		return 1;
	}

	// Prepare server address
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

	// Preparing request
	stun_header_t request = {0};
	request.type = htons(STUN_BINDING_REQUEST);
	request.magic_cookie = htonl(0x2112A442);
	generate_transaction_id(request.transaction_id);
	request.length = htons(0);

	// Sending the request
	ssize_t sent = sendto(server_fd, &request, sizeof(request), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if (sent < 0) {
		perror("Failed to send data");
	}

	// Receive the response
	struct sockaddr_in from_addr;
	socklen_t addr_len = sizeof(from_addr);

#define BUFFER_SIZE 1024
	uint8_t buffer[BUFFER_SIZE];

	ssize_t received = recvfrom(server_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&from_addr, &addr_len);
	if (received < 0) {
		perror("Failed to receive response");
		return 1;
	}

	// Extract mapped address from response
	stun_mapped_address_t* mapped_address = (stun_mapped_address_t*)(buffer + sizeof(stun_header_t));

	char public_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &mapped_address->address, public_ip, INET_ADDRSTRLEN);
	int public_port = htons(mapped_address->port);

	printf("Your Public IP: %s, Public Port: %d\n", public_ip, public_port);

	close(server_fd);
	
	return 0;
}
