#pragma once

#include <fstream>
#include <list>
#include <queue>
#include <string>

#include "IEncoder.h"
#include "BitWriterReader.h"
#include "frequancyEntropy.h"

using namespace std;
 
/// Алгоритм Хаффмана
class Huffman : public IEncoder
{
public:
    /// Кодирование файла по методу Хаффмана
    /// \param file Поток исходного файла
    /// \param directory Путь до папки, в которой лежит файл
    /// \param fileName Имя кодируемого файла
    void pack(ifstream& file, string directory, string fileName)
    {
        // Получение исходных данных: частоты
        unsigned int* freq;     // массив частот
        FrequancyEntropy::countFrequancy(file, freq);

        // Заполнение частотами
        sheets.reserve(256);
        for (int i = 0; i < 256; i++)
            addChance(i, freq[i]);

        // Запуск алгоритма
        build();

        // Упаковка данных в файл
        BitWriter bw(directory + "pack/" + fileName + ".haff");

        // Запись частот в файл
        for (int i = 0; i < 256; i++)
            bw << sheets[i]->freq;

        // Запись закодированного сообщения в битах в файл
        char ch;
        list<bool> code;

        file.clear();
        file.seekg(0);

        while (file.get(ch))
        {
            code = getSymbolCode((unsigned char)ch);

            for (bool bit : code)
                bw << bit;
        }

        // Определение коэффицента сжатия
        file.clear();
        file.seekg(0, file.end);
        compression = file.tellg() / (double)bw.getFileSize();

        // Освобождение ресурсов
        bw.close();
        sheets.clear();
        delete[] freq;
        delete topNode;
    }

    /// Декодирование закодированного файла по методу Хаффмана
    /// \param file Поток исходного файла
    /// \param directory Путь до папки, в которой лежит файл
    /// \param fileName Имя кодируемого файла
    void unpack(string directory, string fileName)
    {
        // Открытие упакованного файла
        BitReader br(directory + "pack/" + fileName + ".haff");

        // Считывание массива частот
        unsigned int fr;
        for (int i = 0; i < 256; i++)
        {
            br >> fr;
            addChance(i, fr);
        }

        // Восстановление дерева по массиву частот
        build();

        // Готовим файл для записи раскодированного сообщения
        ofstream encodeFile;
        encodeFile.open(directory + "unpack/" + fileName + ".unhaff", ios::binary);

        // Считывание битов и проход по дереву кодов
        bool bit;
        Node* currentNode = topNode;

        while (br >> bit)
        {
            currentNode = bit ? currentNode->oneChild : currentNode->zeroChild;

            if (currentNode->isLeaf())
            {
                encodeFile << (char)currentNode->ind; // добавление в файл символа
                currentNode = topNode;
            }
        }

        // Освобождение ресурсов
        br.close();
        encodeFile.close();
        sheets.clear();
        delete topNode;
    }

    /// Коэффицент сжатия для данного алгоритма
    /// \return Отношение объема исходных данных к закодированным (больше - лучше)
    double getCompression()
    {
        return compression;
    }

    string getName()
    {
        return "Haffman";
    }

private:
    /// Построение дерева 
    void build()
    {
        while (queue.size() != 1)
        {
            // Извлечение двух минимальных элементов кучи
            Node* node1 = queue.top();
            queue.pop();

            Node* node2 = queue.top();
            queue.pop();

            // Составление из них одного и добавление в кучу
            Node* newNode = *node1 + *node2;

            queue.push(newNode);
        }

        topNode = queue.top();      // получение верхнего элемента дерева
        queue.pop();
    }

    /// Добавление в коллекции нового символа
    /// \param ind Номер символы
    /// \param chance Частота появления символа
    void addChance(unsigned char ind, unsigned int freq)
    {
        sheets.push_back(new Node(ind, freq));
        if (freq != 0) 
            queue.push(sheets.back());
    }

    /// Получение кода символа по его индексу
    /// \param i Индекс элемента
    /// \return Код элемента  
    list<bool> getSymbolCode(unsigned char i)
    {
        Node* node = sheets[i];
        list<bool> code;

        // Проход по дереву вверх 
        while (node->father)
        {
            code.push_front(node->getCode());
            node = node->father;
        }

        return code;
    }

private:
    /// Класс, представлющий собой узел дерева
    class Node
    {
    public:
        Node(unsigned char ind, unsigned int value)
        {
            this->ind = ind;
            this->freq = value;
            zeroChild = nullptr;
            oneChild = nullptr;
        }

        Node(Node& zeroChild, Node& oneChild)
        {
            this->freq = zeroChild.freq + oneChild.freq;
            this->zeroChild = &zeroChild;
            this->oneChild = &oneChild;

            zeroChild.father = this;
            oneChild.father = this;
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

    public:
        friend Node* operator +(Node& n1, Node& n2)
        {
            Node* node;

            if (n1.freq < n2.freq)
                node = new Node(n2, n1);
            else node = new Node(n1, n2);

            return node;
        }

    public:
        /// Получение кода символа на данном уровне дерева
        bool getCode()
        {
            return this == this->father->zeroChild ? false : true;
        }

    public:
        unsigned char ind;     // номер символа
        unsigned int freq;     // частота встречаемости
        
        Node* father;
        Node* zeroChild;
        Node* oneChild;
    };

    /// Класс - Компаратор 
    class Compare
    {
    public:
        bool operator()(const Node* node1, const Node* node2) const
        {
            return node1->freq > node2->freq;
        }
    };

    double compression;
    Node* topNode;
    vector<Node*> sheets;                                   // хранилище всех узлов
    priority_queue<Node*, vector<Node*>, Compare> queue;    // очередь с приорететом (бинарная куча)
};