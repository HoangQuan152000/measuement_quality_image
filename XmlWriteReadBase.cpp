#include "XmlWriteReadBase.h"
#include <string>

using namespace System::IO;
using namespace System::Text;

#ifdef _DEBUG
using namespace System::Diagnostics;
#endif // _DEBUG

bool XmlWriteReadBase::SaveToXml(String^ fileName)
{

#ifdef _DEBUG
    Trace::WriteLine(GetType()->Name);
#endif // _DEBUG
    bool rtn = false;
    TextWriter^ writer = gcnew StreamWriter(fileName);
    try
    {
        XmlSerializer^ serializer = gcnew XmlSerializer(GetType());
        //serializer = gcnew XmlSerializer(GetType());
        serializer->Serialize(writer, this);

        rtn = true;
    }
    catch (Exception^ ex)
    {
#ifdef _DEBUG
        Trace::WriteLine(ex->ToString());
#endif // _DEBUG
        rtn = false;
    }
    finally
    {
        writer->Close();
    }

    return rtn;
}

XmlWriteReadBase^ XmlWriteReadBase::LoadFromXml(String^ fileName)
{
    //Console.WriteLine(GetType().Name);
    XmlWriteReadBase^ tmp;
    // Load the data from file
    // Create an instance of the XmlSerializer class;
    // specify the type of object to be deserialized.
    XmlSerializer^ serializer = gcnew XmlSerializer(GetType());

    /* If the XML document has been altered with unknown
    nodes or attributes, handle them with the
    UnknownNode and UnknownAttribute events.*/
    //serializer.UnknownNode += new XmlNodeEventHandler(serializer_UnknownNode);
    //serializer.UnknownAttribute += new XmlAttributeEventHandler(serializer_UnknownAttribute);

	if (System::IO::File::Exists(fileName))
	{
	    // A FileStream is needed to read the XML document.
	    FileStream^ fs = gcnew FileStream(fileName, FileMode::Open);
	    try
	    {
	        // Declare an object variable of the type to be deserialized.
	        /* Use the Deserialize method to restore the object's state with
	        data from the XML document. */
	        //tmp = Convert.ChangeType(serializer.Deserialize(fs), GetType());

	        //this = serializer->Deserialize(fs);
	        tmp = (XmlWriteReadBase^)Convert::ChangeType(serializer->Deserialize(fs), GetType());
	        fs->Close();

	    }
	    catch (Exception^ ex)
	    {
#ifdef _DEBUG
	        Trace::WriteLine(ex);
#endif // _DEBUG
	        // If can not read config from file, let write the default config
	        if(fs!=nullptr) fs->Close();
	        //tmp = new ClassType();
	        //tmp.SaveToXml(fileName);
	    }
	    finally
	    {
	        if (fs != nullptr) fs->Close();
	    }
    }

    return tmp;
}
