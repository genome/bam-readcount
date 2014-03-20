#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include "bamrc/auxfields.hpp"
#include "bamrc/ReadWarnings.hpp"
#include <boost/program_options.hpp>
#include "bamrc/BasicStat.hpp"

#include <stdio.h>
#include <memory>
#include <string.h>
#include "sam.h"
#include "faidx.h"
#include "khash.h"
#include "sam_header.h"
#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cmath>
#include <map>
#include <set>
#include <queue>


using namespace std;
namespace po = boost::program_options;

//Start of header file
class counter
{
	public:
		int count(int argc, char *argv[]);
		
	private:
};




