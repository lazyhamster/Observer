#include "stdafx.h"
#include "InternalMembers.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Reflection;

InternalMembers::InternalMembers()
{
	m_members = gcnew Dictionary<String^, MemberInfo^>();
}

Object^ InternalMembers::Get(Object^ obj, String^ name)
{
	Type^ type = obj->GetType();
	String^ key = type->FullName + "." + name;

	MemberInfo^ member;
	if (!m_members->TryGetValue(key, member))
	{
		BindingFlags flags = BindingFlags::Instance | BindingFlags::Public | BindingFlags::NonPublic;
		member = type->GetField(name, flags);
		if (member == nullptr)
			member = type->GetProperty(name, flags);
		if (member == nullptr)
			throw gcnew MissingMemberException(type->FullName, name);
		m_members[key] = member;
	}

	FieldInfo^ field = dynamic_cast<FieldInfo^>(member);
	return (field != nullptr) ? field->GetValue(obj) : ((PropertyInfo^)member)->GetValue(obj);
}

Object^ InternalMembers::Invoke(MethodInfo^ method, Object^ target, ... array<Object^>^ args)
{
	try
	{
		return method->Invoke(target, args);
	}
	catch (TargetInvocationException^ ex)
	{
		throw ex->InnerException;
	}
}

MethodInfo^ InternalMembers::FindMethod(Type^ type, String^ name, Type^ argType)
{
	for each (MethodInfo^ method in type->GetMethods(BindingFlags::Instance | BindingFlags::NonPublic))
	{
		if (method->Name != name) continue;

		array<ParameterInfo^>^ params = method->GetParameters();
		if (params->Length == 1 && params[0]->ParameterType->IsAssignableFrom(argType))
			return method;
	}
	return nullptr;
}
