#ifndef HEAP_SORT_INCLUDED
#define HEAP_SORT_INCLUDED


#include <memory.c>

/*
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
}


void heap_sort(
	Byte*  array,
	Number element_size,
	Number number_of_elements,
	Signed_Number (*compare)(Byte* element1, Byte* element2)
)
{
	Number i;
	
	for(i = 1; i < number_of_elements; ++i) {
		add_in_priority_queue(array, array + i * element_size, element_size, i, compare);
	}
	
	for(i = number_of_elements; i; --i) {
		remove_from_priority_queue(array, element_size, i, compare);
	}
}*/


void heap_sort(
	Byte*  array,
	Number element_size,
	Number number_of_elements,
	Signed_Number (*compare)(Byte* element1, Byte* element2)
)
{
	Number i;
	
	Number top;
	Number current;
	Number left;
	Number right;
	Number next;
	Byte*  next_element;
	Byte*  top_element;
	
	for(i = 1; i < number_of_elements; ++i) {
		current = i;
		top = (current - 1) / 2;
		
		for(; current && compare(array + top * element_size, array + current * element_size) < 0; ) {
			swap_bytes(array + top * element_size, array + current * element_size, element_size);
			current = top;
			top = (current - 1) / 2;
		}
	}
	
	for(i = number_of_elements; i; ) {
		--i;
		swap_bytes(array, array + i * element_size, element_size);
		
		top = 0;
	
		for(;;) {
			left = top * 2 + 1;
			
			if(left >= i) {
				break;
			}
			
			right = left + 1;
			
			if(right < i && compare(array + left * element_size, array + right * element_size) < 0) {
				next = right;
			}
			else {
				next = left;
			}
			
			next_element = array + next * element_size;
			top_element = array + top * element_size;
			
			if(compare(next_element, top_element) < 0) {
				break;
			}
			
			swap_bytes(next_element, top_element, element_size);
			
			top = next;
		}
	}
}


#endif//HEAP_SORT_INCLUDED

/*
Signed_Number compare(Number8* number1, Number8* number2)
{
	return *number1 - *number2;
}


Number main(Number number_of_arguments, Byte** arguments)
{
	Number i;
	Byte bytes[] = {5, 4, 2, 3, 1};

	heap_sort(bytes, 1, sizeof(bytes), &compare);
	
	for(i = 0; i < sizeof(bytes); ++i) {
		print("%d ", bytes[i]);
	}
	
	return 0;
}*/