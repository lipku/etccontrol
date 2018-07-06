/* 
 * File:   BufferedAsyncSerial.cpp
 * Author: Terraneo Federico
 * Distributed under the Boost Software License, Version 1.0.
 * Created on January 6, 2011, 3:31 PM
 */

#include "BufferedAsyncSerial.h"

#include <string>
#include <algorithm>
#include <iostream>
#include <boost/bind.hpp>

using namespace std;
using namespace boost;

//
//Class BufferedAsyncSerial
//

BufferedAsyncSerial::BufferedAsyncSerial(): AsyncSerial()
{
    setReadCallback(boost::bind(&BufferedAsyncSerial::readCallback, this, _1, _2));
}

BufferedAsyncSerial::BufferedAsyncSerial(const std::string& devname,
        unsigned int baud_rate,
        asio::serial_port_base::parity opt_parity,
        asio::serial_port_base::character_size opt_csize,
        asio::serial_port_base::flow_control opt_flow,
        asio::serial_port_base::stop_bits opt_stop)
        :AsyncSerial(devname,baud_rate,opt_parity,opt_csize,opt_flow,opt_stop)
{
    setReadCallback(boost::bind(&BufferedAsyncSerial::readCallback, this, _1, _2));
}

size_t BufferedAsyncSerial::read(char *data, size_t size)
{
    lock_guard<mutex> l(readQueueMutex);
    size_t result=min(size,readQueue.size());
    vector<unsigned char>::iterator it=readQueue.begin()+result;
    copy(readQueue.begin(),it,data);
    readQueue.erase(readQueue.begin(),it);
    return result;
}

std::vector<unsigned char> BufferedAsyncSerial::read()
{
    lock_guard<mutex> l(readQueueMutex);
    vector<unsigned char> result;
    result.swap(readQueue);
    return result;
}

std::string BufferedAsyncSerial::readString()
{
    lock_guard<mutex> l(readQueueMutex);
    string result(readQueue.begin(),readQueue.end());
    readQueue.clear();
    return result;
}

bool BufferedAsyncSerial::GetOneMsg(std::vector<unsigned char>& result)  
{
    bool ok  = false;
    lock_guard<mutex> l(readQueueMutex);
    vector<unsigned char>::iterator it1;
    
    for(it1=readQueue.begin();it1!=readQueue.end();it1++)
    {
         printf("%02X ",*it1);
    }
    printf("\n");

    vector<unsigned char> delim;
    delim.push_back(0xff);
    delim.push_back(0xff);
    
    
    vector<unsigned char>::iterator iBegin=findBytesInVector(readQueue,0,delim);  //protocol 4.2  分割buffer， 0xffff消息头
    
    if(iBegin==readQueue.end() || readQueue.end()-iBegin <= 2) return false;
    delim.clear();
    delim.push_back(0xff);
    vector<unsigned char>::iterator iEnd=findBytesInVector(readQueue,iBegin-readQueue.begin()+2,delim);
    
    if(iEnd==readQueue.end()) return false;
    vector<unsigned char>::iterator it;
    
    //(*ok) = true;
    result.assign(iBegin+2,iEnd);
    readQueue.erase(readQueue.begin(),iEnd+1);

    vector<unsigned char> origin,replaced;  //protocol 4.3 特殊字节转义处理
    origin.push_back(0xfe);
    origin.push_back(0x01);
    replaced.push_back(0xff);
    replaceBytesInVector(result,origin,replaced);

    origin.clear();
    replaced.clear();
    origin.push_back(0xfe);
    origin.push_back(0x00);
    replaced.push_back(0xfe);
    replaceBytesInVector(result,origin,replaced);

    unsigned char bcc = result[0];  //异或校验
    for (int i=1;i<result.size()-1;i++){
       bcc = bcc ^ result[i];
    }
    unsigned char bcc1 = result[result.size()-1];
    if (bcc==bcc1)
      ok = true;
    else
    {
     printf("bcc!=bcc1\n");
    }

    printf("333333333\n");
    
    return ok;
}

std::string BufferedAsyncSerial::readStringUntil(const std::string delim)
{
    lock_guard<mutex> l(readQueueMutex);
    vector<unsigned char>::iterator it=findStringInVector(readQueue,delim);
    if(it==readQueue.end()) return "";
    string result(readQueue.begin(),it);
    it+=delim.size();//Do remove the delimiter from the queue
    readQueue.erase(readQueue.begin(),it);
    return result;
}

void BufferedAsyncSerial::readCallback(const char *data, size_t len)
{
    lock_guard<mutex> l(readQueueMutex);
    readQueue.insert(readQueue.end(),data,data+len);
}

std::vector<unsigned char>::iterator BufferedAsyncSerial::findBytesInVector(
        std::vector<unsigned char>& v, int find_offset, const std::vector<unsigned char>& s)
{
    if(s.size()==0) return v.end();

    vector<unsigned char>::iterator it=v.begin()+find_offset;
    
    for(;;)
    {
        vector<unsigned char>::iterator result=find(it,v.end(),s[0]);
        if(result==v.end()) return v.end();//If not found return

        for(size_t i=0;i<s.size();i++)
        {
            vector<unsigned char>::iterator temp=result+i;
            if(temp==v.end()) return v.end();
            if(s[i]!=*temp) goto mismatch;
        }
        //Found
        return result;

        mismatch:
        it=result+1;
    }
}

std::vector<unsigned char>::iterator BufferedAsyncSerial::replaceBytesInVector(
        std::vector<unsigned char>& v, const std::vector<unsigned char>& s, const std::vector<unsigned char>& r)
{
    if(s.size()==0) return v.end();

    vector<unsigned char>::iterator it=v.begin();
    for(;;)
    {
        vector<unsigned char>::iterator result=find(it,v.end(),s[0]);
        if(result==v.end()) return v.end();//If not found return

        for(size_t i=0;i<s.size();i++)
        {
            vector<unsigned char>::iterator temp=result+i;
            if(temp==v.end()) return v.end();
            if(s[i]!=*temp) goto mismatch;
        }
        //Found
        if(s.size()>r.size())
        {
            *result=r[0];
            vector<unsigned char>::iterator temp=result+1;
            v.erase(temp);
        }

        mismatch:
        it=result+1;
    }
}


std::vector<unsigned char>::iterator BufferedAsyncSerial::findStringInVector(
        std::vector<unsigned char>& v,const std::string& s)
{
    if(s.size()==0) return v.end();

    vector<unsigned char>::iterator it=v.begin();
    for(;;)
    {
        vector<unsigned char>::iterator result=find(it,v.end(),s[0]);
        if(result==v.end()) return v.end();//If not found return

        for(size_t i=0;i<s.size();i++)
        {
            vector<unsigned char>::iterator temp=result+i;
            if(temp==v.end()) return v.end();
            if(s[i]!=*temp) goto mismatch;
        }
        //Found
        return result;

        mismatch:
        it=result+1;
    }
}

BufferedAsyncSerial::~BufferedAsyncSerial()
{
    clearReadCallback();
}
