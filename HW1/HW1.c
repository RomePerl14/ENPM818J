#include <stdio.h> // used for user input/output
#include <unistd.h> // used for forking... are you forking kidding me? get it? HA!

// Don't need argc or argv necessarily since I'm going to be using child processes to read in inputs
int main(int argc, char** argv)
{
    // Declare the name of the file to transport data
    char filename[] = "transporter";

    // Make the first pid number collector
    pid_t child1_PID;
    pid_t child2_PID;

    // Fork one child
    child1_PID = fork();

    // find the child and parent, and make the parent fork one more time
    // This first child will be the one to create the file and use user inputs
    if(child1_PID == 0)
    {
        // Make the file to read/write from
        // To be safe, we'll make two files - one for signalling that we've received an input, and one for the input
        FILE* signal_hermano = fopen("./signal", "w+");
        FILE* send_inputs = fopen("./inputs", "w+");

        // close the files immediate now that they are created
        fclose(signal_hermano);
        fclose(send_inputs);

        while(1)
        {   
            
            printf("\nWelcome to romeo's addR!\n");

            // number inputs
            float num1, num2;
            int rc;

            printf("\tInput the first number you'd like you'd like to add: \n\t");
            scanf("%f", &num1);

            // open da files back up to write to them
            signal_hermano = fopen("./signal", "w");
            fprintf(signal_hermano,"0"); // reset the signal
            fclose(signal_hermano);

            printf("\tInput the second number you'd like you'd like to add: \n\t");
            scanf("%f", &num2);

            printf("\tadding the following numbers: \n\t\t%f, %f", num1, num2);
            send_inputs = fopen("./inputs", "w");
            fprintf(send_inputs, "%f\n%f", num1, num2);
            fclose(send_inputs);

            printf("\n\tnumber sent off to adder sibling!\n\n");

            // Inform our sibling that we are ready to read
            signal_hermano = fopen("./signal", "w");
            fprintf(signal_hermano,"1"); // reset the signal
            fclose(signal_hermano);

        }

        // if the parent tells us to kill ourselves, we follow their orders
        // now remove them suckas
        remove("./signal");
        remove("./inputs");

        // and finally, exit
        return 1;

    }
    else if(child1_PID > 0)
    {
        // Fork one more time
        child2_PID = fork(); // reuse this variable as the child process will never have acces to it, unless.....

        // This child will read in inputs from the file and add two ints
        if(child2_PID == 0) 
        {
            // read
            FILE* hermano_signal = fopen("./signal", "r+");
            FILE* inputs_sent = fopen("./inputs", "r+");

            int signal, rc;
            while(1)
                rc = fscanf(hermano_signal, "%d", &signal);
                if(rc != EOF)
                {
                    if(signal == 1)
                    printf("\n\t\t\tchild 2 - got inputs!\n");
                }
        }
        else if(child2_PID > 0)
        {
            // The parent will just monitor the children playing, and tell them them it's bedtime when we ctrl-c the process. As in the parent will KILL the children. thats right. you heard me.
            // Monitor for ctrl-c, somehow?
            while(1)
            {

            }

        }
    }

}