#include <iostream>
#include <string>
#include <tuple>
#include <stdint.h>
#include <type_traits>

// 定义类型信息模板，用于获取类型的名称和大小
template<typename T>
struct TypeInfo;

#define DEFINE_TYPE_INFO(type, typeName)    \
template<>                                  \
struct TypeInfo<type> {                     \
    static std::string getName() { return typeName; }  \
    static size_t getSize() { return sizeof(type); }   \
};

// 为基本类型提供特化版本
DEFINE_TYPE_INFO(char, "char")
DEFINE_TYPE_INFO(unsigned char, "uchar")
DEFINE_TYPE_INFO(short, "short")
DEFINE_TYPE_INFO(unsigned short, "ushort")
DEFINE_TYPE_INFO(int, "int")
DEFINE_TYPE_INFO(unsigned int, "uint")
DEFINE_TYPE_INFO(float, "float")
DEFINE_TYPE_INFO(double, "double")

// 宏：用于定义结构体并注册成员
#define REFLECTABLE(...) \
    static const auto getMembers() { return std::make_tuple(__VA_ARGS__); }

#define MEMBER_INFO(type, member, memberType) \
    std::make_tuple(#member, TypeInfo<memberType>::getName(), sizeof(memberType), &type::member)

// 示例结构体
struct MyStruct {
    int a;
    uint g;
    float b;
    double c;
    short d;
    ushort e;
    char f;
    // 使用宏注册结构体成员
    REFLECTABLE(MEMBER_INFO(MyStruct, a, int),MEMBER_INFO(MyStruct, g, uint), MEMBER_INFO(MyStruct, b, float), MEMBER_INFO(MyStruct, c, double),
    MEMBER_INFO(MyStruct, d, short),MEMBER_INFO(MyStruct, e, ushort),MEMBER_INFO(MyStruct, f, char)
    )
};

// 辅助函数：递归打印成员信息
template <typename T, std::size_t... I>
void printMembersImpl(const T& t, std::index_sequence<I...>) {
    ((std::cout << "Name: " << std::get<0>(std::get<I>(t)) 
                << ", Type: " << std::get<1>(std::get<I>(t)) 
                << ", Size: " << std::get<2>(std::get<I>(t)) << " bytes\n"), ...);
}

template <typename T>
void printMembers(const T& t) {
    const auto members = T::getMembers();
    printMembersImpl(members, std::make_index_sequence<std::tuple_size_v<decltype(members)>>{});
}

int main() {
    MyStruct s;
    printMembers(s); // 打印结构体成员信息
    return 0;
}
