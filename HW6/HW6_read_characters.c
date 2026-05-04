/* ****************************************************************
 * (c) Copyright 2025                                             *
 * Romeo Perlstein                                                *
 * ENPM818J - Real Time Operating Systems                         *
 * College Park, MD 20742                                         *
 * https://terpconnect.umd.edu/~romeperl/                         *
 ******************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h> // used for signaling things, i think, idk what it does


static struct termios oldt, newt;
int looping = 1;

void handle_exit(int signal_thingy)
{
   if(signal_thingy == SIGINT)
   {
      printf("\nExiting\n");
      looping = 0;
   }
}

int main(int argc, char** argv)
{

   // set up the signal catcher
   struct sigaction act; //sig action struct
   act.sa_handler = &handle_exit; // Set the signal handler as the default action
   act.sa_flags = 0;
   sigaction(SIGINT, &act, NULL); // set it

   // get the user input for their sentence
   printf("\n\t\tWelcome to romeo's word and letter counter!\n");

   int letter_limit = 16384; // just make it huge
   char words[letter_limit]; // allocated words buffer to a set size

   while(looping)
   {
      printf("\nInput a letter, word, or even a sentance!\n");
      for(int i=0;i<letter_limit;i+=1){words[i] = '\0';}
      fgets(words, letter_limit, stdin);
      if(!looping) continue;
      int is_word = 0;
      int is_letter = 0;
      int is_space = 0;
      int total_characters = 0;
      char previous_letter = words[0];
      printf("\tInputed text\t| ");
      for(int i=0;i<letter_limit;i+=1)
      {
         if(words[i] != '\0')
         {
            is_letter += 1;
            // now, start counting letters and spaces
            if(words[i] == ' ')
            {
               is_word += 1; // note that we've read a word
               is_space += 1; // not that we've read a space
               is_letter -= 1;
            }
            if(words[i] == '\n' )
            {
               total_characters -= 1;
               is_letter -= 1;
               is_word += 1;

               if(previous_letter == ' ')
               {
                  is_word -= 1;
               }
            }
            previous_letter = words[i];
            total_characters += 1;
            printf("%c", words[i]);
         }
      }
      // format output
      printf("\tWord count\t| %i\n", is_word);
      printf("\tLetter count\t| %i\n", is_letter);
      printf("\tSpace count\t| %i\n", is_space);
      printf("\tCharacter count\t| %i\n", total_characters);
   }
   return 0;
   
 }