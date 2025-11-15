#pragma once

#include <unordered_map>
#include <span>
#include <optional>
#include <stdexcept>
#include <vector>
#include <algorithm>

#include "structuretype.h"

namespace vknc {

struct BasicNextChainHeader
{
    VkStructureType    sType;
    void*              pNext;
};

template <typename T>
class NextChain
{
public:
    NextChain(T _main) : main { _main }
    {
        main.sType = StructureType<T>::value;
        main.pNext = nullptr;
    }

    NextChain() : main { .sType = StructureType<T>::value }
    {}

    NextChain(const NextChain &other) :
        main { other.main },
        chain { other.chain }
    {
        main.pNext = nullptr;
        for (auto &entry : chain)
        {
            reinterpret_cast<BasicNextChainHeader*>(entry.second.data())->pNext = main.pNext;
            main.pNext = entry.second.data();
        }
    }

    NextChain & operator = (const NextChain &other)
    {
        main = other.main;
        chain = other.chain;
        main.pNext = nullptr;
        for (auto &entry : chain)
        {
            reinterpret_cast<BasicNextChainHeader*>(entry.second.data())->pNext = main.pNext;
            main.pNext = entry.second.data();
        }
        return *this;
    }

    NextChain(NextChain &&other) : 
        main { other.main },
        chain { std::move(other.chain) }
    {
        /*
        // This should not be required here
        main.pNext = nullptr;
        for (auto &entry : chain)
        {
            reinterpret_cast<BasicNextChainHeader*>(entry.second.data())->pNext = main.pNext;
            main.pNext = entry.second.data();
        }
        */
    }

    NextChain &operator = (NextChain &&other)
    {
        main = other.main;
        chain = std::move(other.chain);
        /*
        // This should not be required here
        main.pNext = nullptr;
        for (auto &entry : chain)
        {
            reinterpret_cast<BasicNextChainHeader*>(entry.second.data())->pNext = main.pNext;
            main.pNext = entry.second.data();
        }
        */
        return *this;
    }

    T *operator -> ()
    {
        return &main;
    }

    const T *operator -> () const
    {
        return &main;
    }

    operator T*()
    {
        return &main;
    }

    operator const T*() const
    {
        return &main;
    }

    template <typename U>
    U *getStruct()
    {
        if constexpr (StructureType<U>::value == StructureType<T>::value)
            return &main;

        std::vector<char> &data = chain[StructureType<U>::value];
        if (data.empty())
        {
            U entry { .sType = StructureType<U>::value };
            data.resize(sizeof(U));

            entry.pNext = main.pNext;
            main.pNext = data.data();

            *reinterpret_cast<U*>(data.data()) = entry;
        }

        return reinterpret_cast<U*>(data.data());
    }

    std::span<char> getStruct(VkStructureType sType, size_t size = 0)
    {
        if (sType == main.sType)
        {
            if (size != 0 && sizeof(T) != size)
                throw std::runtime_error("Wrong size for struct supplied!");
            return std::span<char>(reinterpret_cast<char*>(&main), sizeof(T));
        }
        auto it = chain.find(sType);
        if (it == chain.end())
        {
            if (size == 0)
                return std::span<char>();
            
            std::vector<char> &data = chain[sType];
            data.resize(size);
            reinterpret_cast<BasicNextChainHeader*>(data.data())->sType = sType;
            reinterpret_cast<BasicNextChainHeader*>(data.data())->pNext = main.pNext;
            main.pNext = data.data();
            return data;
        }

        if (size != 0 && it->second.size() != size)
            throw std::runtime_error("Wrong size for struct supplied!");
        return it->second;
    }

    template <typename U>
    std::optional<U*> getStructOptional()
    {
        if constexpr (StructureType<U>::value == StructureType<T>::value)
            return &main;

        auto it = chain.find(StructureType<U>::value);
        if (it == chain.end())
            return {};

        return reinterpret_cast<U*>(it->second.data());
    }

    template <typename U>
    std::optional<const U*> getStructOptional() const
    {
        if constexpr (StructureType<U>::value == StructureType<T>::value)
            return &main;

        auto it = chain.find(StructureType<U>::value);
        if (it == chain.end())
            return {};

        return reinterpret_cast<const U*>(it->second.data());
    }

    std::optional<std::span<char>> getStructOptional(VkStructureType sType)
    {
        if (sType == main.sType)
            return std::span<char>(reinterpret_cast<char*>(&main), sizeof(T));

        auto it = chain.find(sType);
        if (it == chain.end())
            return {};

        return it->second;
    }

    std::optional<std::span<const char>> getStructOptional(VkStructureType sType) const
    {
        if (sType == main.sType)
            return std::span<const char>(reinterpret_cast<const char*>(&main), sizeof(T));

        auto it = chain.find(sType);
        if (it == chain.end())
            return {};

        return it->second;
    }

    std::vector<VkStructureType> enumerateStructureTypes() const
    {
        std::vector<VkStructureType> keys { main.sType };
        std::transform(chain.begin(), chain.end(), std::back_inserter(keys), [](const auto &entry)->auto { return entry.first; } );
        return keys;
    }

private:
    T main;
    std::unordered_map<VkStructureType, std::vector<char>> chain;
};

} // namespace vknc
