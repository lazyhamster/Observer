#pragma once

// Access to non-public DiscUtils members, used where its public API is missing something or is too slow
ref class InternalMembers
{
private:
	System::Collections::Generic::Dictionary<System::String^, System::Reflection::MemberInfo^>^ m_members;

public:
	InternalMembers();

	// Value of field or property, member lookups are cached per type
	System::Object^ Get(System::Object^ obj, System::String^ name);

	// Calls method and rethrows exception from inside instead of TargetInvocationException
	static System::Object^ Invoke(System::Reflection::MethodInfo^ method, System::Object^ target, ... array<System::Object^>^ args);

	// Non-public instance method with one parameter, which accepts values of argType
	static System::Reflection::MethodInfo^ FindMethod(System::Type^ type, System::String^ name, System::Type^ argType);
};
