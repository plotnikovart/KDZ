// КДЗ по дисциплине Алгоритмы и структуры данных 2017-2018 уч.год
// Плотников Артем Денисович, группа БПИ-164, дата (27.03.2018)
// Visual Studio 2017
// Сделано: все кодировки, тестировщик, замеряющий время, bitreader и bitwriter
// Не сделано: LZW кодировка

#include <iostream>

#include "fileStreams.h"
#include "IEncoder.h"
#include "haffman.h"
#include "shennonFano.h"
#include "lz77.h"
#include "frequancyEntropy.h"

using namespace std;

LARGE_INTEGER fr;
unsigned int testTimePack(IEncoder* ob, ifstream& a, string b, string c);
unsigned int testTimeUnpack(IEncoder* ob, string b, string c);


int main()
{
    // Объекты для кодировок
    IEncoder* code[5] = { new ShannonFano(), new Huffman(), new LZ77(4, 5), new LZ77(8, 10), new LZ77(16, 20) };
    FrequancyEntropy frEn;

    // Подготовка файлов
    ifstream fInput;
    FileStreams results;

    string basicPath = "../resourses/";
    string fileName;
    QueryPerformanceFrequency(&fr);  // замер частоты процессора
    
    for (int i = 1; i <= 36; i++)
    {
        fileName = to_string(i / 10) + to_string(i % 10);
        fInput.open(basicPath + fileName, ios::binary);

        // Подсчет частот встречаемости символов и запись в файл
        frEn.count(fInput);
        results.writeFrequancyEntropy(frEn.getFrequancy(), frEn.getEntropy());
        cout << "File in process: " << fileName << endl << "Frequancy and Entropy are OK\n\n";

        for (int j = 0; j < 5; j++)
        {
            // Кодирование
            unsigned int time = testTimePack(code[j], fInput, basicPath, fileName);
            cout << '\t' << code[j]->getName() << ": coding is OK" << endl;

            results.writePackTime(time);
            results.writeCompression(code[j]->getCompression());
            
            // Декодирование
            time = testTimeUnpack(code[j], basicPath, fileName);
            cout << '\t' << code[j]->getName() << ": decoding is OK" << endl;

            results.writeUnpackTime(time);

            cout << endl;
        }

        results.endL();
        cout << endl;
        fInput.close();
    }
 
    return 0;
}


/// Тестирование среднего времени работы для кодирования в микросекунадх (10^-6)
unsigned int testTimePack(IEncoder* ob, ifstream& a, string b, string c)
{
    LARGE_INTEGER t, t1, t2;

    t = { 0 };

    for (int k = 0; k < 1; k++)
    {
        QueryPerformanceCounter(&t1);
            ob->pack(a, b, c);
        QueryPerformanceCounter(&t2);

        t.QuadPart += t2.QuadPart - t1.QuadPart;
    }

    t.QuadPart /= 1;

    return (unsigned int)((t.QuadPart * 1000000 / fr.QuadPart));
}

/// Тестирование среднего времени работы для декодирования в микросекунадх (10^-6)
unsigned int testTimeUnpack(IEncoder* ob, string b, string c)
{
    LARGE_INTEGER t, t1, t2;

    t = { 0 };

    for (int k = 0; k < 1; k++)
    {
        QueryPerformanceCounter(&t1);
        ob->unpack(b, c);
        QueryPerformanceCounter(&t2);

        t.QuadPart += t2.QuadPart - t1.QuadPart;
    }

    t.QuadPart /= 1;

    return (unsigned int)((t.QuadPart * 1000000 / fr.QuadPart));
}