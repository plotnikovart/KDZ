#pragma once

#include <fstream>
#include "math.h"


using namespace std;

/// Подсчет частоты символов в файле и энтропии
class FrequancyEntropy
{
public:
    /// Подсчет встречаемости каждого символа и количества символов в файле
    /// \param fInput Файл для подсчета
    /// \param quantity Массив, в который заносится количество каждого символа из файла
    /// \return Количество символов в файле
    static unsigned int countFrequancy(ifstream& fInput, unsigned int*& quantity)
    {   
        quantity = new unsigned int[256];

        // Обнуление данных
        for (int i = 0; i < 256; i++)
            quantity[i] = 0;
        
        char ch;
        unsigned int n = 0;
        
        fInput.clear();
        fInput.seekg(0, ios_base::beg);

        // Подсчет
        while (fInput.get(ch))
        {
            quantity[(unsigned char)ch]++;
            n++;
        }

        return n;
    }

public:
    void count(ifstream& fInput)
    {
        // Подсчет количества символов
        unsigned int* quantity;
        unsigned int sum = countFrequancy(fInput, quantity);

        // Подсчет частоты
        freq = new double[256];

        for (int i = 0; i < 256; i++)
            freq[i] = (double)quantity[i] / sum;
        delete[] quantity;

        entropy = 0;

        // Подсчет энтропии
        for (int i = 0; i < 256; i++)
            if (freq[i] != 0) entropy += (-1)*freq[i] * log2(freq[i]);
    }

    double* getFrequancy()
    {
        return freq;
    }

    double getEntropy()
    {
        return entropy;
    }

    ~FrequancyEntropy()
    {
        delete[] freq;
    }

private:
    double* freq;       // частота встречаемости символов
    double entropy;     // энтропия
};