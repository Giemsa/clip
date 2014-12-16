#include <iostream>
#include "clip.hpp"

int main(int argc, const char *argv[])
{
    // command: {app name} -ab 10 20

    // create parser(description, show error)
    clip::Parser parser("this is sample app.", true);

    // create switch option if you want to create Option<bool>
    clip::Option<bool> flag(
        'a',                   // key(short arg) like "-a"
        "arg1",                // key(long arg) like "--arg1"
        "flag argument 1",     // description
        false                  // default value (if you specify default value, this option is optional)
    );

    // register option(s) to parser.
    // if you use C++11, add is defined as variadic template,
    // so you can register options at one time.
    // if you do not want to use C++11, add has only one argument.
    // add returns parser own, so you can use method chain to register some options.
    parser.add(
        flag,
        clip::Option<bool>(
            'b',
            "arg2",
            "flag argument 2"
        ),
        clip::Option<bool>(
            'c',
            "arg3",
            "flag argument 3"
        )
    );

    /*
    // C++98
    parser.add(flag);
    parser.add(
        clip::Option<bool>(
            'b',
            "arg2",
            "flag argument 2"
        )
    );
    parser.add(
        clip::Option<bool>(
            'c',
            "arg3",
            "flag argument 3"
        )
    );
    */

    // parse command line arguments.
    // parse returns true if parse arguments succeeded.
    if(parser.parse(argc, argv))
    {
        // get argument value from Option<T>
        std::cout << std::boolalpha << flag.getValue() << "\n";

        // get value from Parser using index
        std::cout << std::boolalpha << parser.getValue<bool>(0) << "\n";

        // get value from Parser using short arg name
        std::cout << std::boolalpha << parser.getValue<bool>('b') << "\n";

        // get value from Parser using long arg name
        std::cout << std::boolalpha << parser.getValue<bool>("arg3") << "\n\n";

        // getUnlabeledArgs returns unlabeled arguments
        for(const char *arg : parser.getUnlabeledArgs())
        {
            std::cout << arg << "\n";
        }

        std::cout << std::endl;
    }
    else
    {
        // parse error
        // show usage
        parser.showUsage();
    }

    return 0;
}

