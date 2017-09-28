#include <stdio.h>
#include <string.h>

typedef struct
{
	char name[4];
	int score;
} Score;

int score_set(Score* x, char* y, int z)
{
	if (z < 0)
	{
		return 0;
	}
	else if (strlen(y) > 3)
	{
		return 0;
	}
	else
	{
		strcpy(x -> name, y);
		x -> score = z;
		return 1;
	}
}

void score_print(Score* x)
{
	printf("%s %d\n", x -> name, x -> score);
}

int score_compare(Score* x, Score* y)
{
	if (x -> score < y -> score)
	{
		return 1;
	}
	else if (x -> score == y -> score)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int main() {
    /* create an array of scores */
    Score scores[10];

    /* put in some test data */
    score_set(&scores[0], "IAN", 750);
    score_set(&scores[1], "BOB", 1200);
    score_set(&scores[2], "ADA", 3500);
    score_set(&scores[3], "SUE", 900);
    score_set(&scores[4], "EVA", 500);
    score_set(&scores[5], "BEN", 1500);
    score_set(&scores[6], "ROY", 3000);
    score_set(&scores[7], "KIM", 1250);
    score_set(&scores[8], "VIC", 2500);
    score_set(&scores[9], "DAN", 1800);

    /* sort them using the compare function above */
    qsort(scores, 10, sizeof(Score), 
        (int (*) (const void*, const void*)) &score_compare);

    /* display them */
    int i;
    for (i = 0; i < 10; i++) {
        score_print(&scores[i]);
    }

    return 0;
}
