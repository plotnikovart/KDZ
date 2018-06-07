#pragma once

#include <fstream>
#include <string>

using namespace std;

/// Класс, содержащий потоки файлов для записи результатов
class FileStreams
{
public:
    FileStreams()
    {
        fFrequancy.open("../results/frequancy.csv");
        fPackTime.open("../results/packTime.csv");
        fUnpackTime.open("../results/unpackTime.csv");
        fCompression.open("../results/compression.csv");

        string title = "Shennon-Fano;Haffman;LZ77(4, 5);LZ77(8, 10);LZ77(16,20);";
        fPackTime << title << endl;
        fUnpackTime << title << endl;
        fCompression << title << endl;
    }

    ~FileStreams()
    {
        fFrequancy.close();
        fPackTime.close();
        fUnpackTime.close();
        fCompression.close();
    }
    
    void writeFrequancyEntropy(double* freq, double entr)
    {
        if (!fFrequancy) return;

        for (int i = 0; i < 256; i++)
            fFrequancy << freq[i] << ";";

        fFrequancy << entr << endl;
    }

    void writePackTime(unsigned int time)
    {
        fPackTime << time << ";";
    }

    void writeUnpackTime(int time)
    {
        fUnpackTime << time << ";";
    }

    void writeCompression(double c)
    {
        fCompression << c << ";";
    }

    void endL()
    {
        fPackTime << endl;
        fUnpackTime << endl;
        fCompression << endl;
    }

private:
     ofstream fFrequancy, fPackTime, fUnpackTime, fCompression;
};