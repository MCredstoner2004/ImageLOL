/*Copyright (c) 2021 MCredstoner2004*/

#include <iostream>
#include <stb_image.h>
#include <stb_image_write.h>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <limits>
#include <memory>
#include <cstdint>
#include <math.h>
#include <png.h>
#include <ImageLOL.h>

using u64 = ImageLOL::u64;
using byte = ImageLOL::byte;

enum channels
{
		r=0, g=1, b=2, a=3
};
//simple nearest-neighbor sampling, might add another sampling method later.
byte sample_image(byte* image, int width, int height, double x, double y, int num_channels, channels channel)
{
	return image[(static_cast<size_t>(y * static_cast<double>(height-1)) * static_cast<size_t>(width) + static_cast<size_t>(x * static_cast<double>(width-1))) * num_channels + channel];
}

int main(int argc, char** argv)
{

	int arg_count = 0;
	int bit_n = 3;	
	bool embed = false;
	bool extract = false;
	bool bad_usage = false;
	std::string input_image;
	std::string input_file;
	std::string output_image;
	//extremely primitive way of reading the arguments passed
	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] != '-')
		{
			arg_count++;
			switch(arg_count)
			{
			case 1:
				input_image = std::string(argv[i]);
				break;
			case 2:
				input_file = std::string(argv[i]);
				output_image = std::filesystem::path(input_file).filename().string() + ".png";
				break;
			}
		}
		else
		{
			if (argv[i][1] == 'b')
			{
				if ((argv[i][2] - '0' > 0) && (argv[i][2] - '0' < 9))
				{
					bit_n = argv[i][2] - '0';
					std::cout << "will use " << bit_n << " bits" << std::endl;
				}
				else
				{
					bad_usage = true;
				}
			}
			else
			{
				bad_usage = true;
			}
		}
	}
	if (bad_usage)
	{
		arg_count = 0;
	}
	switch(arg_count)
	{
	case 1:
		extract = true;
		break;
	case 2:
		embed = true;
		break;
	default:
		std::cout << "usage: imagelol <input-image> <input-file> [-b<bitdepth>] (embeds a file)\n";
		std::cout << "       imagelol <input-image> (extracts a file)" << std::endl;
	}
	if (extract||embed)
	{
		bool is_png;
		bool could_read_img = true;
		int num_channels = 4;
		size_t input_bytes;
		std::vector<byte> image_bytes;
		int i_width, i_height = 0;
		//reads the image as RGB, ignores any transparency
		std::ifstream image_file;
		image_file.open(input_image, std::ios::binary);
		if (image_file.is_open())
		{	
			image_file.ignore(std::numeric_limits<std::streamsize>::max());
			std::streamsize image_file_size = image_file.gcount();
			image_file.clear();
			image_file.seekg(0, std::ios::beg);
			std::cout << "input_image size: " << image_file_size << std::endl;
			std::vector<byte> image_file_bytes;
			image_file_bytes.resize(image_file_size);
			image_file.read(reinterpret_cast<std::ifstream::char_type *>(image_file_bytes.data()), image_file_bytes.size());
			is_png = (image_file_bytes.size() >= 8) && (!png_sig_cmp(image_file_bytes.data(), 0, 8));
			if (is_png)
			{	
				std::cout << "input_image is a png, will use libpng" << std::endl;
				png_image p_img;
				memset(&p_img, 0, (sizeof p_img));
				p_img.version = PNG_IMAGE_VERSION;
				if (png_image_begin_read_from_memory(&p_img, image_file_bytes.data(), image_file_bytes.size()) != 0)
				{
					if (p_img.format & PNG_FORMAT_FLAG_ALPHA)
					{
						p_img.format = PNG_FORMAT_RGBA;
						num_channels = 4;
					}
					else
					{
						p_img.format = PNG_FORMAT_RGB;
						num_channels = 3;
					}
					p_img.flags = 0;
					input_bytes = PNG_IMAGE_SIZE(p_img);
					image_bytes.resize(input_bytes);
					i_width  = p_img.width;
					i_height = p_img.height;
					if (!image_bytes.empty())
					{
						could_read_img = png_image_finish_read(&p_img, nullptr, image_bytes.data(), 0, nullptr);
						std::cout << "image_read" << std::endl;
					}
					else
					{
						could_read_img = false;
					}
				}
				else
				{
					could_read_img = false;
				}

			}
			else
			{
				num_channels = 3;
				byte* temp = stbi_load_from_memory(image_file_bytes.data(), image_file_bytes.size(), &i_width, &i_height, nullptr, STBI_rgb);
				input_bytes = (static_cast<size_t>(i_width) * static_cast<size_t>(i_height) * static_cast<size_t>(num_channels));
				image_bytes.resize(input_bytes);
				for (size_t i = 0; i < input_bytes; i++)
				{
					image_bytes[i] = temp[i];
				}
				stbi_image_free(temp);
			}
			image_file.close();
			if ((i_width == 0) || (i_height == 0))
			{
				could_read_img = false;
			}
		}
		if (could_read_img)
		{
			if (embed)
			{
				std::ifstream file;
				file.open(input_file, std::ios::binary);
				if(file.is_open()){
					file.ignore(std::numeric_limits<std::streamsize>::max());
					std::streamsize f_size = file.gcount();
					file.clear();
					file.seekg(0, std::ios::beg);
					std::cout << "file size: " << f_size << std::endl;
					if(f_size < INT64_MAX)
					{
						input_file = std::filesystem::path(input_file).filename().string();
						u64 file_size = (u64)f_size;
						u64 name_size = (u64)input_file.size();
						std::cout << "name size: " << name_size << std::endl;
						/* the total_size that will be stored is 1 byte for the numbers of bits to use, 64 bits
						 * for the length of the name, the name of the file, another 64 bits for the size of the
						 * file, and the file itself.
						 */
						u64 total_size = 1 + sizeof(u64) + name_size + sizeof(u64) + file_size;
						u64 total_pixels = ImageLOL::required_pixels(total_size, bit_n, num_channels);
						ImageLOL::dimensions output_size = ImageLOL::minimum_dimensions(total_pixels, i_width, i_height);
						u64 o_width, o_height;
						o_width = output_size.width;
						o_height = output_size.height;
						/*output size will be (close) to the minimum image with the same proportions as the original
						 *that can hold atleast total_size bytes, or the original size if it can already store enough
						 * bytes.
						 */
						std::cout << "ratio: " << static_cast<double>(o_width)/i_width << std::endl;
						std::cout << "output size: " << o_width << "," << o_height << std::endl;
						
						std::vector<byte> output_img;
						output_img.resize(o_width * o_height * num_channels);
						//initialize the image with an scaled version of the original one.
						for (u64 i = 0; i < o_height; i++)
						{
							for (u64 j = 0; j < o_width; j++)
							{
								output_img[(i * o_width + j) * num_channels]     = sample_image(image_bytes.data(), i_width, i_height, ((double)j/o_width),((double)i/o_height), num_channels, channels::r);
								output_img[(i * o_width + j) * num_channels + 1] = sample_image(image_bytes.data(), i_width, i_height, ((double)j/o_width),((double)i/o_height), num_channels, channels::g);
								output_img[(i * o_width + j) * num_channels + 2] = sample_image(image_bytes.data(), i_width, i_height, ((double)j/o_width),((double)i/o_height), num_channels, channels::b);
								if (num_channels == 4)
								{
									output_img[(i * o_width + j) * num_channels + 3] = sample_image(image_bytes.data(), i_width, i_height, ((double)j/o_width),((double)i/o_height), num_channels, channels::a);
								}
							}
						}
						std::vector<byte> file_bytes;
						file_bytes.resize(f_size);
						file.read(reinterpret_cast<std::ifstream::char_type *>(file_bytes.data()), file_bytes.size());
						ImageLOL::Image o_img(output_img.data(), o_width, o_height, num_channels);
						ImageLOL::ImageLOLWriter writer(o_img.getWriter(bit_n));
						writer.write(input_file);
						writer.write(file_bytes);
						//always writes the image into a png.
						png_image p_img;
						memset(&p_img, 0, (sizeof p_img));
						p_img.version = PNG_IMAGE_VERSION;
						p_img.opaque = NULL;
						p_img.width = static_cast<uint32_t>(o_width);
						p_img.height = static_cast<int32_t>(o_height);
						p_img.format = ((num_channels < 4) ? PNG_FORMAT_RGB : PNG_FORMAT_RGBA);
						p_img.flags = 0;
						png_image_write_to_file(&p_img, output_image.c_str(), 0, output_img.data(), 0, nullptr);

					}
					else
					{
						std::cout << "file is too big, sorry :c" << std::endl;
					}
					file.close();
				}
				else
				{
					std::cout << "couldn't open file to embed" << std::endl;
				}
			}
			else
			{
				ImageLOL::Image i_img(image_bytes.data(), i_width, i_height, num_channels);
				ImageLOL::ImageLOLReader reader(i_img.getReader());
				if ((reader.get_total_bytes() > 0) && (reader.get_bit_depth() >0))
				{
					std::cout << "Image uses: " << static_cast<int>(reader.get_bit_depth()) << " bits per byte" << std::endl;
					std::string output_file(reader.read<std::string>());
					std::ofstream file;
					file.open(output_file, std::ios::binary|std::ios::trunc);
					//will only continue if it can open the output file, will probably replace this behaviour in the future.
					if (file.is_open())
					{
						std::vector<byte> file_data(reader.read<std::vector<byte>>());
						file.write(reinterpret_cast<const std::ofstream::char_type*>(file_data.data()), file_data.size());
						file.close();
					}
					else
					{
						std::cout << "couldn't open output file" << std::endl;
					}
					std::cout << "filename is: " << output_file << std::endl;
				}
				else
				{
					std::cout << "The image does not contain valid file data" << std::endl;
				}
			}
		}
		else
		{
			std::cout << "couldn't read input image" << std::endl;
		}
	}
}
