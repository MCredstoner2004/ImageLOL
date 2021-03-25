#include <array>
#include <memory>
#include <type_traits>
namespace ImageLOL
{

	template<typename T> u64 ImageLOLWriter::write(const T& object)
	{
		static_assert(std::is_trivially_copyable_v<T>, "type is not trivially copyable");
		u64 written_bytes = 0;
		std::array<byte, sizeof(T)> bytes;
		const byte* begin = reinterpret_cast<const byte*>(std::addressof(object));
		const byte* end = begin + sizeof(T);
		if (std::is_integral_v<T>)
		{
			std::reverse_copy(begin, end, std::begin(bytes));
		}
		else
		{
			std::copy(begin, end, std::begin(bytes));
		}
		for(byte b : bytes)
		{
			written_bytes += write(b);
		}
		return written_bytes / sizeof(T);
	}

	template<typename T> T ImageLOLReader::read()
	{
		static_assert(std::is_trivially_copyable_v<T>, "type is not trivially copyable");
		T output_object;
		byte* output_bytes = reinterpret_cast<byte *>(std::addressof(output_object));
		if (std::is_integral_v<T>)
		{
			for(size_t read_bytes = 0; read_bytes < sizeof(T) ; read_bytes ++)
			{
				output_bytes[sizeof(T) - read_bytes -1] = read<byte>();
			}
		}
		else
		{
			for(size_t read_bytes = 0; read_bytes < sizeof(T) ; read_bytes ++)
			{
				output_bytes[read_bytes] = read<byte>();
			}
		}
		return output_object;
	}
}
