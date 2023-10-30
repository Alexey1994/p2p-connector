#ifndef MERGE_SORT_INCLUDED
#define MERGE_SORT_INCLUDED


#include <memory.c>
#include <heap.c>


void merge_sort(
	Byte*  array,
	Number element_size,
	Number number_of_elements,
	Signed_Number (*compare)(Byte* element1, Byte* element2)
)
{
	Byte*  left;
	Byte*  right;
	Number number_of_left_elements;
	Number left_size;
	Number number_of_right_elements;
	Number right_size;
	Number left_index;
	Number right_index;
	Number merged_index;
	
	if(number_of_elements <= 1) {
		return;
	}
	
	number_of_left_elements = number_of_elements / 2;
	left_size = number_of_left_elements * element_size;
	left = allocate_memory(left_size);
	copy_bytes(left, array, left_size);
	
	number_of_right_elements = number_of_elements - number_of_left_elements;
	right_size = number_of_right_elements * element_size;
	right = allocate_memory(right_size);
	copy_bytes(right, array + left_size, right_size);
	
	merge_sort(left, element_size, number_of_left_elements, compare);
	merge_sort(right, element_size, number_of_right_elements, compare);
	
	left_index = 0;
	right_index = 0;
	
	for(merged_index = 0; merged_index < number_of_elements; ++merged_index) {
		if(left_index < number_of_left_elements && right_index < number_of_right_elements) {
			if(compare(left + left_index * element_size, right + right_index * element_size) < 0) {
				copy_bytes(array + merged_index * element_size, left + left_index * element_size, element_size);
				++left_index;
			}
			else {
				copy_bytes(array + merged_index * element_size, right + right_index * element_size, element_size);
				++right_index;
			}
		}
		else if(left_index < number_of_left_elements) {
			copy_bytes(array + merged_index * element_size, left + left_index * element_size, element_size);
			++left_index;
		}
		else if(right_index < number_of_right_elements) {
			copy_bytes(array + merged_index * element_size, right + right_index * element_size, element_size);
			++right_index;
		}
		//else {
		//	print_error("out of array\n");
		//}
	}
	
	free_memory(left);
	free_memory(right);
}


#endif//MERGE_SORT_INCLUDED

/*
Signed_Number compare(Number8* number1, Number8* number2)
{
	return *number1 - *number2;
}


Number main(Number number_of_arguments, Byte** arguments)
{
	Number i;
	Byte bytes[] = {5, 4, 2, 3, 1};

	merge_sort(bytes, 1, sizeof(bytes), &compare);
	
	for(i = 0; i < sizeof(bytes); ++i) {
		print("%d ", bytes[i]);
	}
	
	return 0;
}*/