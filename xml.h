#pragma once
#include <string>
#include <typeinfo>
using namespace System::Xml::Serialization;
using namespace System::IO;
using namespace System::Text;
using namespace System;

public ref class BaseXML
{
public:
	virtual bool writeXML(String^ filePath){};
	virtual bool readXML(String^ filePath){};
};