#ifndef PACKET_CONNECT_INCLUDED
#define PACKET_CONNECT_INCLUDED


#include <heap.c>
#include <memory.c>


//based on UDP packet size
#define MAX_PACKET_SIZE (576 - 60 - 8)


typedef enum {
	SEND,
	RECEIVED
}
Packet_Type;


typedef struct {
	Number16 type;
	Number16 index;
}
Packet_Head;


typedef struct {
	Packet_Head head;
	Byte        data[MAX_PACKET_SIZE - sizeof(Packet_Head)];
}
Packet;


typedef struct {
	Number  time_after_send;
	Boolean received;
	Number  size;
	Packet  packet;
}
Packet_Queue_Item;


typedef struct {
	Number             max_queue_size;
	Packet_Queue_Item* send_packets_queue;
	Packet_Queue_Item* receive_packets_queue;
	
	Byte* source;
	Signed_Number (*read)(Byte* source, Byte* bytes, Number number_of_bytes);
	Signed_Number (*write)(Byte* source, Byte* bytes, Number number_of_bytes);
	
	Byte* arguments[4];
}
Packet_Connection;


void initialize_packet_connection(
	Packet_Connection* connection,
	Number max_queue_size,
	Byte* source,
	Signed_Number (*read)(Byte* source, Byte* bytes, Number number_of_bytes),
	Signed_Number (*write)(Byte* source, Byte* bytes, Number number_of_bytes)
)
{
	Number i;
	
	connection->max_queue_size = max_queue_size;
	connection->send_packets_queue = allocate_memory(max_queue_size * sizeof(Packet_Queue_Item));
	connection->receive_packets_queue = allocate_memory(max_queue_size * sizeof(Packet_Queue_Item));
	
	for(i = 0; i < max_queue_size; ++i) {
		connection->send_packets_queue[i].packet.head.type = SEND;
		connection->send_packets_queue[i].packet.head.index = i;
		connection->send_packets_queue[i].received = 0;
		connection->send_packets_queue[i].time_after_send = 100;
		
		connection->receive_packets_queue[i].packet.head.index = i;
		connection->receive_packets_queue[i].received = 0;
	}
	
	connection->source = source;
	connection->read = read;
	connection->write = write;
}


void deinitialize_packet_connection(Packet_Connection* connection)
{
	free_memory(connection->send_packets_queue);
	free_memory(connection->receive_packets_queue);
}


Signed_Number receive_packet(Packet_Connection* connection)
{
	Packet        packet;
	Number        packet_index;
	Packet_Head   response_packet;
	Signed_Number bytes_received;
	
	bytes_received = connection->read(connection->source, &packet, sizeof(packet));
	
	if(bytes_received <= 0) {
		return bytes_received;
	}
	
	if(bytes_received < sizeof(Packet_Head)) {
		return 0;
	}
	
	packet_index = packet.head.index % connection->max_queue_size;
	
	if(packet.head.type == SEND) {
		if(packet.head.index == connection->receive_packets_queue[packet_index].packet.head.index) {
			response_packet.type = RECEIVED;
			response_packet.index = packet.head.index;
			//print_error("отправка подтверждения пакета %d\n", response_packet.index);
			connection->write(connection->source, &response_packet, sizeof(response_packet));
			
			connection->receive_packets_queue[packet_index].received = 1;
			connection->receive_packets_queue[packet_index].size = bytes_received - sizeof(Packet_Head);
			connection->receive_packets_queue[packet_index].packet = packet;
		}
		else {
			response_packet.type = RECEIVED;
			response_packet.index = packet.head.index;
			//print_error("повторная отправка подтверждения пакета %d\n", response_packet.index);
			connection->write(connection->source, &response_packet, sizeof(response_packet));
		}
	}
	else if(packet.head.type == RECEIVED) {
		if(packet.head.index == connection->send_packets_queue[packet_index].packet.head.index) {
			connection->send_packets_queue[packet_index].received = 1;
		}
	}
	
	return bytes_received;
}


//TODO: min size of bytes is max_queue_size * (MAX_PACKET_SIZE - sizeof(Packet_Head))
Signed_Number read_bytes_from_packet_connection(Packet_Connection* connection, Byte* bytes, Number number_of_bytes)
{
	Signed_Number bytes_readed;
	Boolean       all_packets_received;
	Number        i;
	
	receive_packet(connection);
	
	all_packets_received = 1;
	for(i = 0; i < connection->max_queue_size; ++i) {
		if(!connection->receive_packets_queue[i].received) {
			all_packets_received = 0;
			break;
		}
	}
	
	bytes_readed = 0;
	
	if(all_packets_received) {
		for(i = 0; i < connection->max_queue_size; ++i) {
			copy_bytes(bytes, connection->receive_packets_queue[i].packet.data, connection->receive_packets_queue[i].size);
			bytes += connection->receive_packets_queue[i].size;
			bytes_readed += connection->receive_packets_queue[i].size;
		}
		
		for(i = 0; i < connection->max_queue_size; ++i) {
			connection->receive_packets_queue[i].received = 0;
			connection->receive_packets_queue[i].packet.head.index += connection->max_queue_size;
		}
	}
	
	return bytes_readed;
}


Signed_Number write_bytes_in_packet_connection(Packet_Connection* connection, Byte* bytes, Number number_of_bytes)
{
	Signed_Number bytes_writed;
	Boolean       all_packets_sended;
	Number        i;
	
	receive_packet(connection);
	
	all_packets_sended = 1;
	for(i = 0; i < connection->max_queue_size; ++i) {
		if(!connection->send_packets_queue[i].received) {
			all_packets_sended = 0;
			break;
		}
	}
	
	for(i = 0; i < connection->max_queue_size; ++i) {
		if(number_of_bytes < sizeof(connection->send_packets_queue[i].packet.data)) {
			connection->send_packets_queue[i].size = number_of_bytes;
		}
		else {
			connection->send_packets_queue[i].size = sizeof(connection->send_packets_queue[i].packet.data);
		}
		
		copy_bytes(connection->send_packets_queue[i].packet.data, bytes, connection->send_packets_queue[i].size);
		number_of_bytes -= connection->send_packets_queue[i].size;
		bytes += connection->send_packets_queue[i].size;
		
		
		if(!connection->send_packets_queue[i].received) {
			if(connection->send_packets_queue[i].time_after_send >= 100) {
				connection->write(connection->source, &connection->send_packets_queue[i].packet, sizeof(Packet_Head) + connection->send_packets_queue[i].size);
				connection->send_packets_queue[i].time_after_send = 0;
			}
			
			++connection->send_packets_queue[i].time_after_send;
		}
	}
	
	bytes_writed = 0;
	
	if(all_packets_sended) {
		for(i = 0; i < connection->max_queue_size; ++i) {
			bytes_writed += connection->send_packets_queue[i].size;
		}
		
		for(i = 0; i < connection->max_queue_size; ++i) {
			connection->send_packets_queue[i].received = 0;
			connection->send_packets_queue[i].packet.head.index += connection->max_queue_size;
			connection->send_packets_queue[i].time_after_send = 100;
		}
	}
	
	return bytes_writed;
}


#endif//PACKET_CONNECT_INCLUDED

/*
#include <network/p2p-connect2.c>


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
	P2P_Connection    p2p_connection;
	Signed_Number     bytes_readed;
	Signed_Number     bytes_writed;
	Packet_Connection packet_connection;
	
	if(
		!initialize_P2P_connection(
			&p2p_connection,
			"stun.l.google.com", 19302,
			&send_address_using_stdout,
			&receive_address_using_stdin
		)
	) {
		goto error;
	}
	
	initialize_packet_connection(&packet_connection, 2, &p2p_connection, &read_bytes_from_P2P_connection, &write_bytes_in_P2P_connection);
	
	for(;;) {
		for(;;) {
			bytes_writed = write_bytes_in_packet_connection(&packet_connection, "Hi", 3);
			
			if(bytes_writed) {
				break;
			}
			
			sleep_thread(1);
		}
		
		Byte buffer[2 * (MAX_PACKET_SIZE - sizeof(Packet_Head))];
		
		for(;;) {
			bytes_readed = read_bytes_from_packet_connection(&packet_connection, buffer, sizeof(buffer));
			
			if(bytes_readed) {
				break;
			}
			
			sleep_thread(1);
		}
		
		print(buffer);
	}
	
	deinitialize_packet_connection(&packet_connection);
	deinitialize_P2P_connection(&p2p_connection);
	
	return 0;
	
	error: {
		return 1;
	}
}*/