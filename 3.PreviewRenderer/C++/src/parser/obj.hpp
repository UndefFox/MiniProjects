#pragma once

#include <string>
#include <fstream>



namespace Parser::Obj {

template <typename T>
decltype(T::result) readFile(const std::string& filePath) {
    T adapter{};

    std::ifstream file(filePath);

    if (!file.is_open()) { throw std::runtime_error("Couldn't open the file: " + filePath); }

    std::string line;
    while (std::getline(file, line)) {
        const char* str = line.c_str();
        const char* end = str + line.length();

        if (line.starts_with("v") && (line[1] == ' ' || line[1] == '\t')) {
            str += 2;

            if (str >= end) [[unlikely]] { throw std::runtime_error("Error in parsing file: " + filePath); }

            std::array<float, 3> pos;

            auto [ptr1, ec1] = std::from_chars(str, end, pos[0]);
            auto [ptr2, ec2] = std::from_chars(ptr1 + 1, end, pos[1]);
            auto [ptr3, ec3] = std::from_chars(ptr2 + 1, end, pos[2]);

            if (ec1 != std::errc() || ec2 != std::errc() || ec3 != std::errc()) [[unlikely]] { throw std::runtime_error("Error in parsing file: " + filePath); }

            adapter.nextV(pos);
        }
        else if (line.starts_with("vn")) {
            str += 3;

            if (str >= end) [[unlikely]] { throw std::runtime_error("Error in parsing file: " + filePath); }

            std::array<float, 3> normal;

            auto [ptr1, ec1] = std::from_chars(str, end, normal[0]);
            auto [ptr2, ec2] = std::from_chars(ptr1 + 1, end, normal[1]);
            auto [ptr3, ec3] = std::from_chars(ptr2 + 1, end, normal[2]);

            if (ec1 != std::errc() || ec2 != std::errc() || ec3 != std::errc()) [[unlikely]] { throw std::runtime_error("Error in parsing file: " + filePath); }

            adapter.nextVN(normal);
        }
        else if (line.starts_with("vt")) {
            str += 3;

            if (str >= end) [[unlikely]] { throw std::runtime_error("Error in parsing file: " + filePath); }

            std::array<float, 2> cord;

            auto [ptr1, ec1] = std::from_chars(str, end, cord[0]);
            auto [ptr2, ec2] = std::from_chars(ptr1 + 1, end, cord[1]);

            if (ec1 != std::errc() || ec2 != std::errc()) [[unlikely]] { throw std::runtime_error("Error in parsing file: " + filePath); }

            adapter.nextVT(cord);
        }
        else if (line.starts_with("f")) {
            str += 2;

            if (str >= end) [[unlikely]] { throw std::runtime_error("Error in parsing file: " + filePath); }

            std::array<std::array<int, 3>, 3> ids{};

            const char* currentPointer = str;

            for (int faceIndex = 0; faceIndex < 3; faceIndex++) {
                auto [ptr1, ec1] = std::from_chars(currentPointer, end, ids[faceIndex][0]);
                if (ec1 != std::errc()) [[unlikely]] throw std::runtime_error("Error in parsing file: " + filePath);
                currentPointer = ptr1 + 1;

                if (*ptr1 != '/') continue;

                if (*currentPointer != '/') {
                    auto [ptr2, ec2] = std::from_chars(currentPointer, end, ids[faceIndex][1]);
                    if (ec2 != std::errc()) [[unlikely]] throw std::runtime_error("Error in parsing file: " + filePath);
                    currentPointer = ptr2 + 1;

                    if (*ptr2 != '/') continue;
                }
                else {
                    currentPointer += 1;
                }

                auto [ptr3, ec3] = std::from_chars(currentPointer, end, ids[faceIndex][2]);
                if (ec3 != std::errc()) [[unlikely]] throw std::runtime_error("Error in parsing file: " + filePath);
                currentPointer = ptr3 + 1;
            }

            adapter.nextF(ids);
        }
    }

    return adapter.result;
}

}

