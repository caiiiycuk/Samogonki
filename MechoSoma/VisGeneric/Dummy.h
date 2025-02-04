#ifndef __DUUMY_H__
#define __DUUMY_H__

#include <cstring>
#include <vector>

#include "xmath.h"

#ifndef FOR_EACH
#define FOR_EACH(list, iterator) \
  for((iterator) = (list).begin(); (iterator) != (list).end(); ++(iterator))
#endif

using namespace std;

unsigned int CalcType(char* NameMesh);

struct Dummy : Vect3f
{
	unsigned int Type;
	char* name;

	Dummy() : Vect3f(0,0,0)	{ Type=0; name = 0; }
	Dummy(const Vect3f& v, char* n, int t);
	Dummy(const Dummy& d);
	Dummy& operator = (const Dummy& d);
	~Dummy();

	void Scale(float dSx,float dSy,float dSz)				{ x*=dSx; y*=dSy; z*=dSz; }
  	void Translate(float dx,float dy,float dz)				{ x+=dx; y+=dy; z+=dz; }
};

class DummyList : public vector<Dummy>
{
public:
	int Add(const Vect3f& v, char* n);
	void SortByName();
	void Print();

	void Scale(float dSx,float dSy,float dSz);
  	void Translate(float dx,float dy,float dz);

	Dummy& Find(unsigned int Type);
	Dummy& Find(char* name);
	
	Dummy* Search(unsigned int Type);
	Dummy* Search(char* name);
	
	iterator Iterator(char* name_mask);
		
	template <class InputIter>									   
	int Query(char* name_mask, InputIter result)
	{
		iterator i;
		int count = 0;
		FOR_EACH(*this, i)
			if(strstr(i -> name, name_mask)){
				*result = *i;
				++result;
				count++;
				}
		return count;
	}
};

#endif //__DUUMY_H__
