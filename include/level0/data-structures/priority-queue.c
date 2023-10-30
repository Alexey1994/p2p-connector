#ifndef PRIORITY_QUEUE_INCLUDED
#define PRIORITY_QUEUE_INCLUDED


#include <memory.c>


void add_in_priority_queue(
	Byte* queue,
	Byte* element,
	Number element_size,
	Number number_of_elements_in_queue,
	Signed_Number (*compare)(Byte* element1, Byte* element2)
)
{
	Number top;
	Number current;
	
	current = number_of_elements_in_queue;
	top = (current - 1) / 2;
	
	for(; current && compare(queue + top * element_size, queue + current * element_size) < 0; ) {
		swap_bytes(queue + top * element_size, queue + current * element_size, element_size);
		current = top;
		top = (current - 1) / 2;
	}
}


Byte* remove_from_priority_queue(
	Byte* queue,
	Number element_size,
	Number number_of_elements_in_queue,
	Signed_Number (*compare)(Byte* element1, Byte* element2)
)
{
	--number_of_elements_in_queue;
	swap_bytes(queue, queue + number_of_elements_in_queue * element_size, element_size);
	
	Number top;
	Number left;
	Number right;
	Number next;
	
	top = 0;
	
	for(;;) {
		left = 2 * top + 1;
		
		if(left >= number_of_elements_in_queue) {
			break;
		}
		
		right = left + 1;
		
		if(right < number_of_elements_in_queue && compare(queue + left * element_size, queue + right * element_size) < 0) {
			next = right;
		}
		else {
			next = left;
		}
		
		if(compare(queue + next * element_size, queue + top * element_size) < 0) {
			break;
		}
		
		swap_bytes(queue + top * element_size, queue + next * element_size, element_size);
		
		top = next;
	}
	
	return queue + number_of_elements_in_queue * element_size;
}


#endif//PRIORITY_QUEUE_INCLUDED

/*
Signed_Number compare(Number8* number1, Number8* number2)
{
	return *number1 - *number2;
}


Number main(Number number_of_arguments, Byte** arguments)
{
	Number i;
	Byte bytes[] = {3, 4, 2, 5, 1};
	
	for(i = 1; i < sizeof(bytes); ++i) {
		add_in_priority_queue(bytes, bytes + i, 1, i, &compare);
	}
	
	print("%d", *remove_from_priority_queue(bytes, 1, sizeof(bytes), compare));
	
	return 0;
}*/