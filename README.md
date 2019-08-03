tiny-Oh!
--------
tiny-Oh! is a software project I started about a month ago. The goal is to add object oriented programming, aka OOP, to my 1970's software product tiny-c.

The title is intended to express surprise. Surprise that the goal could be done at all, and especially if it is done simply. I have not yet achieved the goal, so have no proof it is simple. But I am making progress.

The project is published on the web at 
	https://github.com/tgibson37/tiny-obj. 
Github doesn't like the bang (the character '!' is called a 'bang' by us techies,) so tiny-obj is the its official name there. 

The starting point is the tiny-c code published also on github. Use tiny-c instead of tiny-obj in the above site. That code is quite stable. In fact the site includes 42 sample tiny-c programs.

The Plan
--------
Warning: this gets wonkish...

Step one is to modify the linker making it flexible enough to link portions of text. As designed the tiny-c linker makes one pass over the entire application text, including all loaded libraries, and builds one large variable table. But the new linker must also go into the subtext of the class body which defines an object, and link that for each object created from that definition. The OOP 'new' operator triggers these sublinkings. As of this writing step one is achieved: coded, and tested. 

Step two is to modify the variable table itself, so it can accomodate the classical tiny-c variables and the new object ones. This is currently underway. The design is partial, but sufficient so coding is underway.

Future steps: implement the 'new' and 'delete' operators to create and remove objects dynamically as the program runs. And then finally the symbol search must use these modified tables to look up object symbols.

Tom Gibson
August 3, 2019


