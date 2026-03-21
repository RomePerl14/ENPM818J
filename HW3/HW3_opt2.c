#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/input.h>
#include <string.h>
#include <signal.h>


int looping = 1;
void loop_handler(int signal_thingy)
{
    looping = 0;
}
sem_t binary_sema;
input_event ev;
int prev_key_press = 0;
int curr_key_press = 0;
int desired_key = 57; // spacebar




void* thread_func(void* arg) 
{
    int in_crit_section;
    int curr = 0;
    int prev = 0;
    int released = 0;
    int id = *(int*)arg;
    while(looping)
    {
        prev = curr;
        curr = curr_key_press;
        if(curr == 0 && prev == 0 && released == 1)
        {
            released = 0;
            in_crit_section = 0;
            sem_post(&binary_sema); // release the mutex
        }
        if(id == 1)
        {
            if(in_crit_section == 0)
            {
                printf("\n[THREAD 1] waiting for access to the critical section");
                sem_wait(&binary_sema); // Wait to get access to the key press variables
                // once we have access, act on the variables
                printf("\n[THREAD 1] accessed the critical section");
                in_crit_section = 1;
                continue; // reloop
            }
            else if(in_crit_section == 1)
            {
                
                if(curr == 0 && prev == 1 && released == 0)
                {
                    printf("\n[THREAD 1] spacebar released, awesome sauce");
                    released = 1;
                }
                else if(curr == 0 && prev == 0 && released == 1)
                {
                    released = 0;
                    in_crit_section = 0;
                    sem_post(&binary_sema); // release the mutex
                }
            }
        }
        if(id == 2)
        {
            if(in_crit_section == 0)
            {
                printf("\n[THREAD 2] waiting for access to the critical section");
                sem_wait(&binary_sema); // Wait to get access to the key press variables
                // once we have access, act on the variables
                printf("\n[THREAD 2] accessed the critical section");
                in_crit_section = 1;
                continue; // reloop
            }
            else if(in_crit_section == 1)
            {
                if(curr == 1 && prev == 0 && released == 0)
                {
                    printf("\n[THREAD 2] spacebar pressed, awesome sauce");
                    released = 1;
                }
                
            }
        }
    }
    return NULL;
}

int main()
{
    signal(SIGTERM, loop_handler);
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
    printf("Press the space bar!\n");
    struct input_event ev; // using the same formatting I found online
    char filename[200];
    snprintf(filename, 200, "/dev/input/%s", search_string);

    pthread_t thread1;
    pthread_t thread2;
    int id1 = 1;
    int id2 = 2;
    
    // create threads
    pthread_create(&thread1, NULL, thread_func, &id1);
    pthread_create(&thread2, NULL, thread_func, &id2);

    int fd = open(filename, O_RDONLY);
    while(looping)
    {
        read(fd,&ev, sizeof(ev)); // [ev.value] 0: unpressed, 1: pressed, 2: held
        if(ev.type == 1)
        {
            
            if(ev.value == 2)
            {
                // do nothing, as we don't care about held inputs
            }
            else 
            {   
                if(ev.code == 57) // spacebar
                {
                    curr_key_press = ev.value;
                    
                }
            }
            
        }
        usleep(10000); // only read every 100 hertz
    }

    // Wait for threads to finish
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    // Destroy the semaphore
    sem_destroy(&binary_sema);
    close(fd);

    
}