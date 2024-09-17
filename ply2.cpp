
#include <fstream>
#include <sstream>

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

#pragma pack(push, 1)
// 示例结构体
struct CustomVertex {
    float x, y, z;
    unsigned char r, g, b;
    // 使用宏注册结构体成员
    REFLECTABLE(
        MEMBER_INFO(CustomVertex, x, float),
        MEMBER_INFO(CustomVertex, y, float), 
        MEMBER_INFO(CustomVertex, z, float), 
        MEMBER_INFO(CustomVertex, r, unsigned char),
        MEMBER_INFO(CustomVertex, g, unsigned char),
        MEMBER_INFO(CustomVertex, b, unsigned char)
    )
};
#pragma pack(pop)

// 重载输出操作符 <<
std::ostream& operator<<(std::ostream& os, const CustomVertex& vertex) {
    os << "Position: (" << vertex.x << ", " << vertex.y << ", " << vertex.z << ") ";
    os << "Color: (" << static_cast<int>(vertex.r) << ", " << static_cast<int>(vertex.g) << ", " << static_cast<int>(vertex.b) << ")";
    //os << "intensity: " << vertex.intensity;
    return os;
}

// 重载输入操作符 >>
std::istream& operator>>(std::istream& is, CustomVertex& vertex) {
    int r, g, b;
    is >> vertex.x >> vertex.y >> vertex.z >> r >> g >> b;
    vertex.r = static_cast<unsigned char>(r);
    vertex.g = static_cast<unsigned char>(g);
    vertex.b = static_cast<unsigned char>(b);
    return is;
}

// 辅助函数：递归打印属性列表信息
template <typename T, std::size_t... I>
void printMembersImpl(const T& t, std::index_sequence<I...>,std::ofstream& file) {
    ((file << "property " << std::get<1>(std::get<I>(t)) 
                << " " << std::get<0>(std::get<I>(t)) << "\n"), ...);
}

bool isLittleEndian() {
    unsigned int x = 1;
    char *ptr = (char*)&x;
    return ptr[0] == 1;
}

// 字节序转换函数：大端到小端，或小端到大端
template<typename T>
T swapEndian(T value) {
    static_assert(std::is_trivially_copyable_v<T>, "必须是可复制的基本类型");
    T result;
    auto valuePtr = reinterpret_cast<unsigned char*>(&value);
    auto resultPtr = reinterpret_cast<unsigned char*>(&result);
    for (size_t i = 0; i < sizeof(T); i++) {
        resultPtr[i] = valuePtr[sizeof(T) - 1 - i];
    }
    return result;
}

class PlyBinaryIO {
public:
    // 构造函数传入文件名
    PlyBinaryIO(const std::string& filename) : filename_(filename) {}

    // 读取 PLY 文件
    template<typename VertexType>
    void read(std::vector<VertexType>& vertices) {
        std::ifstream file(filename_, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for reading: " + filename_);
        }

        bool fileIsLittleEndian = readHeader<VertexType>(file);
        bool systemIsLittleEndian = isLittleEndian();

        // 读取顶点数据
        vertices.resize(vertex_count_);
        for (auto& vertex : vertices) {
            file.read(reinterpret_cast<char*>(&vertex), sizeof(vertex));

            if (fileIsLittleEndian != systemIsLittleEndian) {
                swapEndian<float>(vertex.x);
                swapEndian<float>(vertex.y);
                swapEndian<float>(vertex.z);
            }
        }
        file.close();
    }

    // 写入 PLY 文件
    template<typename VertexType>
    void write(const std::vector<VertexType>& vertices) {
        std::ofstream file(filename_, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for writing: " + filename_);
        }

        vertex_count_ = vertices.size();
        // 写入头部
        writeHeader<VertexType>(file);

        // 写入顶点数据
        for (const auto& vertex : vertices) {
            file.write(reinterpret_cast<const char*>(&vertex), sizeof(vertex));
        }
        file.close();
    }

private:
    
    std::string filename_;
    size_t vertex_count_ = 0;
    
    // 读取头部，自动识别属性
    template<typename VertexType>
    bool readHeader(std::ifstream& file) {
        std::string line;
        bool fileIsLittleEndian = true;  // 默认小端序
        while (std::getline(file, line)) {
            if (line == "end_header") break;
            if (line.find("element vertex") != std::string::npos) {
                vertex_count_ = std::stoi(line.substr(line.find_last_of(' ') + 1));
            }
            if (line.find("format") != std::string::npos) {
                if (line.find("binary_big_endian") != std::string::npos) {
                    fileIsLittleEndian = false;
                } else if (line.find("binary_little_endian") != std::string::npos) {
                    fileIsLittleEndian = true;
                }
            }
        }
        return fileIsLittleEndian;
    }

    // 写入头部
    template<typename VertexType>
    void writeHeader(std::ofstream& file) const {
        file << "ply\n";
        if(isLittleEndian())
            file << "format binary_little_endian 1.0\n";
        else
            file << "format binary_big_endian 1.0\n";
        file << "element vertex " << vertex_count_ << "\n";
        
        const auto members = VertexType::getMembers();
        printMembersImpl(members, std::make_index_sequence<std::tuple_size_v<decltype(members)>>{},file);

        file << "end_header\n";
    }
};

// 函数用于分割字符串，根据多个分隔符进行分割
std::vector<std::string> split(const std::string& s, const std::string& delimiters) {
    std::vector<std::string> tokens;
    size_t start = 0, end = 0;
    while ((end = s.find_first_of(delimiters, start)) != std::string::npos) {
        if (end != start) {
            tokens.emplace_back(s.substr(start, end - start));
        }
        start = end + 1;
    }
    if (start < s.size()) {
        tokens.emplace_back(s.substr(start));
    }
    return tokens;
}

// 读取asc文件
bool readAscFile(const std::string& filename, std::vector<CustomVertex>& points) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return false;
    }

    std::string line;
    CustomVertex point;
    int count = 0;
    while (std::getline(file, line)) {
        // 使用空格、制表符、逗号作为分隔符
        std::vector<std::string> tokens = split(line, " \t,");
        if (tokens.size() < 6) {
            std::cerr << "无效的点数据: " << line << std::endl;
            continue;
        }

        // 读取前6个字段 (XYZRGB)
        point.x = std::stof(tokens[0]);
        point.y = std::stof(tokens[1]);
        point.z = std::stof(tokens[2]);
        point.r = static_cast<unsigned char>(std::stof(tokens[3]) * 255);
        point.g = static_cast<unsigned char>(std::stof(tokens[4]) * 255);
        point.b = static_cast<unsigned char>(std::stof(tokens[5]) * 255);

        points.push_back(point);
    }

    file.close();
    return true;
}

int main() {
    // std::cout << "sizeof(CustomVertex):" << sizeof(CustomVertex) << std::endl;
    // return 1;

    PlyBinaryIO ply("example.ply");

    // 自定义的顶点数据
    std::vector<CustomVertex> vertices;
    readAscFile("/Users/gsl/work/das/vsg/plylib/GIR100_240228_145252_color_cloud.txt",vertices);
    std::cout << "read txt" << std::endl;
    int i = 0;
    for (const auto& v : vertices) {
        i++;
        if(i > 10)
            break;
        std::cout << v << std::endl;
        //std::cout << "Vertex: " << v.x << ", " << v.y << ", " << v.z << ", " << v.r<< ", " << v.g<< ", " << v.b <<"\n";
    }
    // 写入 PLY 文件
    ply.write(vertices);

    vertices.clear();
    vertices.shrink_to_fit();
    // 读取 PLY 文件
    std::vector<CustomVertex> readVertices;
    ply.read(readVertices);

    // // 输出读取的顶点数据
    std::cout << "read ply" << std::endl;
    i = 0;
    for (const auto& v : readVertices) {
        i++;
        if(i > 10)
            break;
        std::cout << v << std::endl;
        //std::cout << "Vertex: " << v.x << ", " << v.y << ", " << v.z << ", " << v.r<< ", " << v.g<< ", " << v.b <<"\n";
    }

    return 0;
}
