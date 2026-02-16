#include <stdio.h>
#include <unistd.h>

int main()
{
    // // first, create a file
    // FILE* balls = fopen("./shaft.txt", "w+");
    // printf("balls\n");
    // fclose(balls);

    // printf("deleting file now\n");
    // remove("./shaft.txt");

    float balls, dick;
    scanf("%f, %f", &balls, &dick);
    printf("%f\n%f\n", balls, dick);
}