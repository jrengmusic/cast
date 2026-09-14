#if JUCE_WINDOWS

#include <jam_core/utilities/jam_Platform.h>
#pragma warning(push)
#pragma warning(disable:4005)
#define interface struct
#pragma warning(pop)
#include <shlobj.h>
#undef interface

namespace jam
{
/*____________________________________________________________________________*/

void NativeFileChooser::openDirectory (juce::Component* parentWindow,
                                       const juce::String& startingPath,
                                       std::function<void (const juce::String&)> onSelected)
{
    if (parentWindow != nullptr)
    {
        if (auto* peer = parentWindow->getPeer())
        {
            HWND hwnd = (HWND) peer->getNativeHandle();

            IFileOpenDialog* dialog { nullptr };
            // COM CoCreateInstance requires void** output parameter; static_cast cannot bridge pointer-to-pointer across type hierarchies.
            HRESULT hr = CoCreateInstance (CLSID_FileOpenDialog,
                                          nullptr,
                                          CLSCTX_ALL,
                                          IID_IFileOpenDialog,
                                          reinterpret_cast<void**> (&dialog));

            if (SUCCEEDED (hr))
            {
                DWORD options { 0 };
                dialog->GetOptions (&options);
                dialog->SetOptions (options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

                if (startingPath.isNotEmpty())
                {
                    IShellItem* folder { nullptr };

                    // COM SHCreateItemFromParsingName requires void** output parameter; static_cast cannot bridge pointer-to-pointer across type hierarchies.
                    if (SUCCEEDED (SHCreateItemFromParsingName (startingPath.toWideCharPointer(), nullptr, IID_IShellItem, reinterpret_cast<void**> (&folder))))
                    {
                        dialog->SetFolder (folder);
                        folder->Release();
                    }
                }

                hr = dialog->Show (hwnd);

                if (SUCCEEDED (hr))
                {
                    IShellItem* item { nullptr };
                    hr = dialog->GetResult (&item);

                    if (SUCCEEDED (hr))
                    {
                        PWSTR filePath { nullptr };
                        hr = item->GetDisplayName (SIGDN_FILESYSPATH, &filePath);

                        if (SUCCEEDED (hr))
                        {
                            juce::String path (filePath);
                            CoTaskMemFree (filePath);

                            juce::MessageManager::callAsync ([onSelected, path]()
                            {
                                if (onSelected != nullptr)
                                    onSelected (path);
                            });
                        }

                        item->Release();
                    }
                }

                dialog->Release();
            }
        }
    }
}

/**_____________________________END_OF_NAMESPACE______________________________*/
}// namespace jam

#endif
