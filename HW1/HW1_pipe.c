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
    int pipe_fd_notification[2];
    int rc = pipe(pipe_file_descriptor);
    if(rc < 0)
    {
        printf("ERMMM, FAILED TO FREAKIN PIPE - GET F'ed!");
        return -1; // return this for giggles
    }
    rc = pipe(pipe_fd_notification);
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

    if(child1_PID == 0 && child2_PID == -1)
    {
        signal(SIGTERM, child1_handler);

        while(loop_child1)
        {   
            // number inputs
            float num1_buff, num2_buff;

            printf("\n[child 1: receiver] Input the 1st number you'd like you'd like to add: ");
            if(scanf("%f", &num1_buff) != 1)
            {
                getchar(); // YES - this repeats until the input buffer is cleared
                printf("\n\tbad input!");
                continue;
            }
            write(pipe_file_descriptor[1], &num1_buff, sizeof(float));
            fflush(stdin);
            printf("[child 1: receiver] Input the 2nd number you'd like you'd like to add: ");
            if(scanf("%f", &num2_buff) != 1)
            {
                getchar(); // YES - this repeats until the input buffer is cleared
                printf("\n\tbad input!");
                continue;
            }
            write(pipe_file_descriptor[1], &num2_buff, sizeof(float));
            printf("[child 1: reciever] Adding the following numbers: %f, %f\n", num1_buff, num2_buff);
            
            // block until the child process finishes adding... so that our output looks nice!
            __uint8_t nothing_burger;
            read(pipe_fd_notification[0], &nothing_burger, sizeof(__uint8_t));
        }

        return 1;
    }
    else if(child2_PID == 0)
    {
        signal(SIGTERM, child2_handler);

        float num1, num2, final;
        int read1, read2;
        while(loop_child2)
        {
            int rc = read(pipe_file_descriptor[0], &num1, sizeof(float));
            if(rc > 0 && read1 == 0 && read2 == 0)
            {
                read1 = 1;
            }
            rc = read(pipe_file_descriptor[0], &num2, sizeof(float));
            if(rc > 0 && read1 == 1 && read2 == 0)
            {
                read2 = 1;
            }
            
            if(read1 == 1 && read2 == 1)
            {
                final = num1 + num2;
                printf("[child 2: responder] Sum of %f + %f = %f\n", num1, num2, final);
                read1 = 0;
                read2 = 0;
                // inform the other child that we've finished adding lol
                __uint8_t nothing_burger = 1;
                write(pipe_fd_notification[1], &nothing_burger, sizeof(__uint8_t));
            }
        }        
        return 1;
    }
    else if(child1_PID > 1 && child2_PID > 1)
    {
        // The parent will just monitor the children playing, and tell them them it's bedtime when we ctrl-c the process. As in the parent will KILL the children. thats right. you heard me.
        // Monitor for ctrl-c, somehow?

        signal(SIGINT, parent_handler);
        while(loop_parent)
        {

        }
        printf("\n\nTerminating the children, as it's annoying to deal with the scanf's");
        printf("\nI've learned about sigaction, but I'm SOOOO LAZZZYYYYY");
        kill(child1_PID, SIGTERM);
        kill(child2_PID, SIGTERM);
        close(pipe_file_descriptor[1]);
        close(pipe_file_descriptor[0]);
        close(pipe_file_descriptor[1]);
        close(pipe_file_descriptor[0]);
        printf("\n\nClosing process! Thanks for ADDing :D\n");
    }
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