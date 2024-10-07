#pragma once
#include "XmlWriteReadBase.h"

[Serializable]
public ref class TypicalValue :
    public XmlWriteReadBase
{
public:
	double SNR;
	double CNR;
	double CTF;

	TypicalValue();
	~TypicalValue();
};

