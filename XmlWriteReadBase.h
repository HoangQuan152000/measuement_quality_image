#pragma once
#include <string>
using namespace System;
using namespace System::Xml::Serialization;

// !!!!Caution: To use the save and load xml function, the subclass need to declared the tag [Serializable] before its definition.
[Serializable]
public ref class XmlWriteReadBase
{
public:
   virtual bool SaveToXml(System::String^ fileName);

   XmlWriteReadBase^ LoadFromXml(System::String^ fileName);
};

