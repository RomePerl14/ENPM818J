#!/bin/bash

echo building exectuables! 
echo read characters file: ./rc
echo linked_allocation simulation: ./linked

g++ HW6_read_characters.c -o rc
g++ HW6_linked_allocation.c -o linked

echo done!