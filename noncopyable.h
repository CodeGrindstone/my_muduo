#pragma once


/*
noncopyable作用：
    1. noncopyable不允许在类外部构造和析构
    2. 继承noncopyable的类，不允许拷贝和赋值操作
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