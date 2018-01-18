/*
 * Logger.h
 *
 * Header Header
 *
 * Copyright (C) 2014-2015  Yaochuang Ding - <ych_ding@163.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to 
 * deal in the Software without restriction, including without limitation the 
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 *    this list of conditions and the following disclaimer in the documentation 
 *    and/or other materials provided with the distribution, and in the same 
 *    place and form as other copyright, license and disclaimer information.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 *
 */
#ifndef LOGGER_H_
#define LOGGER_H_

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <ctime>

// Log levels
enum logger_level 
{
  LOG_VERBOSE = 0,
  LOG_DEBUG,
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR,
  LOG_CRITICAL
} ;

class InternalStreamBuf: public std::stringbuf
{
    private:
        std::ostream&  mScreenStream;
        std::ostream&  mFileStream;
        logger_level   mLevel;
        logger_level   mLineLevel;
    public:
        InternalStreamBuf(std::ostream& screenStream, std::ostream& fileStream) 
                    : mScreenStream(screenStream)
                    , mFileStream(fileStream)
                    , mLevel(LOG_DEBUG)
                    , mLineLevel(LOG_DEBUG)
        { }

        /*  implement the virtual function to sync string buffer content */
        virtual int sync ( )
        {
            if (mLevel >= mLineLevel)
            {
                mFileStream << get_time() << " [ " << levelToStr(mLevel) << " ] " << str();
        	    mScreenStream << get_time() << " [ " << levelToStr(mLevel) << " ] " << str();
            }
            str(""); /* clear string buffer content */
    	    mFileStream.flush();
    	    mScreenStream.flush();
            return 0;
        }
        
        inline const char* levelToStr(const logger_level& level);
        inline std::string get_time() const;
        inline void setLevel(const logger_level& level);
        inline void setLineLevel(const logger_level& level);
};

inline void InternalStreamBuf::setLineLevel(const logger_level& level)
{
    mLineLevel = level;
}

inline void InternalStreamBuf::setLevel(const logger_level& level)
{
    mLevel = level;
}

inline std::string InternalStreamBuf::get_time() const
{
  struct tm *timeinfo;
  time_t rawtime;
  char *time_buf;
  
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  time_buf = asctime(timeinfo);
  
  std::string ret(time_buf);
  if (!ret.empty() && ret[ret.length() - 1] == '\n') 
  {
    ret.erase(ret.length()-1);
  }
  
  return (ret);
}
inline const char* InternalStreamBuf::levelToStr(const logger_level& level)
{
  switch (level) 
  {
  case LOG_VERBOSE:
    return ("VRB");
  case LOG_DEBUG:
    return ("DBG");
  case LOG_INFO:
    return ("INF");
  case LOG_WARNING:
    return ("WRN");
  case LOG_ERROR:
    return ("ERR");
  case LOG_CRITICAL:
    return ("CRT");
  default:
    assert(false);
  }
} 
class Logger: public std::ostream
{ 
    private:
        std::ofstream       __mLogFile;
        InternalStreamBuf   __mBuffer;
        
public:
    static Logger& GetLogger();

    public:
        Logger(const char* filename) : std::ostream(&__mBuffer)
                                     , __mLogFile(filename)
                                     , __mBuffer(std::cout, __mLogFile)
        { /* do nothing */ }

        virtual ~Logger() { __mLogFile.close(); }
	    inline void setLineLevel(const logger_level& level)
        {
            __mBuffer.setLineLevel(level);
        }
        inline void setLevel(const logger_level& level)
        {
            __mBuffer.setLevel(level);
        }
};

extern Logger logger;

#define ENTER_FUNCTION logger << ">>>>" << __FUNCTION__ << std::endl
#define EXIT_FUNCTION  logger << "<<<<" << __FUNCTION__ << std::endl

#endif /* MYLOG_H_ */
