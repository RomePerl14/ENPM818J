/* ****************************************************************
 * (c) Copyright 2025                                             *
 * Romeo Perlstein                                                *
 * ENPM818J - Real Time Operating Systems                         *
 * College Park, MD 20742                                         *
 * https://terpconnect.umd.edu/~romeperl/                         *
 ******************************************************************/

#include <stdio.h> // used for user input/output
#include <unistd.h> // used for forking... are you forking kidding me? get it? HA!
#include <signal.h> // used for signaling things, i think, idk what it does
#include <stdlib.h> // using for atof() atoffee()

// signal processing function that supposedly catches ctrl-c, from https://stackoverflow.com/questions/4217037/catch-ctrl-c-in-c
int loop_parent = 1; // I don't want to make this global but I'm to lazy
int loop_child1 = 1;
int loop_child2 = 1;
int max_buff_size = 100; // I KNOW this is hardcoded, but we're just gonna enforce it 
void parent_handler(int signal_thingy)
{
    loop_parent = 0;
}
void child1_handler(int signal_thingy)
{
    loop_child1 = 0;
    char data[5] = "0\n0\n";
    write(STDIN_FILENO, data, 5);
}
void child2_handler(int signal_thingy)
{
    loop_child2 = 0;
}

// Don't need argc or argv necessarily since I'm going to be using child processes to read in inputs
int main(int argc, char** argv)
{
    // Declare the name of the file to transport data
    int pipe_file_descriptor[2];
    int rc = pipe(pipe_file_descriptor);
    if(rc < 0)
    {
        printf("ERMMM, FAILED TO FREAKIN PIPE - GET F'ed!");
        return -1; // return this for giggles
    }


    // Make the first pid number collector
    pid_t child1_PID;
    pid_t child2_PID = -1; // this is a signed integer

    printf("\n\t\tWelcome to romeo's addR!\n");

    // Fork one child
    child1_PID = fork();

    // Fork the next child
    if(child1_PID > 0)
    {
        // Make the second child
        child2_PID = fork();
    }
    printf("%d\n", child1_PID);

    if(child1_PID == 0 && child2_PID == -1)
    {
        while(1)
        {
            char test[] = "test message\n";
            printf("[child 1: receiver] test\n");
            sleep(1);
            write(pipe_file_descriptor[1], test, max_buff_size);
        }
    }
    else if(child2_PID == 0)
    {
        while(1)
        {
            printf("[child 2: responder] test\n");
            sleep(1);
            char test[max_buff_size];
            int reading = 1;
            while(reading > 0 )
            {
                reading = read(pipe_file_descriptor[0], test, max_buff_size);
            }
            
            printf("[child 2: responder] %d\n", reading);
            printf("[child 2: responder] %s\n", test);
        }
    }
    else if(child1_PID > 1 && child2_PID > 1)
    {
        while(1)
        {
            printf("[parent] test\n");
            sleep(1);
        }
    }
    // if(child1_PID == 0)
    // {
    //     signal(SIGTERM, child1_handler);
    //     while(loop_child1)
    //     {   
    //         printf("[child1: receiver] BALLS");
    //         sleep(1);
    //         // // number inputs
    //         // char num1_buff[max_buff_size], num2_buff[max_buff_size];

    //         // printf("\n[child 1: receiver] Input the 1st number you'd like you'd like to add: ");
    //         // if(scanf(" %99[ 0-9.]", num1_buff) != 1)
    //         // {
    //         //     getchar(); // YES - this repeats until the input buffer is cleared
    //         //     printf("\n\tbad input!");
    //         //     continue;
    //         // }
    //         // write(pipe_file_descriptor[1], num1_buff, max_buff_size);
    //         // fflush(stdin);
    //         // printf("[child 1: receiver] Input the 2nd number you'd like you'd like to add: ");
    //         // if(scanf(" %99[ 0-9.]", num2_buff) != 1)
    //         // {
    //         //     getchar(); // YES - this repeats until the input buffer is cleared
    //         //     printf("\n\tbad input!");
    //         //     continue;
    //         // }
    //         // write(pipe_file_descriptor[1], num2_buff, max_buff_size);
    //         // printf("[child 1: reciever] Adding the following numbers: %s, %s\n", num1_buff, num2_buff);
    //     }

    //     return 1;
    // }
    // // else if(child2_PID == 0)
    // // {
    // //     signal(SIGTERM, child2_handler);

    // //     char test[max_buff_size];
    // //     float num1, num2, final;
    // //     int read1, read2;
    // //     while(loop_child2)
    // //     {
    // //         printf("BALLS");
    // //         int rc = read(pipe_file_descriptor[0], test, sizeof(max_buff_size));
    // //         printf("%d", rc);
    // //         if(rc > 0 && read1 == 0 && read2 == 0)
    // //         {
    // //             num1 = atof(test);
    // //             read1 = 1;
    // //             printf("[child 2: responder] Got first number!");
    // //         }
    // //         else if(rc > 0 && read1 == 1 && read2 == 0)
    // //         {
    // //             num2 = atof(test);
    // //             read2 = 1;
    // //             printf("[child 2: responder] Got second number!");
    // //         }
            
    // //         if(read1 == 1 && read2 == 1)
    // //         {
    // //             final = num1 + num2;
    // //             printf("[child 2: responder] Sum of %f + %f = %f\n", num1, num2, final);
    // //             read1 = 0;
    // //             read2 = 0;
    // //         }
    // //     }        
    // //     return 1;
    // // }

    // // // OKAY SO - index 1 is the writing file descriptor, index 0 is the reading file descriptor, got it!

    
    // // else if(child2_PID == 0)
    // // {
    // //     signal(SIGTERM, child2_handler);

    // //     char test[max_buff_size];
    // //     float num1, num2, final;
    // //     int read1, read2;
    // //     while(loop_child2)
    // //     {
    // //         int rc = read(pipe_file_descriptor[0], test, sizeof(max_buff_size));
    // //         printf("%d", rc);
    // //         if(rc > 0 && read1 == 0 && read2 == 0)
    // //         {
    // //             num1 = atof(test);
    // //             read1 = 1;
    // //             printf("[child 2: responder] Got first number!");
    // //         }
    // //         else if(rc > 0 && read1 == 1 && read2 == 0)
    // //         {
    // //             num2 = atof(test);
    // //             read2 = 1;
    // //             printf("[child 2: responder] Got second number!");
    // //         }
            
    // //         if(read1 == 1 && read2 == 1)
    // //         {
    // //             final = num1 + num2;
    // //             printf("[child 2: responder] Sum of %f + %f = %f\n", num1, num2, final);
    // //             read1 = 0;
    // //             read2 = 0;
    // //         }
    // //     }        
    // //     return 1;
    // // }
    // // // The parent
    // else if(child2_PID > 0 && child1_PID > 0)
    // {
    //     // The parent will just monitor the children playing, and tell them them it's bedtime when we ctrl-c the process. As in the parent will KILL the children. thats right. you heard me.
    //     // Monitor for ctrl-c, somehow?

    //     signal(SIGINT, parent_handler);
    //     while(loop_parent)
    //     {

    //     }
    //     printf("\nTerminating the children, as it's annoying to deal with the scanf's");
    //     kill(child1_PID, SIGTERM);
    //     // kill(child2_PID, SIGTERM);
    //     // close(pipe_file_descriptor[1]);
    //     // close(pipe_file_descriptor[0]);
    //     printf("\n\nClosing process! Thanks for ADDing :D\n");
    // }

}