#ifndef DYNAMIC_STACK_INCLUDED
#define DYNAMIC_STACK_INCLUDED


#include <heap.c>
#include <data-structures/stack.c>


typedef struct {
	Stack  stack;
	Byte*  buffer;
	Number buffer_size;
}
Dynamic_Stack;


void initialize_dynamic_stack(Dynamic_Stack* dyn, Number reserve)
{
	dyn->buffer_size = reserve;
	dyn->buffer      = allocate_memory(dyn->buffer_size);
	initialize_stack(&dyn->stack, dyn->buffer, dyn->buffer_size);
}


void deinitialize_dynamic_stack(Dynamic_Stack* dyn)
{
	free_memory(dyn->buffer);
}


void reserve_bytes_in_dynamic_stack(Dynamic_Stack* dyn, Number number_of_bytes)
{
	Number new_buffer_size;
	Byte*  new_buffer;
	
	while(dyn->buffer_size - dyn->stack.size < number_of_bytes) {
		new_buffer_size = dyn->buffer_size * 2;
		new_buffer = reallocate_memory(dyn->buffer, new_buffer_size);
		
		if(!new_buffer) {
			return;
		}
		
		dyn->buffer = new_buffer;
		dyn->buffer_size = new_buffer_size;
	}
	
	dyn->stack.data      = dyn->buffer;
	dyn->stack.data_size = dyn->buffer_size;
}


Number write_bytes_in_dynamic_stack(Dynamic_Stack* dyn, Byte* bytes, Number number_of_bytes)
{
	reserve_bytes_in_dynamic_stack(dyn, number_of_bytes);
	return write_bytes_in_stack(&dyn->stack, bytes, number_of_bytes);
}


Number read_bytes_from_dynamic_stack(Dynamic_Stack* dyn, Byte* bytes, Number number_of_bytes)
{
	return read_bytes_from_stack(&dyn->stack, bytes, number_of_bytes);
}


#endif//DYNAMIC_STACK_INCLUDED

/*
Number main(Number number_of_arguments, Byte** arguments)
{
	Dynamic_Stack dyn;
	Byte          data[256];

	initialize_dynamic_stack(&dyn, 1);
	write_bytes_in_dynamic_stack(&dyn, "Hi", 3);
	//print("%d bytes\n", read_bytes_from_stack(&dyn.stack, data, sizeof(data)));
	print("%d bytes\n", read_bytes_from_dynamic_stack(&dyn, data, sizeof(data)));
	print("%s\n", data);
	deinitialize_dynamic_stack(&dyn);
	
	return 0;
}*/