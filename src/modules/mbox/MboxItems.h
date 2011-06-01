#ifndef MboxItems_h__
#define MboxItems_h__

struct MBoxItem
{
	__int64 StartPos;
	__int64 EndPos;

	std::string Subject;
	std::string Sender;
	std::string Date;
};

#endif // MboxItems_h__