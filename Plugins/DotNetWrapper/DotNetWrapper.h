// DotNetWrapper.h
// This dll must be installed at the same place as the viewer.
//
// Author : Thierry Parent
// Version : 11.0
//
// HomePage :  http://www.codeproject.com/csharp/TraceTool.asp
// Download :  http://sourceforge.net/projects/tracetool/
// See License.txt for license information 

#pragma once

using namespace System;
using namespace System::Text;
using namespace System::Reflection;   // Assembly
using namespace System::Collections;
using namespace System::IO;           // Stream

enum PluginStatus
{
    /// <summary>
    /// Unloaded
    /// </summary>
    Unloaded,
    /// <summary>
    /// Loaded, not started
    /// </summary>
    Loaded,
    /// <summary>
    /// Loaded and started
    /// </summary>
    Started
};


// the Singleton and the CppPluginLoader classes are not put on a namespace

//---------------------------------------------------------------------------------------------------------------
public ref class Singleton
{
public:
    static System::Collections::Hashtable^ PlugList = gcnew Hashtable();

    static void trace(String^ source)
    {
        FileStream^ f;
        String^ FileToWrite = gcnew String("c:\\temp\\DotNetWrapperLog.txt");

        // check if exist

        if (File::Exists(FileToWrite) == false)
        {
            f = gcnew FileStream(FileToWrite, FileMode::Create);
        }
        else {  // append only the node
            f = File::Open(FileToWrite, FileMode::Open, FileAccess::ReadWrite, FileShare::ReadWrite);
            f->Seek(0, SeekOrigin::End);
        }
        array<Byte>^ info = (gcnew UTF8Encoding(true))->GetBytes((DateTime::Now.ToString("yyyyMMdd HH:mm:ss:fff ") + source)->ToString());
        f->Write(info, 0, info->Length);
        f->Close();
        f = nullptr;
    }  // trace()
};   // Singleton

//---------------------------------------------------------------------------------------------------------------


public delegate String^ Delegate_GetPlugName();
public delegate void Delegate_Start(String^ strParameter);
public delegate void Delegate_Stop();
public delegate void Delegate_OnTimer();
public delegate bool Delegate_OnBeforeDelete(String^ WinId, String^ NodeId);
public delegate bool Delegate_OnAction(String^ WinId, int ResourceId, String^ NodeId);

// Each managed plugin are loaded in separated domain.
// a PLugin loader load only one plugin. 
// PluginLoader is created from separated AppDomain
[Serializable]
public ref class CppPluginLoader : MarshalByRefObject
{
private :

    // delegate to the differents ITracePLugin functions
    Delegate_GetPlugName^ _delegate_GetPlugName;
    Delegate_Start^ _delegate_Start;
    Delegate_Stop^ _delegate_Stop;
    Delegate_OnAction^ _delegate_OnAction;
    Delegate_OnBeforeDelete^ _delegate_OnBeforeDelete;
    Delegate_OnTimer^ _delegate_OnTimer;

    // Point to the plugin (type ITracePLugin). Needed to unloaded the plugin
    Object^ _plugin;

    // Domain that host the plugin. Needed to unloaded the domain
    AppDomain^ _domain;  

    // Status of the plugin
    PluginStatus _status;

public:

    AppDomain^ GetDomain()
    {
        //Singleton::trace("    Loader : GetDomain\n");
        return _domain;
    }

    void SetDomain(AppDomain^ domain)
    {
        //Singleton::trace("    Loader : SetDomain\n");
        _domain = domain; 
    }

    String^ SatusToString(PluginStatus status)
    {
        String^ result;
        switch (status)
        {
        case Unloaded: result = gcnew String("Unloaded"); break;
        case Loaded:   result = gcnew String("Loaded");   break;
        case Started:  result = gcnew String("Started");  break;
        }
        return result;    
    }

    PluginStatus GetStatus()
    {
        //Singleton::trace("    Loader : GetStatus : " + SatusToString(_status) + "\n");
        return _status;
    }

    void SetStatus(PluginStatus status)
    {
        //Singleton::trace("    Loader : SetStatus : " + SatusToString(status) + "\n");
        _status = status;
    }
    
    String^ GetPlugName()
    {
        //Singleton::trace("    Loader : GetPlugName\n");
        String^ name;
        name = gcnew String(_delegate_GetPlugName());
        //Singleton::trace("    Loader : GetPlugName : " + name + "\n");

        return name; //  gcnew String("my plugin"); // _delegate_GetPlugName();
    }

    void UnloadPlugin()
    {
        //Singleton::trace("    Loader : UnloadPlugin\n");
        _plugin = nullptr;  // Garbage collected
    }

    void StartPlugin(String^ strParameter)
    {
        //Singleton::trace("    Loader : StartPlugin begin\n");
        _delegate_Start(strParameter);
        //Singleton::trace("    Loader : StartPlugin end\n");
    }
    
    void StopPlugin() 
    { 
        //Singleton::trace("    Loader : StopPlugin begin\n");
        _delegate_Stop();
        //Singleton::trace("    Loader : StopPlugin end\n");
    }

    bool OnAction(String^ strWinId, int ResourceId, String^ strNodeId)
    {
        //Singleton::trace("    Loader : OnAction start\n");
        bool result = _delegate_OnAction(strWinId, ResourceId, strNodeId);
        //Singleton::trace("    Loader : OnAction end \n");
        return result;
    }

    bool OnBeforeDelete(String ^ strWinId, String ^ strNodeId)
    {
        //Singleton::trace("    Loader : OnBeforeDelete start\n");
        bool result = _delegate_OnBeforeDelete(strWinId, strNodeId);
        //Singleton::trace("    Loader : OnBeforeDelete end \n");
        return result;
    }

    void OnTimer()
    {
        //Singleton::trace("    Loader : OnTimer start\n");
        _delegate_OnTimer();
        //Singleton::trace("    Loader : OnTimer end\n");
    }

    //---------------------------------------------------------------------------------------------------------------

    // check if the assembly containt a class that implement the Tracetool.ITracePLugin interface
    // throw exception on error
    void CheckPlugInFile(String^ FileName)
    {
        //Singleton::trace("    Loader : CheckPlugInFile : " + FileName + "\n");
        //name = nullptr;
        _plugin = nullptr;

        // Load the assembly in the current domain (the one created with AppDomain.CreateDomain() )
        //------------------------------------------------------------------------------------------

        // exception can occur on assembly load or GetTypes
        Assembly^ assem;
        try {
            AssemblyName^ name = AssemblyName::GetAssemblyName(FileName);
            assem = AppDomain::CurrentDomain->Load(name);
        }
        catch (Exception ^ ex) {
            Singleton::trace("    Loader : CheckPlugInFile : " + FileName + "\n");
            Singleton::trace("    Loader : CheckPlugInFile : AppDomain::CurrentDomain->Load() exception : " + ex->Message + "\n");
            throw gcnew Exception(ex->Message);
        }

        // get all types for the assembly, check if one of the type implement the TraceTool.ITracePLugin
        //-----------------------------------------
        array<Type^>^ types;
        try {
            // exception can be throw if the assembly make reference to an unavailable assembly (like tracetool)
            types = assem->GetTypes();
        }
        catch (ReflectionTypeLoadException ^ ex) {
            Singleton::trace("    Loader : CheckPlugInFile : " + FileName + "\n");
            Singleton::trace("    Loader : CheckPlugInFile : assem->GetTypes() exception : " + ex->Message + "\n");

            StringBuilder^ errMsg = gcnew StringBuilder(ex->Message);
            errMsg->Append("\n");
            array<Exception^>^ err = ex->LoaderExceptions;
            for (int c = 0; c < err->Length; c++)
            {
                Exception^ e = err[c];
                Singleton::trace(e->Message + "\n");
                errMsg->Append(e->Message);
            }
            throw gcnew Exception(errMsg->ToString());   // throw original exception concatened with LoaderExceptions messages
        }
        catch (Exception ^ ex) {
            Singleton::trace("    Loader : CheckPlugInFile : " + FileName + "\n");
            Singleton::trace("    Loader : CheckPlugInFile : assem->GetTypes() exception : " + ex->Message + "\n");
            throw gcnew Exception(ex->Message);
        }

        if (types->Length == 0) {
            Singleton::trace("    Loader : CheckPlugInFile : " + FileName + "\n");
            Singleton::trace("    Loader : CheckPlugInFile : The Assembly don't contain any classes. Try to load it as a Win32 plugin\n");
            throw gcnew Exception("The Assembly don't contain any classes. Try to load it as a Win32 plugin");
        }
        
        //Singleton::trace ("    Loader : CheckPlugInFile : Assembly contains " + types->Length.ToString() + " classes or interfaces\n") ;
        for (int c = 0; c < types->Length; c++)
        {
            Type^ OneType = types[c];

            // check if the type implement the Tracetool.ITracePLugin interface
            array<Type^>^ interfaces = OneType->GetInterfaces();

            for (int d = 0; d < interfaces->Length; d++)
            {
                Type^ OneInterface = interfaces[d];
                if (OneInterface->FullName->Equals("TraceTool.ITracePLugin"))
                {
                    // create an instance of the type and save it in the PluginLoader
                    // Error can occur if the type require parameters.

                    //Singleton::trace("    Loader : CheckPlugInFile : TraceTool.ITracePLugin found. Create instance\n");
                    _plugin = Activator::CreateInstance(OneType);

                    // create delegates
                    //--------------------
                    try {
                        //Singleton::trace("    Loader : CheckPlugInFile : getting delegates\n");
                        _delegate_GetPlugName    = (Delegate_GetPlugName^)    Delegate::CreateDelegate(Delegate_GetPlugName   ::typeid, _plugin, "GetPlugName");
                        _delegate_Start          = (Delegate_Start^)          Delegate::CreateDelegate(Delegate_Start         ::typeid, _plugin, "Start");
                        _delegate_Stop           = (Delegate_Stop^)           Delegate::CreateDelegate(Delegate_Stop          ::typeid, _plugin, "Stop");
                        _delegate_OnTimer        = (Delegate_OnTimer^)        Delegate::CreateDelegate(Delegate_OnTimer       ::typeid, _plugin, "OnTimer");
                        _delegate_OnBeforeDelete = (Delegate_OnBeforeDelete^) Delegate::CreateDelegate(Delegate_OnBeforeDelete::typeid, _plugin, "OnBeforeDelete");
                        _delegate_OnAction       = (Delegate_OnAction^)       Delegate::CreateDelegate(Delegate_OnAction      ::typeid, _plugin, "OnAction");
                    }
                    catch (Exception ^ ex) {
                        Singleton::trace("    Loader : CheckPlugInFile : " + FileName + "\n");
                        Singleton::trace("    Loader : CheckPlugInFile : CreateDelegate() exception : " + ex->Message + "\n");
                        throw gcnew Exception(ex->Message);
                    }

                    // Get the plugin name
                    // -------------------
                    //try {
                    //    name = gcnew String(_delegate_GetPlugName());  // MAKE A COPY !!!
                    //    //Singleton::trace("    Loader : CheckPlugInFile : plugin name = " + name + "\n");
                    //}
                    //catch (Exception ^ ex) {
                    //    Singleton::trace("    Loader : CheckPlugInFile : " + FileName + "\n");
                    //    Singleton::trace("    Loader : CheckPlugInFile : delegate_GetPlugName exception : " + ex->Message + "\n");
                    //    throw gcnew Exception(ex->Message);
                    //}

                    // this plugin will be added to Singleton::PlugList
                    
                    //Singleton::trace("    Loader : CheckPlugInFile : DONE\n");
                    return;
                }
            }  // next interface
        }     // next type

        // no TraceTool.ITracePLugin found.
        // generate exception

        StringBuilder^ sb = gcnew StringBuilder("PluginLoader : Assembly <");
        sb->Append(FileName)->Append("> contains ")->Append(types->Length)
            ->Append(" classe(s), \n but none implements TraceTool.IPplugin interface. It's perhaps a Win32 plugin");

        Singleton::trace("    Loader : CheckPlugInFile : " + FileName + "\n" + sb->ToString() + "\n");
        throw gcnew Exception(sb->ToString());
    }  // CheckPlugInFile

};    // CppPluginLoader class

//}   // namespace TraceTool

