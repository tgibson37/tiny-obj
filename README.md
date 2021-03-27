tiny-obj
--------
tiny-obj is a software project I started June, 2019. The goal is to add object oriented programming, aka OOP, to my 1970's software product tiny-c.

The project is published on the web at 
	https://github.com/tgibson37/tiny-obj. 

The starting point is the tiny-c code published also on github. Use tiny-c instead of tiny-obj from the above tgibson37 site. The tiny-c code is quite stable. In fact the site includes 42 sample tiny-c programs. Look at tiny-obj if you are curious, or want to experiment with me. The download includes a regression test bash script with so far 10 tests. Read these tests. They are like "bread crumbs" revealing the path I have taken so far.

After downloading the zip (or cloning if you are ambitious) follow these steps.
	1 - The zip is a folder, tiny-obj-master. Unzip and that folder will appear in your current working directory. 
	2 - cd into tiny-obj-master. Check it has a source directory, src. and bash scripts test and testall. 
	3 - If not there already, mkdir two new directories, obj and bin. Leave them empty, but make expects them to be there.
	4 - Do a make. It should compile ok. (I always follow a git push with a github download into a clean directory and do a final test there.)
	5 - ./testall will run three suites of tests. Lots of drawings pop up. Hit alt-f4 for each (about a dozen). ./test alone runs the most recent suite.    ./test -? for more info on these test scripts.
	6 - There should be a soft link, toc, which points to bin/toc. And sample programs in SamplePrograms. So .toc Sa*/pf.tc runs pyrhanna fish, a classic
	tiny-c program, and ./toc Sa*/ot01* runs ot01basics.toc. The (currently) ten toc programs otNN... are the ten tests.
Tom Gibson, March 26, 2021

Original from August, 2019...
The Plan
--------
Step one is to modify the linker making it flexible enough to link portions of text. As designed the tiny-c linker makes one pass over the entire application text, including all loaded libraries, and builds one large variable table. But the new linker must also go into the subtext of the class body which defines an object, and link that for each object created from that definition. The OOP 'new' operator triggers these sublinkings. As of this writing step one is achieved: coded, and tested. 

Step two is to modify the variable table itself, so it can accomodate the classical tiny-c variables and the new object ones. (Aug 4, 2019 update): Both class and oref entries DONE.

Future steps: implement the 'new' and 'delete' operators to create and remove objects dynamically as the program runs. And then finally the symbol search must use these modified tables to look up object symbols. Status: new creates an object blob with var table and value space. 

Overall status: One OO program, first.toc, runs to completion on Linux. It is in the SamplePrograms folder. Also several legacy tc programs run. A pretty cool bash script, test, does regression testing. See the dir TF for the test files.

Tom Gibson
August 14, 2019
Updated, October 25, 2019


