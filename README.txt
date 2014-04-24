By Yang Du, Bing Xue

**************************

Update:

TODO:

1. Need to pass working directory of user in msg
2. Need to add linking info in msg
3. Need to chagen gotoDirectory to receive working directory as arguement for
relative path 
4. Need to free all Inode read from readInode ####
5. Need to update nlink in mkDir

6. Need to link chDir back to user

7. fileName is not freed in all cases
***************************

TODO:
    1. need to change gotoDirectory to return filename with check for existing
	directory, also need to distinguish the trailing '/' in path name for
	mkdir.
	
	2. Should keep a list of children in the parent and their working
	directory <--- currently we only host one common working directory, we
	need to store separate working direcotries for all user programs
