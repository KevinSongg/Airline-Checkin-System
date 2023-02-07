V00927475
A01
Kunlun Song

Instructions to run the airline check in system

1. Put all files in the same folder
2. Run the command 'make' from the terminal
3. Next do ./ACS <input text file>  in the terminal

The sample input file will have to be in this structure:
1. The first character specifies the unique ID of customers.
2. A colon(:) immediately follows the unique number of the customer.
3. Immediately following is an integer equal to either 1 (indicating the customer belongs to business class) or 0 (indicating the customer belongs to economy class).
4. A comma(,) immediately follows the previous number.
5. Immediately following is an integer that indicates the arrival time of the customer.
6. A comma(,) immediately follows the previous number.
7. Immediately following is an integer that indicates the service time of the customer.
8. A newline (\n) ends a line.

Here is an example: 
8
1:0,2,60
2:0,4,70
3:0,5,50
4:1,7,30
5:1,7,40
6:1,8,50
7:0,10,30
8:0,11,50

Note: When the queue reaches greater than 2, the customer id will print the front of the linked list instead of the back. Thus, this will result in the same customer id to be printed over and over as if the same customer is being ran again and again. For example, if the linked list has customers 4,3,2,1 and 1 is the first in the queue, it will continuously print 4 even though 1 will be removed.This is just a UI bug and it will actually run the right customer until linkedlist is empty. I have tried to fix this problem but ran into a bunch of segmentation faults so I left it as a UI bug for now.