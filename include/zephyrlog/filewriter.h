#pragma once
#include <iostream>
#include <fstream>
#include <memory>
#include "zephyrlog/logline.h"

namespace zephyrlog
{
// ========== FileWriter：文件写入器 ==========
/*
* 负责将日志写入文件
*/

class FileWriter
{
public:
    FileWriter(const std::string& logDirectory, 
        const std::string& logFileName, 
        uint32_t rollSize = 10 * 1024 * 1024)
        : m_fileNumber(0),
          m_bytesWritten(0),
          m_rollSize(rollSize),
          m_name(logDirectory + logFileName)
    {
        rollFile();
    }

    ~FileWriter()
    {
        if (m_os)
        {
            m_os->flush();
            m_os->close();
        }
    }

    void write(LogLine& logline)
    {
        auto pos = m_os->tellp();
        logline.stringify(*m_os);
        m_bytesWritten += m_os->tellp() - pos;
        if(m_bytesWritten > m_rollSize)
        {
            rollFile();
        }
    }

private:
    void rollFile()
    {
        if (m_os)
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
    uint32_t m_fileNumber;                    // 当前文件编号
    std::streamoff m_bytesWritten;            // 当前文件已写入字节数
    uint32_t m_rollSize;                      // 滚动阈值（字节）
    std::string m_name;                        // 文件名前缀（含路径）
    std::unique_ptr<std::ofstream> m_os; 
};

}