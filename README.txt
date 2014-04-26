COMP421 Lab3 YalnixFileSystem

by Yang Du (yd15), Bing Xue (bx3)

1. Organization

   global.h : contains all the global variables, struct and type defitions and
   util function headers

   yfs.c : contains the YFS server code 

   iolib.c: contains the User library code

2. Functionality
   
   #Create, Open, Close fully functional
   #MkDir, ChDir, RmDir fully functional
   #Read, Write fully functional
   #Link, SymLink, ReadLink fully functional
   #Unlink seem to have bug in dealing with type INODE_SYMLINK

3. Testing
   We used trace printing extensively to test our program, for tracelevel:
   0: you can print the function entrance and output, this is also the 
      level with all the ERROR messages
   300-350: there is detailed display of all the stages in a function call
            and will print out the accessed inode/dirEntry
   350-500: will print out disk info along the way
   >500: this will enable all the traceprint in the code

4. Notes
   
   Symlink traversal does not have limited recursion depth, we rely on
   checking the length of the pathname given before we proceed to symlink
   traversal.

   Link does not return error on invalid oldpathname but create a symlink
   for it.
