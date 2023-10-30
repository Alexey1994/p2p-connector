#include <network/p2p-connect2.c>
#include <network/packet-connect3.c>
#include <thread.c>
#include <file.c>


#define QUEUE_SIZE 32


Packet_Connection address_connection;

Address           proxy_address;
TCP_Server        proxy_server;
TCP_Connection    client_tcp_connection;

Byte*   connect_to;

Byte*   read_pipe = 0;
Byte*   write_pipe = 0;


Number32 execute(Byte* command)
{
	Number16*           unicode_command;
	Number              status;
	Process_Information process_info;
	Sturtup_Info        startup_info = {.size = sizeof(Sturtup_Info)};
	Number32            exit_code;
	
	unicode_command = convert_utf8_to_unicode(command);
	status = CreateProcessW(0, unicode_command, 0, 0, 0, 0, 0, 0, &startup_info, &process_info);
	free_memory(unicode_command);
	
	if(!status) {
		return 1;
	}
	
	WaitForSingleObject(process_info.process, 0xFFFFFFFF);
	GetExitCodeProcess(process_info.process, &exit_code);
	
	return exit_code;
}


Byte* start_process(Byte* command, Sturtup_Info* info)
{
	Number16*           unicode_command;
	Number              status;
	Process_Information process_info;
	
	unicode_command = convert_utf8_to_unicode(command);
	status = CreateProcessW(0, unicode_command, 0, 0, 1, 0, 0, 0, info, &process_info);
	free_memory(unicode_command);
	
	return process_info.process;
}


Boolean start_receiving_using_mail(P2P_Connection* connection)
{
	Byte command[256];
	
	SECURITY_ATTRIBUTES security_attributes = {
		.length = sizeof(SECURITY_ATTRIBUTES),
		.security_descriptor = 0,
		.inherit_handle = 1
	};
	
	if(!CreatePipe(&read_pipe, &write_pipe, &security_attributes, 0)) {
		print_error("pipe not created\n");
	}
	
	print("receiving from %s\n", connect_to);
	
	print_in_string(command,
		"receive-address.bat "
		"%s "
		"%d",
		
		connect_to,
		connection->udp_connection.socket
	);
	
	Sturtup_Info info = {
		.size = sizeof(Sturtup_Info),
		.flags = STARTF_USESTDHANDLES,
		.std_out = write_pipe,
	};
	Byte* receive_process = start_process(command, &info);
}


Boolean send_address_using_mail(P2P_Connection* connection)
{
	start_receiving_using_mail(connection);
	
	Byte command[256];
	
	print_in_string(command,
		"send-address.bat "
		"%s "
		"%d.%d.%d.%d:%d "
		"%d",
		
		connect_to,
		
		connection->mapped_address.ip_v4 & 0xFF,
		(connection->mapped_address.ip_v4 >> 8) & 0xFF,
		(connection->mapped_address.ip_v4 >> 16) & 0xFF,
		(connection->mapped_address.ip_v4 >> 24) & 0xFF,
		((connection->mapped_address.port >> 8) & 0xFF) | ((connection->mapped_address.port << 8) & 0xFF00),
		
		connection->udp_connection.socket
	);
	execute(command);
	
	return 1;
}


Boolean receive_address_using_mail(P2P_Connection* connection)
{
	Byte   destination[256];
	Byte   c;
	Number i;
	
	read_bytes_from_file(read_pipe, &c, 1);
	for(i = 0; i < sizeof(destination);) {
		if(read_bytes_from_file(read_pipe, &c, 1)) {
			if(c == '\n') {
				destination[i] = '\0';
				break;
			}
			
			destination[i] = c;
			++i;
		}
	}
	
	print("server address is %s\n", destination);
	initialize_address_by_url(&connection->udp_connection.destination_address, destination);
	
	return 1;
}


Boolean send_address_using_address_connection(P2P_Connection* connection)
{
	Number time;
	
	print_error("sending ");
	print_address(&connection->mapped_address);
	print_error("\n");
	
	for(time = 0; time < 5000; ++time) {
		if(write_bytes_in_packet_connection(&address_connection, &connection->mapped_address, sizeof(connection->mapped_address))) {
			return 1;
		}
		
		sleep_thread(1);
	}
	
	return 0;
}


Boolean receive_address_using_address_connection(P2P_Connection* connection)
{
	Number time;
	
	print_error("receiving ");
	print_address(&connection->udp_connection.destination_address);
	print_error("\n");
	
	for(time = 0; time < 5000; ++time) {
		if(read_bytes_from_packet_connection(&address_connection, &connection->udp_connection.destination_address, sizeof(Address))) {
			return 1;
		}
		
		sleep_thread(1);
	}
	
	return 0;
}


typedef struct {
	struct Client_Connection* next;
	
	//Socket            client_socket;
	TCP_Connection    tcp_connection;
	P2P_Connection    p2p_connection;
	Packet_Connection packet_connection;
	Number            unusage_time;
	Number            receive_time;
	Number            send_time;
	Signed_Number     receive_buffer_size;
	Signed_Number     send_buffer_size;
	Byte              receive_buffer[QUEUE_SIZE * (MAX_PACKET_SIZE - sizeof(Packet_Head))];
	Byte              send_buffer[QUEUE_SIZE * (MAX_PACKET_SIZE - sizeof(Packet_Head))];
}
Client_Connection;


Client_Connection* client_connections = 0;


void connect_client()
{
	Client_Connection* client_connection;
	
	client_connection = allocate_memory(sizeof(Client_Connection));
	
	client_connection->unusage_time = 0;
	client_connection->receive_buffer_size = 0;
	client_connection->send_buffer_size = 0;
	
	copy_bytes(&client_connection->tcp_connection, &client_tcp_connection, sizeof(TCP_Connection));
	
	client_connection->next = 0;
	
	Client_Connection** connection_pointer;
	for(connection_pointer = &client_connections; *connection_pointer; connection_pointer = &(*connection_pointer)->next) {
		
	}
	
	*connection_pointer = client_connection;
	
	
	if(
		!initialize_P2P_connection(
			&client_connection->p2p_connection,
			"stun.l.google.com", 19302,
			&send_address_using_address_connection,
			&receive_address_using_address_connection
		)
	) {
		goto error_and_close_tcp;
	}
	
	initialize_packet_connection(&client_connection->packet_connection, QUEUE_SIZE, &client_connection->p2p_connection, &read_bytes_from_P2P_connection, &write_bytes_in_P2P_connection);
	
	return;
	
	error_and_close_tcp: {
		*connection_pointer = (*connection_pointer)->next;
		deinitialize_TCP_connection(&client_connection->tcp_connection);
	}
	
	error: {
		free_memory(client_connection);
		print_error("client not connected\n");
	}
}


void close_client(Client_Connection* connection)
{
	deinitialize_TCP_connection(&connection->tcp_connection);
	free_memory(connection);
}


Number main(Number number_of_arguments, Byte** arguments)
{
	connect_to = arguments[1];
	
	if(!initialize_address(&proxy_server.address, "127.0.0.1", 80, TCP_PROTOCOL)) {
		print_error("proxy адрес не доступен\n");
		goto error;
	}
	
	if(!initialize_TCP_server(&proxy_server)) {
		goto error;
	}
	
	P2P_Connection p2p_connection;
	
	if(
		!initialize_P2P_connection(
			&p2p_connection,
			"stun.l.google.com", 19302,
			&send_address_using_mail,
			&receive_address_using_mail
		)
	) {
		goto error;
	}
	
	initialize_packet_connection(&address_connection, 1, &p2p_connection, &read_bytes_from_P2P_connection, &write_bytes_in_P2P_connection);
	
	
	Signed_Number bytes_readed;
	Signed_Number bytes_writed;
	Number        time;
	
	time = 0;
	
	for(;;) {
		if(time % 30000 == 29999) {
			write_bytes_in_UDP_connection(&p2p_connection.udp_connection, 0, 0);
		}
		
		if(read_client_connection_from_TCP_server(&proxy_server, &client_tcp_connection)) {
			connect_client();
		}
		
		
		Client_Connection** client_connection_pointer;
		Client_Connection*  client_connection;
		
		for(client_connection_pointer = &client_connections; *client_connection_pointer; client_connection_pointer = &(*client_connection_pointer)->next) {
			client_connection = *client_connection_pointer;
			++client_connection->unusage_time;
			
			if(client_connection->unusage_time >= 300000) {
				print_error("client connection closed: unusage timeout\n");
				*client_connection_pointer = (*client_connection_pointer)->next;
				close_client(client_connection);
				break;
			}
			
			
			if(client_connection->receive_buffer_size > 0) {
				client_connection->unusage_time = 0;
				bytes_writed = write_bytes_in_TCP_connection(&client_connection->tcp_connection, client_connection->receive_buffer, client_connection->receive_buffer_size);
				
				if(bytes_writed > 0) {
					client_connection->receive_buffer_size -= bytes_writed;
				}
				else if(bytes_writed < 0) {
					print_error("client connection closed: TCP error2\n");
					*client_connection_pointer = (*client_connection_pointer)->next;
					close_client(client_connection);
					break;
				}
				else {
					if(client_connection->receive_time >= 5000) {
						print_error("client connection closed: write in TCP timeout\n");
						*client_connection_pointer = (*client_connection_pointer)->next;
						close_client(client_connection);
						break;
					}
					
					++client_connection->receive_time;
				}
			}
			else if(client_connection->receive_buffer_size < 0) {
				
			}
			else {
				client_connection->receive_time = 0;
				client_connection->receive_buffer_size = read_bytes_from_packet_connection(&client_connection->packet_connection, client_connection->receive_buffer, sizeof(client_connection->receive_buffer));
			}
			
			
			if(client_connection->send_buffer_size > 0) {
				client_connection->unusage_time = 0;
				bytes_writed = write_bytes_in_packet_connection(&client_connection->packet_connection, client_connection->send_buffer, client_connection->send_buffer_size);
			
				if(bytes_writed > 0) {
					client_connection->send_buffer_size -= bytes_writed;
				}
				else {
					if(client_connection->send_time >= 5000) {
						print_error("client connection closed: write packets timeout\n");
						*client_connection_pointer = (*client_connection_pointer)->next;
						close_client(client_connection);
						break;
					}
					
					++client_connection->send_time;
				}
			}
			else if(client_connection->send_buffer_size < 0) {
				print_error("client connection closed: TCP error\n");
				*client_connection_pointer = (*client_connection_pointer)->next;
				close_client(client_connection);
				break;
			}
			else {
				client_connection->send_time = 0;
				client_connection->send_buffer_size = read_bytes_from_TCP_connection(&client_connection->tcp_connection, client_connection->send_buffer, sizeof(client_connection->send_buffer));
			}
		}
		
		
		sleep_thread(1);
		++time;
	}
	
	return 0;

	error: {
		return 1;
	}
}