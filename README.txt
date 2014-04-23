By Yang Du, Bing Xue

***************************

TODO:
    1. need to change gotoDirectory to return filename with check for existing
	directory, also need to distinguish the trailing '/' in path name for
	mkdir.
	
	2. Should keep a list of children in the parent and their working
	directory <--- currently we only host one common working directory, we
	need to store separate working direcotries for all user programs
