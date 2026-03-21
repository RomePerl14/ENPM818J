#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/input.h>
#include <string.h>

struct args
{
    u_int8_t id;
    char fn[200];
};

sem_t binary_sema;
void* thread_func(void *input) 
{
    int fd = open(((struct args*)input)->fn, O_RDONLY);
    int prev_key_press = 0;
    int curr_key_press = 0;
    int pressed_key;

    if(((struct args*)input)->id == 1)
    {
        while(1)
        {
            // printf("Thread %d is in the critical section.\n", ((struct args*)input)->id);
            input_event ev;
            read(fd,&ev, sizeof(ev)); // [ev.value] 0: unpressed, 1: pressed, 2: held
            
            if(ev.type == 1)
            {
                
                if(ev.value == 2)
                {
                    // do nothing, as we don't care about held inputs
                }
                else 
                {   
                    // printf("%i\n", ev.code);
                    if(ev.code == 57) // spacebar
                    {

                        curr_key_press = ev.value;
                        if(curr_key_press == 1 && prev_key_press == 0)
                        {
                            // initiate access to the critical section now that we've pressed a key. Basically, latch the key
                            printf("[thread %i] waiting for access to the critical section\n", ((struct args*)input)->id);
                            sem_wait(&binary_sema); // Wait (P)
                            // When we have access, print that we capture a key input
                            printf("[thread %i] access gained, idling for shits n giggles\n", ((struct args*)input)->id);
                            sleep(1);
                            printf("[thread %i] pretended to do a task...\n", ((struct args*)input)->id);
                            sleep(1);
                            printf("[thread %i] pretended to do a task...\n", ((struct args*)input)->id);
                            sleep(1);
                            printf("[thread %i] ding!\n", ((struct args*)input)->id);
                            sem_post(&binary_sema); // Signal (V)
                        }
                        else if(curr_key_press == 0 && prev_key_press == 1)
                        {

                            printf("[thread %i] spacebar released, awesome sauce\n", ((struct args*)input)->id);
                        }
                        prev_key_press = curr_key_press;
                    }
                }
                
            }
        }
    }
    else if(((struct args*)input)->id == 2)
    {
        while(1)
        {
            // printf("Thread %d is in the critical section.\n", ((struct args*)input)->id);
            input_event ev;
            read(fd,&ev, sizeof(ev)); // [ev.value] 0: unpressed, 1: pressed, 2: held
            
            if(ev.type == 1)
            {
                
                if(ev.value == 2)
                {
                    // do nothing, as we don't care about held inputs
                }
                else 
                {   
                    // printf("%i\n", ev.code);
                    if(ev.code == 57) // spacebar
                    {
                        curr_key_press = ev.value;
                        if(curr_key_press == 0 && prev_key_press == 1)
                        {
                            // initiate access to the critical section now that we've pressed a key. Basically, latch the key
                            printf("[thread %i] waiting for access to the critical section\n", ((struct args*)input)->id);
                            sem_wait(&binary_sema); // Wait (P)
                            // When we have access, print that we capture a key input
                            printf("[thread %i] access gained, idling for shits n giggles\n", ((struct args*)input)->id);
                            sleep(1);
                            printf("[thread %i] pretended to do a task...\n", ((struct args*)input)->id);
                            sleep(1);
                            printf("[thread %i] pretended to do a task...\n", ((struct args*)input)->id);
                            sleep(1);
                            printf("[thread %i] ding!\n", ((struct args*)input)->id);
                            sem_post(&binary_sema); // Signal (V)
                        }
                        else if(curr_key_press == 1 && prev_key_press == 0)
                        {

                            printf("[thread %i] spacebar was pressed, awesome sauce\n", ((struct args*)input)->id);
                        }
                        prev_key_press = curr_key_press;
                    }
                }
                
            }
        }
    }

    close(fd);
    return NULL;
}

int main()
{
    // First, read from /proc/bus/input/devices to figure out what the keyboard event is
    FILE* kb = fopen("/proc/bus/input/devices", "r"); // if it doesn't exist, we're in bad shape
    int loop = 1;
    int found_keyboard = 0;
    int found_event = 0;
    char buffer[1000];
    int count;
    char search_string[100];
    while(loop == 1)
    {
        if(!fgets(buffer, sizeof(buffer), kb))
        {
            printf("no keyboard device found! exiting");
            exit(-1);
        }
        // Check if we haven't found the keyboard, and if we have't, THEN check the substring
        if(found_keyboard == 0 && strstr(buffer, "keyboard") != NULL)
        {
            found_keyboard = 1;
        }
        
        if(found_keyboard == 1 && found_event == 0 && strstr(buffer, "event") != NULL)
        {
            // gonna brute force it cause IDK how else to do this right now and I don't have the time to be clever
            int we_loopin_in_here = 1;
            while(we_loopin_in_here)
            {
                snprintf(search_string, 100, "event%d", count);
                if(strstr(buffer, search_string))
                {
                    we_loopin_in_here = 0;
                    break;
                }
                count += 1;
                if(count >= 94) // should be the largest number we can fit into our buffer
                {
                    we_loopin_in_here = 0;
                    break;
                }
            }
            printf("found keyboard at event number: %d\n", count);
            loop = 0;
            break;
        }
        
    }
    fclose(kb);

    // Initialize the binary semaphore with a value of 1
    sem_init(&binary_sema, 0, 1);

    printf("Don't forget to run with sudo perms!! trust me... I'm not devious!\n");
    struct input_event ev; // using the same formatting I found online
    char filename[200];
    snprintf(filename, 200, "/dev/input/%s", search_string);

    pthread_t thread;

    struct args arg1;
    for (int i = 0; i < 200; i+=1) 
    {
        arg1.fn[i] = filename[i]; 
    }

    printf("Please enter the number 1 or 2\n\tOPTION 1: task that prints key released but waits when it is pressed\n\tOPTION 2: task which prints when key is pressed but waits when it is released\n");
    int bugg;
    scanf("%i", &bugg);
    arg1.id = bugg;


    // create threads
    pthread_create(&thread, NULL, thread_func, &arg1);

    // Wait for threads to finish
    pthread_join(thread, NULL);
    // Destroy the semaphore
    sem_destroy(&binary_sema);

    
}