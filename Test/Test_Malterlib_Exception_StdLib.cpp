// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#ifndef DMalterlib

#include <ios>
#include <iostream>
#include <exception>

void fg_TestException4(int *_pValue)
{
	throw *_pValue;
}

void fg_TestException3(int *_pValue)
{
	fg_TestException4(_pValue);
}

struct CTestDestory
{
	~CTestDestory()
	{
		std::cout << "Destructor called\n";
	}
};

void fg_TestException2(int *_pValue)
{
	CTestDestory ToDestroy;
	fg_TestException3(_pValue);
}

void fg_TestException()
{
	int Value = 5;
	try
	{
		fg_TestException2(&Value);
	}
	catch (int const& _Exception)
	{
		std::cout << "Caught " << _Exception << "\n";
	}
}


int
#ifdef _MSC_VER
__cdecl 
#endif
main()
{
	
	std::cout << "Hello!\n";
	
	fg_TestException();
	
	return 0;
}

int 
#ifdef _MSC_VER
__cdecl 
#endif
wmain()
{
	
	std::cout << "Hello!\n";
	
	fg_TestException();
	
	return 0;
}

#endif
