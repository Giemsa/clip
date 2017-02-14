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

    // create argument(variadic argument)
    clip::Argument<std::vector<int> > numbers(
        "numbers",
        "a list of numbers"
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
        ),
        numbers
    );

    /*
    // C++98
    parser
        .add(flag)
        .add(
            clip::Option<bool>(
                'b',
                "arg2",
                "flag argument 2"
            )
        )
        .add(
            clip::Option<bool>(
                'c',
                "arg3",
                "flag argument 3"
            )
        )
        .add(numbers);
    */

    // parse command line arguments.
    const clip::ParseResult result = parser.parse(argc, argv);
    switch(result)
    {
    case clip::ParseResult::HelpShown:
    // case clip::ParseResult_HelpShown: // if C++98
        return 0;
    case clip::ParseResult::Failure:
    // case clip::ParseResult_Failure: // if C++98
        return -1;
    case clip::ParseResult::Success:
    // case clip::ParseResult_Success: // if C++98
        break;
    }

    // get argument value from Option<T>
    std::cout << std::boolalpha << flag.getValue() << "\n";

    // get value from Parser using index
    std::cout << std::boolalpha << parser.getOption<bool>(0) << "\n";

    // get value from Parser using short arg name
    std::cout << std::boolalpha << parser.getOption<bool>('b') << "\n";

    // get value from Parser using long arg name
    std::cout << std::boolalpha << parser.getOption<bool>("arg3") << "\n\n";

    // get arguments
    // parser.getArgument<std::vector<int> >(0)
    // parser.getArgument<std::vector<int> >("numbers")
    for(int arg : numbers.getValue())
    {
        std::cout << arg << "\n";
    }

    std::cout << std::endl;

    return 0;
}

