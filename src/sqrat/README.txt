Sqrat - Squirrel Binding Utility

© 2009 Brandon Jones
© 2011-2013 Li-Cheng (Andy) Tai


Sqrat is a C++ binding utility for the Squirrel language.
See the docs directory for usage info.

Sqrat only contains C++ headers so for installation you just 
need to copy the files in the include directory to 
some common header path, such as /usr/local/include.

However, an autotool based build system can be checked out from
the SVN repository (not in the relase tar file to reduce file size)
which  allows you to install, on POSIX systems, with 
the familiar commands of 

autotool/configure --prefix=<prefix path>
make && make install

You can then run

make check

to run the unit tests.

Alternatively you can follow the steps in 
running_tests.txt

to directly run unit tests.

For more information, see index.html in the docs subdirectory

Discussion and User Support

Discussion about Sqrat happens at the Squirrel language forum,
the Bindings section
http://squirrel-lang.org/forums/default.aspx?g=topics&f=4

Bug Reporting

Bug reports or feature enhancement requests and patches can 
be submitted at the SourceForge Sqrat site
https://sourceforge.net/tracker/?group_id=261661&atid=2349886
