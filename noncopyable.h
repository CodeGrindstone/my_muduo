#pragma once


/*
* noncopyable不允许在类外部构造和析构
* noncopyable被继承以后，派生类可以类内部可以正常调用noncopyable的构造和析构
* 但是无法在外部调用拷贝构造和拷贝赋值
*/
class noncopyable
{
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};