#ifndef MITHRILWORKS_CLIP_H
#define MITHRILWORKS_CLIP_H

/*
 * clip -tiny command line argument parser-
 * Copyright (c) 2014 Giemsa/MithrilWorks.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *    1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 *    2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 *    3. This notice may not be removed or altered from any source
 *    distribution.
 */

#if __cplusplus > 199711L
#   define MW_CPP11
#   define ENUM_ELEM(T, E) T :: E
#else
#   define ENUM_ELEM(T, E) T ## _ ## E
#   define final
#   define override
#   define nullptr NULL
#endif

#include <exception>
#include <vector>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace clip
{
    class Parser;

#ifdef MW_CPP11
    enum class ParseResult
    {
        Success,
        Failure,
        HelpShown
    };
#else
    enum ParseResult
    {
        ParseResult_Success,
        ParseResult_Failure,
        ParseResult_HelpShown
    };
#endif

    namespace detail
    {
        template<typename T>
        struct TypeID final
        {
            static const std::size_t getUID() { return reinterpret_cast<std::size_t>(&getUID); }
        };

        class Base
        {
            friend clip::Parser;
        private:
            std::size_t uid_;
            std::string name_, desc_;
            bool optional_, set_, ref_;

            template<typename T>
            bool is() const { return uid_ == TypeID<T>::getUID(); }
            std::size_t getUID() const { return uid_; }
            bool isSet() const { return set_; }
            virtual void buildArguments(std::ostringstream &oss) const = 0;
        protected:
            bool used(const bool f) { return set_ = !f; }
        public:
            Base(const std::size_t uid, const char *name, const char *desc, const bool optional)
            : uid_(uid), name_(name), desc_(desc), ref_(false), set_(false), optional_(optional)
            { }
            virtual ~Base() { }

            virtual bool setValue(const char *value) = 0;
            virtual bool isMultiArg() const { return false; }
            const std::string &getName() const { return name_; }
            const std::string &getDesc() const { return desc_; }
            bool isOptional() const { return optional_; }
        };

        class OptionBase : public Base
        {
            friend class clip::Parser;
        private:
            char key_;
            std::string longkey_;

        public:
            OptionBase(
                const std::size_t uid, const char key, const char *longkey, const char *name,
                const char *desc, const bool optional
            )
            : Base(uid, name, desc, optional), key_(key), longkey_(longkey)
            { }

            virtual ~OptionBase() { }

            char getKey() const { return key_; }
            const std::string &getLongKey() const { return longkey_; }
        };

        class ArgumentBase : public Base
        {
            friend class clip::Parser;
        public:
            ArgumentBase(const std::size_t uid, const char *name, const char *desc, const bool optional)
            : Base(uid, name, desc, optional)
            { }

            virtual ~ArgumentBase() { }
        };

        /* key comparator */
        class KeyComparator
        {
        private:
            char key_;
        public:
            KeyComparator(const char key) : key_(key) { }
            bool operator()(const OptionBase *option) const { return key_ == option->getKey(); }
        };

        class LongKeyComparator
        {
        private:
            const char *key_;
        public:
            LongKeyComparator(const char *key) : key_(key) { }
            bool operator()(const OptionBase *option) const { return key_ == option->getLongKey(); }
        };

        class NameComparator
        {
        private:
            const char *name_;
        public:
            NameComparator(const char *name) : name_(name) { }
            bool operator()(const ArgumentBase *arg) const { return name_ == arg->getName(); }
        };

        struct KeySort
        {
            bool operator()(const OptionBase *lhs, const OptionBase *rhs) const { return lhs->getKey() < rhs->getKey(); }
        };

        struct Separator
        {
            bool operator()(const char c) const { return c == '\\' || c == '/'; }
        };
    }

    /* option */
    template<typename T>
    class Option final : public detail::OptionBase
    {
    private:
        T value_, defaultValue_;

        bool setValue(const char *value) override
        {
            std::stringstream ss(value);
            ss >> value_;
            return used(ss.fail());
        }

        void buildArguments(std::ostringstream &oss) const override
        {
            if(isOptional()) { oss << "[-" << getKey() << " <" << getName() << ">] "; }
            else             { oss <<  "-" << getKey() << " <" << getName() << "> ";  }
        }

    public:
        Option(const char key, const char *longkey, const char *name, const char *desc)
        : detail::OptionBase(detail::TypeID<T>::getUID(), key, longkey, name, desc, false)
        , value_(T()), defaultValue_(T())
        { }

        Option(const char key, const char *longkey, const char *name, const char *desc, const T &defaultValue)
        : detail::OptionBase(detail::TypeID<T>::getUID(), key, longkey, name, desc, true)
        , value_(T()), defaultValue_(defaultValue)
        { }

        T getValue() const { return value_; }
    };

    /* option specialized for switch */
    template<>
    class Option<bool> final : public detail::OptionBase
    {
    private:
        bool value_, defaultValue_;

        bool setValue(const char *value) override
        {
            value_ = true;
            return used(false);
        }

        void buildArguments(std::ostringstream &oss) const override
        {
            if(isOptional()) { oss << "[-" << getKey() << "] "; }
            else             { oss <<  "-" << getKey() << " "; }
        }

    public:
        Option(const char key, const char *longkey, const char *desc)
        : detail::OptionBase(detail::TypeID<bool>::getUID(), key, longkey, "", desc, true)
        , value_(false), defaultValue_(false)
        { }

        Option(const char key, const char *longkey, const char *desc, const bool defaultValue)
        : detail::OptionBase(detail::TypeID<bool>::getUID(), key, longkey, "", desc, true)
        , value_(false), defaultValue_(defaultValue)
        { }

        bool getValue() const { return value_; }
    };

    /* option specialized for multiple argument */
    template<typename T>
    class Option<std::vector<T> > final : public detail::OptionBase
    {
    private:
        std::vector<T> value_, defaultValue_;

        bool setValue(const char *value) override
        {
            T tmp;
            std::stringstream ss(value);
            ss >> tmp;
            value_.push_back(tmp);
            return used(ss.fail());
        }

        void buildArguments(std::ostringstream &oss) const override
        {
            if(isOptional()) { oss << "[-" << getKey() << " <" << getName() << "...>] "; }
            else             { oss <<  "-" << getKey() << " <" << getName() << "...> ";  }
        }

        bool isMultiArg() const override { return true; }
    public:
        Option(const char key, const char *longkey, const char *name, const char *desc)
        : detail::OptionBase(detail::TypeID<std::vector<T> >::getUID(), key, longkey, name, desc, false)
        , value_(false)
        { }

        Option(const char key, const char *longkey, const char *name, const char *desc, const std::vector<T> &defaultValue)
        : detail::OptionBase(detail::TypeID<std::vector<T> >::getUID(), key, longkey, name, desc, true)
        , value_(false), defaultValue_(defaultValue)
        { }

        std::vector<T> getValue() const { return value_; }
    };

    template<typename T>
    class Argument final : public detail::ArgumentBase
    {
    private:
        T value_, defaultValue_;

        bool setValue(const char *value) override
        {
            std::stringstream ss(value);
            ss >> value_;
            return used(ss.fail());
        }

        void buildArguments(std::ostringstream &oss) const override
        {
            if(isOptional()) { oss << "[" << getName() << "] "; }
            else             { oss <<        getName() << " ";  }
        }
    public:
        Argument(const char *name, const char *desc)
        : ArgumentBase(detail::TypeID<T>::getUID(), name, desc, false)
        , value_(T()), defaultValue_(T())
        { }

        Argument(const char *name, const char *desc, const T &defaultValue)
        : ArgumentBase(detail::TypeID<T>::getUID(), name, desc, true)
        , value_(T()), defaultValue_(defaultValue)
        { }

        T getValue() const { return value_; }
    };

    template<typename T>
    class Argument<std::vector<T> > final : public detail::ArgumentBase
    {
    private:
        std::vector<T> value_, defaultValue_;

        bool isMultiArg() const override { return true; }
        bool setValue(const char *value) override
        {
            T tmp;
            std::stringstream ss(value);
            ss >> tmp;
            value_.push_back(tmp);
            return used(ss.fail());
        }

        void buildArguments(std::ostringstream &oss) const override
        {
            if(isOptional()) { oss << "[" << getName() << "...] "; }
            else             { oss <<        getName() << "... ";  }
        }
    public:
        Argument(const char *name, const char *desc)
        : ArgumentBase(detail::TypeID<std::vector<T> >::getUID(), name, desc, false)
        { }

        Argument(const char *name, const char *desc, const std::vector<T> &defaultValue)
        : ArgumentBase(detail::TypeID<std::vector<T> >::getUID(), name, desc, true)
        , defaultValue_(defaultValue)
        { }

        std::vector<T> getValue() const { return value_; }
    };

    class Parser final
    {
        typedef std::vector<detail::OptionBase *> OptionList;
        typedef std::vector<detail::ArgumentBase *> ArgumentList;
        enum ArgType
        {
            ArgType_Value,
            ArgType_Key,
            ArgType_LongKey
        };

    private:
        std::string description_, appName_, usage_;
        std::ostringstream errorMsg_;
        OptionList options_;
        ArgumentList arguments_;
        bool dirty_, showErrors_, varg_;

        template<typename T>
        void clear(T &list)
        {
            for(typename T::const_iterator it = list.begin(); it != list.end(); ++it)
            {
                if(!(*it)->ref_) { delete *it; }
            }

            list.clear();
        }

        void init()
        {
            options_.push_back(
                new Option<bool>(
                    'h',
                    "help",
                    "display usage and information.",
                    false
                )
            );
        }

        void setAppName(const std::string &str)
        {
            appName_.assign(
                std::find_if(str.rbegin(), str.rend(), detail::Separator()).base(),
                str.end()
            );
        }

        template<typename T, typename U>
        typename T::value_type find(const T &begin, const T &end, const U& comp) const
        {
            const T it = std::find_if(begin, end, comp);
            return it == end ? nullptr : *it;
        }

        detail::OptionBase *findByKey(const char key) const { return find(options_.begin(), options_.end(), detail::KeyComparator(key)); }
        detail::OptionBase *findByLongKey(const char *key) const { return find(options_.begin(), options_.end(), detail::LongKeyComparator(key)); }
        detail::ArgumentBase *findByName(const char *name) const { return find(arguments_.begin(), arguments_.end(), detail::NameComparator(name)); }

        ArgType checkType(const char *key) const
        {
            if(*key == '-')
            {
                if(*++key == '-') { return ArgType_LongKey; }
                return ArgType_Key;
            }

            return ArgType_Value;
        }

        bool parseArguments(ParseResult &r, int &i, const int argc, const char *argv[], detail::OptionBase *option, const char *key)
        {
            if(!option)
            {
                errorMsg_ << "invalid argument name specified: " << key << "\n";
                r = end();
                return true;
            }

            if(option->isMultiArg())
            {
                for(++i; i < argc; ++i)
                {
                    const char *value = argv[i];
                    if(checkType(value) != ArgType_Value)
                    {
                        --i;
                        break;
                    }

                    if(!option->setValue(value))
                    {
                        r = invalidTypeError(option);
                        return true;
                    }
                }
            }
            else
            {
                if(option->is<bool>())
                {
                    option->setValue(nullptr);
                    return false;
                }

                if(++i < argc)
                {
                    const char *value = argv[i];
                    if(checkType(value) == ArgType_Value)
                    {
                        if(option->isSet() || !option->setValue(value))
                        {
                            r = fewArgError(option);
                            return true;
                        }
                    }
                }
                else
                {
                    r = fewArgError(option);
                    return true;
                }
            }

            return false;
        }

        bool showHelp()
        {
            const detail::OptionBase *option = findByKey('h');
            if(option && option->is<bool>() && static_cast<const Option<bool> *>(option)->getValue())
            {
                showUsage();
                return true;
            }

            return false;
        }

        ParseResult fewArgError(const detail::OptionBase *option)
        {
            errorMsg_ << "argument should be specified for -" << option->getKey() << "(" << option->getLongKey() << ")"<< "\n";
            return end();
        }

        ParseResult invalidTypeError(const detail::OptionBase *option)
        {
            errorMsg_ << "invalid type was specified for -" << option->getKey() << "(" << option->getLongKey() << ")\n";
            return end();
        }

        ParseResult end()
        {
            if(showHelp()) { return ENUM_ELEM(ParseResult, HelpShown); }
            const bool r = getErrorMessage().empty();
            if(showErrors_ && !r)
            {
                std::cerr << getErrorMessage() << std::endl;
            }

            return r ? ENUM_ELEM(ParseResult, Success) : ENUM_ELEM(ParseResult, Failure);
        }

        template<typename T, typename U>
        T getValue(const detail::Base *base) const throw(std::bad_cast)
        {
            if(!base->is<T>()) { throw std::bad_cast(); }
            return std::move(static_cast<const U *>(base)->getValue());
        }

        Parser &add(detail::OptionBase *option) throw(std::runtime_error)
        {
            if(!findByKey(option->getKey()) && !findByLongKey(option->getLongKey().c_str()))
            {
                options_.push_back(option);
                dirty_ = true;
            }
            else
            {
                throw std::runtime_error("key is already registered in option list.");
            }

            return *this;
        }

        Parser &add(detail::ArgumentBase *arg) throw(std::runtime_error)
        {
            if(varg_) { throw std::runtime_error("argument added after variable arguments."); }
            if(!findByName(arg->getName().c_str()))
            {
                arguments_.push_back(arg);
                if(arg->isMultiArg()) { varg_ = true; }
                dirty_ = true;
            }
            else
            {
                throw std::runtime_error("name is already registered in argument list.");
            }

            return *this;
        }
    public:
        Parser()
        : dirty_(true), showErrors_(false), varg_(false)
        { init(); }

        Parser(const char *desc)
        : description_(desc), dirty_(true), showErrors_(true), varg_(false)
        { init(); }

        Parser(const char *desc, const bool showErrors)
        : description_(desc), dirty_(true), showErrors_(showErrors), varg_(false)
        { init(); }

        ~Parser()
        {
            clear(options_);
            clear(arguments_);
        }

        ParseResult parse(const int argc, const char *argv[])
        {
            ArgumentList::iterator it = arguments_.begin();
            setAppName(argv[0]);

            for(int i = 1; i < argc; ++i)
            {
                const char *arg = argv[i];
                switch(checkType(arg))
                {
                    case ArgType_Value:
                        {
                            if(arguments_.empty()) { break; }
                            if(it == arguments_.end())
                            {
                                detail::ArgumentBase *a = *(it - 1);
                                if(a->isMultiArg()) { a->setValue(arg); }
                            }
                            else
                            {
                                (*it++)->setValue(arg);
                            }
                        }
                        break;
                    case ArgType_Key:
                        {
                            const char *keys = arg + 1;
                            if(strlen(keys) == 1)
                            {
                                // argument mode
                                ParseResult r;
                                char kn[2] = { *keys, 0 };
                                if(parseArguments(r, i, argc, argv, findByKey(*keys), kn))
                                {
                                    return r;
                                }
                            }
                            else
                            {
                                // switch mode
                                while(*keys)
                                {
                                    detail::OptionBase *option = findByKey(*keys);
                                    if(!option)
                                    {
                                        errorMsg_ << "invalid argument name specified: -" << *keys << "\n";
                                        return end();
                                    }

                                    if(option->is<bool>())
                                    {
                                        if(!option->setValue(nullptr))
                                        {
                                            return invalidTypeError(option);
                                        }
                                    }
                                    else
                                    {
                                        return invalidTypeError(option);
                                    }

                                    ++keys;
                                }
                            }

                        }
                        break;
                    case ArgType_LongKey:
                        {
                            ParseResult r;
                            if(parseArguments(r, i, argc, argv, findByLongKey(arg + 2), arg))
                            {
                                return r;
                            }
                        }
                        break;
                }
            }

            for(OptionList::const_iterator it = options_.begin(); it != options_.end(); ++it)
            {
                const detail::OptionBase *option = *it;
                if(!option->isSet() && !option->isOptional())
                {
                    errorMsg_ << "-" << option->getKey() << "(" << option->getLongKey() << ") should be specified.\n";
                    return end();
                }
            }

            for(ArgumentList::const_iterator it = arguments_.begin(); it != arguments_.end(); ++it)
            {
                const detail::ArgumentBase *arg = *it;
                if(!arg->isSet() && !arg->isOptional())
                {
                    errorMsg_ << arg->getName() << " should be specified.\n";
                    return end();
                }
            }

            return end();
        }

        std::string getUsage()
        {
            if(dirty_)
            {
                std::ostringstream oss;
                oss << "Usage:\n    " << appName_ << " ";

                // sort keys
                OptionList sorted(options_);
                std::sort(sorted.begin(), sorted.end(), detail::KeySort());
                std::size_t olen = 0;
                for(OptionList::const_iterator it = sorted.begin(); it != sorted.end(); ++it)
                {
                    const detail::OptionBase *option = *it;
                    option->buildArguments(oss);

                    olen = std::max(olen, option->getLongKey().size());
                }

                std::size_t alen = 0;
                for(ArgumentList::const_iterator it = arguments_.begin(); it != arguments_.end(); ++it)
                {
                    const detail::ArgumentBase *arg = *it;
                    arg->buildArguments(oss);

                    alen = std::max(alen, arg->getName().size());
                }

                // build arguments
                {
                    alen += 2;
                    oss << "\n\nArguments:\n";

                    for(ArgumentList::const_iterator it = arguments_.begin(); it != arguments_.end(); ++it)
                    {
                        const detail::ArgumentBase *arg = *it;
                        oss << "    " << std::setw(static_cast<int>(alen)) << std::left << arg->getName() << "  ";
                        if(arg->isOptional()) { oss << "(optional) "; }
                        oss << arg->getDesc() << "\n";
                    }
                }

                // build options
                {
                    olen += 2;
                    oss << "\nOptions:\n";

                    for(OptionList::const_iterator it = sorted.begin(); it != sorted.end(); ++it)
                    {
                        const detail::OptionBase *option = *it;
                        oss <<
                            "    -" << option->getKey() << "  " <<
                            std::setw(static_cast<int>(olen)) << std::left <<
                            ("--" + option->getLongKey()) << "  ";
                        if(option->isOptional() && option->getKey() != 'h')
                        {
                            oss << "(optional) ";
                        }

                        oss << option->getDesc() << "\n";
                    }
                }

                oss << "\n" << description_ << std::endl;
                usage_ = oss.str();
                dirty_ = false;
            }

            return usage_;
        }

        void showUsage() { std::cout << getUsage() << std::endl; }
        std::string getErrorMessage() const { return errorMsg_.str(); }
        std::string getAppName() const { return appName_; }

        template<typename T>
        T getOption(const int index) const { return getValue<T, Option<T> >(options_.at(index + 1)); }

        template<typename T>
        T getOption(const char key) const { return getValue<T, Option<T> >(findByKey(key)); }

        template<typename T>
        T getOption(const char *longkey) const { return getValue<T, Option<T> >(findByLongKey(longkey)); }

        template<typename T>
        T getArgument(const int index) const { return getValue<T, Argument<T> >(arguments_.at(index)); }

        template<typename T>
        T getArgument(const char *name) const { return getValue<T, Argument<T> >(findByName(name)); }

#ifdef MW_CPP11
        template<
            typename T,
            typename... Args,
            typename = typename std::enable_if<(sizeof...(Args) > 0)>::type
        >
        Parser &add(T &&t, Args &&...args) throw(std::runtime_error)
        {
            add(std::forward<T>(t));
            return add(std::forward<Args>(args)...);
        }
#endif

        /* options */
        template<typename T>
        Parser &add(const Option<T> &option) { return add(new Option<T>(option)); }

        template<typename T>
        Parser &add(Option<T> &option)
        {
            option.ref_ = true;
            return add(&option);
        }

        /* arguments */
        template<typename T>
        Parser &add(const Argument<T> &arg) { return add(new Argument<T>(arg)); }

        template<typename T>
        Parser &add(Argument<T> &arg)
        {
            arg.ref_ = true;
            return add(&arg);
        }
    };
}

#ifndef MW_CPP11
#   undef final
#   undef override
#   undef nullptr
#endif
#undef ENUM_ELEM
#undef MW_CPP11

#endif

