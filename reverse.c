#include <stdio.h>
#include <string.h>

#define SIZE 100

int main()
{
	char input[SIZE];
	int i;
	char temp;
	int length;
	
	printf("Enter a string: ");
	scanf("%99[^\n]", input);
	length = strlen(input);

	/*Iterates through array swapping positions.*/
	for (i = 0; i < length / 2; i++)
	{
		temp = input[i];
		input[i] = input[(length - 1) - i];
		input[(length - 1) - i] = temp;
	}

	printf("Reversed string: %s\n", input);

	return 0;
}
