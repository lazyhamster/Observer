//! \mainpage
//! \author Terry Mahaffey
//! 
//! \section main_intro Introduction
//! This is the API Reference for the PST File Format SDK. This library is
//! header only; meaning there is nothing to build or link to in order to use
//! it - simply include the proper header file.
//!
//! The SDK is organized into layers (represented as "Modules" in this
//! reference). If you don't know where to start, there is a good chance you
//! can work in the "PST" Layer and ignore everything else.
//!
//! This project is distributed under the
//! <a href="http://www.apache.org/licenses">Apache v2.0 License</a>.
//!
//! \section main_requirements System Requirements
//! - Boost v1.42 or greater (older versions may work, but are untested). 
//! The following Boost components are used:
//!      - Boost.Iostreams
//!      - Boost.Iterators
//!      - Boost.Utility
//! - GCC 4.4.x (with -std=c++0x) or Visual Studio 2010
//!
//! Partial support is offered for some older versions of Visual Studio and
//! GCC. See the <a href="http://pstsdk.codeplex.com">CodePlex site</a> for
//! details.
//!
//! \section main_getting_help Additional Documentation
//! See the Documentation on the 
//! <a href="http://pstsdk.codeplex.com">CodePlex site</a> for a series of 
//! "Quick Start" guides; one for each layer. 
//!
//! The [MS-PST] reference documentation is included in this distribution 
//! in the "doc" directory, but it may be out of date. The latest version can
//! be found on MSDN.
//!
//! \section main_samples Sample Code
//! There are several sample included in the "samples" directory of the 
//! distribution. Additionally, the unit test code contained in the "test" 
//! directory may be of some value as sample code.
//!
//! \section main_help Getting Help
//! The Discussions section of the
//! <a href="http://pstsdk.codeplex.com">CodePlex site</a> is a good place
//! to start for any questions or comments about the SDK. Please do not contact
//! me directly with questions; instead post your question to the appropriate 
//! discussion topic (or create one), and I'll do my best to answer there. This
//! way, the question and answer will be publicly available for anyone having
//! similar issues.
//!
//! \section main_bugs Bug Reports
//! If you find a bug either in the SDK or the documentation, create a work 
//! item for it on the Issue Tracker section of the 
//! <a href="http://pstsdk.codeplex.com">CodePlex site</a>. Feel free to vote
//! up existing issues you feel are important.
//!
//! \section main_contributions Contributing
//! Contributions in the form of patches for either bug fixes or new features
//! may be submitted in the Source Code section of the 
//! <a href="http://pstsdk.codeplex.com">CodePlex site</a>
//!
//! Contact me directly on CodePlex if you or your company is interested in
//! becoming a top level developer on this project.

//! \defgroup pst PST Layer
#ifndef PSTSDK_PST_H
#define PSTSDK_PST_H

#include "pstsdk/pst/pst.h"
#include "pstsdk/pst/folder.h"
#include "pstsdk/pst/message.h"

#endif
