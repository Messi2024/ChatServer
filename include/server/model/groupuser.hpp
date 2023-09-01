#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

//群组用户类，多了一个role角色信息，其他信息从User类中继承，复用User的其他信息
class GroupUser : public User
{
public:
    void setRole(string role){this->role = role;}
    string getRole(){return this->role;}
private:
    string role;
};

#endif
