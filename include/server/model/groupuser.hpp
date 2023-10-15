#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

// 群组用户，多了一个role角色信息，从User类直接继承，复用User的其它信息
//相对于user的信息，还要知道其在群组中的role
class GroupUser : public User//从user继承而来，组员信息
{
public:
    void setRole(string role) { this->role = role; }
    string getRole() { return this->role; }

private:
    string role;//只是添加了派生类的特殊成员变量role
};

#endif