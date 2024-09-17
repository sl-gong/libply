#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <tuple>
#include <unordered_map>
#include <stdexcept>
#include <cstring>  // For memcpy
#include <type_traits>

// 示例：用户自定义的点结构体
struct CustomVertex {
    float x, y, z;  // 必须的坐标属性
    unsigned char r, g, b;  // 自定义属性
};

// 重载输出操作符 <<
std::ostream& operator<<(std::ostream& os, const CustomVertex& vertex) {
    os << "Position: (" << vertex.x << ", " << vertex.y << ", " << vertex.z << ") ";
    os << "Color: (" << static_cast<int>(vertex.r) << ", " << static_cast<int>(vertex.g) << ", " << static_cast<int>(vertex.b) << ")";
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

// 属性类型映射
enum class PropertyType {
    CHAR,
    UCHAR,
    SHORT,
    USHORT,
    INT,
    UINT,
    FLOAT,
    DOUBLE
};

// 定义属性类型的大小和属性名映射
template<typename T>
struct TypeInfo;

template<>
struct TypeInfo<char> {
    static constexpr PropertyType type = PropertyType::CHAR;
    static constexpr const char* name = "char";
};

template<>
struct TypeInfo<unsigned char> {
    static constexpr PropertyType type = PropertyType::UCHAR;
    static constexpr const char* name = "uchar";
};

template<>
struct TypeInfo<short> {
    static constexpr PropertyType type = PropertyType::SHORT;
    static constexpr const char* name = "short";
};

template<>
struct TypeInfo<unsigned short> {
    static constexpr PropertyType type = PropertyType::USHORT;
    static constexpr const char* name = "ushort";
};

template<>
struct TypeInfo<int> {
    static constexpr PropertyType type = PropertyType::INT;
    static constexpr const char* name = "int";
};

template<>
struct TypeInfo<unsigned int> {
    static constexpr PropertyType type = PropertyType::UINT;
    static constexpr const char* name = "uint";
};

template<>
struct TypeInfo<float> {
    static constexpr PropertyType type = PropertyType::FLOAT;
    static constexpr const char* name = "float";
};

template<>
struct TypeInfo<double> {
    static constexpr PropertyType type = PropertyType::DOUBLE;
    static constexpr const char* name = "double";
};

// 提取结构体成员属性名、偏移量和类型
#define DEFINE_PROPERTY_INFO(Type, Member) {#Member, offsetof(Type, Member), TypeInfo<decltype(Type::Member)>::type}

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

        // 读取头部
        readHeader<VertexType>(file);

        // 读取顶点数据
        vertices.resize(vertex_count_);
        for (auto& vertex : vertices) {
            // 读取 xyz 坐标
            //file.read(reinterpret_cast<char*>(&vertex.x), sizeof(vertex.x));
            //file.read(reinterpret_cast<char*>(&vertex.y), sizeof(vertex.y));
            //file.read(reinterpret_cast<char*>(&vertex.z), sizeof(vertex.z));

            // 读取其他属性
            for (const auto& property : custom_properties_) {
                char* vertex_ptr = reinterpret_cast<char*>(&vertex);
                file.read(vertex_ptr + property.offset, property.size);
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
            // 写入 xyz 坐标
            //file.write(reinterpret_cast<const char*>(&vertex.x), sizeof(vertex.x));
            //file.write(reinterpret_cast<const char*>(&vertex.y), sizeof(vertex.y));
            //file.write(reinterpret_cast<const char*>(&vertex.z), sizeof(vertex.z));

            // 写入其他属性
            for (const auto& property : custom_properties_) {
                const char* vertex_ptr = reinterpret_cast<const char*>(&vertex);
                file.write(vertex_ptr + property.offset, property.size);
            }
        }
        file.close();
    }

private:
    struct Property {
        std::string name;
        size_t offset;
        size_t size;
        PropertyType type;
    };

    std::string filename_;
    size_t vertex_count_ = 0;
    std::vector<Property> custom_properties_;

    // 读取头部，自动识别属性
    template<typename VertexType>
    void readHeader(std::ifstream& file) {
        std::string line;
        while (std::getline(file, line)) {
            if (line == "end_header") break;

            if (line.find("element vertex") != std::string::npos) {
                vertex_count_ = std::stoi(line.substr(line.find_last_of(' ') + 1));
            }

            custom_properties_.clear();
            custom_properties_.emplace_back(Property{"x",0,4,PropertyType::FLOAT});
            custom_properties_.emplace_back(Property{"y",4,4,PropertyType::FLOAT});
            custom_properties_.emplace_back(Property{"z",8,4,PropertyType::FLOAT});
            size_t offset = 12;
            if (line.find("property") != std::string::npos) {
                auto tokens = tokenize(line);
                if (tokens.size() >= 3) {
                    std::string property_type = tokens[1];
                    std::string property_name = tokens[2];

                    if (property_type == TypeInfo<unsigned char>::name) {
                        custom_properties_.emplace_back(Property{property_name, offset, sizeof(unsigned char), PropertyType::UCHAR});
                        offset += sizeof(unsigned char);
                    }
                    else if (property_type == TypeInfo<char>::name) {
                        custom_properties_.emplace_back(Property{property_name, offset, sizeof(char), PropertyType::CHAR});
                        offset += sizeof(char);
                    }
                    else if (property_type == TypeInfo<short>::name) {
                        custom_properties_.emplace_back(Property{property_name, offset, sizeof(short), PropertyType::SHORT});
                        offset += sizeof(short);
                    }
                    else if (property_type == TypeInfo<ushort>::name) {
                        custom_properties_.emplace_back(Property{property_name, offset, sizeof(ushort), PropertyType::USHORT});
                        offset += sizeof(ushort);
                    }
                    else if (property_type == TypeInfo<int>::name) {
                        custom_properties_.emplace_back(Property{property_name, offset, sizeof(int), PropertyType::INT});
                        offset += sizeof(int);
                    }
                    else if (property_type == TypeInfo<uint>::name) {
                        custom_properties_.emplace_back(Property{property_name, offset, sizeof(uint), PropertyType::UINT});
                        offset += sizeof(uint);
                    }
                    else if (property_type == TypeInfo<float>::name) {
                        custom_properties_.emplace_back(Property{property_name, offset, sizeof(float), PropertyType::FLOAT});
                        offset += sizeof(float);
                    }
                    else if (property_type == TypeInfo<double>::name) {
                        custom_properties_.emplace_back(Property{property_name, offset, sizeof(double), PropertyType::DOUBLE});
                        offset += sizeof(double);
                    }
                }
            }
        }
    }

    // 写入头部
    template<typename VertexType>
    void writeHeader(std::ofstream& file) const {
        file << "ply\n";
        file << "format binary_little_endian 1.0\n";
        file << "element vertex " << vertex_count_ << "\n";
        //file << "property float x\n";
        //file << "property float y\n";
        //file << "property float z\n";

        for (const auto& property : custom_properties_) {
            switch (property.type) {
                case PropertyType::UCHAR:
                    file << "property uchar " << property.name << "\n";
                    break;
                case PropertyType::CHAR:
                    file << "property char " << property.name << "\n";
                    break;
                case PropertyType::SHORT:
                    file << "property short " << property.name << "\n";
                    break;
                case PropertyType::USHORT:
                    file << "property ushort " << property.name << "\n";
                    break;
                case PropertyType::INT:
                    file << "property int " << property.name << "\n";
                    break;
                case PropertyType::UINT:
                    file << "property uint " << property.name << "\n";
                    break;
                case PropertyType::FLOAT:
                    file << "property float " << property.name << "\n";
                    break;
                case PropertyType::DOUBLE:
                    file << "property double " << property.name << "\n";
                    break;
                default:
                    break;
            }
        }
        file << "end_header\n";
    }

    // 将字符串按空格拆分为 tokens
    std::vector<std::string> tokenize(const std::string& line) const {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(line);
        while (std::getline(tokenStream, token, ' ')) {
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }
        return tokens;
    }
};

int main() {
    PlyBinaryIO ply("example.ply");

    // 自定义的顶点数据
    std::vector<CustomVertex> vertices = {
        {0, 22, 1, 255, 0, 0},
        {1, 33, 20, 0, 255, 0},
        {2, 44, 3, 0, 0, 255}
    };

    // 写入 PLY 文件
    ply.write(vertices);

    // 读取 PLY 文件
    std::vector<CustomVertex> readVertices;
    ply.read(readVertices);

    // 输出读取的顶点数据
    for (const auto& v : readVertices) {
        std::cout << v << std::endl;
        //std::cout << "Vertex: " << v.x << ", " << v.y << ", " << v.z << ", " << v.r<< ", " << v.g<< ", " << v.b <<"\n";
    }

    return 0;
}
