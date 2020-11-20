#pragma once

#include <string>
#include <zlib.h>
#include <functional>
#include <mutex>
#include <optional>

static unsigned long StringKeyCrc32(std::string key)
{
    return crc32(0x8000000, (Bytef *)key.c_str(), (int)key.size());
}

template<class TKey, class T>
class Bag16
{
public:
    void Store(TKey key, T info)
    {
        infos[key] = info;
    }

    std::optional<T> Load(const TKey &key)
    {
        auto r = infos.find(key);
        if(r == infos.end())
            return std::nullopt;

        return r->second;
    }
private:
    std::map<TKey, T> infos;
};

template<class TKey, class T>
class Bag8
{
public:
    void Store(TKey key, T info, unsigned short c16)
    {
        Bag16<TKey, T>* bag;
        auto v = bags.find(c16);
        if( v == bags.end() )
            bags[c16] = bag = new Bag16<TKey, T>();
        else
            bag = v->second;
            
        bag->Store(key, info);
    }

    std::optional<T> Load(const TKey &key, unsigned short c16)
    {
        auto v = bags.find(c16);
        if( v == bags.end() )
            return std::nullopt;
        return v->second->Load(key);
    }

    ~Bag8()
    {
        for(auto v : bags)
            delete v.second;
        bags.clear();
    }
private:
    std::map<unsigned short, Bag16<TKey, T>*> bags;
};

template<class TKey, class T>
class BagMutexed
{
public:
    void Store(TKey key, T info, unsigned char sec, unsigned short c16)
    {
        std::lock_guard<std::mutex> guard(rw_mutex);
        Bag8<TKey, T>* bag;
        auto v = bags.find(sec);
        if( v == bags.end() )
            bags[sec] = bag = new Bag8<TKey, T>();
        else
            bag = v->second;
        bag->Store(key, info, c16);
    }

    std::optional<T> Load(const TKey &key, unsigned char sec, unsigned short c16)
    {
        Bag8<TKey, T>* bag;
        {
            std::lock_guard<std::mutex> guard(rw_mutex);
            auto v = bags.find(sec);
            if( v == bags.end() )
                return std::nullopt;
            else
                bag = v->second;
        }

        return bag->Load(key, c16);
    }

    ~BagMutexed()
    {
        std::lock_guard<std::mutex> guard(rw_mutex);
        for(auto v : bags)
            delete v.second;
        bags.clear();
    }
private:
    std::mutex rw_mutex;
    std::map<unsigned char, Bag8<TKey, T>*> bags;
};

template<class TKey, class T>
class Storage
{
public:
    Storage(std::function<unsigned long(const TKey)> keyBuilder)
    {
        this->keyBuilder = keyBuilder;
    }

    void StoreInfo(TKey key, T info)
    {
        auto crc = keyBuilder(key);
        auto first = (unsigned char)(crc & 0xff);
        auto second = (unsigned char)((crc & 0xff00) >> 8);
        auto c16 = (unsigned short)(crc >> 16);
        BagMutexed<TKey, T>* bag;
        auto v = bags.find(first);
        if( v == bags.end() )
        {
            std::lock_guard<std::mutex> guard(rw_mutex);
            bags[first] = bag = new BagMutexed<TKey, T>();
        }
        else
            bag = v->second;        
        
        bag->Store(key, info, second, c16);
    }

    std::optional<T> Load(std::string key)
    {
        auto crc = keyBuilder(key);
        auto first = (unsigned char)(crc & 0xff);
        auto second = (unsigned char)((crc & 0xff00) >> 8);
        auto c16 = (unsigned short)(crc >> 16);

        
        typename std::map<unsigned char, BagMutexed<TKey, T>*>::iterator v;
        {
            std::lock_guard<std::mutex> guard(rw_mutex);
            v = bags.find(first);
        }
        
        if(v == bags.end() )
            return std::nullopt;

        return v->second->Load(key, second, c16);
    }

    ~Storage()
    {
        for(auto v : bags)
            delete v.second;
        bags.clear();
    }

private:
    std::function<unsigned long(const TKey)> keyBuilder;
    std::mutex rw_mutex;
    std::map<unsigned char, BagMutexed<TKey, T>*> bags;
};