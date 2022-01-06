#ifndef IMAGELOL_H
#define IMAGELOL_H
#include <cstddef>
#include <cstdint>
#include <utility>
#include <string>
#include <vector>
namespace ImageLOL
{

	using byte = unsigned char;
	using u64 = uint64_t;

	struct dimensions
	{
		u64 width;
		u64 height;
		dimensions(u64 = 0, u64 = 0);
		~dimensions();
	};

	u64 required_pixels(u64 total_bytes, unsigned int bit_depth, unsigned int channels = 3);

	dimensions minimum_dimensions(u64 pixel_number, double ratio);

	dimensions minimum_dimensions(u64 pixel_number, u64 width, u64 height);

	class Image;
	class ImageLOL;
	class ImageLOLWriter;
	class ImageLOLReader;

	class Image
	{
		byte* image_data;
		dimensions size;
		byte channels:3;
		byte has_alpha:1;
	public:
		Image();
		Image(byte* image, u64 width, u64 height, unsigned int channels = 3);
		~Image();
		ImageLOLWriter getWriter(byte bit_depth = 3);
		ImageLOLReader getReader(byte bit_depth = 0);
	friend class ImageLOL;
	friend class ImageLOLWriter;
	friend class ImageLOLReader;
	};

	class ImageLOL
	{
		Image image;
		u64 total_bytes;
		byte bit_depth;
		u64 position;
		byte bit;
		ImageLOL(Image image, byte bit_depth = 0);
		~ImageLOL();
	public:
		u64 get_total_bytes();
		byte get_bit_depth();
	friend class ImageLOLWriter;
	friend class ImageLOLReader;
	};

	class ImageLOLWriter: public ImageLOL
	{
		ImageLOLWriter(Image image, byte bit_depth);
	public:
		~ImageLOLWriter();
		u64 write(const byte object);
		u64 writeByteBuffer(const byte* buffer, u64 size);
		template<typename T> u64 write(const T& object);
	friend class Image;
	friend class ImageLOL;
	};

	template<> u64 ImageLOLWriter::write<std::string>(const std::string& object);
	template<> u64 ImageLOLWriter::write<std::vector<byte>>(const std::vector<byte>& object);

	class ImageLOLReader: public ImageLOL
	{
		ImageLOLReader(Image image, byte bit_depth = 0);
	public:
		~ImageLOLReader();
		template<typename T> T read();
		u64 readByteBuffer(byte* buffer, u64 size);
	friend class Image;
	friend class ImageLOL;
	};

	template<> byte ImageLOLReader::read<byte>();
	template<> std::string ImageLOLReader::read<std::string>();
	template<> std::vector<byte> ImageLOLReader::read<std::vector<byte>>();
}
#include "ImageLOL.inl"
#endif
