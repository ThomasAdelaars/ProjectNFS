#pragma once
#include <vector>

template<typename T>
class IReadRepository
{
public:
    virtual ~IReadRepository() = default;

    virtual std::vector<T> getAll() = 0;
};