#ifndef NETWORK_INCLUDED
#define NETWORK_INCLUDED


#ifdef __WIN32__


#include <WinAPI/ws2_32.c>
#include <endianness.c>
#include <memory.c>
#include <reader.c>


static Windows_Sockets windows_sockets = {0};


Boolean initialize_Windows_sockets()
{
	if(!windows_sockets.version) {
		return !WSAStartup(0x0202, &windows_sockets);
	}

	return 1;
}


void deinitialize_Windows_sockets()
{
	if(windows_sockets.version) {
		WSACleanup();
	}

	windows_sockets.version = 0;
}


#define ANY_INTERNET_PROTOCOL     0
#define IPv4                      2
#define IPv6                      23

#define SOCKET_STREAM             1
#define SOCKET_DATAGRAMS          2

typedef enum {
	TCP_PROTOCOL = 6,
	UDP_PROTOCOL = 17
}
Network_Protocol;


typedef struct {
	Byte     family;
	Byte     zero;
	Number16 port;

	union {
		Number32 ip_v4;
		Number32 ip_v6_flow_info;
	};

	Byte ip_v6[16];

	union {
		Number32 ip_v6_scope_id;

		union {
			struct {
				Number32 zone: 28;
				Number32 level: 4;
			};

			Number32 value;
		};
	};
}
Address;


Boolean initialize_address(Address* address, Byte* host, Number16 port, Network_Protocol protocol)
{
	Address_Info* address_info;
	Address_Info  hints;
	Byte          port_string[6];

	if(!initialize_Windows_sockets()) {
		goto error;
	}

	clean_bytes(&hints, sizeof(Address_Info));

	if(protocol == UDP_PROTOCOL) {
		hints.family      = IPv4;
		hints.socket_type = SOCKET_DATAGRAMS;
		hints.protocol    = UDP_PROTOCOL;
	}
	else if(protocol == TCP_PROTOCOL) {
		hints.family      = IPv4;
		hints.socket_type = SOCKET_STREAM;
		hints.protocol    = TCP_PROTOCOL;
	}
	else {
		goto error;
	}
	
	
	Byte* port_source = port_string;
	write_Number(&port_source, &write_character_in_bytes, port);
	write_character_in_bytes(&port_source, '\0');

	if(getaddrinfo(host, port_string, &hints, &address_info)) {
		goto error;
	}

	clean_bytes(address, sizeof(Address));
	copy_bytes(address, address_info->address, address_info->address_length);

	freeaddrinfo(address_info);

	return 1;

	error: {
		return 0;
	}
}


Boolean initialize_address_by_url(Address* address, Byte* url)
{
	Number i;
	Byte*  ip;
	Reader reader;
	Byte   reader_buffer[64];
	
	initialize_reader(&reader, &url, &read_from_bytes, reader_buffer, sizeof(reader_buffer));
	clean_bytes(address, sizeof(Address));
	
	address->family = IPv4;
	
	ip = &address->ip_v4;
	
	for(i = 0; i < 4; ++i) {
		ip[i] = read_Number(&reader);
		
		if(!read_if(&reader, ".")) {
			if(i != 3 || !read_if(&reader, ":")) {
				goto error;
			}
		}
	}
	
	address->port = read_Number(&reader);
	address->port = get_big_endian_Number16(address->port);

	return 1;

	error: {
		return 0;
	}
}


void print_address(Address* address)
{
	print_error("%d.%d.%d.%d:%d",
		address->ip_v4 & 0xFF,
		(address->ip_v4 >> 8) & 0xFF,
		(address->ip_v4 >> 16) & 0xFF,
		(address->ip_v4 >> 24) & 0xFF,
		((address->port >> 8) & 0xFF) | ((address->port << 8) & 0xFF00)
	);
}




Socket create_socket(Number32 family, Number32 type, Number32 protocol)
{
	if(!initialize_Windows_sockets()) {
		goto error;
	}
	
	Socket socket_index;
	
	socket_index = socket(family, type, protocol);
	
	if(socket_index == -1) {
		goto error;
	}
	
	return socket_index;
	
	error: {
		return -1;
	}
}




typedef struct {
	Address destination_address;
	Socket  socket;
}
UDP_Connection;


#define SIZE_OF_UDP_HEADER 8


Boolean initialize_UDP_connection(UDP_Connection* connection)
{
	connection->socket = create_socket(IPv4, SOCKET_DATAGRAMS, UDP_PROTOCOL);
	
	if(connection->socket == -1) {
		goto error;
	}
	
	Number32 async = 1;
	ioctlsocket(connection->socket, WS_NON_BLOCKING_MODE, &async);
	
	return 1;
	
	error: {
		return 0;
	}
}


Signed_Number write_bytes_in_UDP_connection(UDP_Connection* connection, Byte* bytes, Number number_of_bytes)
{
	Signed_Number32 bytes_sended;
	
	bytes_sended = sendto(connection->socket, bytes, number_of_bytes, 0, &connection->destination_address, sizeof(Address));

	if(bytes_sended < 0) {
		if(WSAGetLastError() == WS_RESOURCE_BLOCKED) {
			bytes_sended = 0;
		}
	}
	else {
		bytes_sended += SIZE_OF_UDP_HEADER;
	}
	
	return bytes_sended;
}


Signed_Number read_bytes_from_UDP_connection(UDP_Connection* connection, Byte* bytes, Number number_of_bytes)
{
	Signed_Number32 bytes_received;
	Number32        sizeof_address;
	
	sizeof_address = sizeof(Address);
	bytes_received = recvfrom(connection->socket, bytes, number_of_bytes, 0, &connection->destination_address, &sizeof_address);
	
	if(bytes_received < 0) {
		if(WSAGetLastError() == WS_RESOURCE_BLOCKED) {
			bytes_received = 0;
		}
	}
	else {
		bytes_received += SIZE_OF_UDP_HEADER;
	}
	
	return bytes_received;
}


void deinitialize_UDP_connection(UDP_Connection* connection)
{
	closesocket(connection->socket);
}




typedef struct {
	Address address;
	Address destination_address;
	Socket  socket;
}
UDP_Server;


Boolean initialize_UDP_server(UDP_Server* server)
{
	server->socket = create_socket(IPv4, SOCKET_DATAGRAMS, UDP_PROTOCOL);
	
	if(server->socket == -1) {
		goto error;
	}
	
	if(bind(server->socket, &server->address, sizeof(Address))) {
		goto error;
	}
	
	Number32 async = 1;
	ioctlsocket(server->socket, WS_NON_BLOCKING_MODE, &async);
	
	return 1;
	
	error: {
		return 0;
	}
}

/*
Signed_Number read_client_connection_from_UDP_server(UDP_Server* server, UDP_Connection* client_connection)
{
	Signed_Number32 bytes_received;
	Number32        sizeof_address;
	
	sizeof_address = sizeof(Address);
	bytes_received = recvfrom(server->socket, bytes, number_of_bytes, 0, &server->destination_address, &sizeof_address);
	
	if(bytes_received < 0) {
		if(WSAGetLastError() == WS_RESOURCE_BLOCKED) {
			bytes_received = 0;
		}
	}
	else {
		bytes_received += SIZE_OF_UDP_HEADER;
	}
	
	client_connection->socket = server->socket;
	
	return bytes_received;
}*/


Signed_Number write_bytes_in_UDP_server(UDP_Server* server, Byte* bytes, Number number_of_bytes)
{
	Signed_Number32 bytes_sended;
	
	bytes_sended = sendto(server->socket, bytes, number_of_bytes, 0, &server->destination_address, sizeof(Address));

	if(bytes_sended < 0) {
		if(WSAGetLastError() == WS_RESOURCE_BLOCKED) {
			bytes_sended = 0;
		}
	}
	else {
		bytes_sended += SIZE_OF_UDP_HEADER;
	}
	
	return bytes_sended;
}


Signed_Number read_bytes_from_UDP_server(UDP_Server* server, Byte* bytes, Number number_of_bytes)
{
	Signed_Number32 bytes_received;
	Number32        sizeof_address;
	
	sizeof_address = sizeof(Address);
	bytes_received = recvfrom(server->socket, bytes, number_of_bytes, 0, &server->destination_address, &sizeof_address);
	
	if(bytes_received < 0) {
		if(WSAGetLastError() == WS_RESOURCE_BLOCKED) {
			bytes_received = 0;
		}
	}
	else {
		bytes_received += SIZE_OF_UDP_HEADER;
	}
	
	return bytes_received;
}


void deinitialize_UDP_server(UDP_Server* server)
{
	closesocket(server->socket);
}




typedef struct {
	Address destination_address;
	Socket  socket;
}
TCP_Connection;


//Boolean initialize_TCP_connection(TCP_Connection* connection, Address* destination_address)
Boolean initialize_TCP_connection(TCP_Connection* connection)
{
	connection->socket = create_socket(IPv4, SOCKET_STREAM, TCP_PROTOCOL);
	
	if(connection->socket == -1) {
		goto error;
	}
	
	if(connect(connection->socket, &connection->destination_address, sizeof(Address))) {
		closesocket(connection->socket);
		goto error;
	}
	
	Number32 async = 1;
	ioctlsocket(connection->socket, WS_NON_BLOCKING_MODE, &async);
	
	return 1;
	
	error: {
		return 0;
	}
}


Signed_Number write_bytes_in_TCP_connection(TCP_Connection* connection, Byte* bytes, Number number_of_bytes)
{
	Signed_Number bytes_writed;
	Number32      error_code;
	
	bytes_writed = send(connection->socket, bytes, number_of_bytes, 0);
	
	if(bytes_writed < 0) {
		if(WSAGetLastError() == WS_RESOURCE_BLOCKED) {
			bytes_writed = 0;
		}
	}
	
	return bytes_writed;
}


Signed_Number read_bytes_from_TCP_connection(TCP_Connection* connection, Byte* bytes, Number number_of_bytes)
{
	Signed_Number bytes_readed;
	
	bytes_readed = recv(connection->socket, bytes, number_of_bytes, 0);
	
	if(bytes_readed < 0) {
		if(WSAGetLastError() == WS_RESOURCE_BLOCKED) {
			bytes_readed = 0;
		}
	}
	
	return bytes_readed;
}


void deinitialize_TCP_connection(TCP_Connection* connection)
{
	closesocket(connection->socket);
}




typedef struct {
	Address address;
	Socket  socket;
}
TCP_Server;


//Boolean initialize_TCP_server(TCP_Server* server, Address* address)
Boolean initialize_TCP_server(TCP_Server* server)
{
	server->socket = create_socket(IPv4, SOCKET_STREAM, TCP_PROTOCOL);
	
	if(server->socket == -1) {
		goto error;
	}
	
	if(bind(server->socket, &server->address, sizeof(Address)) == -1) {
		goto error_with_closesocket;
	}
	
	if(listen(server->socket, 10000)) {
		goto error_with_closesocket;
	}
	
	Number32 async = 1;
	ioctlsocket(server->socket, WS_NON_BLOCKING_MODE, &async);
	
	return 1;
	
	error_with_closesocket:{
		closesocket(server->socket);
	}
	
	error: {
		return 0;
	}
}


Signed_Number read_client_connection_from_TCP_server(TCP_Server* server, TCP_Connection* client_connection)
{
	Signed_Number connections_readed;
	Number32      sizeof_address;
	
	sizeof_address = sizeof(Address);
	client_connection->socket = accept(server->socket, &client_connection->destination_address, &sizeof_address);
	
	if(client_connection->socket == -1) {
		if(WSAGetLastError() == WS_RESOURCE_BLOCKED) {
			connections_readed = 0;
		}
		else {
			goto error;
		}
	}
	else {
		connections_readed = 1;
	}
	
	return connections_readed;
	
	error: {
		return 0;
	}
}


void deinitialize_TCP_server(TCP_Server* server)
{
	closesocket(server->socket);
}


#endif//__WIN32__


#endif//NETWORK_INCLUDED

/*
Number main(Number number_of_arguments, Byte** arguments)
{
	UDP_Connection connection;
	
	//if(!initialize_address(&connection.destination_address, "stun.l.google.com", 19302, UDP_PROTOCOL)) {
	if(!initialize_address(&connection.destination_address, "localhost", 8080, UDP_PROTOCOL)) {
		goto error;
	}
	
	if(!initialize_UDP_connection(&connection)) {
		goto error;
	}
	
	Number bytes_writed;
	Number bytes_readed;
	
	Byte request[] = {
		0x00, 0x01, 0x00, 0x00, 0x21, 0x12, 0xA4, 0x42,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x0B
	};
	do {
		bytes_writed = write_bytes_in_UDP_connection(&connection, request, sizeof(request));
	}
	while(bytes_writed == 0);
	
	Byte response[512];
	do {
		bytes_readed = read_bytes_from_UDP_connection(&connection, response, sizeof(response));
	}
	while(bytes_readed == 0);
	
	print("w%d, r%d\n", bytes_writed, bytes_readed);
	
	print_address(&connection.destination_address);
	print_error("\n");
	
	return 0;

	error: {
		return 1;
	}
}*/

/*
Number main(Number number_of_arguments, Byte** arguments)
{
	UDP_Server server;
	
	if(!initialize_address(&server.address, "localhost", 8080, UDP_PROTOCOL)) {
		goto error;
	}
	
	if(!initialize_UDP_server(&server)) {
		goto error;
	}
	
	Number bytes_writed;
	Number bytes_readed;
	
	Byte request[512];
	
	do {
		bytes_readed = read_bytes_from_UDP_server(&server, request, sizeof(request));
	}
	while(!bytes_readed);
	
	do {
		bytes_writed = write_bytes_in_UDP_server(&server, "Hi", 3);
	}
	while(!bytes_writed);
	
	print("w%d, r%d\n", bytes_writed, bytes_readed);
	
	print_address(&server.destination_address);
	print_error("\n");
	
	return 0;

	error: {
		return 1;
	}
}*/

/*
Number main(Number number_of_arguments, Byte** arguments)
{
	TCP_Server     server;
	TCP_Connection server_client;
	Signed_Number  connections_readed;
	
	if(!initialize_address(&server.address, "localhost", 8080, TCP_PROTOCOL)) {
		goto error;
	}
	
	if(!initialize_TCP_server(&server)) {
		goto error;
	}
	
	do {
		connections_readed = read_client_connection_from_TCP_server(&server, &server_client);
		
		if(connections_readed < 0) {
			goto error;
		}
	}
	while(!connections_readed);
	
	print_address(&server_client.destination_address);
	print_error("\n");
	
	
	Number bytes_writed;
	Number bytes_readed;
	
	Byte request[512];
	
	do {
		bytes_readed = read_bytes_from_TCP_connection(&server_client, request, sizeof(request));
	}
	while(!bytes_readed);
	
	print("%s", request);
	
	return 0;

	error: {
		return 1;
	}
}*/