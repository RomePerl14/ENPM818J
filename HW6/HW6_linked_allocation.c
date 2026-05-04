/* ****************************************************************
 * (c) Copyright 2025                                             *
 * Romeo Perlstein                                                *
 * ENPM818J - Real Time Operating Systems                         *
 * College Park, MD 20742                                         *
 * https://terpconnect.umd.edu/~romeperl/                         *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void set_arrays_equal(char* arr1, char* arr2)
{
    for(int i=0;i<1024;i+=1)
    {
        arr2[i] = arr1[i];
    }
}

void fill_array_with_null(char* arr, int size)
{
    for(int i =0; i<size;i+=1)
    {
        arr[i] = '\0';
    }
}

void fill_array_with_zero(int* arr, int size)
{
    for(int i =0; i<size;i+=1)
    {
        arr[i] = 0;
    }
}

void fill_array_with_number(int* arr, int size, int number)
{
    for(int i =0; i<size;i+=1)
    {
        arr[i] = number;
    }
}

void fill_multi_array_with_null(int size1, char arr[][1024]) 
{
    for(int i=0;i<size1;i+=1)
    {
        fill_array_with_null(arr[i], 1024);
    }
    
}

int Equal(char* arr1, char* arr2, int size)
{
    int equal_characters = 0;
    for(int i=0;i<size;i+=1)
    {
        if(arr1[i] != arr2[i] && arr1[i] != ' ' && arr1[i] != '\n' && arr1[i] != '\0' && arr2[i] != ' ' && arr2[i] != '\n' && arr2[i] != '\0')
        {
            return 0; // if they are at any point not the same, they are not equal
        }
    }
    return 1;
}

int main(int argc, char** argv)
{
    srand(time(NULL));

    int linked_files[16];
    for(int i=0;i<16;i+=1)
    {
        linked_files[i] = 101; // going to indicate free files as 0
    }
    printf("You have 16 blocks to store file data in! Use them wisely...\n");
    printf("Some of the blocks reserved, randomly assigning them now! standby...\n");
    int num_reserved_files = rand() % 16; // generate a number or reserved files
    printf("Number of reserved blocks: %i\n", num_reserved_files);
    int dupe = 0;
    for(int i=0;i<num_reserved_files;i+=1)
    {
        int reserved_file = rand() % 16; // generate a number or reserved files

        if(linked_files[reserved_file] != -1)
        {
            linked_files[reserved_file] = -1;
        }
        else
        {
            printf("Duplicate reserved file, skipping...\n");   
            dupe += 1;
        }
    }
    if(dupe > 0)
    {
        printf("Tried to reserve the same block %i times!\n", dupe);
    }   
    int reserved_blocks = num_reserved_files-dupe;
    printf("Number of reserved blocks: %i\n", reserved_blocks);
    printf("Current disk block (101 is an EMPTY BLOCK):\n\t%i", 0);
    for(int i=1;i<16;i+=1)
    {
        printf("\t| %i", i);
    }
    printf("\n\t%i", linked_files[0]);
    for(int i=1;i<16;i+=1)
    {
        printf("\t| %i", linked_files[i]);
    }
    printf("\n");

    // Read in from a pre-programmed file that manages dynamically allocating memory to files
    FILE *fptr_names;
    FILE *fptr_data;

    char line_in_file[1024];
    fill_array_with_null(line_in_file, 1024);

    int count = 0;
    // Open a file in read mode
    fptr_names = fopen("HW6_linked_allocation_filenames.dat", "r");
    // go until we reach end of file
    printf("\nFILES IN ORDER OF REQUEST\n---------------------------------\n");
    while(fgets(line_in_file, 1024, fptr_names))
    {
        printf("%s", line_in_file);
        count += 1;
    }
    int array_size = count; // store the length of the files - should be the same for data
    fclose(fptr_names);

    char lines_in_filenames[count][1024];
    fill_multi_array_with_null(count, lines_in_filenames);
    count = 0;
    fptr_names = fopen("HW6_linked_allocation_filenames.dat", "r");
    // go until we reach end of file
    while(fgets(line_in_file, 1024, fptr_names))
    {
        for(int i=0;i<1024;i+=1)
        {
            if(line_in_file[i] != '\0')
            {
                lines_in_filenames[count][i] = line_in_file[i];
            }
        }
        
        count += 1;
    }
    fclose(fptr_names);
    printf("\n");

    count = 0;
    // Open a file in read mode
    fptr_data = fopen("HW6_linked_allocation_filesizes.dat", "r");
    // go until we reach end of file
    while(fgets(line_in_file, 1024, fptr_names))
    {
        // printf("%s", line_in_file);
        count += 1;
    }
    if(count != array_size)
    {
        printf("\nERROR - Filename list and data in said files list are not the same length!! What are you doing!!!\n");
        return 1;
    }
    fclose(fptr_data);

    int lines_in_filedata[count];
    fill_array_with_zero(lines_in_filedata, count);
    count = 0;
    fptr_data = fopen("HW6_linked_allocation_filesizes.dat", "r");
    // go until we reach end of file
    while(fgets(line_in_file, 1024, fptr_names))
    {
        lines_in_filedata[count] = atoi(line_in_file);
        count += 1;
    }
    fclose(fptr_data);
    printf("\n");


    // get unique elements
    char get_unique_elements[array_size][1024];
    fill_multi_array_with_null(array_size, get_unique_elements);
    // fill up the first index with the first element
    set_arrays_equal(lines_in_filenames[0], get_unique_elements[0]);
    int get_unique_elements_size = 1;

    // go through the rest of the list and pull out the unique elements
    for(int i=1;i<array_size;i+=1)
    {
        int unique = 1;
        for(int ii=0;ii<get_unique_elements_size;ii+=1)
        {
            if(Equal(lines_in_filenames[i], get_unique_elements[ii], 1024) == 1)
            {
                unique = 0; // we found a unique item
            }
        }

        // if it's unique, insert it into the array
        if(unique == 1)
        {
            set_arrays_equal(lines_in_filenames[i], get_unique_elements[get_unique_elements_size]);
            get_unique_elements_size+=1;
        }
    }

    char unique_elements[get_unique_elements_size][1024];
    fill_multi_array_with_null(get_unique_elements_size, unique_elements);
    for(int ii=0;ii<get_unique_elements_size;ii+=1)
    {
        set_arrays_equal(get_unique_elements[ii], unique_elements[ii]);
    }

    // Now, we have a list of the unique elements, the actual elements, and the data they hold.
    // Lets now fill our memory blocks with the files and their data using a linked list
    int idx = 0;
    // I think, make another 2D array
    int linked_files_previous_index[get_unique_elements_size]; // this array is 1:1 with the unique_elements_array, and will store the last block with data in it in the lnked_files array
    int linked_files_start_index[get_unique_elements_size];
    char linked_files_order[get_unique_elements_size][1024];
    int linked_files_order_count[get_unique_elements_size];
    fill_multi_array_with_null(get_unique_elements_size, linked_files_order);
    fill_array_with_zero(linked_files_previous_index, get_unique_elements_size);
    fill_array_with_zero(linked_files_order_count, get_unique_elements_size);
    fill_array_with_number(linked_files_start_index, get_unique_elements_size, -1);

    for(int i=0;i<array_size;i+=1)
    {
        // First, lets get the data size for this current file we're interacting with
        int data_size = lines_in_filedata[i];

        // next, lets get the file name;
        char file_name[1024];
        set_arrays_equal(lines_in_filenames[i], file_name);
        int file_index = 0;

        // get the file index
        for(int iii=0; iii<get_unique_elements_size; iii+=1)
        {
            if(Equal(file_name, unique_elements[iii], 1024) == 1)
            {
                // if it's equal, get the index it's equal at
                file_index = iii; // get the file we're currently on
                break;
            }
        }

        // Now, lets put fill the first set of open blocks with our current files current size
        int loop_num = 0;
        int out_of_mem = 0;
        for(int ii=0;ii<data_size;ii+=1)
        {
            while(linked_files[idx] == -1 || linked_files[idx] != 101) // keep iterating until we reach a non-reserved block
            {
                idx+=1;
                if(idx > 15)
                {
                    idx = 0;
                    loop_num += 1;
                }
                if(loop_num == 2)
                {
                    out_of_mem = 1;
                    break;
                }
            }
            if(out_of_mem == 1)
            {
                break;
            }
            // check if we've started the file yet, and if not, get the starting index
            if(linked_files_start_index[file_index] == -1)
            {
                linked_files_start_index[file_index] = idx;
                linked_files_previous_index[file_index] = idx;
            }

            linked_files[linked_files_previous_index[file_index]] = idx; // go back to the previous index, and set it to the current index
            linked_files_previous_index[file_index] = idx; // store the current index for next time
            linked_files[idx] = 0; // assume that this is the current EOF, until out next iteration lol

            // convert the value to a 'string'
            int num_size = 10;
            char str[num_size];
            fill_array_with_null(str, num_size);
            sprintf(str, "%i,", idx);
            int num_size_actual = 0;
            for(int j=0;j<num_size;j+=1)
            {
                if(str[j] == '\0')
                {
                    num_size_actual = j;
                    break;
                }
            }
            for(int k=0;k<num_size_actual;k+=1)
            {
                linked_files_order[file_index][linked_files_order_count[file_index]+k] = str[k];
            }
            linked_files_order_count[file_index] += num_size_actual;
            // printf("%s ", linked_files_order[file_index]);
            idx += 1; // iterate up
            
        }
        if(out_of_mem == 1)
        {
            printf("\nOut of memory! Unable to fully allocated file data\n");
            printf("Stopped attempting at file #%i, [file] %s", i+1, lines_in_filenames[i]);
            break;
        }
    }

    for(int i=0;i<get_unique_elements_size;i+=1)
    {
        for(int ii=0;ii<1024;ii+=1)
        {
            if(unique_elements[i][ii] == '\n')
            {
                unique_elements[i][ii] = '\0';
            }
        }
    }
    // Now, print a table showing the indexing of each file
    printf("\n\tFILE ACCESS TABLE\n\t---------------------------------------\n\tfile\t\t|File Access Order\n\t---------------------------------------\n\t");
    for(int i=0;i<get_unique_elements_size;i+=1)
    {
        printf("%s\t| %s\n\t",unique_elements[i], linked_files_order[i]);
    }

    printf("FINAL MEMORY BLOCK:\n\t%i", 0);
    for(int i=1;i<16;i+=1)
    {
        printf("\t| %i", i);
    }
    printf("\n\t%i", linked_files[0]);
    for(int i=1;i<16;i+=1)
    {
        printf("\t| %i", linked_files[i]);
    }
    printf("\n");

    // for(int i=0;i<array_size;i+=1)
    // {
    //     printf("%s", get_unique_elements[i]);
    // }
    printf("\n");
    

}