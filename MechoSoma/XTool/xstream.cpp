#include "xtool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <iostream>

#include "filesystem.h"
#include "port.h"

static const char *openMSG = "CREATE/OPEN FAILURE";
static const char *sizeMSG = "FILE SIZE CALCULATION ERROR";

unsigned xsReadBytes = 0;
unsigned xsReadBytesDelta = 0;
unsigned xsWriteBytes = 0;
unsigned xsWriteBytesDelta = 0;

unsigned xsRdWrResolution = 500000U;

void (*xsReadHandler)(unsigned) = NULL;
void (*xsWriteHandler)(unsigned) = NULL;

void xsSetReadHandler(void (*fp)(unsigned),unsigned res)
{
	xsReadHandler = fp;
	xsRdWrResolution = res;
	xsReadBytes = 0;
}

void xsSetWriteHandler(void (*fp)(unsigned),unsigned res)
{
	xsWriteHandler = fp;
	xsRdWrResolution = res;
	xsWriteBytes = 0;
}

std::fstream *open_file(const char* name, unsigned f)
{
	std::ios::openmode mode;
	mode = std::ios::binary;
	if (f & XS_IN)
		mode |= std::ios::in;
	if (f & XS_OUT)
		mode |= std::ios::out;
	if (f & XS_APPEND)
		mode |= std::ios::app;

	return new std::fstream(name, mode);
}

XStream::XStream(int err)
{
	ErrHUsed = err;
	handler  = NULL;
	eofFlag  = 1;
	radix	 = XS_DEFRADIX;
	digits	 = XS_DEFDIGITS;
	extSize  = -1;
	extPos	 = 0;
}

XStream::XStream(const char* name, unsigned flags,int err)
{
	ErrHUsed = err;
	handler  = NULL; // (XSHANDLE)-1;
	eofFlag  = 1;
	radix	 = XS_DEFRADIX;
	digits	 = XS_DEFDIGITS;
	extSize  = -1;
	extPos	 = 0;
	open(name,flags);
}

XStream::~XStream()
{
	close();
}

int XStream::open(const char* name, unsigned f)
{
#ifdef XSTREAM_DEBUG
	std::cerr << "DBG: XStream::open(\"" << name << "\", 0x" << std::hex << f << ")" << std::endl;
#endif

	const auto file_path = file::normalize_path(name);

	std::fstream *file = open_file(file_path.c_str(), f);
	handler = file;
	if (file->is_open()) {
		fname = name;
		pos = file->tellg();
		eofFlag = 0;
	} else {
		#ifdef XSTREAM_DEBUG
			std::cerr << "ERR: XStream::open(\"" << name << "\", 0x" << std::hex << f << ")" << std::endl;
		#endif
		if(ErrHUsed)
			ErrH.Abort(openMSG,XERR_USER,0,"");
		else
			return 0;
	}
	return 1;
}

int XStream::open(XStream* owner,long s,long ext_sz)
{
	/* Full stream debug
	std::fstream debug("openfile.txt", std::ios::out|std::ios::app);
	if (debug.is_open())
		debug<<"OPEN_XSTREAM "<<owner -> fname<<std::endl;
	debug.close();
	*/
	fname = owner -> fname;
	handler = owner -> handler;
	pos = 0;
	owner -> seek(s,XS_BEG);
	eofFlag = owner -> eof();
	extSize = ext_sz;
	extPos = s;
	return 1;
}

void XStream::close(void)
{
	
	if(handler == NULL)
		return;
	//std::cout<<"XStream::close: "<<fname<<std::endl;
	/* Full stream debug
	std::fstream debug("openfile.txt", std::ios::out|std::ios::app);
	if (debug.is_open()&&fname!=NULL)
		debug<<"CLOSE "<<fname<<std::endl;
	debug.close();
	*/
	//if(extSize == -1 && !CloseHandle(handler) && ErrHUsed)
	//	ErrH.Abort(closeMSG,XERR_USER,GetLastError(),fname);

	if(extSize == -1){
		auto internal_handler = reinterpret_cast<std::fstream*>(handler);

		if (internal_handler->is_open())
			internal_handler->close();
		delete internal_handler;
	}

	handler = NULL;
	//fname = "";
	pos = 0L;
	eofFlag = 1;
	extSize = -1;
	extPos = 0;
}

unsigned long XStream::read(void* buf, unsigned long len)
{
	unsigned long ret;
	auto internal_handler = reinterpret_cast<std::fstream*>(handler);
	/*if(!ReadFile(handler,buf,len,&ret,0))
		if(ErrHUsed) ErrH.Abort(readMSG,XERR_USER,GetLastError(),fname);
		else return 0U;
	if(ret < len) eofFlag = 1;*/
	internal_handler->read((char *)buf, len);
	len = internal_handler->gcount();
	ret = len;
	pos += ret;
	if(extSize != -1 && pos >= extSize) eofFlag = 1;
	
	if(xsReadHandler){
		xsReadBytesDelta += ret;
		if(xsReadBytesDelta >= xsRdWrResolution){
			xsReadBytes += xsReadBytesDelta;
			xsReadBytesDelta = 0;
			(*xsReadHandler)(xsReadBytes);
		}
	}

	// Full stream debug
	/*std::fstream debug("openfile.txt", std::ios::out|std::ios::app);
	if (debug.is_open())
		debug<<"READ "<<fname<<" len:"<<len<<std::endl;
		//debug<<"READ "<<fname<<" len:"<<len<<" buf:"<<(char *)buf<<std::endl;
	debug.close();*/
	return ret;
}

unsigned long XStream::write(const void* buf, unsigned long len)
{
	/* Full stream debug
	std::fstream debug("openfile.txt", std::ios::out|std::ios::app);
	if (debug.is_open())
		debug<<"WRITE "<<fname<<" len:"<<len<<" buf:"<<(char*)buf<<std::endl;
	debug.close();
	*/
	unsigned long ret;
	auto internal_handler = reinterpret_cast<std::fstream*>(handler);
	/*std::cout<<"Start write to:"<<fname
			<<" len:"<<len
			<<" "<<((std::fstream *)handler)->fail()
			<<" "<<((std::fstream *)handler)->good()
			<<" "<<((std::fstream *)handler)->eof()
			<<" "<<((std::fstream *)handler)->bad()
			<<" open:"<<((std::fstream *)handler)->is_open()
			<<" tellp:"<<((std::fstream *)handler)->tellp()<<std::endl;*/
	internal_handler->write((char *)buf, len);
	/*
		std::cout<<"END write to:"<<fname
			<<" len:"<<len
			<<" "<<((std::fstream *)handler)->fail()
			<<" "<<((std::fstream *)handler)->good()
			<<" "<<((std::fstream *)handler)->eof()
			<<" "<<((std::fstream *)handler)->bad()
			<<" tellp:"<<((std::fstream *)handler)->tellp()<<std::endl;
		std::cout<<(char *)buf<<std::endl;*/
	
	ret = len;
	pos += ret;

	if(xsWriteHandler){
		xsWriteBytesDelta += ret;
		if(xsWriteBytesDelta >= xsRdWrResolution){
			xsWriteBytes += xsWriteBytesDelta;
			xsWriteBytesDelta = 0;
			(*xsWriteHandler)(xsWriteBytes);
		}
	}
	return ret;
}

long XStream::seek(long offset, int dir)
{
	/* Full stream debug*/
	/*std::fstream debug("openfile.txt", std::ios::out|std::ios::app);
	if (debug.is_open())
		debug<<"SEEK START "<<fname<<" tellg:"<<((std::fstream *)handler)->tellg()
		<<" addr:"<<handler<<" curo:"<<offset<<std::endl;
	debug.close();*/

	auto internal_handler = reinterpret_cast<std::fstream*>(handler);
	long ret=0;
	if(extSize != -1){
		switch(dir){
			case XS_BEG:
				//ret = SetFilePointer(handler,extPos + offset,0,dir) - extPos;
				if (internal_handler->flags() & std::ios::out) {
					internal_handler->seekp(extPos + offset, std::ios_base::beg);
				}
				else {
					internal_handler->seekg(extPos + offset, std::ios_base::beg);
				}
				break;
			case XS_END:
				//ret = SetFilePointer(handler,extPos + extSize - offset - 1,0,XS_BEG) - extPos;
				if (internal_handler->flags() & std::ios::out) {
					internal_handler->seekp(extPos + extSize - offset - 1, std::ios_base::beg);
				} else {
					internal_handler->seekg(extPos + extSize - offset - 1, std::ios_base::beg);
				}
				break;
			case XS_CUR:
				//ret = SetFilePointer(handler,extPos + pos + offset,0,XS_BEG) - extPos;
				//((std::fstream *)handler)->clear();
				if (internal_handler->flags() & std::ios::out) {
					internal_handler->seekp(extPos + pos + offset, std::ios_base::beg);
				} else {
					internal_handler->seekg(extPos + pos + offset, std::ios_base::beg);
				}
				break;
		}

		if (internal_handler->flags() & std::ios::out)
			ret = internal_handler->tellp() - (std::streamoff)extPos;
		else
			ret = internal_handler->tellg() - (std::streamoff)extPos;
	}
	else
	{
		//ret = SetFilePointer(handler,offset,0,dir);
		//std::cout<<"SEEK:"<<fname<<std::endl;
		switch(dir){
			case XS_BEG:
				if (internal_handler->flags() & std::ios::out)
					internal_handler->seekp(offset, std::ios_base::beg);
				else
					internal_handler->seekg(offset, std::ios_base::beg);
				break;
			case XS_END:
				if (internal_handler->flags() & std::ios::out)
					internal_handler->seekp(offset, std::ios_base::end);
				else
					internal_handler->seekg(offset, std::ios_base::end);
				break;
			case XS_CUR:
				if (internal_handler->flags() & std::ios::out)
					internal_handler->seekp(offset, std::ios_base::cur);
				else
					internal_handler->seekg(offset, std::ios_base::cur);
				break;
		}
		if (internal_handler->flags() & std::ios::out)
			ret = internal_handler->tellp();
		else
			ret = internal_handler->tellg();

	}
	/* Full stream debug*/
	/*debug.open("openfile.txt", std::ios::out|std::ios::app);
	if (debug.is_open())
		debug<<"SEEK "<<fname<<" offset:"<<offset<<" dir:"<<dir
		<<" extSize:"<<extSize<<" pos:"<<pos<<" extPos:"<<extPos
		<<" ret:"<<ret<<std::endl;
	debug.close();*/
	if (internal_handler->fail()) {
		std::cout<<"Warning: Bad seek in file."<<std::endl;
		}
	if (ret == -1L)
		{
		std::cout<<"Warning: XStream::seek applies when the end of the file reached. "<<fname<<std::endl;
		return -1L;
		}

	if (ret >= size() - 1) eofFlag = 1;  else eofFlag = 0;

	return pos = ret;
}

long XStream::size()
{
//std::cout<<"XStream::size()"<<std::endl;
	auto internal_handler = reinterpret_cast<std::fstream*>(handler);
	long tmp = extSize;
	long int tmp2;
	if(tmp == -1){
		//tmp=GetFileSize(handler,0);
		if (internal_handler->flags() & std::ios::in) {
			tmp2 = internal_handler->tellg();
			internal_handler->seekp(0, std::ios_base::end);
			tmp = internal_handler->tellg();
			internal_handler->seekp(tmp2);
		} else {
			tmp2 = internal_handler->tellp();
			internal_handler->seekg(0, std::ios_base::end);
			tmp = internal_handler->tellp();
			internal_handler->seekg(tmp2);
		}
		if (tmp == -1L) {
			if (ErrHUsed)
				ErrH.Abort(sizeMSG,XERR_USER,0,"");
			else
				return -1;
		}
	}
	/* Full stream debug
	std::fstream debug("openfile.txt", std::ios::out|std::ios::app);
	if (debug.is_open())
		debug<<"SIZE "<<fname<<" size:"<<tmp<<std::endl;
	debug.close();
	*/
	return tmp;
}

XStream& XStream::operator< (const char* v)
{
	write(v,(unsigned)strlen(v));
	return *this;
}

XStream& XStream::operator< (char v)
{
	write(&v,(unsigned)sizeof(char));
	return *this;
}

XStream& XStream::operator< (unsigned char v)
{
	write(&v,(unsigned)sizeof(char));
	return *this;
}

XStream& XStream::operator< (short v)
{
	write(&v,(unsigned)sizeof(int16_t));
	return *this;
}

XStream& XStream::operator< (unsigned short v)
{
	write(&v,(unsigned)sizeof(uint16_t));
	return *this;
}

XStream& XStream::operator< (int v)
{
	write(&v,(unsigned)sizeof(int));
	return *this;
}

XStream& XStream::operator< (unsigned v)
{
	write(&v,(unsigned)sizeof(unsigned));
	return *this;
}

XStream& XStream::operator< (long v)
{
	write(&v,(unsigned)sizeof(int32_t));
	return *this;
}

XStream& XStream::operator< (unsigned long v)
{
	write(&v,(unsigned)sizeof(uint32_t));
	return *this;
}

XStream& XStream::operator< (double v)
{
	write(&v,(unsigned)sizeof(double));
	return *this;
}

XStream& XStream::operator< (float v)
{
	write(&v,(unsigned)sizeof(float));
	return *this;
}

XStream& XStream::operator< (long double v)
{
	write(&v,(unsigned)sizeof(long double));
	return *this;
}

XStream& XStream::operator> (char* v)
{
	read(v,(unsigned)strlen(v));
	return *this;
}

XStream& XStream::operator> (char& v)
{
	read(&v,(unsigned)sizeof(char));
	return *this;
}

XStream& XStream::operator> (unsigned char& v)
{
	read(&v,(unsigned)sizeof(char));
	return *this;
}

XStream& XStream::operator> (short& v)
{
	read(&v,(unsigned)sizeof(int16_t));
	return *this;
}

XStream& XStream::operator> (unsigned short& v)
{
	read(&v,(unsigned)sizeof(uint16_t));
	return *this;
}

XStream& XStream::operator> (int& v)
{
	read(&v,(unsigned)sizeof(int));
	return *this;
}

XStream& XStream::operator> (unsigned& v)
{
	read(&v,(unsigned)sizeof(unsigned));
	return *this;
}

XStream& XStream::operator> (long& v)
{
        v = 0;
	read(&v,(unsigned)sizeof(int32_t));
	return *this;
}

XStream& XStream::operator> (unsigned long& v)
{
        v = 0;
	read(&v,(unsigned)sizeof(uint32_t));
	return *this;
}

XStream& XStream::operator> (double& v)
{
	read(&v,(unsigned)sizeof(double));
	return *this;
}

XStream& XStream::operator> (float& v)
{
	read(&v,(unsigned)sizeof(float));
	return *this;
}

XStream& XStream::operator> (long double& v)
{
	read(&v,(unsigned)sizeof(long double));
	return *this;
}

XStream& XStream::operator<= (char var)
{
	char* s = port_itoa(var,_ConvertBuffer,radix);
	write(s,strlen(s));
	return *this;
}

XStream& XStream::operator<= (unsigned char var)
{
	char* s = port_itoa(var,_ConvertBuffer,radix);
	write(s,strlen(s));
	return *this;
}

XStream& XStream::operator<= (short var)
{
	char* s = port_itoa(var,_ConvertBuffer,radix);
	write(s,strlen(s));
	return *this;
}

XStream& XStream::operator<= (unsigned short var)
{
	char* s = port_ltoa(var,_ConvertBuffer,radix);
	write(s,strlen(s));
	return *this;
}

XStream& XStream::operator<= (int var)
{
	char* s = port_itoa(var,_ConvertBuffer,radix);
	write(s,strlen(s));
	return *this;
}

XStream& XStream::operator<= (unsigned var)
{
	char* s = port_ultoa(var,_ConvertBuffer,radix);
	write(s,strlen(s));
	return *this;
}

XStream& XStream::operator<= (long var)
{
	char* s = port_ltoa(var,_ConvertBuffer,radix);
	write(s,strlen(s));
	return *this;
}

XStream& XStream::operator<= (unsigned long var)
{
	char* s = port_ultoa(var,_ConvertBuffer,radix);
	write(s,strlen(s));
	return *this;
}

XStream& XStream::operator<= (float var)
{
	int len = snprintf(_ConvertBuffer, _CONV_BUFFER_LEN, "%.*g", digits, var);
	write(_ConvertBuffer,len);
	return *this;
}

XStream& XStream::operator<= (double var)
{
	int len = snprintf(_ConvertBuffer, _CONV_BUFFER_LEN, "%.*g", digits, var);
	write(_ConvertBuffer,len);
	return *this;
}

XStream& XStream::operator<= (long double var)
{
	int len = snprintf(_ConvertBuffer, _CONV_BUFFER_LEN, "%.*Lg", digits, var);
	write(_ConvertBuffer,len);
	return *this;
}

XStream& XStream::operator>= (char& var)
{
	int ret = read(_ConvertBuffer,4);
	if(!ret) return *this;
	char* p = _ConvertBuffer;
	p[ret] = ' ';
	var = (char)strtol(p,&p,0);
	seek(p - _ConvertBuffer + 1 - ret,XS_CUR);
	return *this;
}

XStream& XStream::operator>= (unsigned char& var)
{
	int ret = read(_ConvertBuffer,4);
	if(!ret) return *this;
	char* p = _ConvertBuffer;
	p[ret] = ' ';
	var = (unsigned char)strtoul(p,&p,0);
	seek(p - _ConvertBuffer + 1 - ret,XS_CUR);
	return *this;
}

XStream& XStream::operator>= (short& var)
{
	int ret = read(_ConvertBuffer,6);
	if(!ret) return *this;
	char* p = _ConvertBuffer;
	p[ret] = ' ';
	var = (short)strtol(p,&p,0);
	seek(p - _ConvertBuffer + 1 - ret,XS_CUR);
	return *this;
}

XStream& XStream::operator>= (unsigned short& var)
{
	int ret = read(_ConvertBuffer,6);
	if(!ret) return *this;
	char* p = _ConvertBuffer;
	p[ret] = ' ';
	var = (unsigned short)strtoul(p,&p,0);
	seek(p - _ConvertBuffer + 1 - ret,XS_CUR);
	return *this;
}

XStream& XStream::operator>= (int& var)
{
	int ret = read(_ConvertBuffer,16);
	if(!ret) return *this;
	char* p = _ConvertBuffer;
	p[ret] = ' ';
	var = strtol(p,&p,0);
	//std::cout<<"seek - p:"<<(long long)p<<" _ConvertBuffer:"<<(long)_ConvertBuffer<<" ret:"<<var<<" result:"<<p - _ConvertBuffer + 1 - ret<<std::endl;
	seek(p - _ConvertBuffer + 1 - ret,XS_CUR);
	//std::cout<<"Pos:"<<ppos<<std::endl;
	return *this;
}

XStream& XStream::operator>= (unsigned& var)
{
	int ret = read(_ConvertBuffer,16);
	if(!ret) return *this;
	char* p = _ConvertBuffer;
	p[ret] = ' ';
	var = strtoul(p,&p,0);
	seek(p - _ConvertBuffer + 1 - ret,XS_CUR);
	return *this;
}

XStream& XStream::operator>= (long& var)
{
	int ret = read(_ConvertBuffer,16);
	if(!ret) return *this;
	char* p = _ConvertBuffer;
	p[ret] = ' ';
	var = strtol(p,&p,0);
	seek(p - _ConvertBuffer + 1 - ret,XS_CUR);
	return *this;
}

XStream& XStream::operator>= (unsigned long& var)
{
	int ret = read(_ConvertBuffer,16);
	if(!ret) return *this;
	char* p = _ConvertBuffer;
	p[ret] = ' ';
	var = strtoul(p,&p,0);
	seek(p - _ConvertBuffer + 1 - ret,XS_CUR);
	return *this;
}

XStream& XStream::operator>= (double& var)
{
	int ret = read(_ConvertBuffer,_CONV_BUFFER_LEN);
	if(!ret) return *this;
	char* p = _ConvertBuffer;
	p[ret] = ' ';
	var = strtod(p,&p);
	seek(p - _ConvertBuffer + 1 - ret,XS_CUR);
	return *this;
}

XStream& XStream::operator>= (float& var)
{
	int ret = read(_ConvertBuffer,_CONV_BUFFER_LEN);
	if(!ret) return *this;
	char* p = _ConvertBuffer;
	p[ret] = ' ';
	var = (float)strtod(p,&p);
	seek(p - _ConvertBuffer + 1 - ret,XS_CUR);
	return *this;
}

XStream& XStream::operator>= (long double& var)
{
	int ret = read(_ConvertBuffer,_CONV_BUFFER_LEN);
	if(!ret) return *this;
	char* p = _ConvertBuffer;
	p[ret] = ' ';
	var = strtod(p,&p);
	seek(p - _ConvertBuffer + 1 - ret,XS_CUR);
	return *this;
}

char* XStream::getline(char* buf, unsigned len)
{
	int i = -1;
	do {
		i++;
		read(&buf[i],1);
	   } while(buf[i] != '\n' && i < (int)len && !eof());
	if(eof())
		buf[i] = '\0';
	else
		buf[i - 1] = '\0';
	return buf;
}
