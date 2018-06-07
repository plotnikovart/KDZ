#pragma once

#include <fstream>
#include <string>

using namespace std;

class BitWriter
{
public:
    BitWriter(const string& path)
    {
        file.open(path, ios::binary);

        bufferLength = 0;
        byte = 0;
    }

    ~BitWriter()
    {
        close();
    }

    BitWriter& operator<<(bool bit)
    {
        if (bit)
            byte |= byteRank[bufferLength];

        bufferLength++;
        
        if (bufferLength == 8) 
            writeByte();

        return *this;
    }

    BitWriter& operator<<(unsigned int uint)
    {
        // Сливаем остатки битового буфера (если буфер заполнен не полностью, то остаток байта запишется нулями,
        // а уже следующие 4 байта будут записаны как unsigned int)
        if (bufferLength != 0)
            writeByte();

        // Побайтово записываем uint в файл
        unsigned char ch[4];
        for (int i = 3; i >= 0; i--)
        {
            ch[i] = 0;
            ch[i] |= uint;
            uint >>= 8;
        }

        for (int i = 0; i < 4; i++)
            file << ch[i];

        return *this;
    }

    BitWriter& operator<<(unsigned short int uint)
    {
        // Сливаем остатки битового буфера (если буфер заполнен не полностью, то остаток байта запишется нулями,
        // а уже следующие 2 байта будут записаны как unsigned short int)
        if (bufferLength != 0)
            writeByte();

        // Побайтово записываем uint в файл
        unsigned char ch[2];
        for (int i = 1; i >= 0; i--)
        {
            ch[i] = 0;
            ch[i] |= uint;
            uint >>= 8;
        }

        for (int i = 0; i < 2; i++)
            file << ch[i];

        return *this;
    }

    BitWriter& operator<<(char ch)
    {
        // Сливаем остатки битового буфера (если буфер заполнен не полностью, то остаток байта запишется нулями,
        // а уже в следующий байт пишем наш символ)
        if (bufferLength != 0)
            writeByte();

        file << ch;

        return *this;
    }

    void close()
    {
        if (bufferLength != 0) 
            writeByte();

        file.close();
    }

    unsigned int getFileSize()
    {
        return (unsigned int)file.tellp() + (bufferLength == 0 ? 0 : 1);
    }

private:
    void writeByte()
    {
        file << byte;
        bufferLength = 0;
        byte = 0;
    }

private:
    ofstream file;

    int byteRank[8] = {128, 64, 32, 16, 8, 4, 2, 1 };
    int bufferLength;       // длина буфера (количество битов в буфере на данный момент)
    unsigned char byte;
};


class BitReader
{
public:
    BitReader(const string& path)
    {
        file.open(path, ios::binary);
        
        bufferRead = 8;
        byte = 0;
    }

    BitReader& operator>>(bool& bit)
    {
        if (bufferRead == 8)    // все биты прочитаны, нужно считать новый байт
            readByte();
       
        bit = (byte == (byte | byteRank[bufferRead++]));

        return *this;
    }

    BitReader& operator>>(unsigned int& uint)
    {
        uint = 0;
        char ch;
        // Информация в буфере теряется, считываются следущие 4 байта 
        for (int i = 0; i < 4; i++)
        {
            uint <<= 8;
            file.get(ch);
            uint |= (unsigned char)ch;
        }

        return *this;
    }

    BitReader& operator>>(unsigned short int& uint)
    {
        uint = 0;
        char ch;
        // Информация в буфере теряется, считываются следущие 4 байта 
        for (int i = 0; i < 2; i++)
        {
            uint <<= 8;
            file.get(ch);
            uint |= (unsigned char)ch;
        }

        return *this;
    }

    BitReader& operator>>(char& ch)
    {
        ch = 0;
        file.get(ch);

        return *this;
    }

    void close()
    {
        file.close();
    }

    operator bool()
    {
        return (bool)file;
    }

private:
    void readByte()
    {
        char ch;
        file.get(ch);

        byte = (unsigned char)ch;
        bufferRead = 0;
    }

private:
    ifstream file;

    int byteRank[8] = { 128, 64, 32, 16, 8, 4, 2, 1 };
    int bufferRead;             // количество прочитаных битов из буфера
    unsigned char byte;
};