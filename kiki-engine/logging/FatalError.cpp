#include "FatalError.hpp"

#include <sstream>

// TODO: Update with new logging
namespace Kiki {
    char const* FatalError::what() const noexcept
	{
#		if !KIKI_USE_STACKTRACE
		return mMsg.c_str();
#		else // USE_STACKTRACE
		if( mBuffer.empty() )
		{
			//mBuffer = std::format( "{}\n-- from --\n{}\n--\n", mMsg, mTrace );

			std::ostringstream oss;
			oss << mMsg << "\n";
		
			// Skip the first stack frame, since that's always inside the
			// Error::Error constructor, and therefore not all that useful.
			// Plus Error::Error() is a templated function, so it has quite an
			// ugly signature.
			std::size_t i = 1;
			for( auto it = std::next( mTrace.begin(), 1 ); it != mTrace.end(); ++it )
				oss << " | " << (i++) << "# " << *it << "\n";
			mBuffer = oss.str();
		}
		return mBuffer.c_str();
#		endif // ~ USE_STACKTRACE
	}
}