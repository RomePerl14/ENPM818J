#include <stdio.h> // used for user input/output
#include <unistd.h> // used for forking... are you forking kidding me? get it? HA!

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
        // Make the file to read/write from
        // To be safe, we'll make two files - one for signalling that we've received an input, and one for the input
        FILE* transport_data = fopen(filename, "w+");

        // close the file immediately now that they are created
        fclose(transport_data);

        while(1)
        {   
            // number inputs
            float num1, num2;
            int rc;

            // open da files back up to write to them
            transport_data = fopen(filename, "w");
            fprintf(transport_data,"0\n"); // reset the signal
            fclose(transport_data);

            printf("\n[child 1: receiver] Input the 1st number you'd like you'd like to add: ");
            scanf("%f", &num1);

            printf("[child 1: receiver] Input the 2nd number you'd like you'd like to add: ");
            scanf("%f", &num2);

            printf("[child 1: reciever] Adding the following numbers: %f, %f\n", num1, num2);

            // Inform our sibling that we are ready to read
            transport_data = fopen(filename, "w");
            fprintf(transport_data,"1\n%f\n%f", num1, num2); // reset the signal
            fclose(transport_data);
            
        }

        return 1;

    }
    // The second child
    else if(child2_PID == 0)
    {
        // read
        FILE* transporter = fopen(filename, "r+");

        // close the files immediate now that they are created
        fclose(transporter);

        int signal, reading, rc;
        float num1, num2, final;
        while(1)
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
                    printf("[child 2: responder] Sum of %f and %f: \t%f\n", num1, num2, final);
                }
                else if(signal == 0 && reading == 1)
                {
                    reading = 0;
                }
            }
            // close the files immediate now that they are created
            fclose(transporter);
        }

    }
    // The parent
    else if(child2_PID > 0 && child1_PID > 0)
    {
        // The parent will just monitor the children playing, and tell them them it's bedtime when we ctrl-c the process. As in the parent will KILL the children. thats right. you heard me.
        // Monitor for ctrl-c, somehow?

        while(1)
        {

        }

    }

}