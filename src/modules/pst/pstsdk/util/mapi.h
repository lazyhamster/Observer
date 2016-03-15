//! \file
//! \brief MAPI definitions
//! \author Ariman
//! \ingroup util

//! \defgroup exception Exceptions
//! \ingroup util

#ifndef PSTSDK_MAPI_H
#define PSTSDK_MAPI_H

namespace pstsdk
{

	enum PropType
	{
		PropMessageSubject				= 0x0037,
		
		PropMessageDeliveryTime			= 0x0e06,
		PropMessageFlags				= 0x0e07,
		PropMessageSize					= 0x0e08,
		
		PropMessageAttachmentSize		= 0x0e20,
		
		PropMessageBodyPlainText		= 0x1000,
		PropMessageBodyCompressedRTF	= 0x1009,
		PropMessageBodyHTML				= 0x1013,

		PropAttachmentDataObject		= 0x3701,
		
		PropAttachmentFilenameShort		= 0x3704,
		PropAttachmentMethod			= 0x3705,
		PropAttachmentFilenameLong		= 0x3707
	};

} // end namespace

#endif
