#include "xml.h"

bool BaseXML::writeXML(String^ filePath)
{
	bool result = true;
	TextWriter^ writer = gcnew StreamWriter(filePath);
	try
	{
		XmlSerializer^ serializer = gcnew XmlSerializer(GetType());
		serializer->Serialize(writer, this);
		result = true;
	}
	catch(Exception^ e)
	{
		Console::WriteLine(e->Message);
		result = false;
	}
	finally
	{
		writer->Close();
	}

	return result;
}

bool BaseXML::readXML(String^ filePath)
{
	bool result = true;
	Type^ tmp;
	XmlSerializer^ serializer = gcnew XmlSerializer(tmp);

	FileStream^ fs = gcnew FileStream(filePath, FileMode::Open);
	try
	{
		tmp = (Type^)serializer->Deserialize(fs);
	}
	catch (Exception^ e)
	{
		Console::WriteLine(e->Message);
		result = false;

		// If can not read config from file, let write the default config
        fs->Close();
	}
	finally
	{
		fs->Close();
	}
	return result;
}