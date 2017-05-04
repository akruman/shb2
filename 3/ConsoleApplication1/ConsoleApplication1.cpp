// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <exception>
#include <map>
#include <ctype.h>
#include <thread>
using namespace std;

class Cop {
public:
	class ReadException : public exception {};
	Cop(uint32_t len) : len(len) {}
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
	CopAdd(uint8_t prm, uint32_t len) :Cop(len), prm(prm) {}
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

Cop* CopFactory(istream& istr) throw(Cop::ReadException) {
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
Cop* CopFactory(uint8_t op, uint8_t prm, uint32_t len) throw(Cop::ReadException) {
	switch (op) {
	case 0: return new CopXor(prm, len);
	case 1: return new CopAdd(prm, len);
	case 2: return new CopSub(prm, len);
	default:throw Cop::ReadException();
	}
	return nullptr;
}

class Cseed {
public:
	Cseed():t(0) {}
	Cseed(uint64_t r) :t(r) {}
	Cseed& operator++() { ++t; return *this; }
	explicit operator vector<const Cop*>() const{
		vector<const Cop*> ret;

		uint64_t a; a = t & 0xffff;
		ret.push_back(CopFactory(((a>>12)&0xc0)%3,a&0xff,(a>>8)&0x3f));
		a = (t >> 16) & 0xffff;
		ret.push_back(CopFactory(((a >> 12) & 0xc0) % 3, a & 0xff, (a >> 8) & 0x3f));
		a = (t >> 32) & 0xffff;
		ret.push_back(CopFactory(((a >> 12) & 0xc0) % 3, a & 0xff, (a >> 8) & 0x3f));
		return ret;
	}
	uint64_t t;
};

class Ckey {
public:
	Ckey(istream& istr) {
		try {
			while (istr.good())
				operators.push_back(CopFactory(istr));
		}
		catch (Cop::ReadException &e) { (void)e; }
	}
	Ckey() {}
	Ckey(Cseed &seed) {
		operators = (vector<const Cop*>)seed;
	}

	void decrypt(string &buf) const {
		size_t pos = 0;
		int dir = 1;
		for (auto it = operators.begin(); it != operators.end(); ++it)
			(*it)->decrypt(buf, pos, dir);
	}
	virtual ~Ckey() {
		for (auto it = operators.begin(); it != operators.end(); ++it)
			delete *it;
	}
	vector<const Cop*> operators;

};



class Cmessage {
public:
	friend ostream& operator<<(ostream& ostr, const Cmessage& msg);
	Cmessage(istream& istr) {
		// read the whole file into string
		istr.seekg(0, ios::end);
		buf.reserve((unsigned int)istr.tellg());
		istr.seekg(0, ios::beg);
		buf.assign(istreambuf_iterator<char>(istr), istreambuf_iterator<char>());
	}
	Cmessage(const Cmessage &m) {
		buf = m.buf;
	}
	void decrypt(const Ckey &k) { // inplace
		k.decrypt(buf);
	}
	int rank() {
		int r=0;
		int consec = 0;
		int spaces = 0;
		for (size_t i = 0; i < buf.length(); ++i) {
			if (buf[i] == '.' ||
				buf[i] == '?' ||
				buf[i] == ' ' ||
				('a' <= buf[i] && buf[i] <= 'z') ||
				('A' <= buf[i] && buf[i] <= 'Z')) {
				r -= 10;
				if (consec) consec++;
			}
			else {
				if (consec)
					r -= consec * 30;
				consec = 0;
				r++;
			}
			spaces += buf[i] == ' ';
		}
		r += spaces * 50;
		return r;
	}

	string buf;
};
ostream& operator<<(ostream& ostr, const Cmessage& msg) {
	return (ostr << msg.buf);
}

#include <stdlib.h>
#include <time.h>
#include <sstream>

void f(Cmessage msg, string *ps, Cseed s) {
	int min_rank = msg.rank(); Cseed min_seed = 0;
	for (int i = 0; i < 100000; ++i, ++s) {
		Cmessage m = msg;
		m.decrypt(Ckey(s));
		if (m.rank() < min_rank) {
			min_rank = m.rank();
			min_seed = s;
		}
	}
	stringstream sstr;
	msg.decrypt(Ckey(min_seed));
	sstr << min_rank << ' ' << msg;
	*ps = sstr.str();
}

int main(int argc, char* argv[])
{
	if (argc < 2)exit(0);
	
	Cmessage msg(ifstream(argv[1], ios::binary));
	

	srand(time(NULL));

	cout << thread::hardware_concurrency() << endl;
	vector<thread> vt;

	vector<string*> vs;
	for (int i = 0; i < 8; i++) {
		string *ps = new string;
		vs.push_back(ps);
		vt.push_back(thread(f, msg, ps, Cseed((((uint64_t)rand()) << 16) + rand())));
	}
	for (int i = 0; i < 8; i++) {
		vt[i].join();
		cout << (*vs[i]) << endl;
	}
	cout << msg << endl;
	
	return 0;
}

