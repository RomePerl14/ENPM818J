#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/input.h>
#include <string.h>

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

    printf("Don't forget to run with sudo perms!! trust me... I'm not devious!\n");
    struct input_event ev; // using the same formatting I found online
    char filename[200];
    snprintf(filename, 200, "/dev/input/%s", search_string);

    int fd = open(filename, O_RDONLY);

    while(1)
    {
            read(fd,&ev, sizeof(ev)); // [ev.value] 0: unpressed, 1: pressed, 2: held
            if(ev.type == 1 && ev.value == 1)
            {
                printf("Key: %i State: %i\n",ev.code,ev.value);
            }
    }

    close(fd);
}