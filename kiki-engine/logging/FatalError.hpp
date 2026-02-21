#ifndef KIKI_FATALERROR
#define KIKI_FATALERROR

#include <exception>
#include <string>
#include <format>

#if KIKI_USE_STACKTRACE
#	include <stacktrace>
#endif // ~ USE_STACKTRACE

namespace Kiki {
    class FatalError : public std::exception {
        public:
			template<typename... tArgs>
			explicit FatalError(std::format_string<tArgs...>, tArgs&&...);

		public:
			char const* what() const noexcept override;

		private:
			std::string mMsg;
#			if KIKI_USE_STACKTRACE
			std::stacktrace mTrace = std::stacktrace::current();
			mutable std::string mBuffer;
#			endif // ~ USE_STACKTRACE
    };

    template< typename... tArgs > inline
	FatalError::FatalError(std::format_string<tArgs...> aFmt, tArgs&&... aArgs)
		: mMsg(std::format(aFmt, std::forward<tArgs>(aArgs)...))
	{}
}

#endif