#ifndef P2P_CONNECT_INCLUDED
#define P2P_CONNECT_INCLUDED


#include <endianness.c>
#include <network2.c>
#include <data-structures/queue.c>
#include <memory.c>
#include <thread.c>


///////////////// STUN ///////////////////////////////

typedef enum {
	BINDING_REQUEST = 0x0001,
	BINDING_RESPONE = 0x0101,
	BINDING_ERROR_RESPONSE = 0x0111
}
STUN_Message_Type;

typedef enum {
	MAPPED_ADDRESS     = 0x0001,
	XOR_MAPPED_ADDRESS = 0x0020
}
STUN_Attribute_Type;


void write_STUN_message_type(Queue* stun_packet, Number16 type)
{
	type = get_big_endian_Number16(type);
	write_bytes_in_queue(stun_packet, &type, sizeof(type));
}


void write_STUN_message_size(Queue* stun_packet, Number16 size)
{
	size = get_big_endian_Number16(size);
	write_bytes_in_queue(stun_packet, &size, sizeof(size));
}


void write_STUN_cookie(Queue* stun_packet)
{
	Byte cookie[] = {0x21, 0x12, 0xA4, 0x42};
	write_bytes_in_queue(stun_packet, &cookie, sizeof(cookie));
}


void write_STUN_transaction_id(Queue* stun_packet)
{
	Byte i;
	
	for(i = 0; i < 12; ++i) {
		write_bytes_in_queue(stun_packet, &i, sizeof(i)); //random_number() & 0xFF
	}
}


Number16 read_STUN_message_type(Queue* stun_packet)
{
	Number16 type;
	
	read_bytes_from_queue(stun_packet, &type, sizeof(type));
	type = get_big_endian_Number16(type);
	
	return type;
}


Number16 read_STUN_message_size(Queue* stun_packet)
{
	Number16 size;
	
	read_bytes_from_queue(stun_packet, &size, sizeof(size));
	size = get_big_endian_Number16(size);
	
	return size;
}


Number32 read_STUN_cookie(Queue* stun_packet)
{
	Number32 cookie;
	
	read_bytes_from_queue(stun_packet, &cookie, sizeof(cookie));
	
	return cookie;
}


void read_STUN_transaction_id(Queue* stun_packet)
{
	Byte i;
	Byte transaction_id[12];
	
	for(i = 0; i < 12; ++i) {
		read_bytes_from_queue(stun_packet, transaction_id + i, 1);
	}
}


STUN_Attribute_Type read_STUN_attribute_type(Queue* stun_packet)
{
	Number16 type;
	
	read_bytes_from_queue(stun_packet, &type, sizeof(type));
	type = get_big_endian_Number16(type);
	
	return type;
}


Number16 read_STUN_attribute_size(Queue* stun_packet)
{
	Number16 size;
	
	read_bytes_from_queue(stun_packet, &size, sizeof(size));
	size = get_big_endian_Number16(size);
	
	return size;
}


Boolean get_mapped_address(UDP_Connection* connection, Address* mapped_address)
{
	Queue  stun_packet;
	Byte   stun_packet_buffer[512];
	
	Number bytes_writed;
	Number bytes_readed;
	
	clean_bytes(mapped_address, sizeof(Address));
	mapped_address->family = IPv4;
	
	initialize_queue(&stun_packet, stun_packet_buffer, sizeof(stun_packet_buffer));
	
	
	//request
	write_STUN_message_type(&stun_packet, BINDING_REQUEST);
	write_STUN_message_size(&stun_packet, 0);
	write_STUN_cookie(&stun_packet);
	write_STUN_transaction_id(&stun_packet);
	write_bytes_in_UDP_connection(connection, stun_packet.data, stun_packet.size);
	
	
	//response
	for(;;) {
		bytes_readed = read_bytes_from_UDP_connection(connection, stun_packet.data, stun_packet.data_size);
		
		if(bytes_readed) {
			break;
		}
		
		sleep_thread(1);
	}
	
	stun_packet.size = bytes_readed - SIZE_OF_UDP_HEADER;
	stun_packet.start_index = 0;
	
	Number16 type;
	Number16 size;
	
	type = read_STUN_message_type(&stun_packet);
	size = read_STUN_message_size(&stun_packet);
	read_STUN_cookie(&stun_packet);
	read_STUN_transaction_id(&stun_packet);
	
	STUN_Attribute_Type attribute_type;
	Number16            attribute_size;
	
	next_attribute:
	
	attribute_type = read_STUN_attribute_type(&stun_packet);
	attribute_size = read_STUN_attribute_size(&stun_packet);
	
	switch(attribute_type) {
		case MAPPED_ADDRESS:
		case XOR_MAPPED_ADDRESS: {
			if(attribute_size < 8) {
				print_error("размер возвращённого STUN адреса мал");
				goto error;
			}
			
			Byte zero;
			read_bytes_from_queue(&stun_packet, &zero, sizeof(zero));
			
			if(zero) {
				print_error("возвращённый STUN адрес не верен");
				goto error;
			}
			
			Byte ip_type;
			read_bytes_from_queue(&stun_packet, &ip_type, sizeof(ip_type));
			
			if(ip_type != 1) {
				print_error("возвращённый STUN адрес не IPv4");
				goto error;
			}
			
			read_bytes_from_queue(&stun_packet, &mapped_address->port, sizeof(mapped_address->port));
			read_bytes_from_queue(&stun_packet, &mapped_address->ip_v4, sizeof(mapped_address->ip_v4));
			
			break;
		}
		
		default: {
			Byte skip;
			while(attribute_size) {
				read_bytes_from_queue(&stun_packet, &skip, sizeof(skip));
				--attribute_size;
			}
			
			goto next_attribute;
		}
	}
	
	if(attribute_type == XOR_MAPPED_ADDRESS) {
		mapped_address->port ^= get_big_endian_Number16(0x2112);
		mapped_address->ip_v4 ^= get_big_endian_Number32(0x2112A442);
	}
	
	return 1;
	
	error: {
		return 0;
	}
}


///////////////// Connection /////////////////////////

typedef struct {
	UDP_Connection udp_connection;
	Address        mapped_address;
}
P2P_Connection;


Boolean initialize_P2P_connection(
	P2P_Connection* connection,
	Byte*    stun_host,
	Number16 stun_port,
	Boolean (*send_address)(P2P_Connection* connection),
	Boolean (*receive_address)(P2P_Connection* connection)
)
{
	if(!initialize_address(&connection->udp_connection.destination_address, stun_host, stun_port, UDP_PROTOCOL)) {
		print_error("STUN сервер не доступен\n");
		goto error;
	}
	
	if(!initialize_UDP_connection(&connection->udp_connection)) {
		goto error;
	}
	
	get_mapped_address(&connection->udp_connection, &connection->mapped_address);
	
	if(!send_address(connection)) {
		goto error;
	}
	
	if(!receive_address(connection)) {
		goto error;
	}
	
	//ping
	write_bytes_in_UDP_connection(&connection->udp_connection, 0, 0);
	
	return 1;
	
	error: {
		return 0;
	}
}


Signed_Number write_bytes_in_P2P_connection(P2P_Connection* connection, Byte* bytes, Number number_of_bytes)
{
	Signed_Number bytes_writed;
	
	bytes_writed = write_bytes_in_UDP_connection(&connection->udp_connection, bytes, number_of_bytes);
	
	if(bytes_writed < SIZE_OF_UDP_HEADER) {
		return 0;
	}
	
	return bytes_writed - SIZE_OF_UDP_HEADER;
}


Signed_Number read_bytes_from_P2P_connection(P2P_Connection* connection, Byte* bytes, Number number_of_bytes)
{
	Signed_Number bytes_readed;
	
	bytes_readed = read_bytes_from_UDP_connection(&connection->udp_connection, bytes, number_of_bytes);
	
	if(bytes_readed < SIZE_OF_UDP_HEADER) {
		return 0;
	}
	
	return bytes_readed - SIZE_OF_UDP_HEADER;
}


void deinitialize_P2P_connection(P2P_Connection* connection)
{
	deinitialize_UDP_connection(&connection->udp_connection);
}


#endif//P2P_CONNECT_INCLUDED

/*
Boolean send_address_using_stdout(P2P_Connection* connection)
{
	print_address(&connection->mapped_address);
	print_error("\n");
	
	return 1;
}


Number enter_string(Byte* string)
{
	Number character;
	Number string_size;
	
	string_size = 0;
	
	for(;;) {
		character = _getwch();
		_putwch(character);
		//write_character_in_console(get_default_error_writer(), character);
		
		if(character == '\r' || character == '\n') {
			break;
		}
		
		if(character == 27) { //EscapeKey
			exit(1);
		}
		
		string[string_size] = character;
		++string_size;
	}
	
	string[string_size] = '\0';
	++string_size;
	
	return string_size;
}


Boolean receive_address_using_stdin(P2P_Connection* connection)
{
	Byte destination[256];
	
	do {
		print_error("подключиться к: ");
		enter_string(destination);
		print_error("\n\n");
	}
	while(!initialize_address_by_url(&connection->udp_connection.destination_address, destination));
}


Number main(Number number_of_arguments, Byte** arguments)
{
	P2P_Connection connection;
	Signed_Number bytes_readed;
	Signed_Number bytes_writed;
	
	if(
		!initialize_P2P_connection(
			&connection,
			"stun.l.google.com", 19302,
			&send_address_using_stdout,
			&receive_address_using_stdin
		)
	) {
		goto error;
	}
	
	Byte buffer[256];
	
	write_bytes_in_P2P_connection(&connection, "Hi", 3);
	
	for(;;) {
		bytes_readed = read_bytes_from_P2P_connection(&connection, buffer, sizeof(buffer));
		
		if(bytes_readed) {
			break;
		}
		
		sleep_thread(1);
	}
	
	print(buffer);
	
	deinitialize_P2P_connection(&connection);
	
	return 0;
	
	error: {
		return 1;
	}
}*/