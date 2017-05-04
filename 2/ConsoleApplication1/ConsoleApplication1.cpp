// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <exception>
using namespace std;

class Cop {
public:
	class ReadException : public exception {};
	Cop(uint32_t len): len(len){}
	virtual void decrypt_char(string &buf, size_t &pos) const = 0;
	void decrypt(string &buf, size_t &pos, int &dir) const {
		for (uint32_t i = 0; i < len; ++i) {
			decrypt_char(buf, pos);
			if (pos == 0 && dir == -1) {
				dir = 1;
			}
			else if (pos == buf.length() - 1 && dir == 1) {
				dir = -1;
			}
			else {
				pos += dir;
			}
			pos = pos % buf.length();
			
		}
	}
	uint32_t len;
};
class CopAdd : public Cop {
public:
	CopAdd(uint8_t prm, uint32_t len) :Cop(len), prm(prm){}
	void decrypt_char(string &buf, size_t &pos) const {
		buf[pos] = (uint8_t)buf[pos] + prm;
	}
	uint8_t prm;
};

class CopSub : public Cop {
public:
	CopSub(uint8_t prm, uint32_t len) :Cop(len), prm(prm) {}
	void decrypt_char(string &buf, size_t &pos) const {
		buf[pos] = (uint8_t)buf[pos] - prm;
	}
	
	uint8_t prm;
};

class CopXor : public Cop {
public:
	CopXor(uint8_t prm, uint32_t len) :Cop(len), prm(prm) {}
	void decrypt_char(string &buf, size_t &pos) const {
		buf[pos] = (uint8_t)buf[pos] ^ prm;
	}
	
	uint8_t prm;
};

Cop* CopFactory(istream& istr) throw(Cop::ReadException){
	uint8_t op;
	uint8_t prm;
	uint32_t len;
	if (!istr.good())  throw Cop::ReadException();
	istr >> op;
	if (!istr.good())  throw Cop::ReadException();
	istr >> prm;
	if (!istr.good())  throw Cop::ReadException();
	istr.read((char*)&len, sizeof(len));
	if (istr.bad()) throw Cop::ReadException(); // eof is ok
	switch (op) {
	case 0: return new CopXor(prm, len);
	case 1: return new CopAdd(prm, len);
	case 2: return new CopSub(prm, len);
	default:throw Cop::ReadException();
	}
	return nullptr;
}

class Ckey {
public:
	Ckey(istream& istr) {
		try {
			while (istr.good())
				operators.push_back(CopFactory(istr));
		}
		catch (Cop::ReadException &e) { (void)e; }
	}
	
	void decrypt(string &buf) const {
		size_t pos = 0;
		int dir = 1;
		for (auto it = operators.begin(); it != operators.end(); ++it)
			(*it)->decrypt(buf, pos, dir);
	}
	virtual ~Ckey(){
		for (auto it = operators.begin(); it != operators.end(); ++it)
			delete *it;
	}
	vector<Cop*> operators;
	
};

class Cmessage {
public:
	friend ostream& operator<<(ostream& ostr, const Cmessage& msg);
	Cmessage(istream& istr){
		// read the whole file into string
		istr.seekg(0, ios::end);
		buf.reserve((unsigned int)istr.tellg());
		istr.seekg(0, ios::beg);
		buf.assign(istreambuf_iterator<char>(istr), istreambuf_iterator<char>());
	}
	void decrypt(const Ckey &k) { // inplace
		k.decrypt(buf);
	}
	
	string buf;
};
ostream& operator<<(ostream& ostr, const Cmessage& msg) {
	return (ostr << msg.buf);
}


int main(int argc, char* argv[])
{
	if (argc < 3)exit(0);
	Ckey k(ifstream(argv[1], ifstream::binary));
	Cmessage msg(ifstream(argv[2], ios::binary));
	msg.decrypt(k);
	cout << msg << endl;
    return 0;
}

