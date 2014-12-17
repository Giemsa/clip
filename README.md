# clip -tiny command line argument parser-

 clip(tiny **C**ommand **Li**ne argument **P**arser) is a simple and compact command line argument parser for C++. Include only 1 header file and you can parse command line arguments. 
 
## How to use
1. Include "clip.hpp" on your project.
2. Define options using Option\<T\>

		clip::Option<int> option(
			'n',			// label(short name) specified like '-n'
			"number",		// label(long name) specified like '--number'
			"number",		// argument name(used by showing usage)
			"number option" // description
			// ,100			// default value
		);
	 If you specify default value, the option is optional. This example's option accepts only integer. The template argument means the type it accepts.
3. Define arguments using Argument\<T\>

		clip::Argument<std::string> arg(
			"arg",				 // name
			"string argument",   // description
			// , "Hello, World!" // default value 
		);
	If you specify third argument, the argument is optional, the same as option.

4. Create Parser object

		clip::Parser parser("this is test app", true);
	  The first argument is description of this application, used by "--help". And second argument is a setting whether clip shows errors on stdout.
5. Add options and arguments to the parser

		parser.add(option, arg);

	 If you use compiler that supports C++11, "add" is defined as variadic template function. So you can add some options at one time. If your compiler does not support C++11 or you do not want to use C++11, "add" has only one argument. However, you can add many options to parser using method chain because "add" returns parser object itself. 
6. Parse passed arguments

		const bool success = parser.parse(argc, argv);
	"parse" returns result of parsing. If it returns false, some parse error occurred. If you set true at the second argument of Parser's constructor, parse shows errors on stderr automatically.

7. Get option values  
	 If parsing succeeded, you can get values specified as command line arguments. There are two ways to get result.  
	 First, you get value from Option\<T\> object. 

		const int value = arg.getValue();

	"getValue" method returns the value which type is T. T is int on this example, so "getValue" returns int type value. 
	 Second, you can also get value from Parser object.

		const int value = parser.getOption<int>(0); // 0 means getting first argument

	 Parser::getOption\<T\>(N) returns the (N)th argument as type T. In addition to this, there are two ways to get value from Parser object, find by short name or long name.

		const int value = parser.getValue<int>('n');
		const int value = parser.getValue<int>('number');
8. Get argument values  
	You can also get argument value from Argument\<T\> object.

		const std::string &value = arg.getValue();

	Or you can get from Parser::getArgument\<T\>()

		const std::string &value = parser.getArgument<std::string>(0);
		const std::string &value = parser.getArgument<std::string><("arg");

## How to pass arguments
 clip supports only unix style options.

	./App -a -b -c

 The parser that satisfies above options is:

	clip::Option<bool> a('a', "aaa", "desc");
	clip::Option<bool> b('b', "bbb", "desc");
	clip::Option<bool> c('c', "ccc", "desc");
	clip::Parser parser;
	parser.add(a, b, c);

 About Option\<bool\>, please refer to the following section.  
We present more example.

	clip::Option<int> a('a', "aaa", "aaa", "desc");
	clip::Option<double> b('b', "bbb", "bbb", "desc");
	clip::Argument<std::vector<std::string>> args("args", "desc");
	clip::Parser parser;
	parser.add(a, b, args);
	const std::vector<std::string> &args = args.getValue();

 This code can parse following options.

	./App -a 10 --bbb 20.5 hello world


## Show usage
 If you type "-h" or "--help", clip shows usage on stdout. Also you can get usage by "getUsage".

	const std::string &usage = parser.getUsage();

 Or you show on stdout using following function

	parser.showUsage();

## Option
### Option\<T\>
 Option\<T\> means a command line argument. If you specify int to T, the option accepts only int style string. You specify not int buf double, it accepts double style string. Parser uses std::ostringstream::operator\>\>, so Option\<T\> accepts T when the operator can parse passed string.

### Option\<bool\>
 Option\<bool\> is used when options is used as switch. Unlike other options, this option does not take a command line argument. If option specified, the value is true.  
 In addition, there is one more feature. You can pass arguments like following:

	./App -abc

 It is the same meaning as this,

	./App -a -b -c

 Option\<bool\> is a bit different from Option\<T\>. The constructor of Option\<bool\> does not need "name" because it is not used in showing usage.

	clip::Option<bool> arg(
		'a',			// label
		"arg",		  // label
						// name is not needed!
		"bool argument" // description
	):	

### Option\<std::vector\<T\> \>
 If you specify std::vector\<T\> to template argument of Option\<T\>, this option is multiple argument option.

	clip::Option<std::vector<int> > arg(
		'n',
		"numbers",
		"numbers",
		"multiple number argument"
	);

 This option accepts following argument.

	./App -n 10 20 30

 And you can get these values as std::vector.

	const std::vector<int> &values = arg.getValue();
	// values = (10, 20, 30)

## Argument
### Argument\<T\>
 Argument\<T\> is similer to Option\<T\>, but there is a difference that this is used without a label. So the constructor does not have "key" and "longkey" parameters, and the order that is passed is important.
### Argument\<std::vector\<T\>\>
 Argument\<std::vector\<T\>\> is similer to Option\<std::vector\<T\>\>, this is accept variable arguments. Because of the arguments order is important, you must set this to the last of arguments.

## Example
 See "main.cpp" 

## License
This software is released under the zlib License
 