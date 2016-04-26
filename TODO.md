
Known TODO's and FIXME's
------------------------
    main.c:
        Work on creating a main generic system for displaying 'screens'.
            Lots of code could be shared between the LinkScreen and CommentScreen
            structures and functions, and a more generic 'credditScreen' structure
            would be the best option. Some basic inheritance and a basic function
            table setup for each type of screen would work well. These changes
            would allow for a more generic main-loop instead of different
            main-loops for each type of screen and different functions for
            each. It would reduce the large amount of redundancy that's
            currently in the code

            Keypreses could be stored in simple arrays, and then jump to actions
            via function pointers. This could easily be expanded upon to provide some nice
            customization for keypresses.

        Add config file support
            The project itself is starting to support more features, such as
            logging in to Reddit. The simplest way to handle this is to provide a
            simple ~/.credditrc file or similar, which options for Ex. Username,
            Password, perhaps color options, etc. choices about what to use to
            parse the config files, what syntax it should use, what it should
            support, etc. should be considered.
        
        Fix issue with & displaying as &amp; in link or comment text
