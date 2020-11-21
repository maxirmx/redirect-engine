#pragma once

#include <string>
#include <zlib.h>
#include <functional>
#include <mutex>
#include <optional>
#include "RWLock.h"

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

    bool Remove(const TKey &key)
    {
        auto r = infos.find(key);
        if(r == infos.end())
            return false;

        infos.erase(r);
        return true;
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

    bool Remove(const TKey &key, unsigned short c16)
    {
        auto v = bags.find(c16);
        if( v == bags.end() )
            return false;
        return v->second->Remove(key);
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
        RWWriteGuard _l(&this->rw_lock);
        Bag8<TKey, T>* bag;
        auto v = bags.find(sec);
        if( v == bags.end() )
            bags[sec] = bag = new Bag8<TKey, T>();
        else
            bag = v->second;
        bag->Store(key, info, c16);
    }

    bool Remove(const TKey &key, unsigned char sec, unsigned short c16)
    {
        RWWriteGuard _l(&rw_lock);
        
        auto v = bags.find(sec);
        if( v == bags.end() )
            return false;
        
        return v->second->Remove(key, c16);
    }

    std::optional<T> Load(const TKey &key, unsigned char sec, unsigned short c16)
    {
        RWReadGuard _l(&rw_lock);
        
        auto v = bags.find(sec);
        if( v == bags.end() )
            return std::nullopt;
        
        return v->second->Load(key, c16);
    }

    ~BagMutexed()
    {
        RWWriteGuard _l(&this->rw_lock);
        for(auto v : bags)
            delete v.second;
        bags.clear();
    }
private:
    RWLock rw_lock;
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
        RWWriteGuard _g(&this->rwLock);

        auto crc = keyBuilder(key);
        auto first = (unsigned char)(crc & 0xff);
        auto second = (unsigned char)((crc & 0xff00) >> 8);
        auto c16 = (unsigned short)(crc >> 16);
        BagMutexed<TKey, T>* bag;
        auto v = bags.find(first);
        if( v == bags.end() )
            bags[first] = bag = new BagMutexed<TKey, T>();
        else
            bag = v->second;        
        
        bag->Store(key, info, second, c16);
    }

    std::optional<T> Load(std::string key)
    {
        RWReadGuard _l(&this->rwLock);

        auto crc = keyBuilder(key);
        auto first = (unsigned char)(crc & 0xff);
        auto second = (unsigned char)((crc & 0xff00) >> 8);
        auto c16 = (unsigned short)(crc >> 16);

        auto v = bags.find(first);
        if(v == bags.end() )
            return std::nullopt;

        return v->second->Load(key, second, c16);
    }

    bool Remove(const TKey key)
    {
        RWWriteGuard _g(&this->rwLock);

        auto crc = keyBuilder(key);
        auto first = (unsigned char)(crc & 0xff);
        auto second = (unsigned char)((crc & 0xff00) >> 8);
        auto c16 = (unsigned short)(crc >> 16);

        auto v = bags.find(first);
        if(v == bags.end() )
            return false;

        return v->second->Remove(key, second, c16);
    }

    void Clear()
    {
        RWWriteGuard _g(&this->rwLock);

        for(auto v : bags)
            delete v.second;
        bags.clear();
    }

    ~Storage()
    {
        Clear();
    }

private:
    std::function<unsigned long(const TKey)> keyBuilder;
    RWLock rwLock;
    std::map<unsigned char, BagMutexed<TKey, T>*> bags;
};