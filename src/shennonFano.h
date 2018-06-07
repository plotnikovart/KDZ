#pragma once

#include <fstream>
#include <string>
#include <list>

#include "IEncoder.h"
#include "BitWriterReader.h"
#include "frequancyEntropy.h"

using namespace std;

/// Алгоритм Шеннона-Фано
class ShannonFano : public IEncoder
{
private:
    class Node;

public:
    /// Кодирование файла по методу Шенона-Фано
    /// \param file Поток исходного файла
    /// \param directory Путь до папки, в которой лежит файл
    /// \param fileName Имя кодируемого файла
    void pack(ifstream& file, string directory, string fileName)
    {
        // Получение исходных данных: частоты и количество символов
        sum = FrequancyEntropy::countFrequancy(file, freq);

        // Запуск алгоритма
        build(true);

        // Упаковка данных в файл
        BitWriter bw(directory + "pack/" + fileName + ".shan");

        // Запись частот в файл
        for (int i = 0; i < 256; i++)
        {
            if (matr[i] == -1) 
                bw << (unsigned int)0;
            else 
                bw << freq[matr[i]];
        }
            
        // Запись закодированного сообщения в битах в файл
        char ch;
        list<bool> code;
        
        file.clear();
        file.seekg(0);

        while (file.get(ch))
        {
            code = codes[matr[(unsigned char)ch]];

            for (bool c : code)
                bw << c;
        }

        // Определение коэффицента сжатия
        file.clear();
        file.seekg(0, file.end);
        compression = file.tellg() / (double)bw.getFileSize();

        // Освобождение ресурсов
        bw.close();
        delete[] matr;
        delete[] freq;
        delete[] codes;
    }

    /// Декодирование закодированного файла по методу Шенона-Фано
    /// \param file Поток исходного файла
    /// \param directory Путь до папки, в которой лежит файл
    /// \param fileName Имя кодируемого файла
    void unpack(string directory, string fileName)
    {
        freq = new unsigned int[256];
        sum = 0;

        // Считывание массива частот 
        BitReader br(directory + "pack/" + fileName + ".shan");
        for (int i = 0; i < 256; i++)
        {
            br >> freq[i];
            sum += freq[i];
        }

        // Восстановление дерева по массиву частот
        build(false);

        // Готовим файл для записи раскодированного сообщения
        ofstream encodeFile;
        encodeFile.open(directory + "unpack/" + fileName + ".unshan", ios::binary);
        
        // Считывание битов, проход по дереву кодов, запись символов в файл
        bool bit;
        Node* currentNode = rootNode;

        while (br >> bit)
        {
            currentNode = bit ? currentNode->oneChild : currentNode->zeroChild;

            if (currentNode->isLeaf())
            {
                encodeFile << (char)currentNode->value; // добавление в файл символа
                currentNode = rootNode;
            }
        }
        
        // Освобождение ресурсов
        br.close();
        encodeFile.close();
        delete[] matr;
        delete[] freq;
        delete rootNode;
    }

    /// Коэффицент сжатия для данного алгоритма
    /// \return Отношение объема исходных данных к закодированным (больше - лучше)
    double getCompression()
    {
        return compression;
    }

    string getName()
    {
        return "Shanon-Fano";
    }

private:
    /// Точка входа в алгоритм
    /// \param packMode Флаг, отвечающий за то, в каком режиме будет выполнен алгоритм: упаковка или распаковка
    void build(bool packMode)
    {
        // Сортировка исходного массива, получение матрицы переходов и определение индекса 
        int lastInd = sort();

        if (packMode)
        {
            codes = new list<bool>[256];
            recursiveDivision(0, lastInd, sum);
        }
        else
        {
            rootNode = new Node();
            recursiveDivision(0, lastInd, sum, rootNode);
        }
    }

    /// Рекурсивный метод для кодирования методом Шенона. Делит массив частот на две равные половины 
    /// \param left Левая граница интервала в массиве 
    /// \param right Правая граница интервала в массиве
    /// \param sum Сумма символов в данном интервале
    void recursiveDivision(int left, int right, unsigned int sum, Node* currentNode = nullptr)
    {
        if (left == right) return;

        int i;
        unsigned int s = 0;

        for (i = left; i < right; i++)
        {
            s += freq[i];
            if ((int)(sum - 2 * s) <= (int)(2 * (s + freq[i + 1]) - sum)) break;
        }

        // Режим упаковки: дерево не строится, коды каждого символа храняться в векторах
        if (currentNode == nullptr)
        {
            for (int j = left; j <= i; j++)
                codes[j].push_back(true);

            for (int j = i + 1; j <= right; j++)
                codes[j].push_back(false);

            recursiveDivision(left, i, s);
            recursiveDivision(i + 1, right, sum - s);
        }
        // Режим распаковки: строится кодовое дерево, по которому будут восстанвливаться символы
        else    
        {
            Node* zeroChild, *oneChild;

            if (left == i)      // добрались до листа
            {
                int j = 0;
                while (matr[j] != left) j++;      // поиск первоначального индекса (кода символа)

                oneChild = new Node(j);
            }
            else
            {
                oneChild = new Node();
                recursiveDivision(left, i, s, oneChild);
            }

            if (right == i + 1)     // добрались до листа
            {
                int j = 0;
                while (matr[j] != right) j++;     // поиск первоначального индекса (кода символа)

                zeroChild = new Node(j);
            }
            else   
            {
                zeroChild = new Node();
                recursiveDivision(i + 1, right, sum - s, zeroChild);
            }

            currentNode->addChildren(zeroChild, oneChild);
        }
    }

    /// Сортировки исходного массива freq и определение
    /// матрицы перехода между исходным массивом и отсортированным
    /// \return Индекс, с которого начинаются символы, частота которых = 0
    int sort()
    {
        // Инициализация матрицы перехода
        matr = new int[256];
        for (int i = 0; i < 256; i++)
            matr[i] = -1;       // флаг -1 значит, что символов с индексом i не было в последовательности

        unsigned int* mas = new unsigned int[256];  // отсортированный массив

        int maxInd, lastInd = n - 1;
        unsigned int max;

        // Занесение сортированных данных в новый массив
        for (int i = 0; i < 256; i++)
        {
            max = 0;
            for (int j = 0; j < 256; j++)
            {
                if (max < freq[j])
                {
                    max = freq[j];
                    maxInd = j;
                }
            }

            if (max == 0)
            {
                for (int k = i; k < 256; k++)
                    mas[k] = 0;

                lastInd = i - 1;
                break;
            }

            freq[maxInd] = 0;
            mas[i] = max;
            matr[maxInd] = i;
        }

        delete[] freq;
        freq = mas;

        return lastInd;
    }

private:
    /// Класс, представляющий собой узел дерева
    class Node
    {
    public:
        Node()
        {
            value = 0;
        }

        Node(unsigned char value)
        {
            this->value = value;
        }

        ~Node()
        {
            delete zeroChild;
            delete oneChild;
        }

        bool isLeaf()
        {
            if (zeroChild == nullptr) 
                return true;
            else 
                return false;
        }

        void addChildren(Node* zero, Node* one)
        {
            zeroChild = zero;
            oneChild = one;
        }

        Node* zeroChild;
        Node* oneChild;
        unsigned char value;    // код символа
    };

private:
    int n = 256;
    double compression;

    Node* rootNode;         // для распаковки мы строим дерево с кодами
    list<bool>* codes;      // для упаковки мы создаем массив list-ов для хранения кодов, каждое значение по индексу соответствует символу в массиве частот 
                            // Каждый list - это последовательноть бит 

    int* matr;              // матрица перехода между изначальными частотами и отсортированными
    unsigned int sum;       // количество символов в файле
    unsigned int* freq;     // массив частот (количество каждого символа)
};