#ifndef STACK_INCLUDED
#define STACK_INCLUDED


#include <types.c>
#include <memory.c>


typedef struct {
	Number size;
	Byte*  data;
	Number data_size;
}
Stack;


void initialize_stack(Stack* stack, Byte* data, Number data_size)
{
	stack->size      = 0;
	stack->data      = data;
	stack->data_size = data_size;
}


Byte* allocate_bytes_in_stack(Stack* stack, Number number_of_bytes)
{
	Byte* bytes;
	
	if(stack->data_size - stack->size < number_of_bytes) {
		return 0;
	}
	
	bytes = stack->data + stack->size;
	stack->size += number_of_bytes;
	
	return bytes;
}


Number write_bytes_in_stack(Stack* stack, Byte* bytes, Number number_of_bytes)
{
	Number bytes_writed;
	Number i;
	
	bytes_writed = stack->data_size - stack->size;
	
	if(bytes_writed > number_of_bytes) {
		bytes_writed = number_of_bytes;
	}
	
	copy_bytes(allocate_bytes_in_stack(stack, bytes_writed), bytes, bytes_writed);
	
	return bytes_writed;
}


Number read_bytes_from_stack(Stack* stack, Byte* bytes, Number number_of_bytes)
{
	if(number_of_bytes > stack->size) {
		number_of_bytes = stack->size;
	}

	stack->size -= number_of_bytes;
	copy_bytes(bytes, stack->data + stack->size, number_of_bytes);

	return number_of_bytes;
}


void clean_stack(Stack* stack)
{
	stack->size = 0;
}


#endif//STACK_INCLUDED

/*
Number main(Number number_of_arguments, Byte** arguments)
{
	Stack stack;
	Byte  data[256];
	Byte  data2[256];

	initialize_stack(&stack, data, sizeof(data));
	write_bytes_in_stack(&stack, "Hi", 3);
	print("%d bytes\n", read_bytes_from_stack(&stack, data2, sizeof(data2)));
	print("%s\n", data2);
	
	return 0;
}*/