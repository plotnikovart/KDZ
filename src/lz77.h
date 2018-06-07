#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "IEncoder.h"
#include "BitWriterReader.h"

using namespace std;

typedef unsigned short int usint;
typedef unsigned int uint;

class LZ77 : public IEncoder
{
private:
    /// Кольцевой буфер для хранения символов из истории и просмотра
    class Buffer
    {
    public:
        Buffer(uint histBufMax, uint prevBufMax)
        {
            buff.resize(histBufMax + prevBufMax);
            size = histBufMax + prevBufMax;
            sum = 0;
            end = -1;

        }

        ~Buffer()
        {
            buff.clear();
        }

        void addChar(char ch)
        {
            sum++;
            end = (end + 1) % size;
            buff[end] = ch;         
        }

        char getChar(uint i)
        {
            return buff[i % size];
        }

        uint sum;
        usint size;
        int beg, end;
        vector<char> buff;
    };

public:
    void pack(ifstream& file, string directory, string fileName)
    {
        vector<Node*> res;       // вектор с тройками 
        encodeLZ77(file, res);

        BitWriter bw(directory + "pack/" + fileName + ".lz77" + to_string(prevBufMax / 1024));

        // Запись количества троек в файл
        bw << (uint)res.size();

        // Запись троек
        for (int i = 0; i < res.size(); i++)
        {
            bw << res[i]->offs;
            bw << res[i]->len;
            bw << res[i]->ch;
        }

		// Определение коэффицента сжатия
		file.clear();
		file.seekg(0, file.end);
		compression = file.tellg() / (double)bw.getFileSize();
		file.seekg(0, file.beg);
        
        // Очистка памяти
        bw.close();
        for (auto i : res)
            delete i;
        res.clear();
    }

    void unpack(string directory, string fileName)
    {
        // Считывание входных данных
        BitReader br(directory + "pack/" + fileName + ".lz77" + to_string(prevBufMax / 1024));

        uint lenght;
        br >> lenght;

        Node* res = new Node[lenght];
        for (int i = 0; i < lenght; i++)
        {
            br >> res[i].offs;
            br >> res[i].len;
            br >> res[i].ch;
        }

        // Готовим файл для записи раскодированного сообщения
        ofstream encodeFile;
        encodeFile.open(directory + "unpack/" + fileName + ".unlz77" + to_string(prevBufMax / 1024), ios::binary);

        decodeLZ77(res, lenght, encodeFile);

        br.close();
        encodeFile.close();
        delete[] res;
    }

    double getCompression()
    {
        return compression;
    }

    string getName()
    {
        return "LZ77(" + to_string(histBufMax / 1024) + ", " + to_string(prevBufMax / 1024) + ")";
    }

    /// \param histBufMax Максимальный размер буфера предыстории (словаря) в килобайтах
    /// \param prefBufMax Максимальный размер буфера предпросмотра (скользящего окна) в килобайтах
    LZ77(int histBufMax, int prevBufMax)
    {
        this->histBufMax = histBufMax * 1024;
        this->prevBufMax = prevBufMax * 1024;
    }

private:
    LZ77();     // констуруктор по умолчанию запрещен
    class Node;

    /// Кодирует строку по LZ77 алгоритму
    /// \param s Исходная файл
    /// \param res Вектор троек (offs, len, ch)
    void encodeLZ77(ifstream& s, vector<Node*>& res)
    {
        // Создание кольцевого буфера
        Buffer charBuff(histBufMax, prevBufMax);

        // Получение длины файла
		s.clear();
        s.seekg(0, s.end);
        int length = s.tellg();
        s.seekg(0, s.beg);

        char ch;

        // Заполнение буфера
        int max = length < histBufMax + prevBufMax ? length : histBufMax + prevBufMax;
        for (int i = 0; i < max; i++)
        {
            s.get(ch);
            charBuff.addChar(ch);
        }

        for (int i = 0; i < length; i++)
        {
            Node* newNode = findSubString(charBuff, i, i < histBufMax ? i : histBufMax, length - i < prevBufMax ? length : prevBufMax);
            res.push_back(newNode);
            
            if (i >= histBufMax + prevBufMax)
            {
                for (int j = 0; j <= newNode->len; j++)
                {
                    s.get(ch);
                    charBuff.addChar(ch);
                }
            }

            i += newNode->len;
        }
    }

    /// Ищет подстроку максимальной длины в буферах предыстории и предпросмотра
    /// \param str вся строка
    /// \param curPos текущая позиция в строке
    /// \param curHS текущая длина  буфера истории
    /// \param curPS текущая длина буфера просмотра
    /// \return узел с первой позицией вхождения подстроки, ее длиной и символом после
    Node* findSubString(Buffer& str, uint curPos, usint sizeHB, usint sizePB)
    {
        uint* br = new uint[sizePB + sizeHB + 1];
        for (int i = 0; i < sizePB + 1; i++)
            br[i] = 0;

        int max = 0;
        int meet = 0;

        // Заполнение массива граней, i - первоначальная длина буфера просмотра + 0 символов буфера истории; +1; +2 ...
        // То есть количество итераций будет равно длине буфера истории
        for (int i = sizePB - 1; i < sizePB + sizeHB - 1; i++)
        {
            setBoorderSize(str, br, i, i + 1, curPos, sizeHB, sizePB);

            if (br[i + 1] > max)
            {
                max = br[i + 1];
                meet = sizeHB + sizePB + max - (i + 2);
            }
        }

        delete[] br;
        return new Node(meet, max, str.getChar(curPos + max));
    }

    /// Устанавливает значения граней для строки prew|hist
    /// \param br массив со значениями длин граней
    /// \param currentInd длина текущей подстроки
    /// \param lastInd индекс последнего символа
    void setBoorderSize(Buffer& str, uint* br, uint currentInd, uint lastInd, uint curPos, usint sizeHB, usint sizePB)
    {
        if (str.getChar(getIndex(lastInd, curPos, sizeHB, sizePB)) == str.getChar(getIndex(br[currentInd], curPos, sizeHB, sizePB)))
            br[lastInd] = br[currentInd] + 1;
        else
        {
            if (br[currentInd] == 0)    // в подстроке уже нет граней
                br[lastInd] = 0;
            else
                setBoorderSize(str, br, br[currentInd - 1], lastInd, curPos, sizeHB, sizePB);
        }
    }

    /// Получает верный индекс в строке. Рассчитывает смещение
    /// \param ind индекс в измененной строке: prew|hist
    int getIndex(uint ind, uint curPos, usint sizeHB, usint sizePB)
    {
        int currentInd;

        if (ind < sizePB) currentInd = ind + curPos;
        else currentInd = ind + curPos - (sizeHB + sizePB);

        return currentInd;
    }

    /// Декодер
    void decodeLZ77(Node* arr, uint n, ofstream& res)
    {
        // Создание кольцевого буфера
        Buffer charBuf(histBufMax, prevBufMax);

        for (int i = 0; i < n; i++)
        {
            int end = charBuf.sum - arr[i].offs + arr[i].len;
            for (int j = charBuf.sum - arr[i].offs; j < end; j++)
            {
                charBuf.addChar(charBuf.getChar(j));
                res<<charBuf.getChar(j);
            }

            charBuf.addChar(arr[i].ch);
            res << arr[i].ch;
        }
    }

private:
    usint histBufMax, prevBufMax;
	double compression;

    /// Вспомогательный класс, представляет из себя узел  
    class Node
    {
    public:
        usint offs;
        usint len;
        char ch;

        Node(usint o, usint l, char c) : offs(o), len(l), ch(c)
        {};

        Node()
        {};
    };
};