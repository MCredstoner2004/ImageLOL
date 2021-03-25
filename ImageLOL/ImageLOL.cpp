#include <cmath>
#include <climits>
#include <algorithm>
#include "ImageLOL.h"

namespace ImageLOL
{

	dimensions::dimensions(u64 _width, u64 _height): width(_width), height(_height)
	{
	}

	dimensions::~dimensions() = default;

	u64 required_pixels(u64 total_bytes, unsigned int bit_depth, unsigned int channels)
	{
		if (channels > 3)
		{
			if (channels == 4)
			{
				channels = 3;
			}
			else
			{
				return 0;
			}
		}
		if (total_bytes == 0 || bit_depth == 0 || bit_depth > 8)
		{
			return 0;
		}
		u64 required_bits = 8 * total_bytes;
		if (required_bits / total_bytes != 8)
		{
			return 0;
		}
		else
		{
			unsigned int bits_per_pixel = channels * bit_depth;
			return (required_bits / bits_per_pixel) + (required_bits % bits_per_pixel != 0);
		}
		return 0;
	}

	dimensions minimum_dimensions(u64 pixel_number, double ratio)
	{
		if (ratio > 0 && std::isfinite(ratio))
		{
			u64 height, width;
			if (ratio > 1.0)
			{
				double height_squared = pixel_number / ratio;
				height = static_cast<u64>(std::ceil(std::sqrt(height_squared)));
				width = static_cast<u64>(std::ceil(height * ratio));
				while (width * height < pixel_number)
				{
					height++;
					width = static_cast<u64>(std::ceil(height * ratio));
				}
			}
			else
			{
				ratio = 1.0 / ratio;
				double width_squared = pixel_number / ratio;
				width = static_cast<u64>(std::ceil(std::sqrt(width_squared)));
				height = static_cast<u64>(std::ceil(width * ratio));
				while (width * height < pixel_number)
				{
					width++;
					height = static_cast<u64>(std::ceil(width * ratio));
				}
			}
			return dimensions(width, height);
		}
		else
		{
			return dimensions(0, 0);
		}
	}

	dimensions minimum_dimensions(u64 pixel_number, u64 width, u64 height)
	{
		if (width != 0 && height != 0)
		{
			dimensions min_dimensions = minimum_dimensions(pixel_number, static_cast<double>(width) / static_cast<double>(height));
			if (min_dimensions.width < width || min_dimensions.height < height)
			{
				return dimensions(width, height);
			}
			else
			{
				return min_dimensions;
			}
		}
		else
		{
			return dimensions(0, 0);
		}
	}

	Image::Image(): image_data(nullptr), size(0, 0), channels(0), has_alpha(false)
	{
	}

	Image::Image(byte* _image, u64 _width, u64 _height, unsigned int _channels): image_data(_image), size(_width, _height)
	{
		if (_channels <= 4)
		{
			channels = _channels;
			if(_channels == 4)
			{
				has_alpha = true;
			}
			else
			{
				has_alpha = false;
			}
		}
		else
		{
			channels = 3;
		}
	}

	Image::~Image() = default;

	ImageLOLWriter Image::getWriter(byte bit_depth)
	{
		return ImageLOLWriter(*this, bit_depth);
	}

	ImageLOLReader Image::getReader(byte bit_depth)
	{
		return ImageLOLReader(*this, bit_depth);
	}

	ImageLOL::ImageLOL(Image _image, byte _bit_depth): image(_image)
	{
		total_bytes = _image.size.width * _image.size.height * _image.channels;
		if (_bit_depth == 0)
		{
			if (total_bytes > 0)
			{
				bit_depth = 0;
				byte temp = image.image_data[0];
				while (temp != 0)
				{
					if ((temp & 1) == 1)
					{
						bit_depth ++;
					}
					else
					{
						temp = 0;
						total_bytes = 0;
						bit_depth = 0;
					}
					temp >>= 1;
				}
			}
			else
			{
				total_bytes = 0;
				bit_depth = 0;
			}
		}
		else
		{
			if (_bit_depth > 8)
			{
				bit_depth = 8;
			}
			else
			{
				bit_depth = _bit_depth;
			}
			if (total_bytes > 0)
			{
				image.image_data[0] = UCHAR_MAX >> (CHAR_BIT - bit_depth);
				position = 1;
				bit = 0;
			}
		}
		if (total_bytes != 0)
		{
			position = 1;
			bit = 0;
		}
	}

	ImageLOL::~ImageLOL() = default;

	byte ImageLOL::get_bit_depth()
	{
		return bit_depth;
	}

	u64 ImageLOL::get_total_bytes()
	{
		return total_bytes;
	}

	ImageLOLWriter::ImageLOLWriter(Image _image, byte _bit_depth): ImageLOL(_image, _bit_depth)
	{
	}

	ImageLOLWriter::~ImageLOLWriter() = default;

	u64 ImageLOLWriter::write(const byte object)
	{
		byte written_bits = 0;
		byte remaining_bits = CHAR_BIT;
		while ((remaining_bits > 0) && (position < total_bytes))
		{
			byte bits_to_write = std::min<byte>(remaining_bits, bit_depth - bit);
			byte mask = UCHAR_MAX >> (CHAR_BIT - bits_to_write);

			byte current_data = image.image_data[position];
			current_data &= ~(mask << (bit_depth - bit - bits_to_write));
			current_data |= (((object >> (remaining_bits - bits_to_write)) & mask) << (bit_depth - bit - bits_to_write));
			image.image_data[position] = current_data;

			bit += bits_to_write;
			remaining_bits -= bits_to_write;
			written_bits += bits_to_write;
			if (bit >= bit_depth)
			{
				position ++;
				if (image.has_alpha && ((position % 4) == 3))
				{
					position ++;
				}
				bit = 0;
			}
		}
		return written_bits / CHAR_BIT;
	}

	u64 ImageLOLWriter::write(const u64 object)
	{
		byte written_bits = 0;
		byte remaining_bits = CHAR_BIT * sizeof(u64);
		while ((remaining_bits > 0) && (position < total_bytes))
		{
			byte bits_to_write = std::min<byte>(remaining_bits, bit_depth - bit);
			byte mask = UCHAR_MAX >> (CHAR_BIT - bits_to_write);

			byte current_data = image.image_data[position];
			current_data &= ~(mask << (bit_depth - bit - bits_to_write));
			current_data |= (((object >> (remaining_bits - bits_to_write)) & mask) << (bit_depth - bit - bits_to_write));
			image.image_data[position] = current_data;

			bit += bits_to_write;
			remaining_bits -= bits_to_write;
			written_bits += bits_to_write;
			if (bit >= bit_depth)
			{
				position ++;
				if (image.has_alpha && ((position % 4) == 3))
				{
					position ++;
				}
				bit = 0;
			}
		}
		return written_bits / (CHAR_BIT * sizeof(u64));
	}

	template<> u64 ImageLOLWriter::write<std::string>(const std::string& object)
	{
		u64 written_bytes = 0;
		written_bytes += write(static_cast<u64>(object.size())) * sizeof(u64);
		for(byte b : object)
		{
			written_bytes += write(b);
		}
		return written_bytes / (object.size() + sizeof(u64));
	}

	template<> u64 ImageLOLWriter::write<std::vector<byte>>(const std::vector<byte>& object)
	{
		u64 written_bytes = 0;
		written_bytes += write(static_cast<u64>(object.size())) * sizeof(u64);
		for(byte b : object)
		{
			written_bytes += write(b);
		}
		return written_bytes / (object.size() + sizeof(u64));
	}

	ImageLOLReader::ImageLOLReader(Image _image, byte _bit_depth): ImageLOL(_image, _bit_depth)
	{
	}

	ImageLOLReader::~ImageLOLReader() = default;

	template<> byte ImageLOLReader::read<byte>()
	{
		byte remaining_bits = CHAR_BIT;
		byte output = 0;
		while ((remaining_bits > 0) && (position < total_bytes))
		{
			byte bits_to_read = std::min<byte>(remaining_bits, bit_depth - bit);
			byte mask = UCHAR_MAX >> (CHAR_BIT - bits_to_read);
			byte current_bits = image.image_data[position];
			current_bits >>= (bit_depth - bit - bits_to_read);
			current_bits &= mask;
			output |= (static_cast<byte>(current_bits) << (remaining_bits - bits_to_read));

			bit += bits_to_read;
			remaining_bits -= bits_to_read;
			if (bit >= bit_depth)
			{
				position ++;
				if (image.has_alpha && ((position % 4) == 3))
				{
					position ++;
				}
				bit = 0;
			}
		}
		return output;
	}

	template<> u64 ImageLOLReader::read<u64>()
	{
		byte remaining_bits = CHAR_BIT * sizeof(u64);
		u64 output = 0;
		while ((remaining_bits > 0) && (position < total_bytes))
		{
			byte bits_to_read = std::min<byte>(remaining_bits, bit_depth - bit);
			byte mask = UCHAR_MAX >> (CHAR_BIT - bits_to_read);
			byte current_bits = image.image_data[position];
			current_bits >>= (bit_depth - bit - bits_to_read);
			current_bits &= mask;
			output |= (static_cast<u64>(current_bits) << (remaining_bits - bits_to_read));

			bit += bits_to_read;
			remaining_bits -= bits_to_read;
			if (bit >= bit_depth)
			{
				position ++;
				if (image.has_alpha && ((position % 4) == 3))
				{
					position ++;
				}
				bit = 0;
			}
		}
		return output;
	}

	template<> std::string ImageLOLReader::read<std::string>()
	{
		u64 length = read<u64>();
		std::string output(static_cast<size_t>(length), '\0');
		for(size_t read_chars = 0; read_chars < length; read_chars ++)
		{
			output[read_chars] = read<byte>();
		}
		return output;
	}

	template<> std::vector<byte> ImageLOLReader::read<std::vector<byte>>()
	{
		u64 size = read<u64>();
		std::vector<byte> output;
		output.resize(static_cast<size_t>(size));
		for(size_t read_bytes = 0; read_bytes < size; read_bytes ++)
		{
			output[read_bytes] = read<byte>();
		}
		return output;
	}


}

