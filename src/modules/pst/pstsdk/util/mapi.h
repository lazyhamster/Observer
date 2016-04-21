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

		PropMessageClientSubmitTime		= 0x0039,

		PropMessageTransportHeaders		= 0x007d,
		
		PropRecipientType				= 0x0c15,

		PropMessageSenderName			= 0x0c1a,

		PropMessageSenderSearchKey		= 0x0c1d,
		PropMessageSenderAdressType		= 0x0c1e,
		PropMessageSenderEmailAddress	= 0x0c1f,
		
		PropMessageDeliveryTime			= 0x0e06,
		PropMessageFlags				= 0x0e07,
		PropMessageSize					= 0x0e08,
		
		PropMessageAttachmentSize		= 0x0e20,
		
		PropMessageBodyPlainText		= 0x1000,
		PropMessageBodyCompressedRTF	= 0x1009,
		PropMessageBodyHTML				= 0x1013,

		PropDisplayName					= 0x3001,
		PropAddressType					= 0x3002,
		PropEmailAddress				= 0x3003,

		PropMessageCreationTime			= 0x3007,
		PropMessageModificationTime		= 0x3008,

		PropFolderType					= 0x3601,
		PropNumberOfContentItems		= 0x3602,
		PropNumberOfUnreadContentItems	= 0x3603,

		PropHasSubFolders				= 0x360a,
		PropContainerClass				= 0x3613,
		PropNumberOfAssosiatedContent	= 0x3617,

		PropAttachmentDataObject		= 0x3701,
		
		PropAttachmentFilenameShort		= 0x3704,
		PropAttachmentMethod			= 0x3705,
		PropAttachmentFilenameLong		= 0x3707,

		PropAccount						= 0x3a00,

		PropMessageBodyCodepage			= 0x3fde,
		PropMessageCodepage				= 0x3ffd
	};

} // end namespace

#endif
