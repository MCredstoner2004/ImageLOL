#include <cmath>
#include <iostream>
#include <climits>
#include "ImageLOL.h"

namespace ImageLOL
{

const byte masks[] {0b0, 0b1, 0b11, 0b111, 0b1111, 0b11111, 0b111111, 0b1111111, 0b11111111};

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

	u64 writeSingleByte(byte* image_data, u64& pos, byte& bit, byte bit_depth, bool alpha, u64 max_pos, byte data)
	{
		bool fast_path = false;
		u64 position = pos;
		byte current_bit = bit;
		byte remaining_bits = CHAR_BIT;
		byte current_byte;
		byte mask;
		if (CHAR_BIT == 8)
		{
			switch (bit_depth)
			{
			case 1:
			case 2:
			case 4:
			case 8:
				fast_path = true;
			}
		}
		if (fast_path)
		{
			mask = masks[bit_depth];
			while (remaining_bits && (position < max_pos))
			{
				current_byte = image_data[position] & ~mask;
				image_data[position] = current_byte | ((data >> (remaining_bits - bit_depth)) & mask);
				remaining_bits -= bit_depth;
				position ++;
				if (alpha && ((position % 4) == 3))
				{
					position ++;
				}
			}
		}
		else
		{
			byte bits_to_write;
			while ((remaining_bits) && (position < max_pos))
			{
				if (remaining_bits > bit_depth - current_bit)
				{
					bits_to_write = bit_depth - current_bit;
				}
				else
				{
					bits_to_write = remaining_bits;
				}
				mask = masks[bits_to_write];
				current_byte = image_data[position];
				current_byte &= ~mask << (bit_depth - current_bit - bits_to_write);
				current_byte |= ((data >>  (remaining_bits - bits_to_write)) & mask) << (bit_depth - current_bit - bits_to_write);
				image_data[position] = current_byte;
				current_bit += bits_to_write;
				remaining_bits -= bits_to_write;
				if (current_bit >= bit_depth)
				{
					position ++;
					if (alpha && ((position % 4) == 3))
					{
						position ++;
					}
					current_bit = 0;
				}
			}
		}
		pos = position;
		bit = current_bit;
		return (CHAR_BIT - remaining_bits) / CHAR_BIT;
	}

	u64 ImageLOLWriter::write(const byte data)
	{
		return writeSingleByte(image.image_data, position, bit, bit_depth, image.has_alpha, total_bytes, data);
	}

	u64 ImageLOLWriter::writeByteBuffer(const byte* buffer, u64 size)
	{
		byte* image_data = image.image_data;
		u64 pos = position;
		byte current_bit = bit;
		byte depth = bit_depth;
		bool alpha = image.has_alpha;
		u64 max_pos = total_bytes;
		u64 count = 0;
		for (u64 i = 0; i < size; i++ )
		{
			count += writeSingleByte(image_data, pos, current_bit, depth, alpha, max_pos, buffer[i]);
		}
		position = pos;
		bit = current_bit;
		return count;
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
		written_bytes += write(static_cast<u64>(object.size()));
		written_bytes += writeByteBuffer(object.data(), static_cast<u64>(object.size()));
		return written_bytes / (object.size() + sizeof(u64));
	}

	ImageLOLReader::ImageLOLReader(Image _image, byte _bit_depth): ImageLOL(_image, _bit_depth)
	{
	}

	ImageLOLReader::~ImageLOLReader() = default;

	byte readSingleByte(byte* image_data, u64& pos, byte& bit, byte bit_depth, bool alpha, u64 max_pos, u64* count = nullptr)
	{
		bool fast_path = false;
		byte output = 0;
		u64 position = pos;
		byte current_bit = bit;
		byte remaining_bits = CHAR_BIT;
		byte mask;
		if (CHAR_BIT == 8)
		{
			switch (bit_depth)
			{
			case 1:
			case 2:
			case 4:
			case 8:
				fast_path = true;
			}
		}
		if (fast_path)
		{
			mask = masks[bit_depth];
			while (remaining_bits && (position < max_pos))
			{
				output |= (image_data[position] & mask) << (remaining_bits - bit_depth);
				remaining_bits -= bit_depth;
				position ++;
				if (alpha && ((position % 4) == 3))
				{
					position ++;
				}
			}
		}
		else
		{
			byte bits_to_read;
			byte current_byte;
			while ((remaining_bits) && (position < max_pos))
			{
				if (remaining_bits > bit_depth - current_bit)
				{
					bits_to_read = bit_depth - current_bit;
				}
				else
				{
					bits_to_read = remaining_bits;
				}
				current_byte = image_data[position];
				mask = masks[bits_to_read];
				current_byte = (image_data[position] >> (bit_depth - current_bit - bits_to_read)) & mask;
				output |= current_byte << (remaining_bits - bits_to_read);
				current_bit += bits_to_read;
				remaining_bits -= bits_to_read;
				if (current_bit >= bit_depth)
				{
					position ++;
					if (alpha && ((position % 4) == 3))
					{
						position ++;
					}
					current_bit = 0;
				}
			}
		}
		if (count)
		{
			*count += (CHAR_BIT - remaining_bits) / CHAR_BIT;
		}
		pos = position;
		bit = current_bit;
		return output;
	}

	u64 ImageLOLReader::readByteBuffer(byte* output_buffer, u64 size)
	{
		byte* image_data = image.image_data;
		u64 pos = position;
		byte current_bit = bit;
		byte depth = bit_depth;
		bool alpha = image.has_alpha;
		u64 max_pos = total_bytes;
		u64 count = 0;
		for (u64 i = 0; i < size; i++ )
		{
			output_buffer[i] = readSingleByte(image_data, pos, current_bit, depth, alpha, max_pos, &count);
		}
		position = pos;
		bit = current_bit;
		return count;
	}

	template<> byte ImageLOLReader::read<byte>()
	{
		return readSingleByte(image.image_data, position, bit, bit_depth, image.has_alpha, total_bytes);
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
		readByteBuffer(output.data(), size);
		return output;
	}

}

