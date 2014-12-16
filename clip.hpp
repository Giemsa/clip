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
#else
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

    namespace detail
    {
        template<typename T>
        struct TypeID final
        {
            static const std::size_t getUID()
            {
                return reinterpret_cast<std::size_t>(&getUID);
            }
        };

        class OptionBase
        {
            friend class KeyComparator;
            friend class LongKeyComparator;
            friend class clip::Parser;
            friend class KeySort;
        private:
            std::size_t uid_;
            bool optional_, set_, ref_;
            char key_;
            std::string longkey_, desc_, name_;

            template<typename T>
            bool is() const { return uid_ == TypeID<T>::getUID(); }

        protected:
            bool used(const bool f) { return set_ = !f; }
            bool isSet() const { return set_; }
            std::size_t getUID() const { return uid_; }
            virtual void buildArguments(std::ostringstream &oss) const = 0;
            virtual bool setValue(const char *value) = 0;
            virtual bool isMultiArg() const { return false; }

        public:
            OptionBase(
                const std::size_t uid, const char key, const char *longkey, const char *name,
                const char *desc, const bool optional
            )
            : uid_(uid), optional_(optional), set_(false), ref_(false)
            , key_(key), longkey_(longkey), desc_(desc), name_(name)
            { }

            virtual ~OptionBase() { }

            char getKey() const { return key_; }
            const std::string &getLongKey() const { return longkey_; }
            const std::string &getName() const { return name_; }
            const std::string &getDesc() const { return desc_; }
            bool isOptional() const { return optional_; }
        };

        /* key comparator */
        class KeyComparator
        {
        private:
            char key_;
        public:
            KeyComparator(const char key) : key_(key) { }

            bool operator()(const OptionBase *option) const
            {
                return key_ == option->key_;
            }
        };

        class LongKeyComparator
        {
        private:
            const char *key_;
        public:
            LongKeyComparator(const char *key) : key_(key) { }

            bool operator()(const OptionBase *option) const
            {
                return key_ == option->longkey_;
            }
        };

        struct KeySort
        {
            bool operator()(const OptionBase *lhs, const OptionBase *rhs) const
            {
                return lhs->key_ < rhs->key_;
            }
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
            if(isOptional())
            {
                oss << "[-" << getKey() << " <" << getName() << ">] ";
            }
            else
            {
                oss << "-" << getKey() << " <" << getName() << "> ";
            }
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
            if(isOptional())
            {
                oss << "[-" << getKey() << "] ";
            }
            else
            {
                oss << "-" << getKey() << " ";
            }
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
            if(isOptional())
            {
                oss << "[-" << getKey() << " <" << getName() << "...>] ";
            }
            else
            {
                oss << "-" << getKey() << " <" << getName() << "...> ";
            }
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

    class Parser final
    {
        typedef std::vector<detail::OptionBase *> OptionList;
        typedef std::vector<const char *> UnlabeledList;
        enum ArgType
        {
            ArgType_Value,
            ArgType_Key,
            ArgType_LongKey
        };

    private:
        std::string description_;
        std::string appName_;
        std::ostringstream errorMsg_;
        OptionList options_;
        UnlabeledList unlabeldArgs_;
        std::string usage_;
        bool dirty_;
        bool showErrors_;

        void clear()
        {
            for(
                OptionList::const_iterator it = options_.begin();
                it != options_.end();
                ++it
            )
            {
                if(!(*it)->ref_)
                {
                    delete *it;
                }
            }

            options_.clear();
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

        detail::OptionBase *findByKey(const char key) const
        {
            OptionList::const_iterator it = std::find_if(options_.begin(), options_.end(), detail::KeyComparator(key));
            if(it == options_.end())
            {
                return nullptr;
            }

            return *it;
        }

        detail::OptionBase *findByLongKey(const char *key) const
        {
            OptionList::const_iterator it = std::find_if(options_.begin(), options_.end(), detail::LongKeyComparator(key));
            if(it == options_.end())
            {
                return nullptr;
            }

            return *it;
        }

        ArgType checkType(const char *key) const
        {
            if(*key == '-')
            {
                if(*++key == '-')
                {
                    return ArgType_LongKey;
                }

                return ArgType_Key;
            }

            return ArgType_Value;
        }

        bool parseArguments(bool  &r, int &i, const int argc, const char *argv[], detail::OptionBase *option, const char *key)
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
                        if(!option->setValue(value) || option->isSet())
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

        bool fewArgError(const detail::OptionBase *option)
        {
            errorMsg_ << "argument should be specified for -" << option->getKey() << "(" << option->getLongKey() << ")"<< "\n";
            return end();
        }

        bool invalidTypeError(const detail::OptionBase *option)
        {
            errorMsg_ << "invalid type was specified for -" << option->getKey() << "(" << option->getLongKey() << ")\n";
            return end();
        }

        bool end()
        {
            if(showHelp())
            {
                return true;
            }

            const bool r = getErrorMessage().empty();
            if(showErrors_ && !r)
            {
                std::cerr << getErrorMessage() << std::endl;
            }

            return r;
        }

        template<typename T>
        T getValue(const detail::OptionBase *option) const throw(std::bad_cast)
        {
            if(!option->is<T>())
            {
                throw std::bad_cast();
            }

            return std::move(static_cast<const Option<T> *>(option)->getValue());
        }
    public:
        Parser()
        : dirty_(true), showErrors_(false)
        { init(); }

        Parser(const char *desc)
        : description_(desc), dirty_(true), showErrors_(true)
        { init(); }

        Parser(const char *desc, const bool showErrors)
        : description_(desc), dirty_(true), showErrors_(showErrors)
        { init(); }

        ~Parser() { clear(); }

        bool parse(const int argc, const char *argv[])
        {
            setAppName(argv[0]);

            for(int i = 1; i < argc; ++i)
            {
                const char *arg = argv[i];
                switch(checkType(arg))
                {
                    case ArgType_Value:
                        // set normal args
                        unlabeldArgs_.push_back(arg);
                        break;

                    case ArgType_Key:
                        {
                            const char *keys = arg + 1;
                            if(strlen(keys) == 1)
                            {
                                // argument mode
                                bool r;
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
                            bool r;
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
                    errorMsg_ << "-" << option->getKey() << "(" << option->getLongKey() << ") should be specified." << "\n";
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

                std::size_t len = 0;
                for(OptionList::const_iterator it = sorted.begin(); it != sorted.end(); ++it)
                {
                    const detail::OptionBase *option = *it;
                    option->buildArguments(oss);


                    len = std::max(len, option->getLongKey().size());
                }

                len += 2;
                oss << "\n\nOptions:\n";

                for(OptionList::const_iterator it = sorted.begin(); it != sorted.end(); ++it)
                {
                    const detail::OptionBase *option = *it;
                    oss <<
                        "    -" << option->getKey() << "  " <<
                        std::setw(static_cast<int>(len)) << std::left <<
                        ("--" + option->getLongKey()) << "  ";
                    if(option->isOptional() && option->getKey() != 'h')
                    {
                        oss << "(optional) ";
                    }

                    oss << option->getDesc() << "\n";
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
        std::vector<const char *> getUnlabeledArgs() const { return unlabeldArgs_; }

        template<typename T>
        T getValue(const int index) const { return getValue<T>(options_.at(index + 1)); }

        template<typename T>
        T getValue(const char key) const { return getValue<T>(findByKey(key)); }

        template<typename T>
        T getValue(const char *longkey) const { return getValue<T>(findByLongKey(longkey)); }

#ifdef MW_CPP11
        template<
            typename T,
            typename... Args,
            typename = typename std::enable_if<(sizeof...(Args) > 0)>::type
        >
        Parser &add(T &&t, Args &&...args) throw(std::runtime_error)
        {
            add(static_cast<T &&>(t));
            return add(static_cast<Args &&>(args)...);
        }
#endif

        /* const */
        template<typename T>
        Parser &add(const Option<T> &option) throw(std::runtime_error)
        {
            if(!findByKey(option.getKey()) && !findByLongKey(option.getLongKey().c_str()))
            {
                options_.push_back(new Option<T>(option));
                dirty_ = true;
            }
            else
            {
                throw std::runtime_error("key already registered.");
            }

            return *this;
        }

        /* non const */
        template<typename T>
        Parser &add(Option<T> &option) throw(std::runtime_error)
        {
            if(!findByKey(option.getKey()) && !findByLongKey(option.getLongKey().c_str()))
            {
                option.ref_ = true;
                options_.push_back(&option);
                dirty_ = true;
            }
            else
            {
                throw std::runtime_error("key already registered.");
            }

            return *this;
        }
    };
}

#ifndef MW_CPP11
#   undef final
#   undef override
#   undef nullptr
#endif
#undef MW_CPP11

#endif

