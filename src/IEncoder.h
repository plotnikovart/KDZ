#pragma once

#include <fstream>
#include <string>

/// Интерфейс, определяющий структуру алгоритмов кодирования 
class IEncoder
{
public:
    virtual void pack(std::ifstream&, std::string directory, std::string fileName) = 0;

    virtual void unpack(std::string directory, std::string fileName) = 0;

    virtual double getCompression() = 0;

    virtual ~IEncoder() = default;

    virtual string getName() = 0;
};