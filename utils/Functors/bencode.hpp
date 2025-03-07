#pragma once

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <stack>
#include <cstdint>
#include <fstream>
#include <sstream>

namespace bencode
{
    class bencode_object
    {
    public:
        bencode_object() = default;

        int64_t GetInteger() const { return m_integer; }
        std::string GetString() const { return m_string; }
        std::vector<bencode_object> GetList() const { return m_list; }
        std::map<std::string, bencode_object> GetDict() const { return m_dict; }

        bool IsInteger() const { return m_type == Type::integer; }
        bool IsString() const { return m_type == Type::string; }
        bool IsList() const { return m_type == Type::list; }
        bool IsDict() const { return m_type == Type::dict; }

    private:
        friend class FunctorEncode;
        friend class FunctorDecode;
        enum class Type
        {
            none,
            integer,
            string,
            list,
            dict
        };
        Type m_type;
        int64_t m_integer;
        std::string m_string;
        std::map<std::string, bencode_object> m_dict;
        std::vector<bencode_object> m_list;
    };
    class FunctorEncode
    {
    public:
        void operator()(bencode_object &object, std::ostream &fout)
        {
             
            switch (object.m_type)
            {
            case bencode_object::Type::none:
            {
                break;
            }
            case bencode_object::Type::integer:
            {
                fout << "i" << object.GetInteger() << "e";
                break;
            }
            case bencode_object::Type::string:
            {
                fout << object.GetString().length() << ":";
                fout << object.GetString();
                break;
            }
            case bencode_object::Type::list:
            {
                fout << 'l';
                for(auto& el : object.GetList()) {
                    (*this)(el, fout);
                }
                fout << 'e';
                break;
            }
            case bencode_object::Type::dict:
            {
                fout << 'd';
                for(auto& pair : object.GetDict()) {
                    bencode::FunctorEncode encode;
                    fout << pair.first.length() << ":" << pair.first;
                    (*this)(pair.second, fout);
                }
                fout << 'e';
                break;
            }
            default:
            {
                throw std::runtime_error(DEFAULT_ERROR_TEXT);
            }
            }
        }

    private:
        const std::string DEFAULT_ERROR_TEXT = "default error text";
    };
    class FunctorDecode
    {
    public:
        bencode_object operator()(const std::string &str)
        {
            m_object.m_type = bencode_object::Type::none;
            size_t index = 0;
            while (index < str.length())
            {
                if (m_state == State::FINAL)
                    throw std::runtime_error("Something went wrong...");
                std::cout << str[index] << " " << static_cast<std::underlying_type<State>::type>(m_state) << std::endl;
                index += action(str[index]);
            }
            if (m_state != State::FINAL)
                throw std::runtime_error("Something went wrong...");
            return m_object;
        }
        bencode_object operator()(std::istream &stream)
        {
            m_object.m_type = bencode_object::Type::none;

            char ch = stream.get();
            while (stream)
            {
                if (m_state == State::FINAL)
                    throw std::runtime_error("Symbols after Final state");
              //  std::cout << ch << " " << static_cast<std::underlying_type<State>::type>(m_state) << std::endl;
                if (action(ch))
                {
                    ch = stream.get();
                }
            }
            if (m_state != State::FINAL) {
                throw std::runtime_error("File was ended but state is not final");
            }
                

            return m_object;
        }

    private:
        bool action(unsigned char c)
        {
            switch (m_state)
            {
            case State::INITIAL:
            {
                if ('0' <= c && c <= '9')
                {
                    m_state = State::READ_STR_LEN;
                }
                else if (c == 'i')
                {
                    m_state = State::READ_INT;
                    return true;
                }
                else if (c == 'l')
                {
                    m_state = State::READ_LIST;
                    return true;
                }
                else if (c == 'd')
                {
                    m_state = State::READ_DICT;
                    return true;
                }
                else
                    throw std::runtime_error(DEFAULT_ERROR_TEXT);
                break;
            }
            case State::READ_INT:
            {
                if ('0' <= c && c <= '9' || c == '-')
                {
                    m_buffer.push_back(c);
                }
                // Маркер окончания чтения символов целого числа
                else if (c == 'e')
                {
                    m_object.m_integer = std::stoll(m_buffer);
                    m_object.m_type = bencode_object::Type::integer;
                    m_buffer = "";

                    // Если стэк состояний пуст, значит, работа pd-автомата должна быть закончена
                    if (m_stateMagazine.empty())
                    {
                        m_state = State::FINAL;
                    }
                    // В противном случае объект необходимо поместить в стэк bencode-объектов,
                    // Также надо вернуться в предыдущее состояние для записи этого объекта в иерархически вышестоящий объект
                    else
                    {
                        m_state = m_stateMagazine.top();
                        m_objectMagazine.push(std::move(m_object));
                    }
                }
                else
                    throw std::runtime_error("Incorrect Bencode integer format");
                return true;
            }
            case State::READ_STR_LEN:
            {
                if ('0' <= c && c <= '9')
                {
                    m_buffer.push_back(c);
                }
                // Маркер окончания чтения длины строки
                else if (c == ':')
                {
                    m_strLen = std::stoll(m_buffer);
                    std::cout << m_buffer << " " << m_strLen<< std::endl;
                    m_buffer = "";
                    

                    m_state = State::READ_STR;
                }
                else
                    throw std::runtime_error("Incorect string length format");
                return true;
            }
            case State::READ_STR:
            {
                if (!m_strLen)
                {
                    m_object.m_string = std::move(m_buffer);
                    m_object.m_type = bencode_object::Type::string;
                    m_buffer = "";

                    // Если стэк состояний пуст, значит, работа pd-автомата должна быть закончена
                    if (m_stateMagazine.empty())
                    {
                        m_state = State::FINAL;
                    }
                    // В противном случае объект необходимо поместить в стэк bencode-объектов,
                    // Также надо вернуться в предыдущее состояние для записи этого объекта в иерархически вышестоящий объект
                    else
                    {
                        m_state = m_stateMagazine.top();
                        m_objectMagazine.push(std::move(m_object));
                    }
                    return false;
                }
                m_buffer.push_back(c);
                m_strLen--;
                return true;
            }
            case State::READ_LIST:
            {
                // Маркер окончания чтения списка
                if (c == 'e')
                {
                    m_object.m_type = bencode_object::Type::list;

                    // Если стэк состояний пуст, значит, работа pd-автомата должна быть закончена
                    if (m_stateMagazine.empty())
                    {
                        m_state = State::FINAL;
                    }
                    // В противном случае объект необходимо поместить в стэк bencode-объектов,
                    // Также надо вернуться в предыдущее состояние для записи этого объекта в иерархически вышестоящий объект
                    else
                    {
                        m_state = m_stateMagazine.top();
                        m_objectMagazine.push(std::move(m_object));
                    }

                    return true;
                }
                m_objectMagazine.push(std::move(m_object));

                m_stateMagazine.push(State::READ_LIST);
                m_state = State::READ_ITEM;
                break;
            }
            case State::READ_ITEM:
            {
                // Если наверху стэка состояний чтение списка, значит, нужно начать чтение элемента списка
                if (m_stateMagazine.top() == State::READ_LIST)
                {
                    m_stateMagazine.push(State::READ_ITEM);
                    m_state = State::INITIAL;
                }
                // Если наверху стэка состояний чтение элемента списка, то нужно добавить элемент в исходный список
                else if (m_stateMagazine.top() == State::READ_ITEM)
                {
                    // Удаление элемента списка из стэка bencode-объектов с последующим сохранением в буферную переменную
                    bencode_object item = std::move(m_objectMagazine.top());
                    m_objectMagazine.pop();

                    // Удаление списка из стэка bencode-объектов с последующим сохранением в поле
                    m_object = std::move(m_objectMagazine.top());
                    m_objectMagazine.pop();

                    // Непосредственно добавление элемента в список
                    m_object.m_list.push_back(item);

                    // Удаление всех промежуточных состояний(READ_ITEM, READ_LIST)
                    // и возврат к состоянию чтения списка
                    m_stateMagazine.pop();
                    m_stateMagazine.pop();
                    m_state = State::READ_LIST;
                }
                break;
            }
            case State::READ_DICT:
            {
                // Маркер окончания чтения словаря
                if (c == 'e')
                {
                    m_object.m_type = bencode_object::Type::dict;

                    // Если стэк состояний пуст, значит, работа pd-автомата должна быть закончена
                    if (m_stateMagazine.empty())
                    {
                        m_state = State::FINAL;
                    }
                    // В противном случае объект необходимо поместить в стэк bencode-объектов,
                    // Также надо вернуться в предыдущее состояние для записи этого объекта в иерархически вышестоящий объект
                    else
                    {
                        m_state = m_stateMagazine.top();
                        m_objectMagazine.push(std::move(m_object));
                    }
                    return true;
                }
                // Объект добавлен в стек bencode-объектов
                m_objectMagazine.push(std::move(m_object));
                // Состояние добавлено в стек состояний
                m_stateMagazine.push(State::READ_DICT);
                // Осуществляется переход в состояние чтения пары
                m_state = State::READ_PAIR;
                break;
            }
            case State::READ_PAIR:
            {
                // Если наверху стэка состояний чтение словаря, то нужно начать чтение ключа пары
                if (m_stateMagazine.top() == State::READ_DICT)
                {
                    m_stateMagazine.push(State::READ_PAIR);
                    m_state = State::READ_KEY;
                }
                // Если наверху стэка состояний чтение ключа пары, то нужно начать чтение значения пары
                else if (m_stateMagazine.top() == State::READ_KEY)
                {
                    m_state = State::READ_VALUE;
                }
                // Если наверху стэка состояний чтение значения пары, то нужно добавить пару в исходный словарь
                else if (m_stateMagazine.top() == State::READ_VALUE)
                {
                    // Удаление значения пары из стэка bencode-объектов с последующим сохранением в буферную переменную
                    bencode_object value = std::move(m_objectMagazine.top());
                    m_objectMagazine.pop();

                    // Удаление ключа пары из стэка bencode-объектов с последующим сохранением в буферную переменную
                    bencode_object key = std::move(m_objectMagazine.top());
                    m_objectMagazine.pop();

                    // Удаление словаря из стэка bencode-объектов с последующим сохранением в поле
                    m_object = std::move(m_objectMagazine.top());
                    m_objectMagazine.pop();

                    m_object.m_dict[key.GetString()] = value;
                    // Удаление всех промежуточных состояний(READ_VALUE, READ_KEY, READ_PAIR, READ_DICT)
                    // и возврат к состоянию чтения словаря
                    m_stateMagazine.pop();
                    m_stateMagazine.pop();
                    m_stateMagazine.pop();
                    m_stateMagazine.pop();
                    m_state = State::READ_DICT;
                }
                else
                    throw std::runtime_error(DEFAULT_ERROR_TEXT);
                break;
            }
            case State::READ_KEY:
            {
                // Если наверху стэка состояний чтение пары, то нужно начать чтение длины строки, являющейся ключом пары
                if (m_stateMagazine.top() == State::READ_PAIR)
                {
                    m_stateMagazine.push(State::READ_KEY);
                    m_state = State::READ_STR_LEN;
                }
                // Если наверху стэка состояний чтение ключа пары,
                // то нужно вернуться в состояние чтения пары для последующего чтения значения пары
                else if (m_stateMagazine.top() == State::READ_KEY)
                {
                    m_state = State::READ_PAIR;
                }
                else
                    throw std::runtime_error(DEFAULT_ERROR_TEXT);
                break;
            }
            case State::READ_VALUE:
            {
                // Если ключ пары был считан, а значение пары - нет,
                // осуществляется переход в состояние определения типа объекта значения пары
                if (m_stateMagazine.top() == State::READ_KEY)
                {
                    m_stateMagazine.push(State::READ_VALUE);
                    m_state = State::INITIAL;
                }
                // Если значение пары считано, вернуться в состояние чтения пары
                else if (m_stateMagazine.top() == State::READ_VALUE)
                {
                    m_state = State::READ_PAIR;
                }
                else
                    throw std::runtime_error(DEFAULT_ERROR_TEXT);
                break;
            }
            default:
            {
                throw std::runtime_error(DEFAULT_ERROR_TEXT);
                break;
            }
            }
            return false;
        }
        std::string m_buffer;
        int64_t m_strLen = 0;
        bencode_object m_object;
        enum class State
        {
            INITIAL,
            READ_INT,
            READ_STR_LEN,
            READ_STR,
            READ_LIST,
            READ_ITEM,
            READ_DICT,
            READ_PAIR,
            READ_KEY,
            READ_VALUE,
            FINAL
        } m_state = State::INITIAL;
        std::stack<State> m_stateMagazine;
        std::stack<bencode_object> m_objectMagazine;

        const std::string DEFAULT_ERROR_TEXT = "default error text";
    };
}
