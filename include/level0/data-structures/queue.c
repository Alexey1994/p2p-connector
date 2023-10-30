#ifndef QUEUE_INCLUDED
#define QUEUE_INCLUDED


#include <types.c>


typedef struct {
	Byte*  data;
	Number data_size;
	Number size;
	Number start_index;
}
Queue;


void initialize_queue(Queue* queue, Byte* data, Number data_size)
{
	queue->data = data;
	queue->data_size = data_size;
	queue->size = 0;
	queue->start_index = 0;
}


Signed_Number write_bytes_in_queue(Queue* queue, Byte* bytes, Number number_of_bytes)
{
	Number i;
	Number end_index;
	
	if(queue->size + number_of_bytes > queue->data_size) {
		return 0;
	}
	
	for(i = 0; i < number_of_bytes; ++i) {
		end_index = queue->start_index + queue->size;
		
		if(end_index >= queue->data_size) {
			end_index -= queue->data_size;
		}
		
		queue->data[end_index] = *bytes;
			
		++queue->size;
		++bytes;
	}
	
	return number_of_bytes;
}


Signed_Number write_bytes_in_queue_start(Queue* queue, Byte* bytes, Number number_of_bytes)
{
	Number i;
	
	if(queue->size + number_of_bytes > queue->data_size) {
		return 0;
	}
	
	bytes += number_of_bytes;
	for(i = 0; i < number_of_bytes; ++i) {
		*bytes = queue->data[queue->start_index];
		
		if(queue->start_index) {
			--queue->start_index;
		}
		else {
			queue->start_index = queue->data_size - 1;
		}
		
		--bytes;
		++queue->size;
	}
	
	return number_of_bytes;
}


Signed_Number read_bytes_from_queue(Queue* queue, Byte* bytes, Number number_of_bytes)
{
	Number i;
	
	if(queue->size < number_of_bytes) {
		return 0;
	}
	
	for(i = 0; i < number_of_bytes; ++i) {
		*bytes = queue->data[queue->start_index];
		
		++queue->start_index;
		
		if(queue->start_index >= queue->data_size) {
			queue->start_index -= queue->data_size;
		}
		
		++bytes;
		--queue->size;
	}
	
	return number_of_bytes;
}


Signed_Number read_bytes_from_queue_end(Queue* queue, Byte* bytes, Number number_of_bytes)
{
	Number i;
	Number end_index;
	
	if(queue->size < number_of_bytes) {
		return 0;
	}
	
	bytes += number_of_bytes;
	for(i = 0; i < number_of_bytes; ++i) {
		end_index = queue->start_index + queue->size;
		
		if(end_index >= queue->data_size) {
			end_index -= queue->data_size;
		}
		
		*bytes = queue->data[end_index];
		
		--bytes;
		--queue->size;
	}
	
	return number_of_bytes;
}


#endif//QUEUE_INCLUDED

/*
Number main(Number number_of_arguments, Byte** arguments)
{
	Queue queue;
	Byte  buffer[5];
	Byte  b;
	
	initialize_queue(&queue, buffer, sizeof(buffer));
	
	b = 1;
	write_bytes_in_queue(&queue, &b, 1);
	
	read_bytes_from_queue(&queue, &b, 1);
	
	b = 2;
	write_bytes_in_queue(&queue, &b, 1);
	
	b = 3;
	write_bytes_in_queue(&queue, &b, 1);
	
	b = 4;
	write_bytes_in_queue(&queue, &b, 1);
	
	while(queue.size) {
		read_bytes_from_queue(&queue, &b, 1);
		print("%d ", b);
	}
	
	print(" (start_index %d)\n", queue.start_index);
	
	
	b = 1;
	write_bytes_in_queue(&queue, &b, 1);
	
	read_bytes_from_queue(&queue, &b, 1);
	
	b = 2;
	write_bytes_in_queue(&queue, &b, 1);
	
	b = 3;
	write_bytes_in_queue(&queue, &b, 1);
	
	b = 4;
	write_bytes_in_queue(&queue, &b, 1);
	
	while(queue.size) {
		read_bytes_from_queue(&queue, &b, 1);
		print("%d ", b);
	}
	
	print(" (start_index %d)\n", queue.start_index);
	
	
	b = 1;
	write_bytes_in_queue(&queue, &b, 1);
	
	read_bytes_from_queue(&queue, &b, 1);
	
	b = 2;
	write_bytes_in_queue(&queue, &b, 1);
	
	read_bytes_from_queue(&queue, &b, 1);
	
	b = 3;
	write_bytes_in_queue(&queue, &b, 1);
	
	b = 4;
	write_bytes_in_queue(&queue, &b, 1);
	
	b = 5;
	write_bytes_in_queue(&queue, &b, 1);
	
	b = 6;
	write_bytes_in_queue(&queue, &b, 1);
	
	b = 7;
	write_bytes_in_queue(&queue, &b, 1);
	
	while(queue.size) {
		read_bytes_from_queue(&queue, &b, 1);
		print("%d ", b);
	}
	
	print(" (start_index %d)\n", queue.start_index);
	
	
	b = 1;
	write_bytes_in_queue(&queue, &b, 1);
	
	read_bytes_from_queue(&queue, &b, 1);
	
	b = 2;
	write_bytes_in_queue(&queue, &b, 1);
	
	b = 3;
	write_bytes_in_queue(&queue, &b, 1);
	
	b = 4;
	write_bytes_in_queue(&queue, &b, 1);
	
	while(queue.size) {
		read_bytes_from_queue(&queue, &b, 1);
		print("%d ", b);
	}
	
	print(" (start_index %d)\n", queue.start_index);
	
	return 0;
}*/