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

// signal processing function that supposedly catches ctrl-c, from https://stackoverflow.com/questions/4217037/catch-ctrl-c-in-c
int loop_parent = 1; // I don't want to make this global but I'm to lazy
int loop_child1 = 1;
int loop_child2 = 1;
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
    char filename[] = "./transporter";

    // Make the first pid number collector
    pid_t child1_PID;
    pid_t child2_PID = -1; // this is a signed integer

    printf("\n\t\tWelcome to romeo's addR!\n");

    // Fork one child
    child1_PID = fork();
    // Fork the next child
    if(child1_PID > 0)
    {
        // Only let the parent fork
        child2_PID = fork();
    }

    // find the child and parent, and make the parent fork one more time
    // This first child will be the one to create the file and use user inputs
    if(child1_PID == 0 && child2_PID == -1)
    {
        signal(SIGTERM, child1_handler);

        // Make the file to read/write from
        // To be safe, we'll make two files - one for signalling that we've received an input, and one for the input
        FILE* transport_data = fopen(filename, "w+");

        // close the file immediately now that they are created
        fclose(transport_data);

        while(loop_child1)
        {   
            // number inputs
            float num1, num2;
            int rc;

            // open da files back up to write to them
            transport_data = fopen(filename, "w");
            fprintf(transport_data,"0\n"); // reset the signal
            fclose(transport_data);

            printf("\n[child 1: receiver] Input the 1st number you'd like you'd like to add: ");
            if(scanf("%f", &num1) != 1)
            {
                getchar(); // YES - this repeats until the input buffer is cleared
                printf("\n\tbad input!");
                continue;
            }
            printf("[child 1: receiver] Input the 2nd number you'd like you'd like to add: ");
            if(scanf("%f", &num2) != 1)
            {
                getchar(); // YES - this repeats until the input buffer is cleared
                printf("\n\tbad input!");
                continue;
            }
            printf("[child 1: reciever] Adding the following numbers: %f, %f\n", num1, num2);

            // Inform our sibling that we are ready to read
            transport_data = fopen(filename, "w");
            fprintf(transport_data,"1\n%f\n%f", num1, num2); // reset the signal
            fclose(transport_data);
            
        }

        // Child 1 - delete the file, should be called first so it should be okay?
        return 1;

    }
    // The second child
    else if(child2_PID == 0)
    {
        signal(SIGTERM, child2_handler);

        // read
        FILE* transporter = fopen(filename, "r+");

        // close the files immediate now that they are created
        fclose(transporter);

        int signal, reading, rc;
        float num1, num2, final;
        while(loop_child2)
        {
            transporter = fopen(filename, "r");

            rc = fscanf(transporter, "%d", &signal);
            if(rc != EOF)
            {
                if(signal == 1 && reading != 1)
                {
                    reading = 1;
                    fscanf(transporter, "%f", &num1);
                    fscanf(transporter, "%f", &num2);
                    final = num1+num2;
                    printf("[child 2: responder] Sum: %f + %f = \t%f\n", num1, num2, final);
                }
                else if(signal == 0 && reading == 1)
                {
                    reading = 0;
                }
            }
            // close the files immediate now that they are created
            fclose(transporter);
        }        
        return 1;
    }
    // The parent
    else if(child2_PID > 0 && child1_PID > 0)
    {
        // The parent will just monitor the children playing, and tell them them it's bedtime when we ctrl-c the process. As in the parent will KILL the children. thats right. you heard me.
        // Monitor for ctrl-c, somehow?

        signal(SIGINT, parent_handler);

        while(loop_parent)
        {

        }
        printf("\nTerminating the children, as it's annoying to deal with the scanf's");
        kill(child1_PID, SIGTERM);
        kill(child2_PID, SIGTERM);
        printf("\n\nClosing process! Thanks for ADDing :D\n");
    }

}