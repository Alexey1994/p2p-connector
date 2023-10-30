#ifndef ASYNC_QUEUE_INCLUDED
#define ASYNC_QUEUE_INCLUDED


#include <types.c>


typedef struct {
	Byte*  data;
	Number data_size;
	Number start_index;
	Number end_index;
}
Async_Queue;


void initialize_async_queue(Async_Queue* queue, Byte* data, Number data_size)
{
	queue->data        = data;
	queue->data_size   = data_size;
	queue->start_index = 0;
	queue->end_index   = 0;
}


Number calculate_async_queue_size(Async_Queue* queue)
{
	if(queue->end_index >= queue->start_index) {
		return queue->end_index - queue->start_index;
	}
	else {
		return queue->data_size - queue->start_index + queue->end_index;
	}
}


Signed_Number write_bytes_in_async_queue(Async_Queue* queue, Byte* bytes, Number number_of_bytes)
{
	Number bytes_writed;
	Number end_index;
	
	for(bytes_writed = 0; bytes_writed < number_of_bytes; ++bytes_writed) {
		if(calculate_async_queue_size(queue) == queue->data_size) {
			break;
		}
		
		end_index = queue->end_index;
		queue->data[end_index] = *bytes;
		
		++end_index;
		
		if(end_index >= queue->data_size) {
			end_index = 0;
		}
		
		queue->end_index = end_index;
		++bytes;
	}
	
	return bytes_writed;
}


Signed_Number read_bytes_from_async_queue(Async_Queue* queue, Byte* bytes, Number number_of_bytes)
{
	Number bytes_readed;
	Number start_index;
	
	for(bytes_readed = 0; bytes_readed < number_of_bytes; ++bytes_readed) {
		start_index = queue->start_index;
		
		if(start_index == queue->end_index) {
			break;
		}
		
		*bytes = queue->data[start_index];
		
		++start_index;
		
		if(start_index >= queue->data_size) {
			start_index = 0;
		}
		
		queue->start_index = start_index;
		++bytes;
	}
	
	return bytes_readed;
}


#endif//ASYNC_QUEUE_INCLUDED

/*
#include <thread.c>


Async_Queue queue;
Byte        queue_data[512];


void a()
{
	Byte i;
	
	for(i = 0; ; ++i) {
		write_bytes_in_async_queue(&queue, &i, 1);
		
		if(calculate_async_queue_size(&queue) > 500) {
			Sleep(1);
			//print("%d\n", calculate_async_queue_size(&queue));
		}
	}
}


void b()
{
	Byte b;
	
	for(;;) {
		read_bytes_from_async_queue(&queue, &b, sizeof(b));
		
		print("%d ", b);
	}
}


Number main(Number number_of_arguments, Byte** arguments)
{
	initialize_async_queue(&queue, queue_data, sizeof(queue_data));
	
	run_thread(&a, 0);
	run_thread(&b, 0);
	
	for(;;) {
		
	}
	
	return 0;
}*/