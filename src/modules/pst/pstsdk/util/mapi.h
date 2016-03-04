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
		PropMessageBodyPlainText = 0x1000,
		PropMessageBodyCompressedRTF = 0x1009,
		PropMessageBodyHTML = 0x1013
	};

} // end namespace

#endif
