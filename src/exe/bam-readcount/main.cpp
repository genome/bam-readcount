#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include "bamreadcount.h"

using namespace std;

int main(int argc, char *argv[])
{
	counter * myCounter=new counter();
	myCounter->count(argc, argv);
	
	delete myCounter;		
}
