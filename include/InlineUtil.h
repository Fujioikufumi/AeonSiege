#pragma once
template<typename T>
inline void SafeDelete(T*& ptr)
{
    if (ptr != nullptr)
    {
        delete ptr;
        ptr = nullptr;
    }
}

template<typename T>
inline void SafeDeleteArray(T*& ptr)
{
    if (ptr != nullptr)
    {
        delete[] ptr;
        ptr = nullptr;
    }
}

template<typename T>
inline void SafeRelease(T*& ptr)
{
    if (ptr != nullptr)
    {
        ptr->Release();
        ptr = nullptr;
    }
}

template<typename T>
inline void SafeTerm(T*& ptr)
{
    if (ptr != nullptr)
    {
        ptr->Term();
        delete ptr;
        ptr = nullptr;
    }
}