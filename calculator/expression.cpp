#include <iostream>
#include <vector>
#include <assert.h>
#include <string>
#include <stack>
#include "expression.h"
#include <QDebug>
#include "types.h"

using namespace std;

volatile int err = 0;

volatile int* get_err()
{
    return &err;
}

// 定义符号的优先级
int priority(char oper)
{
    if (oper == '(') // 左括号优先级最低，防止其发生计算
        return 0;
    else if (oper == '+' || oper == '-')
        return 1;
    else if (oper == '*' || oper == '/')
        return 2;

    return -1;  //最好不返回未定义的值,随便返回个值算了
}

// 对一个运算符进行一次计算
bool calculate(stack<duint> &nums, stack<char> &symbols)
{
    if (nums.size() < 2) // 数字不够用，说明表达式不合法
    {
        return false;
    }

    duint b = nums.top();
    nums.pop();
    duint a = nums.top();
    nums.pop();

    char oper = symbols.top();
    symbols.pop();

    if (oper == '+')
        nums.push(a + b);
    else if (oper == '-')
        nums.push(a - b);
    else if (oper == '*')
        nums.push(a * b);
    else if (oper == '/')
        nums.push(a / b);
    else if (oper == '|')
        nums.push(a | b);
    else if (oper == '&')
        nums.push(a & b);
    else if (oper == '%')
        nums.push(a % b);
    else if (oper == '^')
        nums.push(a ^ b);

    return true;
}

// 表达式计算，计算过程中会检查合法性
duint expression_calculation(const string &rs)
{
    stack<duint> nums;	 // 数字栈
    stack<char> symbols; // 符号栈（单调栈）

    string s;
    //去除空格,省事
    for(int i = 0; i < rs.length(); i++)
    {
        if(rs[i] != ' ')
            s.push_back(rs[i]);
    }

    for (int i = 0; i < s.length(); i++)
    {
        //如果有.转换为16进制运算
        if(s[i] == '.')
        {
            i++;

            duint hex = 0;
            string str;

            for(;i < s.length() && (bool)isxdigit(s[i]); i++)
            {
                str.push_back(s[i]);
            }

            sscanf_s(str.data(),"%llX",&hex);
            nums.push(hex);
            if (i >= s.length())
                break;
        }
        else if (isdigit(s[i])) // 读取数字
        {
            duint num = 0;
            for (; i < s.length() && isdigit(s[i]); i++)
                num = num * 10 + s[i] - '0';
            nums.push(num);
            if (i >= s.length())
                break;
        }

        // 下面读取运算符，考虑进行计算
        if (s[i] == '(') // 左括号直接入栈
            symbols.push(s[i]);
        else if (s[i] == ')') // 右括号与左括号之间的表达式全部计算
        {
            while (!symbols.empty() && symbols.top() != '(')
            {
                if(!calculate(nums, symbols))
                {
                    err = 1;
                    return -1;
                }
            }
            // 括号内表达式计算完成后，弹出左括号
            if (symbols.empty()) // 符号不够，说明缺少(
            {
                err = 2;
                return -1;
            }
            symbols.pop(); // 弹出(
        }
        else // + - * /
        {
            while (!symbols.empty() && priority(symbols.top()) >= priority(s[i])) // 栈内优先级 高于等于 s[i] 则可以计算
            {
                if(!calculate(nums, symbols))
                {
                    err = 3;
                    return -1;
                }
            }
            symbols.push(s[i]); // 当前运算符入栈
        }
    }
    // 栈中剩余运算符进行计算
    while (!symbols.empty())
    {
        if(!calculate(nums, symbols))
        {
            err = 4;
            return -1;
        }
    }
    // 检查计算结果，表达式是否合法
    if (nums.size() != 1 || !symbols.empty())
    {
        err = 5;
        return -1;
    }

    // 返回结果
    err = 0;
    return nums.top();
}
