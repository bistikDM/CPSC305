#include <stdio.h>

#define ROW 40
#define COLUMN 80

unsigned char matrix[ROW][COLUMN];
FILE* file;
//char* fileName[];

void openFile()
{
	short x = 0;
	short y = 0;
	short z = 0;
	short bit;
	unsigned char line[400];

//	printf("Enter a file name: ");
//	scanf("%49[^\n]", fileName);
//	file = fopen(fileName, "rb");
	
	fread(&line, sizeof(char), 400, file);
	
	//Iterate through the y-axis of the matrix.
	for(y = 0; y < ROW; y++)
	{
		//Iterate through the x-axis of the matrix.
		for(x = 0; x < COLUMN; x++)
		{
			/*
			This loop goes through each line element (8bit) and perform bit operation ~> shifts right and & with 128 to isolate each bit.
			Afterwards, it assigns the value into the matrix.
			*/
			matrix[y][x] = (line[z] & (128 >> bit));
			bit++;
			if(bit == 8)
			{
				bit = 0;
				z++;
			}
		}
	}
	fclose(file);
}

//This function check the matrix element for 1(alive) and replaces it with "O", otherwise it will be " ".
char* cellAlive(int y, int x)
{
	if(matrix[y][x])
		return "O";
	else
		return " ";
}

//This function prints the content of the matrix.
void printGrid()
{;
	int x;
	int y;
	for(y = 0; y < ROW; y++)
	{
		for(x = 0; x < COLUMN; x++)
		{
			printf("%s", cellAlive(y, x));
		}
		printf("\n");
	}
}

//This function checks the surrounding cells.
int cellCheck(int y, int x)
{
	int counter = 0;
	int horizontal;
	int vertical;
	int dump = 0;
	
	//Note to self: check if there's an alternate conditional statement that is more efficient than the ternary operator, the else operation is not necessary.
	if(x == 0 && y == 0) //Checks to see if it's the top left corner.
	{
		matrix[y][x + 1] ? counter++ : dump++;
		matrix[y + 1][x] ? counter++ : dump++;
		matrix[y + 1][x + 1] ? counter++ : dump++;
			
	}
	else if(x == COLUMN && y == 0) //Checks to see if it's the top right corner.
	{
		matrix[y][x - 1] ? counter++ : dump++;
		matrix[y + 1][x] ? counter++ : dump++;
		matrix[y + 1][x - 1] ? counter++ : dump++;
	}
	else if(x == 0 && y == ROW) //Checks to see if it's the bottom left corner.
	{
		matrix[y][x + 1] ? counter++ : dump++;
		matrix[y - 1][x] ? counter++ : dump++;
		matrix[y - 1][x + 1] ? counter++ : dump++;
	}
	else if(x == COLUMN && y == ROW) //Checks to see if it's the bottom right corner.
	{
		matrix[y][x - 1] ? counter++ : dump++;
		matrix[y - 1][x] ? counter++ : dump++;
		matrix[y - 1][x - 1] ? counter++ : dump++;
	}
	else if(x == 0) //Checks to see if it's the leftmost element.
	{
		for(vertical = -1; vertical <= 1; vertical++)
		{
			for(horizontal = 0; horizontal <= 1; horizontal++)
			{
				if(horizontal || vertical) matrix[y + vertical][x + horizontal] ? counter ++ : dump++;
				
			}
		}
		return counter;
	}
	else if(x == COLUMN) //Checks to see if it's the rightmost element.
	{
		for(vertical = -1; vertical <= 1; vertical++)
		{
			for(horizontal = 0; horizontal >= -1; horizontal--)
			{
				if(horizontal || vertical) matrix[y + vertical][x + horizontal] ? counter ++ : dump++;
			}
		}
		return counter;
	}
	else if(y == 0) //Checks to see if it's the top element.
	{
		for(vertical = 0; vertical <= 1; vertical++)
		{
			for(horizontal = -1; horizontal <= 1; horizontal++)
			{
				if(horizontal || vertical) matrix[y + vertical][x + horizontal] ? counter ++ : dump++;
			}
		}
		return counter;
	}
	else if(y == ROW) //Checks to see if it's the bottom element.
	{
		for(vertical = 0; vertical >= -1; vertical--)
		{
			for(horizontal = -1; horizontal <= 1; horizontal++)
			{
				if(horizontal || vertical) matrix[y + vertical][x + horizontal] ? counter ++ : dump++;
			}
		}
		return counter;
	}
	else //Otherwise if the element has 8 sides.
	{
		for(vertical = -1; vertical <= 1; vertical++)
		{
			for(horizontal = -1; horizontal <= 1; horizontal++)
			{
				if(horizontal || vertical) matrix[y + vertical][x + horizontal] ? counter ++ : dump++;
			}
		}
		return counter;
	}
}

/*
This function changes the matrix based on the rules established:
1. Any live cell with fewer than two neighbors is dead in the next generation.
2. Any live cell with more than three neighbors is dead in the next generation.
3. Any live cell with two or three neighbors survives.
4. Any empty cell with exactly three neighbors becomes live in the next generation.
5. Any empty cell with a number of neighbors not equal to three remains empty.
*/
void generation(int turn)
{
//	int turn;
	unsigned char tempMatrix[ROW][COLUMN];
	int currentTurn;
	int x;
	int y;
	int counter;
	
//	printf("Enter number of generations: ");
//	scanf("%d", &turn);
	
	for(currentTurn = 0; currentTurn < turn; currentTurn++)
	{
		for(y = 0; y < ROW; y++)
		{
			for(x = 0; x < COLUMN; x++)
			{
				counter = cellCheck(y, x);
				switch (counter)
				{
					case 2:
						tempMatrix[y][x] = matrix[y][x];
						break;
					case 3:
						if(matrix[y][x]) tempMatrix[y][x] = 1;
						break;
					default:
						tempMatrix[y][x] = 0;
				}
				/*
				if(counter == 2)
				{
					tempMatrix[y][x] = matrix[y][x];
				}
				if(counter == 3)
				{
					if(matrix[y][x]) tempMatrix[y][x] = 1;
				}
				if(counter < 2)
				{
					tempMatrix[y][x] = 0;
				}
				if(counter > 3)
				{
					tempMatrix[y][x] = 0;
				}
				*/
			}
		}
		for(y = 0; y < ROW; y++)
		{
			for(x = 0; x < COLUMN; x++)
			{
				matrix[y][x] = tempMatrix[y][x];
			}
		}
	}
}

int main(int argc, char* argv[])
{
	file = fopen(argv[1], "r");
	openFile();
	generation(atoi(argv[2]));
	printGrid();
/*	int x, y;
	for(y = 0; y < ROW; y++)
	{
		for(x = 0; x < COLUMN; x++)
		{
			printf("%hhu ", matrix[y][x]);
		}
		printf("\n\nBREAK\n\n");
	}
*/
	return 0;
}

/*
The output of the the matrix is always different for each run. Could it be that in the openFile() function after line 22 where it reads the content of the file
and assigns it to the "line" variable, the actual element is never used? I have a feeling that from line 25 to 40 instead of parsing each element, it is working on
the memory address of those element and placing it into the matrix. Which would explain why the output of my program varies from each run. Unfortunately, this is the
only theory as to why my program is not producing the expected output. Please note that i performed the test with 0 generation and therefor should only print the
contents of the matrix right after it is read and before any actions are performed on it.
*/

/*
1. Random bits being turned on and off, most notably bottom left corner of the matrix.
2. Some bits are not being turned on regardless if it meets the criteria, check turn 1 of blinker (large circle pattern in the 4th quadrant of the matrix).
*/