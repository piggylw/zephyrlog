#pragma once
#include <iostream>
#include <fstream>
#include <memory>
#include <sys/stat.h>
#include "zephyrlog/logline.h"

namespace zephyrlog
{

class FileWriter
{
public:
    FileWriter(const std::string& logDirectory,
        const std::string& logFileName,
        uint32_t rollSize = 10 * 1024 * 1024,
        bool terminalOutput = false)
        : m_fileNumber(0),
          m_bytesWritten(0),
          m_rollSize(rollSize),
          m_name(logDirectory + logFileName),
          m_terminalOutput(terminalOutput)
    {
        mkdir(logDirectory.c_str(), 0755);
        rollFile();
    }

    ~FileWriter()
    {
        if (m_os && m_os->is_open())
        {
            m_os->flush();
            m_os->close();
        }
    }

    void write(LogLine& logline)
    {
        if (!m_os || !m_os->is_open())
            return;

        if (m_terminalOutput)
            logline.stringify(std::cout);

        auto pos = m_os->tellp();
        logline.stringify(*m_os);
        auto written = m_os->tellp() - pos;
        if (written > 0)
            m_bytesWritten += written;
        if (m_bytesWritten > m_rollSize)
            rollFile();
    }

private:
    void rollFile()
    {
        if (m_os && m_os->is_open())
        {
            m_os->flush();
            m_os->close();
        }
        m_bytesWritten = 0;
        m_os = std::make_unique<std::ofstream>();
        std::string logFileName = m_name;
        logFileName.append(".");
        logFileName.append(std::to_string(++m_fileNumber));
        logFileName.append(".txt");
        m_os->open(logFileName, std::ofstream::out | std::ofstream::trunc);
    }

private:
    uint32_t m_fileNumber;
    std::streamoff m_bytesWritten;
    uint32_t m_rollSize;
    std::string m_name;
    bool m_terminalOutput;
    std::unique_ptr<std::ofstream> m_os;
};

} // namespace zephyrlog
