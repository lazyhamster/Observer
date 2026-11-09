#pragma once

// Reads long file names directly from FAT directories, as DiscUtils works with short names only.
// Result maps short path (in DiscUtils representation) to the long name of the last path element.
// This data is used only for display, so any invalid entry just leaves the item with its short name.
System::Collections::Generic::Dictionary<System::String^, System::String^>^ ReadFatLongNames(System::IO::Stream^ volume, System::Text::Encoding^ shortNameEncoding);
